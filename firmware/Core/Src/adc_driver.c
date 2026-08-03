#include "adc_driver.h"

/* Uses the CubeMX-generated ADC1 handle. Configure ADC1 in CubeMX with
 * IN0 (PA0) enabled, 12-bit resolution, software-triggered single
 * conversion, and a reasonably long sample time (e.g. 55.5 or 239.5
 * cycles) so the ~40k divider source impedance settles. See
 * firmware/README.md. */
extern ADC_HandleTypeDef hadc1; /* provided by CubeMX-generated code */

void ADC_Init(void)
{
    /* One-time calibration improves accuracy on the F1 ADC. Safe to call
     * once at startup after MX_ADC1_Init(); ignore the return on parts
     * where the HAL doesn't provide it. */
    HAL_ADCEx_Calibration_Start(&hadc1);
}

float ADC_ReadVoltage(void)
{
    uint32_t raw = 0;

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        raw = HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);

    /* Convert code -> voltage at PA0, then undo the input divider to get
     * the voltage the DUT actually sees. */
    float v_pa0 = ((float)raw / (float)ADC_MAX_CODE) * ADC_VREF_VOLTS;
    return v_pa0 * ADC_DIVIDER_RATIO;
}
