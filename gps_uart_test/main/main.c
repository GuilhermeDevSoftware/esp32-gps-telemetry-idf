#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#include "esp_log.h"

#define GPS_UART_PORT      UART_NUM_2
#define GPS_TX_PIN         GPIO_NUM_17
#define GPS_RX_PIN         GPIO_NUM_16
#define GPS_BAUD_RATE      9600

#define GPS_BUFFER_SIZE    1024

static const char *TAG = "GPS_UART";

static void gps_uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = GPS_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    ESP_ERROR_CHECK(uart_driver_install(
        GPS_UART_PORT,
        GPS_BUFFER_SIZE * 2,
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

    ESP_LOGI(TAG, "UART do GPS inicializada");
    ESP_LOGI(TAG, "RX GPIO16 | TX GPIO17 | Baud 9600");
}

static void gps_task(void *arg)
{
    uint8_t data[GPS_BUFFER_SIZE];

    while (1)
    {
        int len = uart_read_bytes(
            GPS_UART_PORT,
            data,
            GPS_BUFFER_SIZE - 1,
            pdMS_TO_TICKS(1000)
        );

        if (len > 0)
        {
            data[len] = '\0';
            printf("%s", (char *)data);
        }
        else
        {
            ESP_LOGW(TAG, "Nenhum dado recebido do GPS");
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Iniciando teste bruto do GPS via UART");

    gps_uart_init();

    BaseType_t task_created = xTaskCreate(
        gps_task,
        "gps_task",
        4096,
        NULL,
        5,
        NULL
    );

    if (task_created != pdPASS)
    {
        ESP_LOGE(TAG, "Falha ao criar a task do GPS");
        return;
    }

    ESP_LOGI(TAG, "Sistema iniciado. Aguardando dados do GPS...");

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "Sistema ativo");
    }
}