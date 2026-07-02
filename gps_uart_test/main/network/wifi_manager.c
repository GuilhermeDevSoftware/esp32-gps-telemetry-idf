#include "wifi_manager.h"
#include "telemetry_config.h"

#include <string.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_MAX_RETRY     5

static const char *TAG = "WIFI_MANAGER";

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_count = 0;
static bool s_connected = false;

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Iniciando conexao Wi-Fi...");
        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_connected = false;

        if (s_retry_count < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_count++;
            ESP_LOGW(TAG, "Tentando reconectar ao Wi-Fi... tentativa %d/%d",
                     s_retry_count, WIFI_MAX_RETRY);
        } else {
            ESP_LOGE(TAG, "Falha ao conectar no Wi-Fi");
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    }

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;

        ESP_LOGI(TAG, "Wi-Fi conectado");
        ESP_LOGI(TAG, "IP obtido: " IPSTR, IP2STR(&event->ip_info.ip));

        s_retry_count = 0;
        s_connected = true;

        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_manager_start(void)
{
    s_wifi_event_group = xEventGroupCreate();

    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Erro ao criar event loop: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        NULL
    ));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL,
        NULL
    ));

    wifi_config_t wifi_config = {0};

    snprintf((char *) wifi_config.sta.ssid,
             sizeof(wifi_config.sta.ssid),
             "%s",
             WIFI_SSID);

    snprintf((char *) wifi_config.sta.password,
             sizeof(wifi_config.sta.password),
             "%s",
             WIFI_PASSWORD);

    ESP_LOGI(TAG, "Conectando ao SSID: %s", WIFI_SSID);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(15000)
    );

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Wi-Fi pronto para uso");
        return ESP_OK;
    }

    if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Nao foi possivel conectar ao Wi-Fi");
        return ESP_FAIL;
    }

    ESP_LOGE(TAG, "Timeout ao conectar no Wi-Fi");
    return ESP_ERR_TIMEOUT;
}

bool wifi_manager_is_connected(void)
{
    return s_connected;
}
