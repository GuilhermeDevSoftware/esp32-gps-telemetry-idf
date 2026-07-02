#ifndef MQTT_PUBLISHER_H
#define MQTT_PUBLISHER_H

#include "esp_err.h"
#include "gps_uart.h"
#include "telemetry.h"

esp_err_t mqtt_publisher_start(void);
esp_err_t mqtt_publisher_publish_status(const char *status);
esp_err_t mqtt_publisher_publish_gps_data(const gps_data_t *gps, const telemetry_data_t *telemetry);

#endif