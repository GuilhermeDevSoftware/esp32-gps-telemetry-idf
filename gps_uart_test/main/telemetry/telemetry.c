#include "telemetry.h"

#include <math.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "TELEMETRY";

#define EARTH_RADIUS_M 6371000.0
#define DEG_TO_RAD     0.017453292519943295

static double degrees_to_radians(double degrees)
{
    return degrees * DEG_TO_RAD;
}

static double haversine_distance_m(double lat1, double lon1, double lat2, double lon2)
{
    double dlat = degrees_to_radians(lat2 - lat1);
    double dlon = degrees_to_radians(lon2 - lon1);

    double rlat1 = degrees_to_radians(lat1);
    double rlat2 = degrees_to_radians(lat2);

    double a = sin(dlat / 2.0) * sin(dlat / 2.0) +
               cos(rlat1) * cos(rlat2) *
               sin(dlon / 2.0) * sin(dlon / 2.0);

    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));

    return EARTH_RADIUS_M * c;
}

void telemetry_init(telemetry_data_t *telemetry)
{
    if (telemetry == NULL) {
        return;
    }

    memset(telemetry, 0, sizeof(telemetry_data_t));
    telemetry->is_moving = false;
    telemetry->has_last_position = false;
}

bool telemetry_update(telemetry_data_t *telemetry, const gps_data_t *gps)
{
    if (telemetry == NULL || gps == NULL) {
        return false;
    }

    if (!gps->has_fix) {
        return false;
    }

    if (gps->latitude == 0.0 && gps->longitude == 0.0) {
        return false;
    }

    if (strncmp(telemetry->last_utc_time, gps->utc_time, sizeof(telemetry->last_utc_time)) == 0) {
        return false;
    }

    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);

    telemetry->current_speed_kmh = gps->speed_kmh;

    if (gps->speed_kmh > telemetry->max_speed_kmh) {
        telemetry->max_speed_kmh = gps->speed_kmh;
    }

    bool moving_now = gps->speed_kmh > TELEMETRY_MOVING_SPEED_THRESHOLD_KMH;
    telemetry->is_moving = moving_now;

    if (telemetry->last_sample_ms != 0) {
        uint32_t delta_ms = now_ms - telemetry->last_sample_ms;
        uint32_t delta_s = delta_ms / 1000;

        if (delta_s == 0) {
            delta_s = 1;
        }

        if (delta_s > 10) {
            delta_s = 10;
        }

        if (moving_now) {
            telemetry->moving_time_s += delta_s;
        } else {
            telemetry->stopped_time_s += delta_s;
        }
    }

    if (telemetry->has_last_position) {
        double distance_m = haversine_distance_m(
            telemetry->last_latitude,
            telemetry->last_longitude,
            gps->latitude,
            gps->longitude
        );

        if (moving_now && distance_m >= TELEMETRY_MIN_DISTANCE_M) {
            telemetry->total_distance_m += distance_m;
        }
    }

    telemetry->last_latitude = gps->latitude;
    telemetry->last_longitude = gps->longitude;
    telemetry->has_last_position = true;

    telemetry->last_sample_ms = now_ms;
    telemetry->valid_samples++;

    snprintf(telemetry->last_utc_time, sizeof(telemetry->last_utc_time), "%s", gps->utc_time);

    if (telemetry->moving_time_s > 0) {
        telemetry->average_speed_kmh =
            (float)((telemetry->total_distance_m / telemetry->moving_time_s) * 3.6);
    } else {
        telemetry->average_speed_kmh = 0.0f;
    }

    return true;
}

void telemetry_log_status(const telemetry_data_t *telemetry, const gps_data_t *gps)
{
    if (telemetry == NULL || gps == NULL) {
        return;
    }

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "============== TELEMETRIA VEICULAR =============");
    ESP_LOGI(TAG, "Status: %s", telemetry->is_moving ? "EM MOVIMENTO" : "PARADO");
    ESP_LOGI(TAG, "UTC: %s", gps->utc_time);
    ESP_LOGI(TAG, "Latitude: %.6f", gps->latitude);
    ESP_LOGI(TAG, "Longitude: %.6f", gps->longitude);
    ESP_LOGI(TAG, "Velocidade atual: %.2f km/h", telemetry->current_speed_kmh);
    ESP_LOGI(TAG, "Velocidade maxima: %.2f km/h", telemetry->max_speed_kmh);
    ESP_LOGI(TAG, "Velocidade media: %.2f km/h", telemetry->average_speed_kmh);
    ESP_LOGI(TAG, "Distancia percorrida: %.2f m", telemetry->total_distance_m);
    ESP_LOGI(TAG, "Tempo parado: %lu s", (unsigned long)telemetry->stopped_time_s);
    ESP_LOGI(TAG, "Tempo em movimento: %lu s", (unsigned long)telemetry->moving_time_s);
    ESP_LOGI(TAG, "Amostras validas: %lu", (unsigned long)telemetry->valid_samples);
    ESP_LOGI(TAG, "Satelites: %d", gps->satellites);
    ESP_LOGI(TAG, "HDOP: %.2f", gps->hdop);
    ESP_LOGI(TAG, "=================================================");
}

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
