#include "sdcard_logger.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"

#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"

static const char *TAG = "SDCARD";

#define SDCARD_SESSION_PREFIX      "session_"
#define SDCARD_SESSION_EXTENSION   ".csv"
#define SDCARD_SESSION_PATH_MAX    96
#define SDCARD_COMMAND_MAX         96
#define SDCARD_MAX_SESSIONS        128

/*
 * Arquivo antigo do projeto.
 * Mantido apenas para export legacy.
 *
 * Normalmente vem do sdcard_logger.h:
 * #define SDCARD_TELEMETRY_FILE "/sdcard/telemetry.csv"
 */
#define SDCARD_LEGACY_FILE         SDCARD_TELEMETRY_FILE

#define SDCARD_CSV_HEADER \
    "utc,latitude,longitude,speed_kmh,max_speed_kmh,avg_speed_kmh,total_distance_m,status,stopped_time_s,moving_time_s,satellites,hdop\n"

static bool s_sdcard_ready = false;
static sdmmc_card_t *s_card = NULL;

static char s_current_file[SDCARD_SESSION_PATH_MAX] = {0};
static char s_last_file[SDCARD_SESSION_PATH_MAX] = {0};

static int s_current_session_number = 0;
static uint32_t s_lines_written = 0;
static bool s_session_ready = false;

static bool file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

static long get_file_size_bytes(const char *path)
{
    struct stat st;

    if (stat(path, &st) != 0) {
        return -1;
    }

    return (long)st.st_size;
}

static void make_session_path(int session_number, char *out, size_t out_size)
{
    snprintf(
        out,
        out_size,
        "%s/%s%03d%s",
        SDCARD_MOUNT_POINT,
        SDCARD_SESSION_PREFIX,
        session_number,
        SDCARD_SESSION_EXTENSION
    );
}

static bool parse_session_filename(const char *filename, int *session_number)
{
    if (filename == NULL) {
        return false;
    }

    size_t prefix_len = strlen(SDCARD_SESSION_PREFIX);

    if (strncmp(filename, SDCARD_SESSION_PREFIX, prefix_len) != 0) {
        return false;
    }

    const char *number_start = filename + prefix_len;
    char *end_ptr = NULL;

    long value = strtol(number_start, &end_ptr, 10);

    if (value <= 0) {
        return false;
    }

    if (strcmp(end_ptr, SDCARD_SESSION_EXTENSION) != 0) {
        return false;
    }

    if (session_number != NULL) {
        *session_number = (int)value;
    }

    return true;
}

static bool write_csv_header(const char *path)
{
    FILE *file = fopen(path, "w");

    if (file == NULL) {
        ESP_LOGE(TAG, "Falha ao criar arquivo CSV: %s | errno=%d", path, errno);
        return false;
    }

    fprintf(file, "%s", SDCARD_CSV_HEADER);
    fflush(file);
    fclose(file);

    ESP_LOGI(TAG, "Cabecalho CSV criado em %s", path);

    return true;
}

static uint32_t count_data_lines(const char *path)
{
    FILE *file = fopen(path, "r");

    if (file == NULL) {
        return 0;
    }

    char line[256];
    bool first_line = true;
    uint32_t count = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        if (first_line) {
            first_line = false;
            continue;
        }

        if (line[0] != '\0' && line[0] != '\n' && line[0] != '\r') {
            count++;
        }
    }

    fclose(file);

    return count;
}

static int find_highest_session_number(void)
{
    DIR *dir = opendir(SDCARD_MOUNT_POINT);

    if (dir == NULL) {
        ESP_LOGW(TAG, "Nao foi possivel abrir diretorio %s", SDCARD_MOUNT_POINT);
        return 0;
    }

    struct dirent *entry;
    int highest = 0;

    while ((entry = readdir(dir)) != NULL) {
        int number = 0;

        if (parse_session_filename(entry->d_name, &number)) {
            if (number > highest) {
                highest = number;
            }
        }
    }

    closedir(dir);

    return highest;
}

