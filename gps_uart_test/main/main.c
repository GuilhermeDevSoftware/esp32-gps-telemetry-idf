#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "gps_diagnostic.h"

#define GPS_UART_PORT UART_NUM_2
#define GPS_UART_BAUD_RATE 9600

#define GPS_RX_PIN GPIO_NUM_16
#define GPS_TX_PIN GPIO_NUM_17

#define UART_BUFFER_SIZE 1024
#define GPS_LINE_BUFFER_SIZE 180

#define GPS_DIAGNOSTIC_INTERVAL_US 3000000ULL

static const char *TAG = "GPS_APP";

static gps_diagnostic_t gps_diag;

static void gps_uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = GPS_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(GPS_UART_PORT, UART_BUFFER_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(GPS_UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(GPS_UART_PORT, GPS_TX_PIN, GPS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART GPS inicializada");
    ESP_LOGI(TAG, "Baud rate: %d", GPS_UART_BAUD_RATE);
    ESP_LOGI(TAG, "RX ESP32: GPIO %d", GPS_RX_PIN);
    ESP_LOGI(TAG, "TX ESP32: GPIO %d", GPS_TX_PIN);
}

static void gps_task(void *pvParameters)
{
    uint8_t uart_data[128];

    char line_buffer[GPS_LINE_BUFFER_SIZE];
    int line_index = 0;

    int64_t last_diagnostic_time = esp_timer_get_time();

    while (1)
    {
        int length = uart_read_bytes(
            GPS_UART_PORT,
            uart_data,
            sizeof(uart_data),
            pdMS_TO_TICKS(200));

        if (length > 0)
        {
            for (int i = 0; i < length; i++)
            {
                char c = (char)uart_data[i];

                if (c == '\n')
                {
                    if (line_index > 0)
                    {
                        line_buffer[line_index] = '\0';

                        if (line_buffer[0] == '$')
                        {
                            gps_diagnostic_process_line(&gps_diag, line_buffer);
                        }

                        line_index = 0;
                    }
                }
                else if (c != '\r')
                {
                    if (line_index < GPS_LINE_BUFFER_SIZE - 1)
                    {
                        line_buffer[line_index++] = c;
                    }
                    else
                    {
                        line_index = 0;
                    }
                }
            }
        }

        int64_t now = esp_timer_get_time();

        if ((now - last_diagnostic_time) >= GPS_DIAGNOSTIC_INTERVAL_US)
        {
            gps_diagnostic_print(&gps_diag, TAG);
            last_diagnostic_time = now;
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Iniciando Etapa 3 - Diagnostico tecnico do GPS");

    gps_diagnostic_init(&gps_diag);
    gps_uart_init();

    xTaskCreate(
        gps_task,
        "gps_task",
        4096,
        NULL,
        5,
        NULL);
}
