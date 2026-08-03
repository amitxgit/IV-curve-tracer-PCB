#include "scpi_parser.h"
#include "dac_driver.h"
#include "ina219_driver.h"
#include "adc_driver.h"
#include "sweep_engine.h"
#include "usbd_cdc_if.h" /* CubeMX-generated -- provides CDC_Transmit_FS */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define FW_IDN_STRING  "IV-TRACER,v1.0,SN001\r\n"

/* Single-entry error record backing SYST:ERR?. Real SCPI instruments
 * keep a FIFO; one deep is enough here and matches how the PC client
 * polls (query after each command). Reading it clears it, per SCPI. */
static int  s_errCode = 0;
static char s_errMsg[48] = "No error";

static void SetError(int code, const char *msg)
{
    s_errCode = code;
    strncpy(s_errMsg, msg, sizeof(s_errMsg) - 1);
    s_errMsg[sizeof(s_errMsg) - 1] = '\0';
}

static void ClearError(void)
{
    s_errCode = 0;
    strncpy(s_errMsg, "No error", sizeof(s_errMsg) - 1);
    s_errMsg[sizeof(s_errMsg) - 1] = '\0';
}

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

/* Currents are uA-mA scale for diodes/small-signal parts, so 4 decimals
 * would floor sub-100uA readings to 0.0000. Use 6 decimals to match the
 * sweep stream format and the MEAS:CURR? example in the SCPI reference. */
static void ReplyCurrent(float amps)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6f\r\n", amps);
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
        SweepEngine_Reset();
        ClearError();
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
        ReplyFloat(ADC_ReadVoltage());
    }
    else if (strcmp(line, "MEAS:CURR?") == 0) {
        ReplyCurrent(INA219_ReadCurrentAmps());
    }
    else if (strcmp(line, "SYST:ERR?") == 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%d,\"%s\"\r\n", s_errCode, s_errMsg);
        Transmit(buf);
        ClearError();
    }
    else {
        SetError(-100, "unrecognized command");
        Transmit("ERR,\"unrecognized command\"\r\n");
    }
}