static int find_previous_session_number(int current_session_number)
{
    DIR *dir = opendir(SDCARD_MOUNT_POINT);

    if (dir == NULL) {
        return 0;
    }

    struct dirent *entry;
    int previous = 0;

    while ((entry = readdir(dir)) != NULL) {
        int number = 0;

        if (parse_session_filename(entry->d_name, &number)) {
            if (number < current_session_number && number > previous) {
                previous = number;
            }
        }
    }

    closedir(dir);

    return previous;
}

static bool create_session(int session_number)
{
    char new_file[SDCARD_SESSION_PATH_MAX] = {0};

    make_session_path(session_number, new_file, sizeof(new_file));

    if (!write_csv_header(new_file)) {
        return false;
    }

    strncpy(s_current_file, new_file, sizeof(s_current_file) - 1);
    s_current_file[sizeof(s_current_file) - 1] = '\0';

    s_current_session_number = session_number;
    s_lines_written = 0;
    s_session_ready = true;

    int previous = find_previous_session_number(s_current_session_number);

    if (previous > 0) {
        make_session_path(previous, s_last_file, sizeof(s_last_file));
    } else {
        s_last_file[0] = '\0';
    }

    ESP_LOGI(TAG, "Sessao atual: %s", s_current_file);

    if (s_last_file[0] != '\0') {
        ESP_LOGI(TAG, "Sessao anterior: %s", s_last_file);
    } else {
        ESP_LOGI(TAG, "Nenhuma sessao anterior encontrada");
    }

    return true;
}

bool sdcard_logger_new_session(void)
{
    if (!s_sdcard_ready) {
        ESP_LOGW(TAG, "microSD indisponivel. Nao foi possivel criar nova sessao.");
        return false;
    }

    if (s_current_file[0] != '\0') {
        strncpy(s_last_file, s_current_file, sizeof(s_last_file) - 1);
        s_last_file[sizeof(s_last_file) - 1] = '\0';
    }

    int highest = find_highest_session_number();
    int new_session = highest + 1;

    return create_session(new_session);
}

bool sdcard_logger_session_init(void)
{
    if (!s_sdcard_ready) {
        ESP_LOGW(TAG, "microSD indisponivel. Sessao nao inicializada.");
        return false;
    }

    if (s_session_ready) {
        return true;
    }

    int highest = find_highest_session_number();
    int new_session = highest + 1;

    return create_session(new_session);
}

const char *sdcard_logger_get_current_file(void)
{
    if (!s_session_ready) {
        sdcard_logger_session_init();
    }

    return s_current_file;
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
        .max_files = 8,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    /*
     * Velocidade reduzida para melhorar estabilidade com modulos microSD,
     * jumpers e montagem em bancada.
     *
     * Sem essa reducao, alguns cartoes podem falhar com:
     * ESP_ERR_INVALID_CRC
     */
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

    s_sdcard_ready = true;

    if (!sdcard_logger_session_init()) {
        ESP_LOGE(TAG, "microSD montado, mas falhou ao criar sessao CSV");
        s_sdcard_ready = false;
        return false;
    }

    ESP_LOGI(TAG, "Arquivo de telemetria atual pronto: %s", s_current_file);

    if (file_exists(SDCARD_LEGACY_FILE)) {
        ESP_LOGI(TAG, "Arquivo legacy encontrado: %s", SDCARD_LEGACY_FILE);
    }

    return true;
}

bool sdcard_logger_is_ready(void)
{
    return s_sdcard_ready && s_session_ready;
}

