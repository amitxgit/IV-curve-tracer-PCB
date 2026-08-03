#ifndef ADS1115_DRIVER_H
#define ADS1115_DRIVER_H

#include "main.h"

/* ADDR pin tied to GND per docs/pin_mapping.md */
#define ADS1115_I2C_ADDR   (0x48 << 1)  /* HAL wants the 8-bit address */

typedef enum {
    ADS1115_CH0_SINGLE_ENDED = 0, /* Kelvin voltage-sense input, see pin_mapping.md */
    ADS1115_CH1_SINGLE_ENDED = 1,
} ADS1115_Channel;

void  ADS1115_Init(void);
float ADS1115_ReadVoltage(ADS1115_Channel ch);

#endif /* ADS1115_DRIVER_H */
