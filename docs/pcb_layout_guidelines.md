# PCB Layout Guidelines

Scope: 2-layer board, JLCPCB-compatible, carrier/shield for the Blue
Pill module (see pin_mapping.md for why this reduces risk).

## 1. Ground strategy
- Use a solid ground plane on the bottom copper layer. Route signal
  traces on top.
- Keep a mental (or silkscreen-marked) split between the "digital"
  area (near the Blue Pill headers, SPI/I2C lines) and the "analogue"
  area (op-amp, DAC output, INA219, ADS1115 input, DUT connector).
  They should still share one ground plane -- don't actually slot/cut
  the plane, that causes more noise problems than it solves on a
  simple 2-layer board. Just keep the physical layout so analogue
  return currents don't have to cross under noisy digital traces.
- Bring all ground connections to a single point near the power entry
  (star point) where practical.

## 2. Decoupling
- Place a 100nF ceramic capacitor within ~5mm of every IC's supply
  pin (MCP4922, op-amp, INA219, ADS1115) -- lead/trace length matters
  more than exact placement.
- Add one 10uF bulk capacitor near the DAC/op-amp supply entry point.

## 3. Sensitive analogue routing
- The ADS1115 AIN0 trace (Kelvin voltage sense) is your most
  sensitive net -- keep it short, keep it away from the SPI clock
  line (PA5/SCK) and I2C lines, and don't run it parallel to them
  for any distance. If it must cross a digital trace, cross
  perpendicular, not parallel.
- Route the DUT connector traces (the pair going to J1) symmetrically
  and keep them short -- this pair carries your actual measurement
  signal.

## 4. SPI / I2C routing
- Keep SPI (PA4/5/6/7) and I2C (PB6/7) traces reasonably short and
  direct; at these clock speeds (a few MHz for SPI, 100-400kHz I2C)
  a 2-layer hobbyist board has plenty of margin, so this is a "don't
  be sloppy" rule rather than a strict impedance-control requirement.
- I2C needs pull-up resistors (4.7k typical) on SDA and SCL to 3V3
  if your ADS1115/INA219 breakout modules don't already have them
  onboard (most breakout modules do -- check before adding your own,
  since doubling up pull-ups changes the effective value).

## 5. Protection & connectors
- Add the R_limit resistor physically close to the op-amp output,
  before the current sensor.
- Use a DUT connector that's easy to swap components in/out quickly
  during validation -- screw terminals or banana jacks are both fine;
  avoid anything that requires soldering per test.
- Clearly silkscreen DUT+ / DUT- polarity next to the connector.

## 6. Practical fabrication notes
- Stick to 2-layer, standard 1.6mm thickness, HASL finish -- JLCPCB's
  cheapest tier. No need for anything exotic here.
- Add your name/project name/version number in silkscreen -- small
  detail, but it's what makes a board look like a real revision-
  controlled instrument rather than a one-off hack, and it's a nice
  detail in interview photos.
- Order 5 boards minimum (standard JLCPCB batch) -- you'll want a
  spare in case of a reflow/solder mistake on your first assembly.
- Add 2-3 test point vias/pads at: DAC output (pre-buffer), op-amp
  output (post-buffer), and the ADS1115 AIN0 node. These make bring-up
  debugging with a multimeter or scope dramatically easier and cost
  nothing to add.

## 7. What NOT to worry about for v1
- No need for 4-layer, impedance-controlled routing, or a dedicated
  analogue ground plane split -- those matter at higher precision /
  higher frequency than this project operates at.
- No need to design your own USB connector or crystal circuit -- the
  Blue Pill module handles that.
