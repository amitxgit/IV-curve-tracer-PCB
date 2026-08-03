# PC Client (iv_tracer)

## Install
```bash
pip install -r requirements.txt
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
