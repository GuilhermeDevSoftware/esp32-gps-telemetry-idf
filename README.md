# ESP32 GPS Telemetry IDF

Sistema de telemetria GPS desenvolvido com **ESP32**, **ESP-IDF**, **GPS GY-NEO6MV2 / NEO-6M**, **UART**, **parser NMEA**, **telemetria embarcada**, **microSD**, **exportação serial USB**, **arquivos CSV por sessão**, **Docker**, **Mosquitto MQTT**, **Node-RED** e futura etapa de **análise de dados com Python**.

O objetivo do projeto é desenvolver uma solução embarcada capaz de coletar dados reais de localização e movimento, interpretar mensagens GPS em formato NMEA, calcular métricas de telemetria localmente no ESP32, armazenar os dados offline em microSD e posteriormente analisar os arquivos gerados no computador com Python.

Este projeto está sendo desenvolvido com foco em práticas profissionais de:

* firmware embarcado;
* ESP-IDF;
* comunicação UART;
* parser de protocolo;
* análise de sinais;
* integração de hardware real;
* armazenamento offline;
* telemetria veicular;
* validação em campo;
* organização de logs por sessão;
* análise offline de dados;
* documentação técnica por etapas.

---

## Status do Projeto

**Em desenvolvimento**

### Status atual

A etapa atual implementa e valida um sistema embarcado de telemetria GPS com:

* leitura UART do módulo GPS no ESP32;
* recepção de sentenças NMEA reais;
* parser das sentenças `GGA`, `RMC` e `VTG`;
* extração de latitude, longitude, velocidade, altitude, satélites, qualidade do fix, HDOP e horário UTC;
* validação com antena GPS externa;
* cálculo local de telemetria veicular;
* identificação de estado `STOPPED` e `MOVING`;
* cálculo de velocidade atual;
* cálculo de velocidade máxima;
* cálculo de velocidade média;
* cálculo de distância percorrida;
* cálculo de tempo parado;
* cálculo de tempo em movimento;
* filtro contra drift do GPS;
* gravação dos dados em arquivos CSV no microSD;
* criação automática de arquivos por sessão;
* exportação de sessões pela serial USB;
* comandos seriais para controle e diagnóstico;
* primeiro teste de campo com power bank.

O sistema já foi testado em bancada, em área externa e em funcionamento autônomo alimentado por power bank.

---

## Marco Atual do Projeto

O projeto evoluiu de um logger GPS simples com arquivo único para um sistema de telemetria com **organização por sessões**.

Fluxo atual validado:

```text
Power bank
   |
   v
ESP32
   |
   +--> GPS GY-NEO6MV2 / NEO-6M
   |
   +--> Parser NMEA
   |
   +--> Cálculo de telemetria
   |
   +--> microSD
          |
          +--> session_001.csv
          +--> session_002.csv
          +--> session_003.csv
```

Cada inicialização cria uma nova sessão de coleta, evitando misturar dados de diferentes testes.

Depois do teste, o ESP32 pode ser conectado novamente ao notebook e o arquivo CSV pode ser exportado pela serial USB.

Exemplo de exportação de sessão:

```text
export 009
```

Exemplo de saída:

```text
---CSV_BEGIN_SESSION_009---
# FILE: /sdcard/session_009.csv
utc,latitude,longitude,speed_kmh,max_speed_kmh,avg_speed_kmh,total_distance_m,status,stopped_time_s,moving_time_s,satellites,hdop
...
---CSV_END_SESSION_009---
```

Esse fluxo validou que o sistema consegue operar sem notebook durante a coleta, gravar os dados localmente, separar os testes por sessão e disponibilizar o histórico posteriormente.

---

## Resultado do Teste com Power Bank

Durante o teste de campo, o ESP32 foi alimentado por power bank e permaneceu gravando dados no microSD.

O teste incluiu:

* período parado em frente à residência;
* caminhada curta;
* pequena corrida;
* retorno e exportação dos dados pela serial USB.

Resumo do resultado obtido:

