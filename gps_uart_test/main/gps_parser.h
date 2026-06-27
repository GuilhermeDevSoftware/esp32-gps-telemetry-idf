#ifndef GPS_PARSER_H
#define GPS_PARSER_H

#include <stdbool.h>

typedef struct
{
    bool rmc_received;
    bool gga_received;

    bool valid_fix;

    double latitude;
    double longitude;

    double speed_kmh;
    double course_deg;

    int satellites;
    int fix_quality;

    float hdop;
    float altitude_m;

    char utc_time[20];
    char date[20];

} gps_data_t;

void gps_data_init(gps_data_t *gps);
bool gps_parse_nmea_sentence(const char *sentence, gps_data_t *gps);

#endif