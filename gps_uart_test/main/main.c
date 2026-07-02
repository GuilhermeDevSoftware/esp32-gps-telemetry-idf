#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

#include "wifi_manager.h"
#include "mqtt_publisher.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "gps_uart.h"
#include "nmea_parser.h"
#include "telemetry.h"
#include "sdcard_logger.h"

static const char *TAG = "GPS_APP";

#define GPS_LOG_INTERVAL_MS        5000
#define TELEMETRY_LOG_INTERVAL_MS  5000
#define NO_NMEA_LOG_INTERVAL_MS    5000
#define SERIAL_CMD_MAX_LEN         96

static bool is_rmc_sentence(const char *sentence)
{
    if (sentence == NULL) {
        return false;
    }

    return strstr(sentence, "RMC") != NULL;
}

static void console_input_init(void)
{
    setvbuf(stdin, NULL, _IONBF, 0);

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);

    if (flags >= 0) {
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }

    ESP_LOGI(TAG, "Entrada serial configurada em modo nao bloqueante");
}

static void check_serial_command(void)
{
    static char cmd_buffer[SERIAL_CMD_MAX_LEN];
    static size_t cmd_index = 0;

    char ch;

    while (read(STDIN_FILENO, &ch, 1) > 0) {
        if (ch == '\r' || ch == '\n') {
            if (cmd_index > 0) {
                cmd_buffer[cmd_index] = '\0';

                ESP_LOGI(TAG, "Comando recebido: %s", cmd_buffer);

                sdcard_logger_handle_command(cmd_buffer);

                cmd_index = 0;
                memset(cmd_buffer, 0, sizeof(cmd_buffer));
            }
        } else {
            if (cmd_index < SERIAL_CMD_MAX_LEN - 1) {
                cmd_buffer[cmd_index++] = ch;
            } else {
                cmd_index = 0;
                memset(cmd_buffer, 0, sizeof(cmd_buffer));

                ESP_LOGW(TAG, "Comando serial muito longo. Buffer reiniciado.");
            }
        }
    }
}

void app_main(void)
{

	ESP_LOGI("APP", "Inicializando Wi-Fi e MQTT...");

	if (wifi_manager_start() == ESP_OK) {
    		ESP_LOGI("APP", "Wi-Fi conectado. Iniciando MQTT...");

    	if (mqtt_publisher_start() == ESP_OK) {
        	vTaskDelay(pdMS_TO_TICKS(3000));
        	mqtt_publisher_publish_status("esp32_mqtt_ok");
    	} else {
        	ESP_LOGE("APP", "Falha ao iniciar MQTT");
    }
	} else {
    ESP_LOGE("APP", "Falha ao iniciar Wi-Fi");
}
    ESP_LOGI(TAG, "Inicializando projeto de telemetria GPS com ESP-IDF");
    ESP_LOGI(TAG, "Etapa atual: microSD logger com sessoes, status, list, export e clear seguro");

    console_input_init();

    gps_uart_init();

    gps_data_t gps;
    gps_data_init(&gps);

    telemetry_data_t telemetry;
    telemetry_init(&telemetry);

    bool sd_ready = sdcard_logger_init();

    if (sd_ready) {
        ESP_LOGI(TAG, "microSD pronto para gravacao de telemetria");
        ESP_LOGI(TAG, "Arquivo atual: %s", sdcard_logger_get_current_file());

        sdcard_logger_print_help();
    } else {
        ESP_LOGW(TAG, "microSD indisponivel. Sistema continuara apenas com logs no terminal.");
        ESP_LOGW(TAG, "Comandos do microSD ficarao indisponiveis.");
    }

    char nmea_line[GPS_LINE_MAX_LEN];

    int64_t last_gps_log_ms = 0;
    int64_t last_telemetry_log_ms = 0;
    int64_t last_waiting_log_ms = 0;
    int64_t last_no_nmea_log_ms = 0;

    while (1) {
        check_serial_command();

        bool received = gps_uart_read_line(
            nmea_line,
            sizeof(nmea_line),
            pdMS_TO_TICKS(250)
        );

        check_serial_command();

        int64_t now_ms = esp_timer_get_time() / 1000;

        if (!received) {
            if ((now_ms - last_no_nmea_log_ms) >= NO_NMEA_LOG_INTERVAL_MS) {
                ESP_LOGW(TAG, "Nenhuma linha NMEA recebida no intervalo");
                last_no_nmea_log_ms = now_ms;
            }

            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        ESP_LOGD(TAG, "NMEA: %s", nmea_line);

        bool parsed = nmea_parse_sentence(nmea_line, &gps);

        if (parsed && gps.has_fix) {
            if ((now_ms - last_gps_log_ms) >= GPS_LOG_INTERVAL_MS) {
                telemetry_log_gps_snapshot(&gps);
                last_gps_log_ms = now_ms;
            }

            if (is_rmc_sentence(nmea_line)) {
                bool telemetry_updated = telemetry_update(&telemetry, &gps);

                if (telemetry_updated) {
                    if (sdcard_logger_is_ready()) {
                        sdcard_logger_log(&gps, &telemetry);
                    }

                    if ((now_ms - last_telemetry_log_ms) >= TELEMETRY_LOG_INTERVAL_MS) {
                        telemetry_log_status(&telemetry, &gps);
                        last_telemetry_log_ms = now_ms;
                    }
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