bool sdcard_logger_log(const gps_data_t *gps, const telemetry_data_t *telemetry)
{
    if (!sdcard_logger_is_ready()) {
        return false;
    }

    if (gps == NULL || telemetry == NULL) {
        return false;
    }

    if (!gps->has_fix) {
        return false;
    }

    if (gps->utc_time[0] == '\0') {
        return false;
    }

    FILE *file = fopen(s_current_file, "a");

    if (file == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo CSV para escrita: %s", s_current_file);
        return false;
    }

    const char *status = telemetry->is_moving ? "MOVING" : "STOPPED";

    int written = fprintf(
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

    if (written < 0) {
        ESP_LOGE(TAG, "Erro ao escrever linha no CSV");
        fclose(file);
        return false;
    }

    fflush(file);
    fclose(file);

    s_lines_written++;

    if ((s_lines_written % 10) == 0) {
        ESP_LOGI(
            TAG,
            "Linhas gravadas na sessao atual: %lu | arquivo: %s",
            (unsigned long)s_lines_written,
            s_current_file
        );
    }

    return true;
}

static bool export_file_to_stdout(const char *path, const char *label)
{
    if (!sdcard_logger_is_ready()) {
        ESP_LOGW(TAG, "microSD indisponivel. Nao foi possivel exportar CSV.");
        return false;
    }

    if (path == NULL || path[0] == '\0') {
        ESP_LOGW(TAG, "Arquivo invalido para exportacao");
        return false;
    }

    FILE *file = fopen(path, "r");

    if (file == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir %s para exportacao", path);
        return false;
    }

    ESP_LOGI(TAG, "Iniciando exportacao serial de %s", path);

    printf("\n---CSV_BEGIN_%s---\n", label);
    printf("# FILE: %s\n", path);

    char line[256];

    while (fgets(line, sizeof(line), file) != NULL) {
        printf("%s", line);
    }

    printf("---CSV_END_%s---\n", label);
    fflush(stdout);

    fclose(file);

    ESP_LOGI(TAG, "Exportacao CSV finalizada");

    return true;
}

bool sdcard_logger_export_csv_to_stdout(void)
{
    return export_file_to_stdout(s_current_file, "CURRENT");
}

bool sdcard_logger_export_last_to_stdout(void)
{
    if (!sdcard_logger_is_ready()) {
        return false;
    }

    if (s_last_file[0] == '\0') {
        int previous = find_previous_session_number(s_current_session_number);

        if (previous > 0) {
            make_session_path(previous, s_last_file, sizeof(s_last_file));
        }
    }

    if (s_last_file[0] == '\0') {
        ESP_LOGW(TAG, "Nao existe sessao anterior para exportar");
        printf("\nNao existe sessao anterior para exportar.\n\n");
        return false;
    }

    return export_file_to_stdout(s_last_file, "LAST");
}

bool sdcard_logger_export_legacy_to_stdout(void)
{
    if (!sdcard_logger_is_ready()) {
        return false;
    }

    if (!file_exists(SDCARD_LEGACY_FILE)) {
        ESP_LOGW(TAG, "Arquivo legacy nao encontrado: %s", SDCARD_LEGACY_FILE);
        printf("\nArquivo legacy nao encontrado: %s\n\n", SDCARD_LEGACY_FILE);
        return false;
    }

    return export_file_to_stdout(SDCARD_LEGACY_FILE, "LEGACY");
}

static void print_status(void)
{
    if (!sdcard_logger_is_ready()) {
        printf("\nMicroSD indisponivel.\n\n");
        return;
    }

    uint32_t current_lines = count_data_lines(s_current_file);
    long current_size = get_file_size_bytes(s_current_file);

    printf("\n");
    printf("========== STATUS MICROSD ==========\n");
    printf("Arquivo atual : %s\n", s_current_file);
    printf("Sessao atual  : %03d\n", s_current_session_number);
    printf("Linhas        : %lu\n", (unsigned long)current_lines);

    if (current_size >= 0) {
        printf("Tamanho       : %ld bytes\n", current_size);
    } else {
        printf("Tamanho       : indisponivel\n");
    }

    if (s_last_file[0] != '\0') {
        uint32_t last_lines = count_data_lines(s_last_file);
        long last_size = get_file_size_bytes(s_last_file);

        printf("Ultimo arquivo: %s\n", s_last_file);
        printf("Linhas ultimo : %lu\n", (unsigned long)last_lines);

        if (last_size >= 0) {
            printf("Tamanho ultimo: %ld bytes\n", last_size);
        } else {
            printf("Tamanho ultimo: indisponivel\n");
        }
    } else {
        printf("Ultimo arquivo: nenhum\n");
    }

    if (file_exists(SDCARD_LEGACY_FILE)) {
        long legacy_size = get_file_size_bytes(SDCARD_LEGACY_FILE);
        uint32_t legacy_lines = count_data_lines(SDCARD_LEGACY_FILE);

        printf("Legacy        : %s\n", SDCARD_LEGACY_FILE);
        printf("Linhas legacy : %lu\n", (unsigned long)legacy_lines);

        if (legacy_size >= 0) {
            printf("Tamanho legacy: %ld bytes\n", legacy_size);
        }
    }

    printf("====================================\n\n");
}

static void sort_sessions(int *sessions, int count)
{
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (sessions[j] < sessions[i]) {
                int temp = sessions[i];
                sessions[i] = sessions[j];
                sessions[j] = temp;
            }
        }
    }
}