```text
GPS com fix válido: SIM
Satélites: até 15
HDOP: até 0.70
Velocidade máxima registrada: 12.35 km/h
Distância acumulada registrada: 27.48 m
Status MOVING detectado: SIM
Gravação em microSD: SIM
Exportação serial: SIM
Alimentação por power bank: SIM
Sessões CSV separadas: SIM
```

As coordenadas reais foram parcialmente omitidas por privacidade.

Exemplo de trecho exportado do CSV:

```csv
utc,latitude,longitude,speed_kmh,max_speed_kmh,avg_speed_kmh,total_distance_m,status,stopped_time_s,moving_time_s,satellites,hdop
202136.000,-21.540XXX,-49.841XXX,7.28,7.48,0.00,0.00,MOVING,229,7,15,0.70
202138.000,-21.540XXX,-49.841XXX,8.19,8.19,0.00,0.00,MOVING,229,8,15,0.70
202140.000,-21.540XXX,-49.841XXX,7.43,8.19,2.00,5.55,MOVING,229,10,15,0.70
202142.000,-21.540XXX,-49.841XXX,9.43,9.43,3.64,11.13,MOVING,229,11,15,0.70
202154.000,-21.540XXX,-49.841XXX,12.35,12.35,3.97,22.07,MOVING,230,20,15,0.70
```

---

## Filtro de Movimento

Durante os testes foi observado que o GPS apresenta pequenas variações de latitude e longitude mesmo parado. Esse comportamento é esperado em receptores GNSS e é conhecido como drift do GPS.

Para evitar falso acúmulo de distância, o firmware utiliza filtros mínimos:

```c
#define TELEMETRY_MOVING_SPEED_THRESHOLD_KMH 5.0f
#define TELEMETRY_MIN_DISTANCE_M             5.0
```

Com isso, o sistema só considera movimento quando:

* o GPS possui fix válido;
* a velocidade está acima de 5 km/h;
* existe deslocamento mínimo relevante entre dois pontos;
* as coordenadas são válidas.

Esse filtro foi mantido em **5 km/h** porque o foco do projeto é telemetria veicular. Os testes a pé foram utilizados apenas para validação inicial do funcionamento em campo.

---

## Arquitetura Atual Implementada

```text
GPS GY-NEO6MV2 / NEO-6M
    |
    +--> Antena GPS externa
    |
    | UART 9600 bps
    v
ESP32 - UART2
    |
    | RX GPIO16
    | TX GPIO17
    v
gps_uart.c
    |
    | Leitura de linhas NMEA
    v
nmea_parser.c
    |
    | Parser GGA / RMC / VTG
    | Conversão de coordenadas NMEA para decimal
    | Extração de velocidade, altitude, satélites, HDOP e UTC
    v
telemetry.c
    |
    | Status parado / em movimento
    | Velocidade atual
    | Velocidade máxima
    | Velocidade média
    | Distância percorrida
    | Tempo parado
    | Tempo em movimento
    | Filtro contra drift
    v
sdcard_logger.c
    |
    | Gravação CSV no microSD
    | Criação automática de sessão
    | Comandos seriais
    | Exportação serial USB
    v
session_XXX.csv
```

---

## Arquitetura Planejada do Sistema Completo

```text
ESP32 no veículo
    |
    +--> GPS GY-NEO6MV2 / NEO-6M
    |
    +--> Parser NMEA
    |
    +--> Cálculo de telemetria
    |
    +--> microSD
    |       - armazenamento offline
    |       - histórico de pontos GPS
    |       - arquivos CSV por sessão
    |       - exportação serial
    |
    +--> Python no PC
    |       - leitura dos CSVs exportados
    |       - limpeza dos dados
    |       - cálculo de métricas
    |       - geração de gráficos
    |       - geração de relatório
    |
    +--> Wi-Fi / MQTT
            |
            v
Servidor local
    |
    +--> Mosquitto MQTT
    |
    +--> Node-RED
    |
    +--> Dashboard
    |
    +--> Banco de dados
    |
    +--> Mapa
```

---

