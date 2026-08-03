#ifndef SCPI_PARSER_H
#define SCPI_PARSER_H

/* Call this once per complete line received over USB-CDC (i.e. once
 * you've buffered bytes up to a '\n' in usbd_cdc_if.c's
 * CDC_Receive_FS callback). line should NOT include the trailing
 * newline. See firmware/README.md for the exact integration point. */
void SCPI_ProcessLine(const char *line);

#endif /* SCPI_PARSER_H */
