#ifndef DAC_DRIVER_H
#define DAC_DRIVER_H

#include "main.h"   /* pulls in HAL types + generated handle externs (hi2c1) */

/* MCP4725 12-bit I2C DAC (source channel, DAC #0 on I2C1).
 * See docs/pin_mapping.md -- this replaces the earlier MCP4922 SPI part.
 *
 * The MCP4725 has no external Vref: its output is ratiometric to VDD
 * (0 .. VDD). Run it from a clean 5V for a 0-5V range. Absolute accuracy
 * is only as good as VDD, so measure the actual DAC supply with a
 * multimeter once the board is built and update DAC_VDD_VOLTS. */
#define DAC_VDD_VOLTS       5.000f
#define DAC_RESOLUTION_BITS 12
#define DAC_MAX_CODE        ((1 << DAC_RESOLUTION_BITS) - 1)

/* A0 tied to GND -> 7-bit address 0x60. HAL wants the 8-bit form. */
#define MCP4725_I2C_ADDR    (0x60 << 1)

void  DAC_Init(void);
void  DAC_SetVoltage(float volts);
float DAC_GetLastCommandedVoltage(void);

#endif /* DAC_DRIVER_H */
