# PC Client (iv_tracer)

## Install
```bash
pip install -r requirements.txt
```

## Try it with no hardware (simulator)
The client ships with a simulator that speaks the exact SCPI wire
protocol the firmware does, so you can run the whole pipeline before
the board exists. It models the DUT *and* the 150R current-limit
resistor, so the reported voltage shows the real Kelvin-sense effect.
```bash
# Simulated silicon diode, sweep the knee region
python -m iv_tracer.cli --simulate --sim-dut diode --start 0 --stop 0.8 --step 0.01 --plot --save diode.csv

# Simulated 1k resistor -- measured slope recovers R (minus the R_limit drop)
python -m iv_tracer.cli --simulate --sim-dut resistor --sim-resistance 1000 --start 0 --stop 3 --step 0.1 --plot
```
Drop `--simulate` and add `--port COM5` (or `/dev/ttyACM0`) to talk to
real hardware -- the client code is identical either way.

## Run the tests
```bash
pytest            # all tests run against the simulator, no hardware needed
```

## Quick sweep + plot
```bash
python -m iv_tracer.cli --port /dev/ttyACM0 --start 0 --stop 3 --step 0.05 --dwell 10 --plot --save diode.csv
```
On Windows, `--port` will be something like `COM5`.

## Using it as a library (e.g. for the diode validation in docs/validation_plan.md)
```python
from iv_tracer.sweep import run_iv_sweep, fit_diode_model
from iv_tracer.plotting import plot_iv

voltages, currents = run_iv_sweep("/dev/ttyACM0", start=0, stop=0.8, step=0.01, dwell_ms=10)
i_s, n, fitted = fit_diode_model(voltages, currents)
print(f"Extracted I_s = {i_s:.3e} A, ideality factor n = {n:.2f}")

fig = plot_iv(voltages, currents, title="1N4148 validation", fitted=fitted)
fig.savefig("diode_validation.png")
```

## Manual single-point interaction (handy for bring-up/debug)
```python
from iv_tracer.instrument import SCPIInstrument

with SCPIInstrument("/dev/ttyACM0") as instr:
    print(instr.idn())
    instr.set_voltage(1.0)
    print("V:", instr.measure_voltage(), "I:", instr.measure_current())
```
