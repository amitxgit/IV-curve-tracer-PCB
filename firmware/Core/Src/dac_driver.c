#include "dac_driver.h"

/* MCP4922 16-bit write frame layout (channel A, buffered, gain=1, active):
 *  bit15    bit14   bit13  bit12  bit11..0
 *  A/B      BUF     GA     SHDN   D11..D0
 *  0        1       1      1      12-bit code
 * (A/B=0 selects DAC A, which is what VoutA / the sweep output uses)
 */
#define MCP4922_CMD_CHA_BUF_GAIN1_ACTIVE  (0x3000u)

extern SPI_HandleTypeDef hspi1; /* provided by CubeMX-generated code */

static float s_lastCommandedVoltage = 0.0f;

static void CS_Low(void)  { HAL_GPIO_WritePin(DAC_CS_GPIO_Port, DAC_CS_Pin, GPIO_PIN_RESET); }
static void CS_High(void) { HAL_GPIO_WritePin(DAC_CS_GPIO_Port, DAC_CS_Pin, GPIO_PIN_SET);   }

void DAC_Init(void)
{
    CS_High();
    DAC_SetVoltage(0.0f);
}

void DAC_SetVoltage(float volts)
{
    if (volts < 0.0f)            volts = 0.0f;
    if (volts > DAC_VREF_VOLTS)  volts = DAC_VREF_VOLTS;

    uint16_t code = (uint16_t)((volts / DAC_VREF_VOLTS) * DAC_MAX_CODE + 0.5f);
    uint16_t frame = MCP4922_CMD_CHA_BUF_GAIN1_ACTIVE | (code & 0x0FFFu);

    uint8_t txBuf[2];
    txBuf[0] = (uint8_t)(frame >> 8);
    txBuf[1] = (uint8_t)(frame & 0xFF);

    CS_Low();
    HAL_SPI_Transmit(&hspi1, txBuf, 2, HAL_MAX_DELAY);
    CS_High();

    s_lastCommandedVoltage = volts;
}

float DAC_GetLastCommandedVoltage(void)
{
    return s_lastCommandedVoltage;
}
