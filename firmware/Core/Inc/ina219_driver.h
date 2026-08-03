#ifndef INA219_DRIVER_H
#define INA219_DRIVER_H

#include "main.h"

/* Address pins A0/A1 tied to GND per docs/pin_mapping.md */
#define INA219_I2C_ADDR       (0x40 << 1)  /* HAL wants the 8-bit address */

/* Measure your actual shunt resistor value and update this.
 * Default assumes a common breakout module's onboard 0.1R shunt. */
#define INA219_SHUNT_OHMS     0.1f

/* Calibration sets the current LSB. This value targets a max expected
 * current of ~100mA with good resolution for small-signal diode/BJT
 * testing -- adjust if you plan to push more current through the DUT. */
#define INA219_MAX_EXPECTED_CURRENT_A  0.1f

void  INA219_Init(void);
float INA219_ReadCurrentAmps(void);
float INA219_ReadShuntVoltageVolts(void);

#endif /* INA219_DRIVER_H */
