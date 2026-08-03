# Schematic Notes

This folder is where your actual KiCad project (`.kicad_pro`,
`.kicad_sch`, `.kicad_pcb`) goes once you capture the schematic --
KiCad project files are binary/format-specific and need to be created
in KiCad itself, not generated externally, so use this file as your
build sheet to enter into KiCad directly.

See `docs/pin_mapping.md` for the full net list and `docs/BOM.csv` for
part numbers. Below is the schematic in block form -- draw this as
four logical sub-circuits in KiCad (hierarchical sheets are a nice
touch: `MCU_Interface`, `DAC_Stage`, `Sense_Stage`, `DUT_IO`).

## Block diagram

```
                         3V3
                          |
   +----------+     +-----+-----+      +--------------+      +-----+     +---------------+
   | Blue Pill|SPI  |  MCP4922  |Vout  |  Op-amp      |Vbuf  | R_  |     |    INA219     |
   |  (MCU)   |---->|  12-bit   |----->|  buffer      |----->| lim |---->|  current      |----+---> DUT+ --> DUT --> GND
   |          |     |  DAC      |      |  (unity gain)|      |150R |     |  sense        |    |
   |          |I2C  +-----------+      +--------------+      +-----+     +---------------+    |
   |          |<-----------------------------------------------------------------------------+
   |          |                                                                    ADS1115
   |          |I2C                                                                 AIN0 taps
   |          |<-------------------------------------------------------------------- here (Kelvin
   |          |                                                                       V-sense)
   +----------+
        |
       USB-CDC
        |
        v
   PC (Python client, SCPI over serial)
```

## Sub-circuit build order (recommended KiCad hierarchical sheets)

1. **MCU_Interface** -- Blue Pill footprint (2x 1x20 female headers so
   the module plugs straight in), decoupling, PC13 LED net label.
2. **DAC_Stage** -- MCP4922, its decoupling, SPI net labels
   (SCK/SDI/CS from MCU_Interface), REFA/REFB tied to VDD, LDAC tied
   to GND, VoutA broken out as a net label into DAC_Stage.
3. **Sense_Stage** -- op-amp buffer (input = DAC_Stage VoutA net,
   output = buffered net), R_limit, INA219 footprint (breakout module
   -- use a generic header footprint if using a module rather than the
   bare IC), ADS1115 footprint likewise, I2C net labels back to
   MCU_Interface.
3. **DUT_IO** -- the actual DUT connector (J1), fed from Sense_Stage's
   output net, with the ADS1115 Kelvin-sense tap wired in on this
   sheet too since it physically sits right at the DUT connector.

## Before you route the PCB

1. Breadboard the full analogue chain first (DAC -> buffer -> R_limit
   -> INA219 -> resistor DUT -> GND, with ADS1115 tapped in) and get a
   sweep working over USB-CDC on the breadboard.
2. Only commit to PCB fab once the breadboard sweep produces a sane
   V-I line for a known resistor. This avoids spending a fab cycle
   (1-2 weeks turnaround + shipping) debugging a design issue that's
   much faster to find on a breadboard.
3. Then transfer the validated schematic into KiCad, run ERC, and
   proceed to layout using `docs/pcb_layout_guidelines.md`.
