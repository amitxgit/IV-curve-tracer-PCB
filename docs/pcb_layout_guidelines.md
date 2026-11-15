# PCB Layout Guidelines

Scope: 2-layer board, JLCPCB-compatible, carrier/shield for the Blue
Pill module (see pin_mapping.md for why this reduces risk).

## 1. Ground strategy
- Use a solid ground plane on the bottom copper layer. Route signal
  traces on top.
- Keep a mental (or silkscreen-marked) split between the "digital"
  area (near the Blue Pill headers, I2C lines) and the "analogue"
  area (op-amp, DAC output, INA219, V-sense divider, DUT connector).
  They should still share one ground plane -- don't actually slot/cut
  the plane, that causes more noise problems than it solves on a
  simple 2-layer board. Just keep the physical layout so analogue
  return currents don't have to cross under noisy digital traces.
- Bring all ground connections to a single point near the power entry
  (star point) where practical.

## 2. Decoupling
- Place a 100nF ceramic capacitor within ~5mm of every IC's supply
  pin (both MCP4725 sockets, op-amp, INA219 socket) -- lead/trace
  length matters more than exact placement.
- Add one 10uF bulk capacitor near the DAC/op-amp supply entry point.

## 3. Sensitive analogue routing
- The divider midpoint into PA0 (VSENSE, Kelvin voltage sense) is
  your most sensitive net -- keep it short, keep it away from the I2C
  lines, and don't run it parallel to them for any distance. If it
  must cross a digital trace, cross perpendicular, not parallel. Put
  the divider next to PA0 and let the low-impedance DUT+ side be the
  long run.
- Route the DUT connector traces (the pair going to J1) symmetrically
  and keep them short -- this pair carries your actual measurement
  signal.

## 4. I2C routing
- Keep the I2C1 (PB6/7) and I2C2 (PB10/11) traces reasonably short and
  direct; at these clock speeds (100-400kHz I2C)
  a 2-layer hobbyist board has plenty of margin, so this is a "don't
  be sloppy" rule rather than a strict impedance-control requirement.
- I2C needs pull-up resistors (4.7k typical) on SDA and SCL to 3V3
  if your MCP4725/INA219 breakout modules don't already have them
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
  output (post-buffer), and the VSENSE node at PA0. These make bring-up
  debugging with a multimeter or scope dramatically easier and cost
  nothing to add.

## 7. What NOT to worry about for v1
- No need for 4-layer, impedance-controlled routing, or a dedicated
  analogue ground plane split -- those matter at higher precision /
  higher frequency than this project operates at.
- No need to design your own USB connector or crystal circuit -- the
  Blue Pill module handles that.

## 8. How rev 1.0 of the board follows these rules

Board: `hardware/kicad/iv-curve-tracer-pcb`, 99.06 x 69.85 mm, 2 layers.

- **Ground:** the whole bottom layer is one GND pour. Signals are on
  the top layer; the bottom carries only a handful of short hops where
  two top-layer runs have to cross.
- **Digital / analogue split:** the Blue Pill sits on the left. I2C1,
  5V and 3V3 leave it along the top edge and drop into the module
  sockets from above. The op-amp, R1, the INA219 force path and the
  DUT terminal sit below the sockets, and no I2C or supply-rail track
  crosses that area.
- **Kelvin sense:** R2/R3/C7 and TP3 sit right beside PA0, so VSENSE is
  only a few millimetres long. The DUT+ sense track leaves the J6 pad
  on its own and runs along the bottom edge; it does not share copper
  with the force track from the INA219.
- **Decoupling:** C1/C2/C3 are directly above the VCC pins of J3/J4/J5,
  C4 is next to U1 pin 8, C5/C6 sit at the supply entry (J8/JP1).
- **Protection:** R1 is the first part after the op-amp output.
- **Test points:** TP1 (DAC out), TP2 (buffer out), TP3 (VSENSE),
  TP4 (GND).
- **Widths:** 0.3 mm signals, 0.5 mm on 5V, 3V3, VBOOST, VOPAMP and the
  source path; 0.2 mm clearance; 0.8/0.4 mm vias.
- **Silkscreen:** project name, revision, DUT+/DUT- polarity, socket
  pin names, jumper positions and the op-amp supply warning.
