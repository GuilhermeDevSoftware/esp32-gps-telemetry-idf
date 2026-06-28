#ifndef GPS_UART_H
#define GPS_UART_H

#include <stdbool.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"

#define GPS_UART_PORT      UART_NUM_2
#define GPS_UART_TX_PIN    17
#define GPS_UART_RX_PIN    16
#define GPS_UART_BAUDRATE  9600
#define GPS_LINE_MAX_LEN   128

void gps_uart_init(void);
bool gps_uart_read_line(char *line, size_t max_len, TickType_t timeout_ticks);

#endif
