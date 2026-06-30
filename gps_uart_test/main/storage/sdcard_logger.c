#include "sdcard_logger.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"

#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"

static const char *TAG = "SDCARD";

static bool s_sdcard_ready = false;
static sdmmc_card_t *s_card = NULL;
static uint32_t s_lines_written = 0;

static bool file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

static bool write_csv_header_if_needed(void)
{
    bool exists = file_exists(SDCARD_TELEMETRY_FILE);

    FILE *file = fopen(SDCARD_TELEMETRY_FILE, exists ? "a" : "w");
    if (file == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo CSV para cabecalho");
        return false;
    }

    if (!exists) {
        fprintf(
            file,
            "utc,latitude,longitude,speed_kmh,max_speed_kmh,avg_speed_kmh,total_distance_m,status,stopped_time_s,moving_time_s,satellites,hdop\n"
        );
        fflush(file);
        ESP_LOGI(TAG, "Cabecalho CSV criado em %s", SDCARD_TELEMETRY_FILE);
    }

    fclose(file);
    return true;
}

bool sdcard_logger_init(void)
{
    ESP_LOGI(TAG, "Inicializando microSD via SPI");
    ESP_LOGI(TAG, "Pinos SPI | CS=%d | SCK=%d | MISO=%d | MOSI=%d",
             SDCARD_PIN_NUM_CS,
             SDCARD_PIN_NUM_CLK,
             SDCARD_PIN_NUM_MISO,
             SDCARD_PIN_NUM_MOSI);

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_PROBING;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SDCARD_PIN_NUM_MOSI,
        .miso_io_num = SDCARD_PIN_NUM_MISO,
        .sclk_io_num = SDCARD_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar barramento SPI: %s", esp_err_to_name(ret));
        s_sdcard_ready = false;
        return false;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SDCARD_PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount(
        SDCARD_MOUNT_POINT,
        &host,
        &slot_config,
        &mount_config,
        &s_card
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao montar microSD: %s", esp_err_to_name(ret));

        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Falha ao montar o sistema de arquivos. Verifique se o cartao esta em FAT32.");
        } else {
            ESP_LOGE(TAG, "Falha de comunicacao com o cartao. Verifique ligacoes SPI e alimentacao.");
        }

        spi_bus_free(host.slot);
        s_sdcard_ready = false;
        return false;
    }

    ESP_LOGI(TAG, "microSD montado com sucesso");
    sdmmc_card_print_info(stdout, s_card);

    if (!write_csv_header_if_needed()) {
        ESP_LOGE(TAG, "microSD montado, mas falhou ao preparar CSV");
        s_sdcard_ready = false;
        return false;
    }

    s_sdcard_ready = true;
    ESP_LOGI(TAG, "Arquivo de telemetria pronto: %s", SDCARD_TELEMETRY_FILE);

    return true;
}

bool sdcard_logger_is_ready(void)
{
    return s_sdcard_ready;
}

bool sdcard_logger_log(const gps_data_t *gps, const telemetry_data_t *telemetry)
{
    if (!s_sdcard_ready) {
        return false;
    }

    if (gps == NULL || telemetry == NULL) {
        return false;
    }

    if (!gps->has_fix) {
        return false;
    }

    FILE *file = fopen(SDCARD_TELEMETRY_FILE, "a");
    if (file == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo CSV para escrita");
        return false;
    }

    const char *status = telemetry->is_moving ? "MOVING" : "STOPPED";

    fprintf(
        file,
        "%s,%.6f,%.6f,%.2f,%.2f,%.2f,%.2f,%s,%lu,%lu,%d,%.2f\n",
        gps->utc_time,
        gps->latitude,
        gps->longitude,
        telemetry->current_speed_kmh,
        telemetry->max_speed_kmh,
        telemetry->average_speed_kmh,
        telemetry->total_distance_m,
        status,
        (unsigned long)telemetry->stopped_time_s,
        (unsigned long)telemetry->moving_time_s,
        gps->satellites,
        gps->hdop
    );

    fflush(file);
    fclose(file);

    s_lines_written++;

    if ((s_lines_written % 10) == 0) {
        ESP_LOGI(TAG, "Linhas gravadas no CSV: %lu", (unsigned long)s_lines_written);
    }

    return true;
}
