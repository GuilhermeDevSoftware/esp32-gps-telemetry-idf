#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "nmea_parser.h"

void telemetry_log_gps_snapshot(const gps_data_t *gps);
void telemetry_log_waiting_fix(const gps_data_t *gps);

#endif