## Estrutura Atual do Projeto

```text
telemetria-gps/
├── database/
├── docs/
│   ├── images/
│   │   ├── gps_uart_saleae_logic.jpg
│   │   ├── gps_uart_espidf_monitor.jpg
│   │   ├── gps-fix-terminal.jpeg
│   │   ├── gps-esp32-montagem-antena.jpeg
│   │   └── gps-ligacao-hardware.jpeg
│   └── logs/
│       ├── gps_fix_real_terminal.txt
│       └── telemetry_powerbank_export_raw.txt
├── gps_uart_test/
│   ├── CMakeLists.txt
│   ├── main/
│   │   ├── CMakeLists.txt
│   │   ├── main.c
│   │   ├── gps/
│   │   │   ├── gps_uart.c
│   │   │   ├── gps_uart.h
│   │   │   ├── nmea_parser.c
│   │   │   └── nmea_parser.h
│   │   ├── telemetry/
│   │   │   ├── telemetry.c
│   │   │   └── telemetry.h
│   │   ├── storage/
│   │   │   ├── sdcard_logger.c
│   │   │   └── sdcard_logger.h
│   │   └── legacy/
│   ├── sdkconfig
│   └── sdkconfig.old
├── mosquitto/
│   ├── config/
│   │   └── mosquitto.conf
│   ├── data/
│   └── log/
├── nodered/
│   └── data/
├── docker-compose.yml
├── .gitignore
└── README.md
```

Estrutura prevista para a próxima etapa com Python:

```text
telemetria-gps/
├── python/
│   ├── analyze_session.py
│   ├── requirements.txt
│   ├── input/
│   │   └── session_009.csv
│   └── output/
│       ├── session_009_summary.json
│       ├── session_009_clean.csv
│       ├── speed_chart.png
│       ├── distance_chart.png
│       └── route.geojson
```

A pasta `build/` do ESP-IDF não deve ser versionada.

---

## Módulos do Firmware

### `gps_uart.c`

Responsável por:

* inicializar a UART2;
* configurar baud rate de 9600 bps;
* configurar RX em GPIO16;
* configurar TX em GPIO17;
* ler bytes recebidos do GPS;
* montar linhas NMEA completas.

### `nmea_parser.c`

Responsável por:

* identificar sentenças `GGA`, `RMC` e `VTG`;
* separar os campos NMEA;
* extrair horário UTC;
* extrair latitude e longitude;
* converter coordenadas NMEA para graus decimais;
* extrair velocidade;
* extrair altitude;
* extrair quantidade de satélites;
* extrair qualidade do fix;
* extrair HDOP;
* atualizar a estrutura `gps_data_t`.

### `telemetry.c`

Responsável por:

* receber os dados tratados do GPS;
* determinar se o sistema está parado ou em movimento;
* calcular velocidade atual;
* registrar velocidade máxima;
* calcular velocidade média;
* calcular distância percorrida;
* calcular tempo parado;
* calcular tempo em movimento;
* aplicar filtro contra drift do GPS;
* exibir o resumo da telemetria no monitor serial.

### `sdcard_logger.c`

Responsável por:

* inicializar o microSD via SPI;
* montar o sistema de arquivos FAT;
* criar arquivos CSV por sessão;
* localizar o próximo número de sessão disponível;
* escrever o cabeçalho CSV;
* gravar as amostras de telemetria;
* manter os dados offline;
* listar sessões gravadas;
* exportar sessões pela serial USB;
* executar limpeza segura da sessão atual.

### `main.c`

Responsável por:

* inicializar o sistema;
* inicializar a UART do GPS;
* inicializar o microSD;
* inicializar as estruturas de dados;
* receber linhas NMEA;
* chamar o parser;
* atualizar a telemetria;
* gravar dados no microSD;
* processar comandos seriais;
* exibir os logs principais no monitor do ESP-IDF.

---

## Estruturas de Dados

### Dados do GPS

