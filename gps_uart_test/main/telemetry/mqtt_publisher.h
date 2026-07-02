#ifndef MQTT_PUBLISHER_H
#define MQTT_PUBLISHER_H

#include "esp_err.h"
#include <stdbool.h>

esp_err_t mqtt_publisher_start(void);
esp_err_t mqtt_publisher_publish(const char *topic, const char *payload);
esp_err_t mqtt_publisher_publish_status(const char *status);
bool mqtt_publisher_is_connected(void);

#endif
