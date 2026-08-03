"""
Hardware-free simulator for the I-V curve tracer.

`SimulatedSerial` is a drop-in replacement for `serial.Serial` that
speaks the exact SCPI-over-USB-CDC wire protocol the STM32 firmware
implements (see docs/scpi_command_reference.md and
firmware/Core/Src/scpi_parser.c). Because it matches the wire format,
`SCPIInstrument` drives it completely unchanged -- the same client
code that talks to the real board talks to this.

It models both the device under test and the 150R current-limit
resistor from the analogue signal chain, and solves for the actual
operating point. That reproduces the Kelvin-sense behaviour the real
instrument depends on: the reported DUT voltage (ADS1115 tap) is the
voltage the DUT actually sees, which is lower than the commanded DAC
voltage whenever current flows through R_limit.

Use it to run the whole CLI end-to-end with no hardware:

    python -m iv_tracer.cli --simulate --sim-dut diode --plot
"""

from __future__ import annotations
import math
import random
from collections import deque

# Matches docs/pin_mapping.md analogue signal chain and firmware defines.
R_LIMIT_OHMS = 150.0
DAC_VREF_VOLTS = 3.3
VT_ROOM_TEMP = 0.02585  # thermal voltage at ~300K, volts

# INA219 full-scale current with the default 0.1R shunt and /8 PGA
# (320mV / 0.1R). Reported current saturates here, like the real chip.
INA219_FULL_SCALE_A = 3.2
# ADS1115 LSB at the +-4.096V PGA the firmware selects (4.096 / 32768).
ADS1115_LSB_VOLTS = 0.000125


class DUT:
    """A device under test: maps its terminal voltage to its current."""

    def current(self, v_dut: float) -> float:
        raise NotImplementedError

    def label(self) -> str:
        return self.__class__.__name__


class ResistorDUT(DUT):
    def __init__(self, ohms: float = 1000.0):
        self.ohms = ohms

    def current(self, v_dut: float) -> float:
        return v_dut / self.ohms

    def label(self) -> str:
        return f"resistor {self.ohms:g}ohm"


class DiodeDUT(DUT):
    """Shockley model. Defaults are typical of a small-signal silicon
    diode (e.g. 1N4148) so the validation fit in sweep.py recovers
    sensible I_s / n values."""

    def __init__(self, i_s: float = 2.5e-9, n: float = 1.8):
        self.i_s = i_s
        self.n = n

    def current(self, v_dut: float) -> float:
        if v_dut <= 0.0:
            return 0.0
        # Guard the exponent so a large commanded voltage can't overflow.
        exponent = min(v_dut / (self.n * VT_ROOM_TEMP), 80.0)
        return self.i_s * (math.exp(exponent) - 1.0)

    def label(self) -> str:
        return f"diode (I_s={self.i_s:g}A, n={self.n:g})"


def solve_operating_point(dut: DUT, v_cmd: float, r_limit: float):
    """Given the commanded (pre-R_limit) voltage, solve for the DUT's
    actual terminal voltage and current. The DUT current and the
    R_limit current must match:  I_dut(v) == (v_cmd - v) / r_limit.
    Both DUT models are monotonic in v, so bisection converges cleanly.
    Returns (v_dut, current_amps)."""
    if v_cmd <= 0.0:
        return 0.0, 0.0

    lo, hi = 0.0, v_cmd
    for _ in range(60):
        mid = 0.5 * (lo + hi)
        # >0 means the DUT wants more current than R_limit can deliver
        # at this node voltage, so the true node voltage is lower.
        if dut.current(mid) - (v_cmd - mid) / r_limit > 0.0:
            hi = mid
        else:
            lo = mid

    v_dut = 0.5 * (lo + hi)
    return v_dut, dut.current(v_dut)


