#ifndef DAC_DRIVER_H
#define DAC_DRIVER_H

#include "main.h"   /* pulls in HAL types + generated handle externs (hspi1) */

/* Measure your actual DAC reference voltage with a multimeter and
 * update this. REFA/REFB are tied to VDD per hardware/schematic_notes.md. */
#define DAC_VREF_VOLTS      3.300f
#define DAC_RESOLUTION_BITS 12
#define DAC_MAX_CODE        ((1 << DAC_RESOLUTION_BITS) - 1)

/* Chip select pin -- matches docs/pin_mapping.md (PA4) */
#define DAC_CS_GPIO_Port    GPIOA
#define DAC_CS_Pin          GPIO_PIN_4

void  DAC_Init(void);
void  DAC_SetVoltage(float volts);
float DAC_GetLastCommandedVoltage(void);

#endif /* DAC_DRIVER_H */
