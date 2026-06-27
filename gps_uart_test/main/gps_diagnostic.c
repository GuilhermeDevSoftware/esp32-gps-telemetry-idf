#include "gps_diagnostic.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_timer.h"

#define NMEA_MAX_LINE_SIZE 160

static int hex_to_int(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }

    if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }

    if (c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }

    return -1;
}

static void clean_line(const char *input, char *output, size_t output_size)
{
    size_t j = 0;

    for (size_t i = 0; input[i] != '\0' && j < output_size - 1; i++)
    {
        if (input[i] == '\r' || input[i] == '\n')
        {
            break;
        }

        output[j++] = input[i];
    }

    output[j] = '\0';
}

static bool validate_nmea_checksum(const char *line)
{
    if (line == NULL || line[0] != '$')
    {
        return false;
    }

    const char *asterisk = strchr(line, '*');

    if (asterisk == NULL)
    {
        return false;
    }

    if (strlen(asterisk) < 3)
    {
        return false;
    }

    uint8_t calculated_checksum = 0;

    for (const char *p = line + 1; p < asterisk; p++)
    {
        calculated_checksum ^= (uint8_t)(*p);
    }

    int high = hex_to_int(asterisk[1]);
    int low = hex_to_int(asterisk[2]);

    if (high < 0 || low < 0)
    {
        return false;
    }

    uint8_t received_checksum = (uint8_t)((high << 4) | low);

    return calculated_checksum == received_checksum;
}

static bool get_nmea_field(const char *sentence, int field_index, char *out, size_t out_size)
{
    if (sentence == NULL || out == NULL || out_size == 0)
    {
        return false;
    }

    int current_field = 0;
    const char *start = sentence;

    while (*start != '\0' && current_field < field_index)
    {
        if (*start == ',')
        {
            current_field++;
        }

        start++;
    }

    if (current_field != field_index)
    {
        out[0] = '\0';
        return false;
    }

    const char *end = start;

    while (*end != '\0' && *end != ',' && *end != '*')
    {
        end++;
    }

    size_t length = (size_t)(end - start);

    if (length >= out_size)
    {
        length = out_size - 1;
    }

    memcpy(out, start, length);
    out[length] = '\0';

    return true;
}

static int field_to_int(const char *field, int default_value)
{
    if (field == NULL || field[0] == '\0')
    {
        return default_value;
    }

    return atoi(field);
}

static void update_status(gps_diagnostic_t *diag)
{
    if (diag->fix_type_gsa == 3)
    {
        diag->status = GPS_STATUS_FIX_3D;
        return;
    }

    if (diag->fix_type_gsa == 2)
    {
        diag->status = GPS_STATUS_FIX_2D;
        return;
    }

    if (diag->fix_quality_gga > 0 || diag->rmc_active)
    {
        diag->status = GPS_STATUS_FIX_2D;
        return;
    }

    if (diag->satellites < 3)
    {
        diag->status = GPS_STATUS_AGUARDANDO_SATELITES;
        return;
    }

    diag->status = GPS_STATUS_SEM_FIX;
}

static void parse_gga(gps_diagnostic_t *diag, const char *sentence)
{
    char field[24];

    get_nmea_field(sentence, 1, field, sizeof(field));

    if (field[0] != '\0')
    {
        strncpy(diag->utc_time, field, sizeof(diag->utc_time) - 1);
        diag->utc_time[sizeof(diag->utc_time) - 1] = '\0';
    }

    get_nmea_field(sentence, 6, field, sizeof(field));
    diag->fix_quality_gga = field_to_int(field, 0);

    get_nmea_field(sentence, 7, field, sizeof(field));
    diag->satellites = field_to_int(field, 0);
}

static void parse_rmc(gps_diagnostic_t *diag, const char *sentence)
{
    char field[24];

    get_nmea_field(sentence, 1, field, sizeof(field));

    if (field[0] != '\0')
    {
        strncpy(diag->utc_time, field, sizeof(diag->utc_time) - 1);
        diag->utc_time[sizeof(diag->utc_time) - 1] = '\0';
    }

    get_nmea_field(sentence, 2, field, sizeof(field));

    if (field[0] == 'A')
    {
        diag->rmc_active = true;
    }
    else if (field[0] == 'V')
    {
        diag->rmc_active = false;
    }
}

static void parse_gsa(gps_diagnostic_t *diag, const char *sentence)
{
    char field[24];

    get_nmea_field(sentence, 2, field, sizeof(field));
    diag->fix_type_gsa = field_to_int(field, 1);
}

