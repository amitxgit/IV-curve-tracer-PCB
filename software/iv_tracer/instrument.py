"""
Thin SCPI-over-serial wrapper, deliberately shaped like a PyVISA
resource (open / write / query) so the workflow here mirrors driving
a real lab SMU with PyVISA -- same mental model, same command style,
just talking to hardware you built yourself.
"""

from __future__ import annotations
import time


class SCPIInstrument:
    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 2.0,
                 transport=None):
        """Open the instrument over a serial port, or drive an injected
        transport instead. `transport` is any object exposing the
        write / readline / close surface of `serial.Serial` -- pass an
        `iv_tracer.simulator.SimulatedSerial` to run with no hardware.
        When a transport is given, `port`/`baudrate` are ignored."""
        if transport is not None:
            self._ser = transport
        else:
            import serial  # imported lazily so the sim path needs no pyserial
            self._ser = serial.Serial(port, baudrate=baudrate, timeout=timeout)
            time.sleep(2.0)  # let USB-CDC enumerate / MCU settle after port open

    def close(self) -> None:
        self._ser.close()

    def write(self, command: str) -> None:
        self._ser.write((command.strip() + "\n").encode("ascii"))

    def read_line(self) -> str:
        return self._ser.readline().decode("ascii", errors="replace").strip()

    def query(self, command: str) -> str:
        self.write(command)
        return self.read_line()

    def idn(self) -> str:
        return self.query("*IDN?")

    def reset(self) -> None:
        self.write("*RST")

    def set_voltage(self, volts: float) -> None:
        self.write(f"SOUR:VOLT {volts:.4f}")

    def get_voltage(self) -> float:
        return float(self.query("SOUR:VOLT?"))

    def measure_voltage(self) -> float:
        return float(self.query("MEAS:VOLT?"))

    def measure_current(self) -> float:
        return float(self.query("MEAS:CURR?"))

    def configure_sweep(self, start: float, stop: float, step: float, dwell_ms: int) -> None:
        self.write(f"SOUR:SWE:STAR {start:.4f}")
        self.write(f"SOUR:SWE:STOP {stop:.4f}")
        self.write(f"SOUR:SWE:STEP {step:.4f}")
        self.write(f"SOUR:SWE:DWEL {dwell_ms}")

    def run_sweep(self):
        """
        Triggers SOUR:SWE:RUN and reads the streamed "V,I" lines until
        the firmware sends END. Returns (voltages, currents) lists.
        """
        self.write("SOUR:SWE:RUN")

        header = self.read_line()  # expected "V,I"
        if header != "V,I":
            raise RuntimeError(f"Unexpected sweep header from instrument: {header!r}")

        voltages, currents = [], []
        while True:
            line = self.read_line()
            if line == "END":
                break
            if not line:
                continue  # tolerate the occasional timeout/empty read
            v_str, i_str = line.split(",")
            voltages.append(float(v_str))
            currents.append(float(i_str))

        return voltages, currents

    # Context manager support: `with SCPIInstrument(port) as instr:`
    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
