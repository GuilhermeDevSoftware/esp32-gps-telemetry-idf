#ifndef NMEA_PARSER_H
#define NMEA_PARSER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool has_fix;

    char utc_time[16];

    double latitude;
    double longitude;

    float speed_kmh;
    float altitude_m;
    float hdop;

    int satellites;
    int fix_quality;

    uint32_t valid_points;
    uint32_t last_update_ms;
} gps_data_t;

void gps_data_init(gps_data_t *gps);
bool nmea_parse_sentence(const char *sentence, gps_data_t *gps);

#endif
