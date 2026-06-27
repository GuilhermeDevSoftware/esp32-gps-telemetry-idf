#include "gps_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_NMEA_TOKENS 25

static int split_csv_preserve_empty(char *str, char **tokens, int max_tokens)
{
    int count = 0;
    char *p = str;

    while (count < max_tokens)
    {
        tokens[count++] = p;

        char *comma = strchr(p, ',');
        if (comma == NULL)
        {
            break;
        }

        *comma = '\0';
        p = comma + 1;
    }

    return count;
}

static bool nmea_checksum_valid(const char *sentence)
{
    if (sentence == NULL || sentence[0] != '$')
    {
        return false;
    }

    const char *asterisk = strchr(sentence, '*');

    if (asterisk == NULL)
    {
        return true;
    }

    uint8_t checksum = 0;

    for (const char *p = sentence + 1; p < asterisk; p++)
    {
        checksum ^= (uint8_t)(*p);
    }

    if (strlen(asterisk) < 3)
    {
        return false;
    }

    char checksum_text[3] = {
        asterisk[1],
        asterisk[2],
        '\0'
    };

    uint8_t expected = (uint8_t)strtol(checksum_text, NULL, 16);

    return checksum == expected;
}

static double nmea_coordinate_to_decimal(const char *value, const char *direction)
{
    if (value == NULL || direction == NULL)
    {
        return 0.0;
    }

    if (value[0] == '\0' || direction[0] == '\0')
    {
        return 0.0;
    }

    double raw = atof(value);

    int degrees = (int)(raw / 100.0);
    double minutes = raw - (degrees * 100.0);

    double decimal = degrees + (minutes / 60.0);

    if (direction[0] == 'S' || direction[0] == 'W')
    {
        decimal *= -1.0;
    }

    return decimal;
}

void gps_data_init(gps_data_t *gps)
{
    if (gps == NULL)
    {
        return;
    }

    memset(gps, 0, sizeof(gps_data_t));
}

static bool parse_rmc(char **tokens, int count, gps_data_t *gps)
{
    if (count < 10)
    {
        return false;
    }

    gps->rmc_received = true;

    snprintf(gps->utc_time, sizeof(gps->utc_time), "%s", tokens[1]);
    snprintf(gps->date, sizeof(gps->date), "%s", tokens[9]);

    char status = tokens[2][0];

    if (status == 'A')
    {
        gps->valid_fix = true;

        gps->latitude = nmea_coordinate_to_decimal(tokens[3], tokens[4]);
        gps->longitude = nmea_coordinate_to_decimal(tokens[5], tokens[6]);

        double speed_knots = atof(tokens[7]);
        gps->speed_kmh = speed_knots * 1.852;

        gps->course_deg = atof(tokens[8]);
    }
    else
    {
        gps->valid_fix = false;
    }

    return true;
}

static bool parse_gga(char **tokens, int count, gps_data_t *gps)
{
    if (count < 10)
    {
        return false;
    }

    gps->gga_received = true;

    snprintf(gps->utc_time, sizeof(gps->utc_time), "%s", tokens[1]);

    gps->fix_quality = atoi(tokens[6]);
    gps->satellites = atoi(tokens[7]);
    gps->hdop = atof(tokens[8]);
    gps->altitude_m = atof(tokens[9]);

    if (gps->fix_quality > 0)
    {
        gps->valid_fix = true;

        gps->latitude = nmea_coordinate_to_decimal(tokens[2], tokens[3]);
        gps->longitude = nmea_coordinate_to_decimal(tokens[4], tokens[5]);
    }

    return true;
}

bool gps_parse_nmea_sentence(const char *sentence, gps_data_t *gps)
{
    if (sentence == NULL || gps == NULL)
    {
        return false;
    }

    if (sentence[0] != '$')
    {
        return false;
    }

    if (!nmea_checksum_valid(sentence))
    {
        return false;
    }

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%s", sentence);

    char *newline = strchr(buffer, '\n');
    if (newline != NULL)
    {
        *newline = '\0';
    }

    char *carriage = strchr(buffer, '\r');
    if (carriage != NULL)
    {
        *carriage = '\0';
    }

    char *asterisk = strchr(buffer, '*');
    if (asterisk != NULL)
    {
        *asterisk = '\0';
    }

    char *tokens[MAX_NMEA_TOKENS] = {0};
    int count = split_csv_preserve_empty(buffer, tokens, MAX_NMEA_TOKENS);

    if (count <= 0)
    {
        return false;
    }

    if (strstr(tokens[0], "RMC") != NULL)
    {
        return parse_rmc(tokens, count, gps);
    }

    if (strstr(tokens[0], "GGA") != NULL)
    {
        return parse_gga(tokens, count, gps);
    }

    return false;
}