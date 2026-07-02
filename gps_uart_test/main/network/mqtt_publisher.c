#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "mqtt_publisher.h"

#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;

/*
 * Broker MQTT do projeto.
 * Ajuste este IP se o IP do seu broker mudar.
 */
#define MQTT_BROKER_URI "mqtt://192.168.56.160:1883"

#define MQTT_TOPIC_STATUS "telemetria/gps/status"
#define MQTT_TOPIC_DATA   "telemetria/gps/data"

#define MQTT_MOVING_SPEED_THRESHOLD_KMH 5.0

static void mqtt_event_handler(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data
)
{
    (void)handler_args;
    (void)base;

    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            mqtt_connected = true;
            ESP_LOGI(TAG, "MQTT conectado ao broker");
            break;

        case MQTT_EVENT_DISCONNECTED:
            mqtt_connected = false;
            ESP_LOGW(TAG, "MQTT desconectado do broker");
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "Mensagem MQTT publicada. msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_ERROR:
            mqtt_connected = false;
            ESP_LOGE(TAG, "Erro no cliente MQTT");
            break;

        default:
            ESP_LOGD(TAG, "Evento MQTT recebido: %ld", (long)event_id);
            break;
    }
}

esp_err_t mqtt_publisher_start(void)
{
    if (mqtt_client != NULL) {
        ESP_LOGW(TAG, "Cliente MQTT ja foi inicializado");
        return ESP_OK;
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "Falha ao inicializar cliente MQTT");
        return ESP_FAIL;
    }

    esp_err_t err = esp_mqtt_client_register_event(
        mqtt_client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao registrar eventos MQTT: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_mqtt_client_start(mqtt_client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar cliente MQTT: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Cliente MQTT iniciado. Broker: %s", MQTT_BROKER_URI);

    return ESP_OK;
}

esp_err_t mqtt_publisher_publish_status(const char *status)
{
    if (status == NULL) {
        status = "unknown";
    }

    if (!mqtt_connected || mqtt_client == NULL) {
        ESP_LOGW(TAG, "MQTT nao conectado. Status nao publicado.");
        return ESP_FAIL;
    }

    char payload[256];

    snprintf(payload, sizeof(payload),
        "{"
        "\"device\":\"esp32_gps\","
        "\"status\":\"%s\","
        "\"stage\":\"gps_mqtt_data_test\""
        "}",
        status
    );

    int msg_id = esp_mqtt_client_publish(
        mqtt_client,
        MQTT_TOPIC_STATUS,
        payload,
        0,
        0,
        0
    );

    if (msg_id < 0) {
        ESP_LOGE(TAG, "Falha ao publicar status MQTT");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Status publicado em %s. msg_id=%d", MQTT_TOPIC_STATUS, msg_id);
    ESP_LOGI(TAG, "Payload: %s", payload);

    return ESP_OK;
}

esp_err_t mqtt_publisher_publish_gps_data(const gps_data_t *gps, const telemetry_data_t *telemetry)
{
    if (gps == NULL || telemetry == NULL) {
        ESP_LOGW(TAG, "Dados GPS/telemetria invalidos");
        return ESP_ERR_INVALID_ARG;
    }

    if (!mqtt_connected || mqtt_client == NULL) {
        ESP_LOGW(TAG, "MQTT nao conectado. Dados GPS nao publicados.");
        return ESP_FAIL;
    }

    /*
     * Regra do projeto:
     * abaixo de 5 km/h = STOPPED
     * igual ou acima de 5 km/h = MOVING
     */
    const char *status = "STOPPED";

    if (gps->speed_kmh >= MQTT_MOVING_SPEED_THRESHOLD_KMH) {
        status = "MOVING";
    }

    /*
     * O telemetry_data_t atual nao possui avg_speed_kmh.
     * Entao calculamos a media em movimento:
     *
     * velocidade media = distancia / tempo
     * m/s para km/h = multiplicar por 3.6
     */
    double avg_speed_kmh = 0.0;

    if (telemetry->moving_time_s > 0) {
        avg_speed_kmh = (telemetry->total_distance_m / (double)telemetry->moving_time_s) * 3.6;
    }

    char payload[512];

    snprintf(payload, sizeof(payload),
        "{"
        "\"device\":\"esp32_gps\","
        "\"latitude\":%.6f,"
        "\"longitude\":%.6f,"
        "\"speed_kmh\":%.2f,"
        "\"max_speed_kmh\":%.2f,"
        "\"avg_speed_kmh\":%.2f,"
        "\"total_distance_m\":%.2f,"
        "\"status\":\"%s\","
        "\"stopped_time_s\":%lu,"
        "\"moving_time_s\":%lu,"
        "\"satellites\":%d,"
        "\"hdop\":%.2f"
        "}",
        gps->latitude,
        gps->longitude,
        gps->speed_kmh,
        telemetry->max_speed_kmh,
        avg_speed_kmh,
        telemetry->total_distance_m,
        status,
        (unsigned long)telemetry->stopped_time_s,
        (unsigned long)telemetry->moving_time_s,
        gps->satellites,
        gps->hdop
    );

    int msg_id = esp_mqtt_client_publish(
        mqtt_client,
        MQTT_TOPIC_DATA,
        payload,
        0,
        0,
        0
    );

    if (msg_id < 0) {
        ESP_LOGE(TAG, "Falha ao publicar dados GPS MQTT");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "GPS publicado em %s. msg_id=%d", MQTT_TOPIC_DATA, msg_id);
    ESP_LOGI(TAG, "Payload: %s", payload);

    return ESP_OK;
}