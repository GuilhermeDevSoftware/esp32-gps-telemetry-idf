#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "esp_log.h"

#include "gps_parser.h"

#define GPS_UART_PORT UART_NUM_2

#define GPS_TX_PIN 17
#define GPS_RX_PIN 16

#define GPS_BAUD_RATE 9600

#define GPS_BUFFER_SIZE 1024
#define GPS_LINE_SIZE 128

static const char *TAG = "GPS_APP";

static gps_data_t gps_data;

static void gps_uart_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = GPS_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(
        GPS_UART_PORT,
        GPS_BUFFER_SIZE,
        0,
        0,
        NULL,
        0
    ));

    ESP_ERROR_CHECK(uart_param_config(GPS_UART_PORT, &uart_config));

    ESP_ERROR_CHECK(uart_set_pin(
        GPS_UART_PORT,
        GPS_TX_PIN,
        GPS_RX_PIN,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE
    ));

    ESP_LOGI(TAG, "UART GPS inicializada");
    ESP_LOGI(TAG, "GPS RX ESP32: GPIO %d", GPS_RX_PIN);
    ESP_LOGI(TAG, "GPS TX ESP32: GPIO %d", GPS_TX_PIN);
    ESP_LOGI(TAG, "Baud rate: %d", GPS_BAUD_RATE);
}

static void print_gps_status(void)
{
    if (gps_data.valid_fix)
    {
        ESP_LOGI(TAG, "========== GPS COM FIX ==========");
        ESP_LOGI(TAG, "Latitude: %.6f", gps_data.latitude);
        ESP_LOGI(TAG, "Longitude: %.6f", gps_data.longitude);
        ESP_LOGI(TAG, "Velocidade: %.2f km/h", gps_data.speed_kmh);
        ESP_LOGI(TAG, "Curso: %.2f graus", gps_data.course_deg);
        ESP_LOGI(TAG, "Satélites: %d", gps_data.satellites);
        ESP_LOGI(TAG, "Qualidade fix: %d", gps_data.fix_quality);
        ESP_LOGI(TAG, "HDOP: %.2f", gps_data.hdop);
        ESP_LOGI(TAG, "Altitude: %.2f m", gps_data.altitude_m);
        ESP_LOGI(TAG, "UTC: %s", gps_data.utc_time);
        ESP_LOGI(TAG, "Data: %s", gps_data.date);
        ESP_LOGI(TAG, "=================================");
    }
    else
    {
        ESP_LOGW(TAG, "GPS sem fix ainda");
        ESP_LOGW(TAG, "Satélites: %d | Qualidade fix: %d | UTC: %s",
                 gps_data.satellites,
                 gps_data.fix_quality,
                 gps_data.utc_time);
    }
}

static void gps_task(void *pvParameters)
{
    uint8_t byte;
    char line[GPS_LINE_SIZE];
    int line_index = 0;

    TickType_t last_status_time = xTaskGetTickCount();

    while (1)
    {
        int len = uart_read_bytes(
            GPS_UART_PORT,
            &byte,
            1,
            pdMS_TO_TICKS(100)
        );

        if (len > 0)
        {
            if (byte == '\n')
            {
                line[line_index] = '\0';

                if (line_index > 0)
                {
                    ESP_LOGI(TAG, "NMEA: %s", line);

                    bool parsed = gps_parse_nmea_sentence(line, &gps_data);

                    if (parsed)
                    {
                        TickType_t now = xTaskGetTickCount();

                        if ((now - last_status_time) >= pdMS_TO_TICKS(3000))
                        {
                            print_gps_status();
                            last_status_time = now;
                        }
                    }
                }

                line_index = 0;
                memset(line, 0, sizeof(line));
            }
            else
            {
                if (line_index < GPS_LINE_SIZE - 1)
                {
                    line[line_index++] = (char)byte;
                }
                else
                {
                    line_index = 0;
                    memset(line, 0, sizeof(line));
                    ESP_LOGW(TAG, "Linha NMEA muito grande. Buffer reiniciado.");
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Iniciando projeto de telemetria GPS");

    gps_data_init(&gps_data);
    gps_uart_init();

    xTaskCreate(
        gps_task,
        "gps_task",
        4096,
        NULL,
        5,
        NULL
    );
}