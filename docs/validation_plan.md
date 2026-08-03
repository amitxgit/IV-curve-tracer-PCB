# Validation Plan

The goal is a documented, repeatable comparison against known
references -- this is what turns "I built a curve tracer" into
"I built and validated a measurement instrument," and it's the section
of your GitHub README an interviewer will actually read closely.

## 1. Known resistor (sanity check)
- Use a 1% tolerance resistor (e.g. 1kΩ) as the DUT.
- Sweep 0-3V in 0.1V steps.
- Expected: straight line, slope = 1/R.
- Compute measured resistance via linear fit of your V-I data and
  compare against the resistor's marked value + multimeter reading.
- Report: measured R, expected R, % error.

## 2. Known diode (the real test)
- Use a common diode with a published datasheet I-V curve (e.g. 1N4148
  or 1N4007).
- Sweep 0-0.8V in small steps (0.01-0.02V) since diode current rises
  exponentially and you want resolution in the knee region.
- Fit your measured data to the Shockley diode equation:
  `I = I_s * (exp(V / (n*Vt)) - 1)`
  and extract saturation current I_s and ideality factor n.
- Compare extracted parameters and the overall curve shape against
  the datasheet plot.
- Report: overlay plot (your data vs datasheet curve), extracted I_s
  and n, and a qualitative/quantitative discussion of deviation.

## 3. Cross-check against a multimeter
- At 2-3 fixed DAC output voltages, independently measure DUT voltage
  and current with a bench multimeter and compare against what the
  tracer reports via `MEAS:VOLT?` / `MEAS:CURR?`.
- This validates your Kelvin-sensing claim -- if your ADS1115 reading
  matches the multimeter better than the raw DAC-commanded voltage
  does, that's your evidence the 4-wire-style sensing is doing its job.

## 4. Repeatability
- Run the same sweep 3-5 times and check point-to-point noise/spread.
- Report the sweep-to-sweep standard deviation at a couple of
  representative points -- gives you a real number to say when someone
  asks "how accurate is it?"

## What to put in the GitHub README
- The resistor and diode overlay plots.
- A short table: parameter, measured, reference, % error.
- One sentence on repeatability.
- This whole section doubles as the "Results" section if you later
  turn this into a HardwareX article or conference paper.
