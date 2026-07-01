#pragma once

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

bool sdcard_logger_session_init(void);
bool sdcard_logger_new_session(void);

const char *sdcard_logger_get_current_file(void);

bool sdcard_logger_export_csv_to_stdout(void);
bool sdcard_logger_export_last_to_stdout(void);
bool sdcard_logger_export_legacy_to_stdout(void);

void sdcard_logger_handle_command(const char *command);
void sdcard_logger_print_help(void);