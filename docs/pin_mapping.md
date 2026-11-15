# Pin Mapping & Interconnect

## Design decision: Blue Pill as a plug-in module

The PCB is a **carrier/shield board**. The STM32F103C8T6 Blue Pill module
plugs into it via two rows of headers, exactly like an Arduino shield.
This means the PCB does NOT need to route a crystal, reset circuit, or
USB connector -- all of that already lives on the Blue Pill module. Your
PCB only needs to route: two I2C buses (to the DACs + current sensor),
the analogue signal chain, the ADC voltage-sense divider, and the DUT
connector.

## Parts this design targets (v1, owned parts)

| Role | Part | Bus |
|------|------|-----|
| Source (voltage out) | **MCP4725 #0** (12-bit I2C DAC) | **I2C1** |
| Gate/base drive (reserved, dual-channel v2) | **MCP4725 #1** | **I2C2** |
| Current measure | **INA219** (0.1R shunt) | **I2C1** |
| Voltage measure (Kelvin) | **STM32 internal 12-bit ADC** via divider | analogue (PA0) |

> This replaces the earlier MCP4922 (SPI) + ADS1115 (I2C) design. There is
> no SPI in this build, and no external ADC.

## Why two I2C buses (the MCP4725 address problem)

Every MCP4725 breakout answers at I2C address `0x60` (or `0x61` if the
`A0` jumper is set). If both of your DAC boards are at the **same**
address on the **same** bus, software cannot tell them apart -- I2C
addressing is resolved in hardware, so both would ACK and drive the bus
simultaneously. You cannot fix that in firmware.

The robust fix, independent of how the boards are jumpered: put each DAC
on its **own hardware I2C peripheral**. The STM32F103C8 has two --
**I2C1** and **I2C2**. So:

- **MCP4725 #0 (source) -> I2C1** (shared with INA219, different address family)
- **MCP4725 #1 (gate/base, v2) -> I2C2** (alone)

In v1 only DAC #0 is populated/used, but the bus split is designed in from
the start so adding the second channel in v2 needs no board change.

## STM32F103C8T6 (Blue Pill) pin usage

| Pin  | Function       | Connects to                                              |
|------|----------------|-----------------------------------------------------------|
| PB6  | I2C1_SCL       | MCP4725 #0 SCL + INA219 SCL (shared bus)                  |
| PB7  | I2C1_SDA       | MCP4725 #0 SDA + INA219 SDA (shared bus)                  |
| PB10 | I2C2_SCL       | MCP4725 #1 SCL (gate/base drive, populated in v2)         |
| PB11 | I2C2_SDA       | MCP4725 #1 SDA (gate/base drive, populated in v2)         |
| PA0  | ADC1_IN0       | Kelvin V-sense divider midpoint (30k/10k) -- see below    |
| PC13 | GPIO (LED)     | Onboard LED -- "sweep in progress" indicator             |
| PA11 | USB D-         | Onboard, native USB (already routed on Blue Pill)         |
| PA12 | USB D+         | Onboard -- **see gotcha below**                          |
| 5V   | Power          | MCP4725 VDD (5V from USB pin) + op-amp/MT3608 rail feed   |
| 3V3  | Power          | INA219 logic, I2C pull-ups, ADC reference domain (VDDA)   |
| GND  | Ground         | Common ground (see PCB layout guidelines for star-ground) |

### Known Blue Pill gotcha
Many cheap Blue Pill clones ship with a 10k pull-up on PA12 instead of
the ~1.5k the USB spec calls for, which causes intermittent enumeration
failures. If USB-CDC doesn't enumerate reliably, replace R10 near the USB
connector with 1.5k, or add a 1.5k from PA12 to 3V3 externally. Test
USB-CDC enumeration on a bare Blue Pill *before* committing to the PCB,
so you're not debugging USB and your board at the same time.

## MCP4725 (I2C DAC) connections

| MCP4725 pin | Connects to                                         |
|-------------|------------------------------------------------------|
| VDD         | 5V (for 0-5V output range) -- or 3V3 for a 0-3.3V range |
| GND         | GND                                                  |
| SCL         | I2C1_SCL (PB6) for DAC #0  /  I2C2_SCL (PB10) for DAC #1 |
| SDA         | I2C1_SDA (PB7) for DAC #0  /  I2C2_SDA (PB11) for DAC #1 |
| OUT         | Op-amp buffer input (this is the raw source voltage) |
| A0          | GND (addr 0x60). Address is irrelevant here because each DAC is on its own bus. |