```c
typedef struct {
    bool has_fix;

    char utc_time[16];

    double latitude;
    double longitude;

    float speed_kmh;
    float altitude_m;
    float hdop;

    int satellites;
    int fix_quality;

    uint32_t valid_points;
    uint32_t last_update_ms;
} gps_data_t;
```

### Dados de Telemetria

```c
typedef struct {
    float current_speed_kmh;
    float max_speed_kmh;
    float average_speed_kmh;

    double total_distance_m;

    uint32_t moving_time_s;
    uint32_t stopped_time_s;

    uint32_t valid_samples;

    bool is_moving;
    bool has_last_position;

    double last_latitude;
    double last_longitude;

    uint32_t last_sample_ms;

    char last_utc_time[16];
} telemetry_data_t;
```

---

## Formato dos Arquivos CSV

Antes, o firmware gravava todos os dados em um único arquivo:

```text
/sdcard/telemetry.csv
```

Agora, o firmware cria arquivos separados por sessão:

```text
/sdcard/session_001.csv
/sdcard/session_002.csv
/sdcard/session_003.csv
```

Cabeçalho do CSV:

```csv
utc,latitude,longitude,speed_kmh,max_speed_kmh,avg_speed_kmh,total_distance_m,status,stopped_time_s,moving_time_s,satellites,hdop
```

Cada linha registra:

* horário UTC;
* latitude;
* longitude;
* velocidade atual;
* velocidade máxima;
* velocidade média;
* distância acumulada;
* status `MOVING` ou `STOPPED`;
* tempo parado;
* tempo em movimento;
* quantidade de satélites;
* HDOP.

---

## Comandos Seriais Implementados

O firmware aceita comandos digitados diretamente no `idf.py monitor`.

### Ver status do sistema

```text
status
```

Esse comando mostra informações gerais da sessão atual, como:

* status do microSD;
* arquivo atual;
* número da sessão atual;
* quantidade de linhas gravadas;
* tamanho aproximado do arquivo;
* comandos disponíveis.

---

### Listar sessões gravadas

```text
list
```

Esse comando lista os arquivos de sessão encontrados no microSD.

Exemplo esperado:

```text
session_001.csv
session_002.csv
session_003.csv
session_009.csv
```

---

### Exportar a sessão atual

```text
export
```

Esse comando exporta a sessão atual pela serial USB.

---

### Exportar uma sessão específica

```text
export 009
```

Esse comando exporta a sessão escolhida.

Exemplo de saída:

```text
---CSV_BEGIN_SESSION_009---
# FILE: /sdcard/session_009.csv
utc,latitude,longitude,speed_kmh,max_speed_kmh,avg_speed_kmh,total_distance_m,status,stopped_time_s,moving_time_s,satellites,hdop
...
---CSV_END_SESSION_009---
```

Isso permite escolher qualquer sessão existente no microSD, desde que o arquivo esteja disponível.

---

### Limpeza segura da sessão atual

```text
clear
```

O comando `clear` não apaga os dados diretamente. Ele apenas exibe um aviso de segurança.

Para confirmar a limpeza da sessão atual:

```text
clear confirm
```

Esse cuidado evita apagar dados de campo por acidente.

---

## Exemplo de Uso no Monitor ESP-IDF

Entrar no monitor:

```bash
idf.py monitor
```

Verificar status:

```text
status
```

Listar sessões:

```text
list
```

Exportar uma sessão específica:

```text
export 009
```

Salvar a saída no notebook:

```bash
idf.py monitor | tee ../docs/logs/session_009_export_raw.txt
```

Para sair do monitor:

```text
CTRL + ]
```

---

## Tecnologias Utilizadas

* ESP32;
* ESP-IDF;
* GPS GY-NEO6MV2 / NEO-6M;
* Antena GPS externa;
* UART;
* SPI;
* microSD;
* Parser NMEA;
* C;
* FreeRTOS;
* Analisador lógico 24 MHz / 8 canais;
* Saleae Logic 2;
* WSL2 Ubuntu;
* Docker;
* Docker Compose;
* Mosquitto MQTT;
* Node-RED;
* Git e GitHub;
* VSCode.

