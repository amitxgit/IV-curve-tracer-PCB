"""
High-level sweep helper and the Shockley diode fit used in
docs/validation_plan.md to validate the tracer against a known diode.
"""

from __future__ import annotations
import numpy as np
from scipy.optimize import curve_fit

from .instrument import SCPIInstrument

VT_ROOM_TEMP = 0.02585  # thermal voltage at ~300K, volts


def run_iv_sweep(port: str, start: float, stop: float, step: float, dwell_ms: int = 10):
    """Runs a full sweep and returns (voltages, currents) as numpy arrays."""
    with SCPIInstrument(port) as instr:
        print("Connected to:", instr.idn())
        instr.configure_sweep(start, stop, step, dwell_ms)
        voltages, currents = instr.run_sweep()
    return np.array(voltages), np.array(currents)


def _shockley(v, i_s, n):
    return i_s * (np.exp(v / (n * VT_ROOM_TEMP)) - 1.0)


def fit_diode_model(voltages: np.ndarray, currents: np.ndarray):
    """
    Fits measured (V, I) data to the Shockley diode equation.
    Returns (i_s, n, fitted_currents) -- see docs/validation_plan.md
    section 2 for how to use this to validate against a datasheet.
    """
    # Reasonable starting guesses for a small-signal silicon diode
    p0 = (1e-9, 1.5)
    popt, _ = curve_fit(_shockley, voltages, currents, p0=p0, maxfev=10000)
    i_s, n = popt
    fitted = _shockley(voltages, i_s, n)
    return i_s, n, fitted


def linear_fit_resistance(voltages: np.ndarray, currents: np.ndarray) -> float:
    """Least-squares resistance estimate for the known-resistor validation step."""
    # I = V / R  =>  slope of I vs V is 1/R
    slope, _intercept = np.polyfit(voltages, currents, 1)
    return 1.0 / slope if slope != 0 else float("inf")
