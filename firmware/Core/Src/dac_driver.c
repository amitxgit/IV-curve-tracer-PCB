#include "dac_driver.h"

/* MCP4725 "fast mode" write: two bytes carry the 12-bit code.
 *   byte0:  0 0 PD1 PD0 D11 D10 D9 D8   (PD1=PD0=0 -> normal power-up)
 *   byte1:  D7 D6 D5 D4 D3 D2 D1 D0
 * This updates the DAC register (not EEPROM), which is what we want for
 * a fast sweep -- we never wear out the EEPROM. See MCP4725 datasheet
 * section 6.1. For the command-type write (with EEPROM) you'd send a
 * 0x40/0x60 command byte first; we deliberately avoid that here.
 */

extern I2C_HandleTypeDef hi2c1; /* provided by CubeMX-generated code */

static float s_lastCommandedVoltage = 0.0f;

void DAC_Init(void)
{
    DAC_SetVoltage(0.0f);
}

void DAC_SetVoltage(float volts)
{
    if (volts < 0.0f)           volts = 0.0f;
    if (volts > DAC_VDD_VOLTS)  volts = DAC_VDD_VOLTS;

    uint16_t code = (uint16_t)((volts / DAC_VDD_VOLTS) * DAC_MAX_CODE + 0.5f);
    if (code > DAC_MAX_CODE) code = DAC_MAX_CODE;

    uint8_t txBuf[2];
    txBuf[0] = (uint8_t)((code >> 8) & 0x0F); /* fast-mode: PD bits 0, top nibble */
    txBuf[1] = (uint8_t)(code & 0xFF);

    HAL_I2C_Master_Transmit(&hi2c1, MCP4725_I2C_ADDR, txBuf, 2, HAL_MAX_DELAY);

    s_lastCommandedVoltage = volts;
}

float DAC_GetLastCommandedVoltage(void)
{
    return s_lastCommandedVoltage;
}
