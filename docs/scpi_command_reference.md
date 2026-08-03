# SCPI Command Reference

Minimal SCPI-inspired command set. Line-terminated ASCII over USB-CDC
(virtual COM port), 115200 baud (irrelevant for native USB-CDC but set
your terminal to something for compatibility), newline (`\n`)
terminated commands and responses.

| Command                        | Type  | Description                                      | Example response |
|---------------------------------|-------|---------------------------------------------------|-------------------|
| `*IDN?`                          | Query | Instrument identification                          | `IV-TRACER,v1.0,SN001` |
| `*RST`                            | Command | Reset DAC output to 0V, clear sweep state        | (none) |
| `SOUR:VOLT <value>`               | Command | Set DAC output voltage immediately (e.g. `SOUR:VOLT 1.65`) | (none) |
| `SOUR:VOLT?`                      | Query | Query last commanded output voltage                | `1.6500` |
| `SOUR:SWE:STAR <value>`           | Command | Set sweep start voltage                          | (none) |
| `SOUR:SWE:STOP <value>`           | Command | Set sweep stop voltage                           | (none) |
| `SOUR:SWE:STEP <value>`           | Command | Set sweep step size                              | (none) |
| `SOUR:SWE:DWEL <ms>`              | Command | Set dwell time per point in milliseconds         | (none) |
| `SOUR:SWE:RUN`                    | Command | Execute the configured sweep, streaming results  | `V,I\n0.0000,0.0001\n0.0500,0.0002\n...\nEND\n` |
| `MEAS:VOLT?`                      | Query | Single-shot DUT voltage (Kelvin-sensed)          | `1.6480` |
| `MEAS:CURR?`                      | Query | Single-shot DUT current in Amps                  | `0.000123` |
| `SYST:ERR?`                       | Query | Last error message, `0,"No error"` if none        | `0,"No error"` |

## Sweep streaming format

`SOUR:SWE:RUN` streams one CSV line per point as `voltage,current`
(volts, amps), followed by a literal `END` line so the PC client
knows the sweep is complete without needing a fixed point count in
advance.

## Error handling
Any malformed command responds immediately with a single line:
```
ERR,"unrecognized command"
```
so the PC client can detect and log parser failures instead of hanging
on a missing response. The same failure is also recorded in a one-deep
error queue: a subsequent `SYST:ERR?` returns `-100,"unrecognized
command"` and reading it clears the queue back to `0,"No error"`
(standard SCPI semantics). `*RST` also clears the queue.

## Design rationale
This deliberately mirrors the subset of real SCPI (`SOUR:`, `MEAS:`,
`*IDN?`, `*RST`) that you'd actually use when driving a Keithley/Keysight
SMU with PyVISA -- the goal is that anyone who has driven a real SMU
recognizes this command set immediately, which is exactly the signal
you want on a test-engineering resume.
