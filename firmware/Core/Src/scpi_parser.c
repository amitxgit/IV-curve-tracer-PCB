#include "scpi_parser.h"
#include "dac_driver.h"
#include "ina219_driver.h"
#include "ads1115_driver.h"
#include "sweep_engine.h"
#include "usbd_cdc_if.h" /* CubeMX-generated -- provides CDC_Transmit_FS */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define FW_IDN_STRING  "IV-TRACER,v1.0,SN001\r\n"

static void Transmit(const char *s)
{
    CDC_Transmit_FS((uint8_t *)s, (uint16_t)strlen(s));
}

static void ReplyFloat(float value)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%.4f\r\n", value);
    Transmit(buf);
}

/* Extracts the numeric argument after a space, e.g. "SOUR:VOLT 1.65" -> 1.65 */
static float ArgFloat(const char *line)
{
    const char *space = strchr(line, ' ');
    return space ? (float)atof(space + 1) : 0.0f;
}

void SCPI_ProcessLine(const char *line)
{
    if (strcmp(line, "*IDN?") == 0) {
        Transmit(FW_IDN_STRING);
    }
    else if (strcmp(line, "*RST") == 0) {
        DAC_SetVoltage(0.0f);
    }
    else if (strncmp(line, "SOUR:VOLT?", 10) == 0) {
        ReplyFloat(DAC_GetLastCommandedVoltage());
    }
    else if (strncmp(line, "SOUR:VOLT", 9) == 0) {
        DAC_SetVoltage(ArgFloat(line));
    }
    else if (strncmp(line, "SOUR:SWE:STAR", 13) == 0) {
        SweepConfig cfg; SweepEngine_GetConfig(&cfg);
        cfg.startVolts = ArgFloat(line);
        SweepEngine_SetConfig(&cfg);
    }
    else if (strncmp(line, "SOUR:SWE:STOP", 13) == 0) {
        SweepConfig cfg; SweepEngine_GetConfig(&cfg);
        cfg.stopVolts = ArgFloat(line);
        SweepEngine_SetConfig(&cfg);
    }
    else if (strncmp(line, "SOUR:SWE:STEP", 13) == 0) {
        SweepConfig cfg; SweepEngine_GetConfig(&cfg);
        cfg.stepVolts = ArgFloat(line);
        SweepEngine_SetConfig(&cfg);
    }
    else if (strncmp(line, "SOUR:SWE:DWEL", 13) == 0) {
        SweepConfig cfg; SweepEngine_GetConfig(&cfg);
        cfg.dwellMs = (uint16_t)ArgFloat(line);
        SweepEngine_SetConfig(&cfg);
    }
    else if (strcmp(line, "SOUR:SWE:RUN") == 0) {
        Transmit("V,I\r\n");
        SweepEngine_Run(Transmit);
    }
    else if (strcmp(line, "MEAS:VOLT?") == 0) {
        ReplyFloat(ADS1115_ReadVoltage(ADS1115_CH0_SINGLE_ENDED));
    }
    else if (strcmp(line, "MEAS:CURR?") == 0) {
        ReplyFloat(INA219_ReadCurrentAmps());
    }
    else if (strcmp(line, "SYST:ERR?") == 0) {
        Transmit("0,\"No error\"\r\n");
    }
    else {
        Transmit("ERR,\"unrecognized command\"\r\n");
    }
}
