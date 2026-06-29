#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "gps_uart.h"
#include "nmea_parser.h"
#include "telemetry.h"

static const char *TAG = "GPS_APP";

#define GPS_LOG_INTERVAL_MS        5000
#define TELEMETRY_LOG_INTERVAL_MS  5000

static bool is_rmc_sentence(const char *sentence)
{
    if (sentence == NULL) {
        return false;
    }

    return strstr(sentence, "RMC") != NULL;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Inicializando projeto de telemetria GPS com ESP-IDF");
    ESP_LOGI(TAG, "Etapa atual: calculo de telemetria veicular embarcada");

    gps_uart_init();

    gps_data_t gps;
    gps_data_init(&gps);

    telemetry_data_t telemetry;
    telemetry_init(&telemetry);

    char nmea_line[GPS_LINE_MAX_LEN];

    int64_t last_gps_log_ms = 0;
    int64_t last_telemetry_log_ms = 0;
    int64_t last_waiting_log_ms = 0;

    while (1) {
        bool received = gps_uart_read_line(
            nmea_line,
            sizeof(nmea_line),
            pdMS_TO_TICKS(2500)
        );

        if (!received) {
            ESP_LOGW(TAG, "Nenhuma linha NMEA recebida no intervalo");
            continue;
        }

        ESP_LOGI(TAG, "NMEA: %s", nmea_line);

        bool parsed = nmea_parse_sentence(nmea_line, &gps);

        int64_t now_ms = esp_timer_get_time() / 1000;

        if (parsed && gps.has_fix) {
            if ((now_ms - last_gps_log_ms) >= GPS_LOG_INTERVAL_MS) {
                telemetry_log_gps_snapshot(&gps);
                last_gps_log_ms = now_ms;
            }

            if (is_rmc_sentence(nmea_line)) {
                bool telemetry_updated = telemetry_update(&telemetry, &gps);

                if (telemetry_updated &&
                    ((now_ms - last_telemetry_log_ms) >= TELEMETRY_LOG_INTERVAL_MS)) {
                    telemetry_log_status(&telemetry, &gps);
                    last_telemetry_log_ms = now_ms;
                }
            }
        } else {
            if ((now_ms - last_waiting_log_ms) >= GPS_LOG_INTERVAL_MS) {
                telemetry_log_waiting_fix(&gps);
                last_waiting_log_ms = now_ms;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
