#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include "main.h"

/* Kelvin voltage sense via the STM32F103 internal 12-bit ADC on
 * ADC1_IN0 (PA0), replacing the earlier external ADS1115.
 * See docs/pin_mapping.md.
 *
 * PA0 taps the DUT+ node through a 30k/10k divider, so it reads the
 * voltage the DUT actually sees (not the DAC-commanded value). The
 * divider scales the 0-10V analogue range into the ADC's 0-VREF window.
 *
 * Both constants are calibratable -- measure them once on the built
 * board (see firmware/README.md):
 *  - ADC_VREF_VOLTS: the actual VDDA rail (~3.3V, not a precision ref).
 *  - ADC_DIVIDER_RATIO: (R2+R3)/R3 = (30k+10k)/10k = 4.0 nominal;
 *    trim against a known applied voltage for best accuracy. */
#define ADC_VREF_VOLTS      3.300f
#define ADC_RESOLUTION_BITS 12
#define ADC_MAX_CODE        ((1 << ADC_RESOLUTION_BITS) - 1)
#define ADC_DIVIDER_RATIO   4.0f

void  ADC_Init(void);
float ADC_ReadVoltage(void); /* returns DUT+ node voltage in volts */

#endif /* ADC_DRIVER_H */