static void list_sessions(void)
{
    if (!sdcard_logger_is_ready()) {
        printf("\nMicroSD indisponivel.\n\n");
        return;
    }

    DIR *dir = opendir(SDCARD_MOUNT_POINT);

    if (dir == NULL) {
        printf("\nErro ao abrir diretorio %s\n\n", SDCARD_MOUNT_POINT);
        return;
    }

    int sessions[SDCARD_MAX_SESSIONS];
    int count = 0;

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        int number = 0;

        if (parse_session_filename(entry->d_name, &number)) {
            if (count < SDCARD_MAX_SESSIONS) {
                sessions[count] = number;
                count++;
            }
        }
    }

    closedir(dir);

    sort_sessions(sessions, count);

    printf("\n");
    printf("========== SESSOES SALVAS ==========\n");

    if (count == 0) {
        printf("Nenhuma sessao encontrada.\n");
    }

    for (int i = 0; i < count; i++) {
        char path[SDCARD_SESSION_PATH_MAX] = {0};

        make_session_path(sessions[i], path, sizeof(path));

        uint32_t lines = count_data_lines(path);
        long size = get_file_size_bytes(path);

        printf(
            "session_%03d.csv | %lu linhas | ",
            sessions[i],
            (unsigned long)lines
        );

        if (size >= 0) {
            printf("%ld bytes", size);
        } else {
            printf("tamanho indisponivel");
        }

        if (strcmp(path, s_current_file) == 0) {
            printf("  <- atual");
        } else if (strcmp(path, s_last_file) == 0) {
            printf("  <- anterior");
        }

        printf("\n");
    }

    if (file_exists(SDCARD_LEGACY_FILE)) {
        uint32_t legacy_lines = count_data_lines(SDCARD_LEGACY_FILE);
        long legacy_size = get_file_size_bytes(SDCARD_LEGACY_FILE);

        printf("\nLegacy encontrado:\n");
        printf(
            "telemetry.csv | %lu linhas | ",
            (unsigned long)legacy_lines
        );

        if (legacy_size >= 0) {
            printf("%ld bytes\n", legacy_size);
        } else {
            printf("tamanho indisponivel\n");
        }
    }

    printf("====================================\n\n");
}

static bool clear_current_session(void)
{
    if (!sdcard_logger_is_ready()) {
        printf("\nMicroSD indisponivel.\n\n");
        return false;
    }

    if (!write_csv_header(s_current_file)) {
        printf("\nFalha ao limpar sessao atual.\n\n");
        return false;
    }

    s_lines_written = 0;

    printf("\n");
    printf("Sessao atual limpa com seguranca.\n");
    printf("Arquivo mantido : %s\n", s_current_file);
    printf("Cabecalho recriado.\n\n");

    return true;
}

static void normalize_command(const char *input, char *output, size_t output_size)
{
    if (input == NULL || output == NULL || output_size == 0) {
        return;
    }

    snprintf(output, output_size, "%s", input);

    size_t len = strlen(output);

    while (len > 0 &&
           (output[len - 1] == '\n' ||
            output[len - 1] == '\r' ||
            output[len - 1] == ' ' ||
            output[len - 1] == '\t')) {
        output[len - 1] = '\0';
        len--;
    }

    char *start = output;

    while (*start == ' ' || *start == '\t') {
        start++;
    }

    if (start != output) {
        memmove(output, start, strlen(start) + 1);
    }

    for (size_t i = 0; output[i] != '\0'; i++) {
        output[i] = (char)tolower((unsigned char)output[i]);
    }
}