Tecnologias planejadas para as próximas etapas:

* Python;
* Pandas;
* Matplotlib;
* geração de relatórios;
* geração de gráficos;
* geração de arquivos GeoJSON;
* MQTT no ESP32;
* SQLite;
* dashboard Node-RED;
* mapa;
* reenvio de dados.

---

## Hardware Utilizado

| Componente              | Função                                   |
| ----------------------- | ---------------------------------------- |
| ESP32 DevKit            | Microcontrolador principal               |
| GPS GY-NEO6MV2 / NEO-6M | Módulo GNSS                              |
| Antena GPS externa      | Melhoria de recepção dos satélites       |
| Módulo microSD          | Armazenamento offline dos dados          |
| Cartão microSD          | Registro local dos arquivos CSV          |
| Power bank              | Alimentação autônoma para teste de campo |
| Analisador lógico       | Validação física do sinal UART           |
| Notebook Windows + WSL2 | Ambiente de desenvolvimento              |
| Docker                  | Infraestrutura local futura              |
| Mosquitto               | Broker MQTT futuro                       |
| Node-RED                | Dashboard e integração futura            |

---

## Ligações entre GPS e ESP32

| GPS | ESP32            |
| --- | ---------------- |
| VCC | 3V3              |
| GND | GND              |
| TX  | GPIO16           |
| RX  | GPIO17, opcional |

Ligação principal para leitura:

```text
GPS TX  -> ESP32 GPIO16
GPS GND -> ESP32 GND
```

---

## Ligações entre microSD e ESP32

| microSD | ESP32  |
| ------- | ------ |
| VCC     | 5V     |
| GND     | GND    |
| CS      | GPIO5  |
| SCK/CLK | GPIO18 |
| MISO/DO | GPIO19 |
| MOSI/DI | GPIO23 |

Observação: durante os testes, a comunicação SPI do microSD apresentou maior estabilidade com velocidade reduzida:

```c
host.max_freq_khz = SDMMC_FREQ_PROBING;
```

Essa configuração evitou falhas de comunicação como `ESP_ERR_INVALID_CRC`.

---

## Como Compilar e Executar o Firmware

Entrar na pasta do projeto ESP-IDF:

```bash
cd ~/projetos/telemetria-gps/gps_uart_test
```

Carregar o ambiente ESP-IDF:

```bash
source ~/esp/esp-idf/export.sh
```

Definir o alvo:

```bash
idf.py set-target esp32
```

Compilar:

```bash
idf.py build
```

Gravar e abrir o monitor serial:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Caso a porta seja diferente:

```bash
ls /dev/ttyUSB*
```

ou:

```bash
ls /dev/ttyACM*
```

Para sair do monitor do ESP-IDF:

```text
CTRL + ]
```

---

## Teste com Power Bank

Procedimento utilizado no teste de campo:

```text
1. Alimentar o ESP32 pelo power bank
2. Deixar o GPS obter fix em área aberta
3. Permanecer parado por alguns minutos
4. Caminhar, correr ou realizar trajeto veicular
5. Desligar o power bank
6. Conectar o ESP32 novamente ao notebook
7. Abrir o monitor serial
8. Digitar status
9. Digitar list
10. Exportar a sessão desejada
11. Salvar a saída em arquivo .txt ou .csv
```

Exemplo:

```text
status
list
export 009
```

Comando utilizado para salvar a exportação no notebook:

```bash
idf.py monitor | tee ../docs/logs/session_009_export_raw.txt
```

O teste confirmou que o projeto já funciona como um data logger GPS autônomo com organização por sessão.

---

## Serviços Docker

O projeto utiliza Docker Compose para subir os serviços locais planejados.

### Mosquitto MQTT

Responsável por receber e distribuir mensagens MQTT.

Porta:

```text
1883
```

### Node-RED

Responsável por processar, visualizar e futuramente armazenar os dados de telemetria.

Porta:

```text
1880
```

Acesso:

```text
http://localhost:1880
```

---

## Como Executar a Infraestrutura Local

