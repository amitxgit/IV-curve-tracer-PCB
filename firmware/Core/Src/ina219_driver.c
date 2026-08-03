#include "ina219_driver.h"

extern I2C_HandleTypeDef hi2c1; /* provided by CubeMX-generated code */

/* INA219 register addresses */
#define REG_CONFIG        0x00
#define REG_SHUNTVOLTAGE  0x01
#define REG_BUSVOLTAGE    0x02
#define REG_CURRENT       0x04
#define REG_CALIBRATION   0x05

static float s_currentLSB_A = 0.0f;

static void WriteReg16(uint8_t reg, uint16_t value)
{
    uint8_t buf[2] = { (uint8_t)(value >> 8), (uint8_t)(value & 0xFF) };
    HAL_I2C_Mem_Write(&hi2c1, INA219_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, 2, HAL_MAX_DELAY);
}

static int16_t ReadReg16(uint8_t reg)
{
    uint8_t buf[2];
    HAL_I2C_Mem_Read(&hi2c1, INA219_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, 2, HAL_MAX_DELAY);
    return (int16_t)((buf[0] << 8) | buf[1]);
}

void INA219_Init(void)
{
    /* Config: 32V bus range, +-320mV shunt range (PGA /8), 12-bit,
     * continuous shunt+bus conversion. See INA219 datasheet Table 4/5
     * if you need a different range for your DUT current levels. */
    const uint16_t config = 0x399Fu;
    WriteReg16(REG_CONFIG, config);

    /* Calibration register per datasheet formula:
     * Cal = trunc(0.04096 / (Current_LSB * R_shunt))
     * Current_LSB chosen so max expected current uses most of the
     * 15-bit current register range (~32767 counts). */
    s_currentLSB_A = INA219_MAX_EXPECTED_CURRENT_A / 32768.0f;
    uint16_t cal = (uint16_t)(0.04096f / (s_currentLSB_A * INA219_SHUNT_OHMS));
    WriteReg16(REG_CALIBRATION, cal);
}

float INA219_ReadCurrentAmps(void)
{
    int16_t raw = ReadReg16(REG_CURRENT);
    return (float)raw * s_currentLSB_A;
}

float INA219_ReadShuntVoltageVolts(void)
{
    int16_t raw = ReadReg16(REG_SHUNTVOLTAGE); /* LSB = 10uV */
    return (float)raw * 0.00001f;
}
