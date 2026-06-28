#include "telemetry.h"

#include "esp_log.h"

static const char *TAG = "TELEMETRY";

void telemetry_log_gps_snapshot(const gps_data_t *gps)
{
    if (gps == NULL) {
        return;
    }

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "============== REGISTRO GPS REAL ===============");
    ESP_LOGI(TAG, "Fix valido: SIM");
    ESP_LOGI(TAG, "UTC: %s", gps->utc_time);
    ESP_LOGI(TAG, "Latitude: %.6f", gps->latitude);
    ESP_LOGI(TAG, "Longitude: %.6f", gps->longitude);
    ESP_LOGI(TAG, "Velocidade: %.2f km/h", gps->speed_kmh);
    ESP_LOGI(TAG, "Altitude: %.2f m", gps->altitude_m);
    ESP_LOGI(TAG, "Satelites: %d", gps->satellites);
    ESP_LOGI(TAG, "Qualidade do fix: %d", gps->fix_quality);
    ESP_LOGI(TAG, "HDOP: %.2f", gps->hdop);
    ESP_LOGI(TAG, "Pontos validos recebidos: %lu", (unsigned long)gps->valid_points);
    ESP_LOGI(TAG, "=================================================");
}

void telemetry_log_waiting_fix(const gps_data_t *gps)
{
    if (gps == NULL) {
        return;
    }

    ESP_LOGW(TAG, "Aguardando fix GPS | Satelites: %d | Fix quality: %d | HDOP: %.2f",
             gps->satellites,
             gps->fix_quality,
             gps->hdop);
}