void gps_diagnostic_init(gps_diagnostic_t *diag)
{
    if (diag == NULL)
    {
        return;
    }

    memset(diag, 0, sizeof(gps_diagnostic_t));

    diag->satellites = 0;
    diag->fix_quality_gga = 0;
    diag->fix_type_gsa = 1;
    diag->rmc_active = false;
    diag->status = GPS_STATUS_AGUARDANDO_SATELITES;

    strcpy(diag->utc_time, "--");
    strcpy(diag->last_valid_message, "Nenhuma mensagem valida ainda");
}

bool gps_diagnostic_process_line(gps_diagnostic_t *diag, const char *line)
{
    if (diag == NULL || line == NULL)
    {
        return false;
    }

    char clean[NMEA_MAX_LINE_SIZE];
    clean_line(line, clean, sizeof(clean));

    if (clean[0] == '\0')
    {
        diag->ignored_count++;
        return false;
    }

    if (clean[0] != '$')
    {
        diag->ignored_count++;
        return false;
    }

    if (!validate_nmea_checksum(clean))
    {
        diag->invalid_checksum_count++;
        return false;
    }

    diag->valid_sentence_count++;

    strncpy(diag->last_valid_message, clean, sizeof(diag->last_valid_message) - 1);
    diag->last_valid_message[sizeof(diag->last_valid_message) - 1] = '\0';

    char sentence[NMEA_MAX_LINE_SIZE];
    strncpy(sentence, clean, sizeof(sentence) - 1);
    sentence[sizeof(sentence) - 1] = '\0';

    char *asterisk = strchr(sentence, '*');

    if (asterisk != NULL)
    {
        *asterisk = '\0';
    }

    if (strlen(sentence) < 6)
    {
        diag->ignored_count++;
        update_status(diag);
        return false;
    }

    char type[4];
    type[0] = sentence[3];
    type[1] = sentence[4];
    type[2] = sentence[5];
    type[3] = '\0';

    if (strcmp(type, "GGA") == 0)
    {
        diag->gga_count++;
        parse_gga(diag, sentence);
    }
    else if (strcmp(type, "RMC") == 0)
    {
        diag->rmc_count++;
        parse_rmc(diag, sentence);
    }
    else if (strcmp(type, "VTG") == 0)
    {
        diag->vtg_count++;
    }
    else if (strcmp(type, "GSA") == 0)
    {
        diag->gsa_count++;
        parse_gsa(diag, sentence);
    }
    else
    {
        diag->ignored_count++;
    }

    update_status(diag);

    return true;
}

const char *gps_status_to_string(gps_status_t status)
{
    switch (status)
    {
    case GPS_STATUS_SEM_FIX:
        return "SEM FIX";

    case GPS_STATUS_AGUARDANDO_SATELITES:
        return "AGUARDANDO SATELITES";

    case GPS_STATUS_FIX_2D:
        return "FIX 2D";

    case GPS_STATUS_FIX_3D:
        return "FIX 3D";

    default:
        return "DESCONHECIDO";
    }
}

void gps_diagnostic_print(gps_diagnostic_t *diag, const char *tag)
{
    if (diag == NULL)
    {
        return;
    }

    diag->uptime_seconds = (uint32_t)(esp_timer_get_time() / 1000000ULL);

    ESP_LOGI(tag, "================ DIAGNOSTICO GPS ================");
    ESP_LOGI(tag, "Tempo desde inicializacao: %lu s", (unsigned long)diag->uptime_seconds);
    ESP_LOGI(tag, "Status GPS: %s", gps_status_to_string(diag->status));
    ESP_LOGI(tag, "Mensagens GGA: %lu", (unsigned long)diag->gga_count);
    ESP_LOGI(tag, "Mensagens RMC: %lu", (unsigned long)diag->rmc_count);
    ESP_LOGI(tag, "Mensagens VTG: %lu", (unsigned long)diag->vtg_count);
    ESP_LOGI(tag, "Mensagens GSA: %lu", (unsigned long)diag->gsa_count);
    ESP_LOGI(tag, "Mensagens ignoradas: %lu", (unsigned long)diag->ignored_count);
    ESP_LOGI(tag, "Checksums invalidos: %lu", (unsigned long)diag->invalid_checksum_count);
    ESP_LOGI(tag, "Mensagens validas: %lu", (unsigned long)diag->valid_sentence_count);
    ESP_LOGI(tag, "Satelites: %d | GGA fix quality: %d | GSA fix type: %d | RMC: %s",
             diag->satellites,
             diag->fix_quality_gga,
             diag->fix_type_gsa,
             diag->rmc_active ? "A" : "V");
    ESP_LOGI(tag, "UTC: %s", diag->utc_time);
    ESP_LOGI(tag, "Ultima mensagem valida: %s", diag->last_valid_message);
    ESP_LOGI(tag, "=================================================");
}
