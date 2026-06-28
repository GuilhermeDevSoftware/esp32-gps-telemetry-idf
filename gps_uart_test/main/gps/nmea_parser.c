#include "nmea_parser.h"
#include "gps_uart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "esp_timer.h"

#define NMEA_MAX_FIELDS 24

static int split_nmea_fields(char *buffer, char **fields, int max_fields)
{
    int count = 0;

    char *checksum = strchr(buffer, '*');
    if (checksum != NULL) {
        *checksum = '\0';
    }

    fields[count++] = buffer;

    for (char *p = buffer; *p != '\0' && count < max_fields; p++) {
        if (*p == ',') {
            *p = '\0';
            fields[count++] = p + 1;
        }
    }

    return count;
}

static double nmea_coord_to_decimal(const char *coord, const char *hemisphere)
{
    if (coord == NULL || hemisphere == NULL || coord[0] == '\0') {
        return 0.0;
    }

    double raw = atof(coord);
    int degrees = (int)(raw / 100);
    double minutes = raw - (degrees * 100);
    double decimal = degrees + (minutes / 60.0);

    if (hemisphere[0] == 'S' || hemisphere[0] == 'W') {
        decimal *= -1.0;
    }

    return decimal;
}

static bool starts_with(const char *text, const char *prefix)
{
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

void gps_data_init(gps_data_t *gps)
{
    if (gps == NULL) {
        return;
    }

    memset(gps, 0, sizeof(gps_data_t));
    gps->has_fix = false;
}

static bool parse_gga(char **fields, int count, gps_data_t *gps)
{
    if (count < 10) {
        return false;
    }

    const char *utc_time = fields[1];
    const char *lat = fields[2];
    const char *lat_hemi = fields[3];
    const char *lon = fields[4];
    const char *lon_hemi = fields[5];
    const char *fix_quality = fields[6];
    const char *satellites = fields[7];
    const char *hdop = fields[8];
    const char *altitude = fields[9];

    int quality = atoi(fix_quality);

    if (utc_time[0] != '\0') {
        snprintf(gps->utc_time, sizeof(gps->utc_time), "%s", utc_time);
    }

    gps->fix_quality = quality;
    gps->satellites = atoi(satellites);
    gps->hdop = atof(hdop);
    gps->altitude_m = atof(altitude);

    if (quality > 0 && lat[0] != '\0' && lon[0] != '\0') {
        gps->latitude = nmea_coord_to_decimal(lat, lat_hemi);
        gps->longitude = nmea_coord_to_decimal(lon, lon_hemi);
        gps->has_fix = true;
        gps->valid_points++;
        gps->last_update_ms = (uint32_t)(esp_timer_get_time() / 1000);
        return true;
    }

    gps->has_fix = false;
    return true;
}

static bool parse_rmc(char **fields, int count, gps_data_t *gps)
{
    if (count < 8) {
        return false;
    }

    const char *utc_time = fields[1];
    const char *status = fields[2];
    const char *lat = fields[3];
    const char *lat_hemi = fields[4];
    const char *lon = fields[5];
    const char *lon_hemi = fields[6];
    const char *speed_knots = fields[7];

    if (utc_time[0] != '\0') {
        snprintf(gps->utc_time, sizeof(gps->utc_time), "%s", utc_time);
    }

    if (status[0] == 'A') {
        if (lat[0] != '\0' && lon[0] != '\0') {
            gps->latitude = nmea_coord_to_decimal(lat, lat_hemi);
            gps->longitude = nmea_coord_to_decimal(lon, lon_hemi);
        }

        gps->speed_kmh = atof(speed_knots) * 1.852f;
        gps->has_fix = true;
        gps->valid_points++;
        gps->last_update_ms = (uint32_t)(esp_timer_get_time() / 1000);
        return true;
    }

    gps->has_fix = false;
    return true;
}

static bool parse_vtg(char **fields, int count, gps_data_t *gps)
{
    if (count < 8) {
        return false;
    }

    const char *speed_kmh = fields[7];

    if (speed_kmh[0] != '\0') {
        gps->speed_kmh = atof(speed_kmh);
        gps->last_update_ms = (uint32_t)(esp_timer_get_time() / 1000);
        return true;
    }

    return false;
}

bool nmea_parse_sentence(const char *sentence, gps_data_t *gps)
{
    if (sentence == NULL || gps == NULL) {
        return false;
    }

    if (sentence[0] != '$') {
        return false;
    }

    char buffer[GPS_LINE_MAX_LEN];

    snprintf(buffer, sizeof(buffer), "%s", sentence);

    char *fields[NMEA_MAX_FIELDS] = {0};
    int count = split_nmea_fields(buffer, fields, NMEA_MAX_FIELDS);

    if (count <= 0) {
        return false;
    }

    if (starts_with(fields[0], "$GNGGA") || starts_with(fields[0], "$GPGGA")) {
        return parse_gga(fields, count, gps);
    }

    if (starts_with(fields[0], "$GNRMC") || starts_with(fields[0], "$GPRMC")) {
        return parse_rmc(fields, count, gps);
    }

    if (starts_with(fields[0], "$GNVTG") || starts_with(fields[0], "$GPVTG")) {
        return parse_vtg(fields, count, gps);
    }

    return false;
}
