#include "mqtt_publisher.h"
#include "telemetry_config.h"

#include <stdio.h>

#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "MQTT_PUBLISHER";

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_mqtt_connected = false;

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t) event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT conectado ao broker");
            s_mqtt_connected = true;
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT desconectado do broker");
            s_mqtt_connected = false;
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "Erro no cliente MQTT");
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "Mensagem MQTT publicada, msg_id=%d", event->msg_id);
            break;

        default:
            break;
    }
}

esp_err_t mqtt_publisher_start(void)
{
    if (s_mqtt_client != NULL) {
        ESP_LOGW(TAG, "Cliente MQTT ja inicializado");
        return ESP_OK;
    }

    esp_mqtt_client_config_t mqtt_config = {
        .broker.address.uri = MQTT_BROKER_URI,
    };

    ESP_LOGI(TAG, "Iniciando MQTT broker: %s", MQTT_BROKER_URI);

    s_mqtt_client = esp_mqtt_client_init(&mqtt_config);

    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "Falha ao inicializar cliente MQTT");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(
        s_mqtt_client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL
    ));

    return esp_mqtt_client_start(s_mqtt_client);
}

esp_err_t mqtt_publisher_publish(const char *topic, const char *payload)
{
    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "Cliente MQTT nao inicializado");
        return ESP_ERR_INVALID_STATE;
    }

    if (!s_mqtt_connected) {
        ESP_LOGW(TAG, "MQTT ainda nao conectado. Publicacao ignorada");
        return ESP_ERR_INVALID_STATE;
    }

    int msg_id = esp_mqtt_client_publish(
        s_mqtt_client,
        topic,
        payload,
        0,
        0,
        0
    );

    if (msg_id < 0) {
        ESP_LOGE(TAG, "Falha ao publicar mensagem MQTT");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Publicado em %s: %s", topic, payload);
    return ESP_OK;
}

esp_err_t mqtt_publisher_publish_status(const char *status)
{
    char payload[160];

    snprintf(
        payload,
        sizeof(payload),
        "{\"device\":\"esp32_gps\",\"status\":\"%s\",\"stage\":\"wifi_mqtt_test\"}",
        status
    );

    return mqtt_publisher_publish("telemetria/gps/status", payload);
}

bool mqtt_publisher_is_connected(void)
{
    return s_mqtt_connected;
}