**Reference note:** the MCP4725 has no external Vref pin -- its output
range is `0 .. VDD` (ratiometric to its supply). Run it from a clean 5V
for a 0-5V range. The DAC's absolute accuracy is therefore only as good
as its VDD; this is why the firmware has a calibratable `DAC_VDD_VOLTS`
constant you measure with a multimeter (see firmware/README.md).

## Analogue signal chain (the important part)

```
MCP4725 #0 OUT --> Op-amp (buffer, or gain=2) --> R_limit(150R) --> INA219(in-line I-sense) --> DUT+ --> DUT --> GND
                                                                                     |
                                            30k/10k divider --> PA0 (STM32 ADC1_IN0) taps DUT+ node (Kelvin V-sense)
```

- **Op-amp stage:** unity-gain buffer for a 0-5V range (simplest first
  bring-up), OR non-inverting gain = 2 (two 10k precision resistors,
  op-amp powered from the ~12V MT3608 rail) for a 0-10V range. Buffering
  matters: the MCP4725 output cannot drive DUT current directly.
- **R_limit (150R)** protects the op-amp and DUT against a short or a
  reversed DUT. 150R at 10V => ~67mA worst case, within a typical op-amp's
  output limit; increase it if your op-amp is weaker.
- **INA219** sits in series and measures current -- this is `MEAS:CURR?`.
- **Kelvin V-sense via STM32 ADC:** PA0 taps the DUT+ node (after R_limit
  and the shunt) through a **30k/10k divider**, so the reported DUT
  voltage is what the DUT actually sees, not the DAC-commanded value. The
  divider scales 0-10V into the ADC's 0-3.3V window (10V * 10k/40k =
  2.5V). This is the single most important accuracy detail and worth
  explaining in an interview.
  - Divider ratio = (R2+R3)/R3 = (30k+10k)/10k = **4.0**. Firmware
    multiplies the ADC reading by this ratio; both the ratio and the ADC
    reference are calibratable constants.

## ADC reference note (accuracy)

The STM32 ADC reference is VDDA (~3.3V from the Blue Pill regulator),
which is not a precision reference. Two options, documented in DESIGN.md:
1. **v1 (no extra parts):** measure actual VDDA with a multimeter, put it
   in `ADC_VREF_VOLTS`, and calibrate the divider ratio against a known
   voltage. Good to ~1%.
2. **optional upgrade:** feed a known LM4040-4.096 / REF3040 into a spare
   ADC channel and use it to correct VDDA drift in software, or drive
   VREF+ if your board breaks it out.

## I2C addressing

| Device      | Bus  | Address pins  | 7-bit address |
|-------------|------|----------------|----------------|
| MCP4725 #0  | I2C1 | A0 -> GND      | 0x60           |
| INA219      | I2C1 | A0,A1 -> GND   | 0x40           |
| MCP4725 #1  | I2C2 | A0 -> GND      | 0x60           |

MCP4725 #0 (0x60) and INA219 (0x40) coexist on I2C1 -- different
addresses, no conflict. The second MCP4725 lives alone on I2C2, so its
identical 0x60 address never clashes.

## Board connectors (rev 1.0)

| Ref | What plugs in | Pins (pin 1 first) |
|-----|---------------|--------------------|
| J1  | Blue Pill, PB12 side | PB12 ... PB9, 5V, GND, 3V3 (pin 1 at the USB end) |
| J2  | Blue Pill, GND side  | GND, GND, 3V3, NRST, PB11 ... PA0, PC15, PC14, PC13, VBAT (pin 1 at the USB end) |
| J3  | MCP4725 #0 (source)  | OUT, GND, SCL, SDA, VCC, GND |
| J4  | MCP4725 #1 (gate, v2)| OUT, GND, SCL, SDA, VCC, GND |
| J5  | INA219               | VCC, GND, SCL, SDA, VIN-, VIN+ |
| J6  | DUT                  | DUT+, DUT- (GND) |
| J7  | Gate/base drive (v2) | GATE, GND |
| J8  | MT3608 (flying leads)| 5V out, GND, GND, VBOOST in |
| JP1 | Op-amp supply        | 5V, VOPAMP, VBOOST (1-2 = 5V, 2-3 = boost) |
| JP2 | Gain                 | open = x1, closed = x2 |

Breakout pin order differs between clone boards. Compare the table
with the printing on your own modules before soldering the sockets.

## I2C pull-ups
Most MCP4725/INA219 breakouts include SDA/SCL pull-ups. If you populate
multiple modules on one bus you may end up with several pull-ups in
parallel (effective resistance drops). Check each module; aim for an
effective ~2.2k-4.7k per bus. Add the `Rpu` 4.7k resistors only on buses
that need them.
