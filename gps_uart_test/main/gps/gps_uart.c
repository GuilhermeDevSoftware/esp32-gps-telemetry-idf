#include "gps_uart.h"

#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "GPS_UART";

void gps_uart_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = GPS_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(GPS_UART_PORT, 2048, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(GPS_UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(
        GPS_UART_PORT,
        GPS_UART_TX_PIN,
        GPS_UART_RX_PIN,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE
    ));

    ESP_LOGI(TAG, "UART GPS inicializada em %d bps | RX=%d | TX=%d",
             GPS_UART_BAUDRATE, GPS_UART_RX_PIN, GPS_UART_TX_PIN);
}

bool gps_uart_read_line(char *line, size_t max_len, TickType_t timeout_ticks)
{
    if (line == NULL || max_len == 0) {
        return false;
    }

    size_t index = 0;
    uint8_t byte = 0;

    while (index < max_len - 1) {
        int len = uart_read_bytes(GPS_UART_PORT, &byte, 1, timeout_ticks);

        if (len <= 0) {
            if (index == 0) {
                return false;
            }
            break;
        }

        if (byte == '\r') {
            continue;
        }

        if (byte == '\n') {
            break;
        }

        line[index++] = (char)byte;
    }

    line[index] = '\0';

    return index > 0;
}
