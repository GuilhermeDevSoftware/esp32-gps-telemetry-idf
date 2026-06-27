#ifndef GPS_DIAGNOSTIC_H
#define GPS_DIAGNOSTIC_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    GPS_STATUS_SEM_FIX = 0,
    GPS_STATUS_AGUARDANDO_SATELITES,
    GPS_STATUS_FIX_2D,
    GPS_STATUS_FIX_3D
} gps_status_t;

typedef struct
{
    uint32_t gga_count;
    uint32_t rmc_count;
    uint32_t vtg_count;
    uint32_t gsa_count;

    uint32_t ignored_count;
    uint32_t invalid_checksum_count;
    uint32_t valid_sentence_count;

    uint32_t uptime_seconds;

    int satellites;
    int fix_quality_gga;
    int fix_type_gsa;

    bool rmc_active;

    char utc_time[20];
    char last_valid_message[128];

    gps_status_t status;

} gps_diagnostic_t;

void gps_diagnostic_init(gps_diagnostic_t *diag);
bool gps_diagnostic_process_line(gps_diagnostic_t *diag, const char *line);
void gps_diagnostic_print(gps_diagnostic_t *diag, const char *tag);
const char *gps_status_to_string(gps_status_t status);

#endif
