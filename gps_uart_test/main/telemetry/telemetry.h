#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdbool.h>
#include <stdint.h>
#include "nmea_parser.h"

#define TELEMETRY_MOVING_SPEED_THRESHOLD_KMH 5.0f
#define TELEMETRY_MIN_DISTANCE_M             5.0

typedef struct {
    float current_speed_kmh;
    float max_speed_kmh;
    float average_speed_kmh;

    double total_distance_m;

    uint32_t moving_time_s;
    uint32_t stopped_time_s;

    uint32_t valid_samples;

    bool is_moving;
    bool has_last_position;

    double last_latitude;
    double last_longitude;

    uint32_t last_sample_ms;

    char last_utc_time[16];
} telemetry_data_t;

void telemetry_init(telemetry_data_t *telemetry);
bool telemetry_update(telemetry_data_t *telemetry, const gps_data_t *gps);
void telemetry_log_status(const telemetry_data_t *telemetry, const gps_data_t *gps);

void telemetry_log_gps_snapshot(const gps_data_t *gps);
void telemetry_log_waiting_fix(const gps_data_t *gps);

#endif