Entrar na pasta principal do projeto:

```bash
cd ~/projetos/telemetria-gps
```

Subir os containers:

```bash
docker compose up -d
```

Verificar containers ativos:

```bash
docker compose ps
```

Parar os containers:

```bash
docker compose down
```

Ver logs do Mosquitto:

```bash
docker logs telemetria-mosquitto
```

Ver logs do Node-RED:

```bash
docker logs telemetria-nodered
```

---

## Validação com Analisador Lógico

A comunicação UART entre o GPS e o ESP32 foi validada também no nível físico utilizando analisador lógico.

O analisador foi conectado em paralelo ao TX do GPS:

```text
GPS TX
   |
   +---- ESP32 GPIO16
   |
   +---- CH1 / D0 do analisador lógico

GPS GND
   |
   +---- ESP32 GND
   |
   +---- GND do analisador lógico
```

Configuração utilizada no Saleae Logic 2:

```text
Decoder: Async Serial
Input Channel: D0
Bit Rate: 9600
Bits per Frame: 8
Stop Bits: 1
Parity: None
Signal: Non-inverted
```

Essa etapa confirmou que:

* o GPS transmitia dados pela UART;
* o baud rate estava correto;
* as mensagens NMEA eram válidas;
* o ESP32 recebia os mesmos dados observados no analisador lógico.

![Captura UART GPS no Saleae Logic](docs/images/gps_uart_saleae_logic.jpg)

![Monitor ESP-IDF recebendo mensagens NMEA](docs/images/gps_uart_espidf_monitor.jpg)

---

## Validação com Antena GPS

A antena externa foi conectada ao módulo GPS e o teste foi realizado em área externa.

O GPS obteve fix válido, confirmando:

* recepção real de satélites;
* funcionamento da antena externa;
* comunicação UART estável;
* dados NMEA válidos;
* leitura de satélites;
* leitura de altitude;
* leitura de HDOP;
* leitura de velocidade;
* leitura de latitude e longitude.

Evidências:

![Validação do fix GPS no terminal ESP-IDF](docs/images/gps-fix-terminal.jpeg)

![Montagem ESP32 com GPS e antena externa](docs/images/gps-esp32-montagem-antena.jpeg)

![Ligação do hardware ESP32 e módulo GPS](docs/images/gps-ligacao-hardware.jpeg)

---

## Etapas Concluídas

### Etapa 1 — Preparação do Ambiente Linux

* WSL2 configurado;
* Ubuntu atualizado;
* integração com VSCode;
* ambiente de desenvolvimento preparado.

Status: concluída.

### Etapa 2 — Instalação e Validação do Docker

* Docker Desktop instalado;
* integração com WSL2 habilitada;
* teste `hello-world` realizado.

Status: concluída.

### Etapa 3 — Infraestrutura Docker

* `docker-compose.yml` criado;
* container Mosquitto configurado;
* container Node-RED configurado;
* pastas persistentes criadas.

Status: concluída.

### Etapa 4 — Teste MQTT Local

* publicação MQTT testada via terminal;
* Node-RED recebeu payload JSON;
* broker Mosquitto validado.

Status: concluída.

### Etapa 5 — Projeto ESP-IDF para UART GPS

* projeto `gps_uart_test` criado;
* ESP32 configurado;
* UART2 configurada;
* firmware compilado e gravado.

Status: concluída.

### Etapa 6 — Validação UART com GPS

* GPS conectado ao ESP32;
* erro inicial de ligação elétrica corrigido;
* mensagens NMEA recebidas no monitor serial.

Status: concluída.

### Etapa 7 — Validação com Analisador Lógico

* sinal UART capturado;
* decoder serial configurado;
* mensagens NMEA decodificadas no Saleae Logic 2;
* comunicação física validada.

Status: concluída.

### Etapa 8 — Parser NMEA Inicial

* identificação de sentenças `GGA`, `RMC` e `VTG`;
* leitura de dados GPS;
* detecção de GPS sem fix.

Status: concluída.

### Etapa 9 — Diagnóstico Técnico do GPS