static bool parse_export_session_command(const char *cmd, int *session_number)
{
    if (cmd == NULL || session_number == NULL) {
        return false;
    }

    const char *number_text = NULL;

    if (strncmp(cmd, "export session ", 15) == 0) {
        number_text = cmd + 15;
    } else if (strncmp(cmd, "export ", 7) == 0) {
        number_text = cmd + 7;
    } else {
        return false;
    }

    if (number_text[0] == '\0') {
        return false;
    }

    for (size_t i = 0; number_text[i] != '\0'; i++) {
        if (!isdigit((unsigned char)number_text[i])) {
            return false;
        }
    }

    int value = atoi(number_text);

    if (value <= 0) {
        return false;
    }

    *session_number = value;
    return true;
}

static bool sdcard_logger_export_session_to_stdout(int session_number)
{
    if (!sdcard_logger_is_ready()) {
        ESP_LOGW(TAG, "microSD indisponivel. Nao foi possivel exportar sessao.");
        return false;
    }

    if (session_number <= 0) {
        printf("\nSessao invalida: %d\n\n", session_number);
        return false;
    }

    char path[SDCARD_SESSION_PATH_MAX] = {0};

    make_session_path(session_number, path, sizeof(path));

    if (!file_exists(path)) {
        printf("\nSessao nao encontrada: %s\n\n", path);
        return false;
    }

    char label[32];

    snprintf(label, sizeof(label), "SESSION_%03d", session_number);

    return export_file_to_stdout(path, label);
}

void sdcard_logger_print_help(void)
{
    printf("\n");
    printf("========== COMANDOS MICROSD ==========\n");
    printf("status             -> mostra arquivo atual, ultimo arquivo, linhas e tamanho\n");
    printf("list               -> lista sessoes salvas no microSD\n");
    printf("new                -> cria nova sessao manualmente\n");
    printf("export             -> exporta sessao atual\n");
    printf("export last        -> exporta a sessao anterior\n");
    printf("export legacy      -> exporta o antigo /sdcard/telemetry.csv\n");
    printf("export 009         -> exporta uma sessao especifica\n");
    printf("export session 009 -> exporta uma sessao especifica\n");
    printf("clear              -> nao apaga; apenas avisa\n");
    printf("clear confirm      -> limpa somente a sessao atual, com cabecalho novo\n");
    printf("help               -> mostra esta ajuda\n");
    printf("======================================\n\n");
}

void sdcard_logger_handle_command(const char *command)
{
    char cmd[SDCARD_COMMAND_MAX] = {0};

    normalize_command(command, cmd, sizeof(cmd));

    if (cmd[0] == '\0') {
        return;
    }

    if (strcmp(cmd, "status") == 0) {
        print_status();
    } else if (strcmp(cmd, "list") == 0) {
        list_sessions();
    } else if (strcmp(cmd, "new") == 0) {
        if (sdcard_logger_new_session()) {
            printf("\nNova sessao criada: %s\n\n", s_current_file);
        } else {
            printf("\nFalha ao criar nova sessao.\n\n");
        }
    } else if (strcmp(cmd, "export") == 0) {
        sdcard_logger_export_csv_to_stdout();
    } else if (strcmp(cmd, "export last") == 0) {
        sdcard_logger_export_last_to_stdout();
    } else if (strcmp(cmd, "export legacy") == 0) {
        sdcard_logger_export_legacy_to_stdout();
    } else {
        int session_number = 0;

        if (parse_export_session_command(cmd, &session_number)) {
            sdcard_logger_export_session_to_stdout(session_number);
        } else if (strcmp(cmd, "clear") == 0) {
            printf("\n");
            printf("Comando bloqueado por seguranca.\n");
            printf("Nada foi apagado.\n");
            printf("Para limpar SOMENTE a sessao atual, digite:\n");
            printf("clear confirm\n\n");
        } else if (strcmp(cmd, "clear confirm") == 0) {
            clear_current_session();
        } else if (strcmp(cmd, "help") == 0) {
            sdcard_logger_print_help();
        } else {
            printf("\nComando desconhecido: %s\n", cmd);
            printf("Digite 'help' para ver os comandos disponiveis.\n\n");
        }
    }
}