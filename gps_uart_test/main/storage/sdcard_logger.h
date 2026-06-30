#ifndef SDCARD_LOGGER_H
#define SDCARD_LOGGER_H

#include <stdbool.h>

#include "nmea_parser.h"
#include "telemetry.h"

#define SDCARD_MOUNT_POINT       "/sdcard"
#define SDCARD_TELEMETRY_FILE    "/sdcard/telemetry.csv"

#define SDCARD_PIN_NUM_MISO      19
#define SDCARD_PIN_NUM_MOSI      23
#define SDCARD_PIN_NUM_CLK       18
#define SDCARD_PIN_NUM_CS        5

bool sdcard_logger_init(void);
bool sdcard_logger_is_ready(void);
bool sdcard_logger_log(const gps_data_t *gps, const telemetry_data_t *telemetry);

#endif