* contadores de mensagens NMEA;
* status do GPS;
* identificação de fix;
* leitura de satélites;
* validação de mensagens recebidas.

Status: concluída.

### Etapa 10 — Validação com Antena GPS

* antena externa instalada;
* fix GPS obtido;
* dados reais recebidos;
* satélites identificados;
* altitude, velocidade, HDOP e UTC lidos corretamente.

Status: concluída.

### Etapa 11 — Modularização do Firmware

* criação do módulo `gps_uart`;
* criação do módulo `nmea_parser`;
* criação do módulo `telemetry`;
* separação das responsabilidades do firmware;
* `main.c` mantido como ponto de integração.

Status: concluída.

### Etapa 12 — Registro de Dados Reais do GPS

* latitude e longitude extraídas;
* velocidade em km/h extraída;
* altitude extraída;
* satélites extraídos;
* qualidade do fix extraída;
* HDOP extraído;
* horário UTC extraído;
* log real salvo para documentação.

Status: concluída.

### Etapa 13 — Telemetria Veicular Embarcada

* cálculo de velocidade atual;
* registro de velocidade máxima;
* cálculo de velocidade média;
* cálculo de distância percorrida;
* cálculo de tempo parado;
* cálculo de tempo em movimento;
* identificação de status `STOPPED` e `MOVING`;
* aplicação de filtro de movimento com limite de 5 km/h;
* aplicação de distância mínima para reduzir drift;
* teste real caminhando, correndo e parado.

Status: concluída.

### Etapa 14 — Gravação em microSD

* inicialização do microSD via SPI;
* montagem do sistema de arquivos FAT;
* criação do arquivo CSV;
* escrita do cabeçalho CSV;
* gravação das amostras de telemetria;
* uso de `fflush()` e `fclose()` após cada linha para reduzir risco de perda de dados;
* ajuste de velocidade SPI para estabilidade.

Status: concluída.

### Etapa 15 — Exportação Serial do CSV

* criação do comando `export`;
* leitura do arquivo CSV;
* envio do conteúdo pela USB serial;
* salvamento da saída no notebook com `tee`;
* validação da exportação após teste de campo.

Status: concluída.

### Etapa 16 — Primeiro Teste de Campo com Power Bank

* ESP32 alimentado por power bank;
* GPS obtendo fix em ambiente externo;
* microSD gravando sem notebook conectado;
* caminhada e corrida curta registradas;
* CSV exportado posteriormente pela serial USB;
* velocidade máxima e status `MOVING` identificados nos dados.

Status: concluída.

### Etapa 17 — Arquivos CSV por Sessão

* substituição do arquivo único por arquivos separados;
* criação automática de nova sessão;
* arquivos nomeados como `session_XXX.csv`;
* separação entre testes de bancada, testes a pé e futuros testes veiculares;
* redução de risco de misturar dados de diferentes coletas.

Status: concluída.

### Etapa 18 — Comandos Seriais de Diagnóstico e Controle

* comando `status` implementado;
* comando `list` implementado;
* comando `export` implementado;
* exportação de sessão específica implementada;
* comando `clear` seguro implementado;
* confirmação obrigatória com `clear confirm`;
* exportação com marcadores de início e fim da sessão.

Status: concluída.

---

## Próxima Etapa — Análise dos Dados com Python

A próxima etapa será criar uma camada de análise offline em Python para processar os arquivos CSV gerados pelo ESP32.

Essa etapa será feita antes da integração com MQTT e Node-RED, porque permite validar os dados gravados no microSD antes de transmitir ou visualizar em dashboard.

### Objetivos da etapa Python

* ler arquivos `session_XXX.csv`;
* aceitar arquivos exportados pela serial USB;
* remover linhas inválidas ou incompletas;
* calcular métricas finais do trajeto;
* gerar resumo da sessão;
* gerar gráficos;
* preparar dados para mapa;
* organizar os resultados em uma pasta de saída.

### Métricas previstas

O script Python deverá calcular:

