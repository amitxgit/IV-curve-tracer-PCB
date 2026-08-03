#include "ads1115_driver.h"

extern I2C_HandleTypeDef hi2c1; /* provided by CubeMX-generated code */

#define REG_CONVERSION  0x00
#define REG_CONFIG      0x01

/* PGA = +-4.096V (good match for a 0-3.3V sweep range with margin) */
#define PGA_LSB_VOLTS   0.000125f  /* 4.096V / 32768 */

static void WriteReg16(uint8_t reg, uint16_t value)
{
    uint8_t buf[2] = { (uint8_t)(value >> 8), (uint8_t)(value & 0xFF) };
    HAL_I2C_Mem_Write(&hi2c1, ADS1115_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, 2, HAL_MAX_DELAY);
}

static int16_t ReadReg16(uint8_t reg)
{
    uint8_t buf[2];
    HAL_I2C_Mem_Read(&hi2c1, ADS1115_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, 2, HAL_MAX_DELAY);
    return (int16_t)((buf[0] << 8) | buf[1]);
}

void ADS1115_Init(void)
{
    /* Nothing persistent to configure -- each read below sets up a
     * fresh single-shot conversion with the right MUX bits. */
}

float ADS1115_ReadVoltage(ADS1115_Channel ch)
{
    /* Config register: single-shot, +-4.096V PGA, 128SPS, single-ended
     * MUX select for AIN0 (100) or AIN1 (101) vs GND, start conversion. */
    uint16_t muxBits = (ch == ADS1115_CH0_SINGLE_ENDED) ? 0x4000u : 0x5000u;
    uint16_t config = 0x8000u /* OS: start single conversion */
                    | muxBits
                    | 0x0200u /* PGA: +-4.096V */
                    | 0x0100u /* MODE: single-shot */
                    | 0x0080u /* DR: 128SPS */
                    | 0x0003u; /* disable comparator */

    WriteReg16(REG_CONFIG, config);
    HAL_Delay(8); /* >1/128s conversion time, rounded up with margin */

    int16_t raw = ReadReg16(REG_CONVERSION);
    return (float)raw * PGA_LSB_VOLTS;
}
