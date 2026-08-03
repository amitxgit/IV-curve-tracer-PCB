#include "sweep_engine.h"
#include "dac_driver.h"
#include "ina219_driver.h"
#include "ads1115_driver.h"
#include <stdio.h>

/* Sensible defaults; overwritten by SOUR:SWE:* commands before RUN */
#define SWEEP_DEFAULT_START_V   0.0f
#define SWEEP_DEFAULT_STOP_V    3.0f
#define SWEEP_DEFAULT_STEP_V    0.1f
#define SWEEP_DEFAULT_DWELL_MS  10

static SweepConfig s_cfg = {
    .startVolts = SWEEP_DEFAULT_START_V,
    .stopVolts  = SWEEP_DEFAULT_STOP_V,
    .stepVolts  = SWEEP_DEFAULT_STEP_V,
    .dwellMs    = SWEEP_DEFAULT_DWELL_MS,
};

void SweepEngine_SetConfig(const SweepConfig *cfg)
{
    s_cfg = *cfg;
}

void SweepEngine_GetConfig(SweepConfig *cfgOut)
{
    *cfgOut = s_cfg;
}

void SweepEngine_Reset(void)
{
    s_cfg.startVolts = SWEEP_DEFAULT_START_V;
    s_cfg.stopVolts  = SWEEP_DEFAULT_STOP_V;
    s_cfg.stepVolts  = SWEEP_DEFAULT_STEP_V;
    s_cfg.dwellMs    = SWEEP_DEFAULT_DWELL_MS;
}

void SweepEngine_Run(void (*txLineFn)(const char *line))
{
    char line[64];

    if (s_cfg.stepVolts <= 0.0f) {
        txLineFn("ERR,\"invalid step\"\r\n");
        return;
    }

    for (float v = s_cfg.startVolts; v <= s_cfg.stopVolts + 1e-6f; v += s_cfg.stepVolts)
    {
        DAC_SetVoltage(v);
        HAL_Delay(s_cfg.dwellMs);

        float vMeasured = ADS1115_ReadVoltage(ADS1115_CH0_SINGLE_ENDED); /* Kelvin sense */
        float iMeasured = INA219_ReadCurrentAmps();

        int len = snprintf(line, sizeof(line), "%.4f,%.6f\r\n", vMeasured, iMeasured);
        (void)len;
        txLineFn(line);
    }

    DAC_SetVoltage(0.0f); /* leave DUT unstressed after the sweep */
    txLineFn("END\r\n");
}
