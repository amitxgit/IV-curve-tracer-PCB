#ifndef SWEEP_ENGINE_H
#define SWEEP_ENGINE_H

#include "main.h"

typedef struct {
    float    startVolts;
    float    stopVolts;
    float    stepVolts;
    uint16_t dwellMs;
} SweepConfig;

void SweepEngine_SetConfig(const SweepConfig *cfg);
void SweepEngine_GetConfig(SweepConfig *cfgOut);

/* Runs the configured sweep synchronously, calling txLineFn once per
 * point with a "V,I\r\n" formatted line, then once more with "END\r\n".
 * Passing the transmit function in as a pointer keeps this file free
 * of any USB/CDC dependency -- scpi_parser.c is the only place that
 * needs to know about CDC_Transmit_FS. */
void SweepEngine_Run(void (*txLineFn)(const char *line));

#endif /* SWEEP_ENGINE_H */