* quantidade total de amostras;
* quantidade de amostras `MOVING`;
* quantidade de amostras `STOPPED`;
* velocidade máxima;
* velocidade média;
* distância total percorrida;
* tempo total parado;
* tempo total em movimento;
* média de satélites;
* melhor e pior HDOP;
* início e fim da sessão;
* possíveis pontos com dados inválidos.

### Arquivos de saída previstos

```text
python/output/session_009_clean.csv
python/output/session_009_summary.json
python/output/speed_chart.png
python/output/distance_chart.png
python/output/route.geojson
```

### Fluxo previsto da etapa Python

```text
CSV exportado do ESP32
        |
        v
Python
        |
        +--> Limpeza dos dados
        |
        +--> Cálculo de métricas
        |
        +--> Geração de gráficos
        |
        +--> Geração de relatório
        |
        +--> Preparação para mapa
```

### Bibliotecas previstas

* `pandas`;
* `matplotlib`;
* `json`;
* `pathlib`;
* `argparse`.

### Exemplo de execução futura

```bash
cd ~/projetos/telemetria-gps/python

python3 -m venv venv
source venv/bin/activate

pip install -r requirements.txt

python analyze_session.py --input input/session_009.csv --output output/
```

Resultado esperado:

```text
Sessão analisada: session_009.csv
Amostras válidas: 120
Velocidade máxima: 12.35 km/h
Distância total: 27.48 m
Tempo parado: 230 s
Tempo em movimento: 20 s
Arquivo limpo gerado: output/session_009_clean.csv
Resumo gerado: output/session_009_summary.json
Gráfico de velocidade gerado: output/speed_chart.png
GeoJSON gerado: output/route.geojson
```

---

## Exemplo de Saída Atual do Firmware

```text
I TELEMETRY: ============== TELEMETRIA VEICULAR =============
I TELEMETRY: Status: MOVING
I TELEMETRY: UTC: 202154.000
I TELEMETRY: Latitude: -21.540XXX
I TELEMETRY: Longitude: -49.841XXX
I TELEMETRY: Velocidade atual: 12.35 km/h
I TELEMETRY: Velocidade maxima: 12.35 km/h
I TELEMETRY: Velocidade media: 3.97 km/h
I TELEMETRY: Distancia percorrida: 22.07 m
I TELEMETRY: Tempo parado: 230 s
I TELEMETRY: Tempo em movimento: 20 s
I TELEMETRY: Satelites: 15
I TELEMETRY: HDOP: 0.70
```

---

## Observações Técnicas

A etapa atual comprova pontos importantes do sistema embarcado:

* comunicação UART funcional;
* baud rate correto;
* recepção real de sentenças NMEA;
* antena GPS externa funcional;
* fix GPS válido;
* extração de dados reais de navegação;
* conversão de coordenadas;
* arquitetura modular em C com ESP-IDF;
* cálculo local de métricas de telemetria;
* tratamento inicial de drift do GPS;
* comunicação SPI com microSD;
* gravação offline em CSV;
* organização de logs por sessão;
* exportação serial posterior;
* comandos de diagnóstico no firmware;
* alimentação autônoma por power bank;
* validação prática em campo.

O projeto agora demonstra competências importantes para firmware embarcado:

* integração de sensor real;
* leitura serial;
* parser de protocolo;
* organização modular;
* cálculo embarcado;
* armazenamento local;
* tratamento de falha de comunicação;
* comandos de operação via terminal;
* validação em bancada;
* validação em campo;
* documentação técnica baseada em testes reais;
* preparação de dados para análise externa.

---

## Autor

Guilherme Costa

Projeto desenvolvido com foco em aprendizado e portfólio profissional nas áreas de:

* Sistemas embarcados;
* Firmware;
* ESP32;
* ESP-IDF;
* IoT;
* UART;
* SPI;
* GPS;
* Parser NMEA;
* microSD;
* Telemetria veicular;
* Linux;
* Docker;
* MQTT;
* Python para análise de dados;
* Validação de hardware com analisador lógico;
* Testes de campo com hardware real.
