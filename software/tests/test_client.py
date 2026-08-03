"""
Tests for the PC-side client. These run entirely against the built-in
simulator (iv_tracer.simulator), so they need no hardware and no
serial port -- `pytest` from the software/ directory runs them all.
"""

from __future__ import annotations
import math

import numpy as np
import pytest

from iv_tracer.instrument import SCPIInstrument
from iv_tracer.simulator import (
    SimulatedSerial,
    ResistorDUT,
    DiodeDUT,
    solve_operating_point,
    R_LIMIT_OHMS,
)
from iv_tracer.sweep import fit_diode_model, linear_fit_resistance


def make_instr(dut):
    return SCPIInstrument("SIM", transport=SimulatedSerial(dut=dut, noise=0.0))


# --- protocol / instrument wrapper ------------------------------------

def test_idn_and_reset():
    with make_instr(ResistorDUT(1000)) as instr:
        assert instr.idn() == "IV-TRACER,v1.0,SN001-SIM"
        instr.set_voltage(1.5)
        assert instr.get_voltage() == pytest.approx(1.5, abs=1e-3)
        instr.reset()
        assert instr.get_voltage() == pytest.approx(0.0, abs=1e-6)


def test_unrecognized_command_sets_error_queue():
    transport = SimulatedSerial(dut=ResistorDUT(1000))
    with SCPIInstrument("SIM", transport=transport) as instr:
        assert instr.query("BOGUS:CMD") == 'ERR,"unrecognized command"'
        # The bad command is recorded, readable once, then cleared.
        assert instr.query("SYST:ERR?") == '-100,"unrecognized command"'
        assert instr.query("SYST:ERR?") == '0,"No error"'


def test_meas_curr_preserves_microamp_resolution():
    # MEAS:CURR? must report 6 decimals -- a diode in its knee draws
    # tens of microamps, which 4-decimal formatting would floor to 0.
    with make_instr(DiodeDUT()) as instr:
        instr.set_voltage(0.5)
        raw = instr.query("MEAS:CURR?")
    assert len(raw.split(".")[1]) == 6      # six decimal places on the wire
    assert float(raw) > 0.0                 # and the reading survives, not floored to 0


def test_sweep_streams_header_points_and_end():
    with make_instr(ResistorDUT(1000)) as instr:
        instr.configure_sweep(0.0, 1.0, 0.25, dwell_ms=0)
        voltages, currents = instr.run_sweep()
    # start..stop inclusive at 0.25 step -> 5 points
    assert len(voltages) == len(currents) == 5
    assert voltages[0] == pytest.approx(0.0, abs=1e-6)


# --- physics: Kelvin sense / operating point --------------------------

def test_resistor_kelvin_divider():
    # Measured DUT voltage should be the R/(R+R_limit) divider of the
    # commanded voltage -- this is the whole point of the 4-wire sense.
    r = 1000.0
    v_dut, current = solve_operating_point(ResistorDUT(r), 1.0, R_LIMIT_OHMS)
    assert v_dut == pytest.approx(r / (r + R_LIMIT_OHMS), rel=1e-4)
    assert current == pytest.approx(1.0 / (r + R_LIMIT_OHMS), rel=1e-4)


def test_operating_point_zero_at_zero_command():
    v_dut, current = solve_operating_point(DiodeDUT(), 0.0, R_LIMIT_OHMS)
    assert v_dut == 0.0
    assert current == 0.0


# --- validation fits (the resume-worthy analysis) ---------------------

def test_linear_fit_recovers_resistance():
    # A sweep of the *measured* (Kelvin) voltage vs current should
    # recover the DUT resistance itself, free of the R_limit drop.
    r = 2200.0
    with make_instr(ResistorDUT(r)) as instr:
        instr.configure_sweep(0.0, 3.0, 0.1, dwell_ms=0)
        voltages, currents = instr.run_sweep()
    r_fit = linear_fit_resistance(np.array(voltages), np.array(currents))
    assert r_fit == pytest.approx(r, rel=0.02)


def test_shockley_fit_recovers_diode_params():
    i_s, n = 2.5e-9, 1.8
    with make_instr(DiodeDUT(i_s=i_s, n=n)) as instr:
        instr.configure_sweep(0.0, 0.7, 0.01, dwell_ms=0)
        voltages, currents = instr.run_sweep()
    v = np.array(voltages)
    c = np.array(currents)
    # Fit only the part of the curve with meaningful conduction.
    mask = c > 1e-9
    fit_is, fit_n, fitted = fit_diode_model(v[mask], c[mask])
    assert fit_n == pytest.approx(n, rel=0.1)
    assert math.isclose(fit_is, i_s, rel_tol=0.5)  # I_s is hard; order of magnitude
    assert fitted.shape == v[mask].shape