class SimulatedSerial:
    """A `serial.Serial`-compatible transport backed by a firmware-
    equivalent SCPI engine. Implements the write / readline / close
    surface that SCPIInstrument uses."""

    def __init__(
        self,
        dut: DUT | None = None,
        r_limit: float = R_LIMIT_OHMS,
        vref: float = DAC_VREF_VOLTS,
        noise: float = 0.0,
        seed: int = 1,
    ):
        self.dut = dut if dut is not None else DiodeDUT()
        self.r_limit = r_limit
        self.vref = vref
        self.noise = noise
        self._rng = random.Random(seed)

        self._rx = bytearray()      # bytes received from the host
        self._tx: deque[bytes] = deque()  # response lines awaiting readline()

        self._cmd_voltage = 0.0
        self._sweep = {"start": 0.0, "stop": 3.0, "step": 0.1, "dwell": 10}
        self._err = (0, "No error")

    # --- serial.Serial-compatible surface -------------------------------
    def write(self, data: bytes) -> int:
        self._rx += data
        while b"\n" in self._rx:
            idx = self._rx.index(b"\n")
            line = bytes(self._rx[:idx])
            del self._rx[: idx + 1]
            self._process(line.decode("ascii", errors="replace").strip())
        return len(data)

    def readline(self) -> bytes:
        # Empty bytes models a read timeout with nothing queued, which
        # is exactly what SCPIInstrument.read_line() tolerates.
        return self._tx.popleft() if self._tx else b""

    def close(self) -> None:
        pass

    # --- internal SCPI engine, mirroring firmware scpi_parser.c ---------
    def _emit(self, text: str) -> None:
        self._tx.append(text.encode("ascii"))

    def _measure(self, v_cmd: float):
        v_dut, current = solve_operating_point(self.dut, v_cmd, self.r_limit)
        current = max(-INA219_FULL_SCALE_A, min(INA219_FULL_SCALE_A, current))
        if self.noise:
            v_dut += self._rng.gauss(0.0, self.noise * ADS1115_LSB_VOLTS * 20.0)
            current += self._rng.gauss(0.0, self.noise * abs(current) + 1e-6)
        return v_dut, current

    def _process(self, line: str) -> None:
        if line == "*IDN?":
            self._emit("IV-TRACER,v1.0,SN001-SIM\r\n")
        elif line == "*RST":
            self._cmd_voltage = 0.0
            self._sweep = {"start": 0.0, "stop": 3.0, "step": 0.1, "dwell": 10}
            self._err = (0, "No error")
        elif line.startswith("SOUR:VOLT?"):
            self._emit(f"{self._cmd_voltage:.4f}\r\n")
        elif line.startswith("SOUR:VOLT"):
            self._cmd_voltage = self._clamp(self._arg(line))
        elif line.startswith("SOUR:SWE:STAR"):
            self._sweep["start"] = self._arg(line)
        elif line.startswith("SOUR:SWE:STOP"):
            self._sweep["stop"] = self._arg(line)
        elif line.startswith("SOUR:SWE:STEP"):
            self._sweep["step"] = self._arg(line)
        elif line.startswith("SOUR:SWE:DWEL"):
            self._sweep["dwell"] = int(self._arg(line))
        elif line == "SOUR:SWE:RUN":
            self._run_sweep()
        elif line == "MEAS:VOLT?":
            v_dut, _ = self._measure(self._cmd_voltage)
            self._emit(f"{v_dut:.4f}\r\n")
        elif line == "MEAS:CURR?":
            _, current = self._measure(self._cmd_voltage)
            self._emit(f"{current:.6f}\r\n")  # 6dp: diode/small-signal currents are uA-mA
        elif line == "SYST:ERR?":
            self._emit(f'{self._err[0]},"{self._err[1]}"\r\n')
            self._err = (0, "No error")
        else:
            self._err = (-100, "unrecognized command")
            self._emit('ERR,"unrecognized command"\r\n')

    def _run_sweep(self) -> None:
        step = self._sweep["step"]
        if step <= 0.0:
            self._emit('ERR,"invalid step"\r\n')
            return

        self._emit("V,I\r\n")
        v = self._sweep["start"]
        # +epsilon matches the firmware loop's stop-inclusive comparison.
        while v <= self._sweep["stop"] + 1e-6:
            v_dut, current = self._measure(self._clamp(v))
            self._emit(f"{v_dut:.4f},{current:.6f}\r\n")
            v += step
        self._cmd_voltage = 0.0  # firmware leaves the DUT unstressed
        self._emit("END\r\n")

    def _clamp(self, volts: float) -> float:
        return max(0.0, min(self.vref, volts))

    @staticmethod
    def _arg(line: str) -> float:
        parts = line.split(" ", 1)
        try:
            return float(parts[1]) if len(parts) > 1 else 0.0
        except ValueError:
            return 0.0


def make_dut(kind: str = "diode", resistance: float = 1000.0,
             i_s: float = 2.5e-9, n: float = 1.8) -> DUT:
    """Factory used by the CLI's --sim-* flags."""
    if kind == "resistor":
        return ResistorDUT(resistance)
    if kind == "diode":
        return DiodeDUT(i_s=i_s, n=n)
    raise ValueError(f"unknown simulated DUT kind: {kind!r}")
