# ESP32 GPS Telemetry IDF

Sistema de telemetria veicular utilizando **ESP32**, **GPS GY-NEO6MV2 / NEO-6M**, **ESP-IDF**, **UART**, **parser NMEA**, **cálculo embarcado de telemetria**, **Docker**, **Mosquitto MQTT**, **Node-RED** e armazenamento offline planejado.

O objetivo do projeto é desenvolver uma solução embarcada capaz de coletar dados reais de localização e movimento de um veículo, interpretar mensagens GPS em formato NMEA, calcular métricas de telemetria localmente no ESP32 e futuramente transmitir essas informações para um servidor local com dashboard, mapa e banco de dados.

Este projeto está sendo desenvolvido com foco em práticas profissionais de:

* firmware embarcado;
* ESP-IDF;
* comunicação UART;
* análise de sinais;
* parser de protocolo;
* tratamento de dados GPS;
* cálculo de telemetria;
* arquitetura modular de firmware;
* validação de hardware real;
* documentação técnica por etapas.

---

## Status do Projeto

**Em desenvolvimento**

### Status atual

A etapa atual implementa um sistema embarcado de telemetria GPS com:

* leitura UART do módulo GPS no ESP32;
* recepção de sentenças NMEA reais;
* parsing das sentenças `GGA`, `RMC` e `VTG`;
* extração de latitude, longitude, velocidade, altitude, satélites, qualidade do fix, HDOP e horário UTC;
* validação real com antena GPS externa;
* cálculo local de telemetria veicular;
* identificação de veículo parado ou em movimento;
* cálculo de velocidade atual;
* cálculo de velocidade máxima;
* cálculo de velocidade média;
* cálculo de distância percorrida;
* cálculo de tempo parado;
* cálculo de tempo em movimento;
* filtro contra drift do GPS;
* organização modular do firmware.

O firmware já foi testado com o GPS parado e em movimentação curta, validando o comportamento do sistema em cenário real.

---

## Resultado Atual da Telemetria

Após a validação com antena externa, o GPS passou a obter fix válido e fornecer dados reais para o ESP32.

Exemplo de dados extraídos pelo firmware:

```text
Fix valido: SIM
UTC: 192516.000
Latitude: -21.540XXX
Longitude: -49.841XXX
Velocidade: 0.00 km/h
Altitude: 399.70 m
Satélites: 9
Qualidade do fix: 1
HDOP: 1.10
```

As coordenadas foram parcialmente mascaradas por privacidade.

Durante o teste com movimentação curta, o firmware identificou corretamente o estado de movimento:

```text
Status: EM MOVIMENTO
Velocidade atual: 3.93 km/h
Velocidade maxima: 6.87 km/h
Velocidade media: 4.12 km/h
Distancia percorrida: 13.73 m
Tempo parado: 97 s
Tempo em movimento: 12 s
Amostras validas: 70
Satélites: 9
HDOP: 1.10
```

Também foi validado o comportamento parado:

```text
Status: PARADO
Velocidade atual: 0.00 km/h
Tempo parado aumentando
Tempo em movimento preservado
Distância percorrida não aumentando com pequenas oscilações
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

Esse filtro torna a telemetria mais adequada para uso veicular, reduzindo falsas leituras causadas por ruído de GPS em ambiente parado ou com movimentação muito curta.

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
Monitor ESP-IDF
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
    |       - velocidade atual
    |       - velocidade máxima
    |       - velocidade média
    |       - distância percorrida
    |       - tempo parado
    |       - tempo em movimento
    |
    +--> microSD
    |       - armazenamento offline
    |       - histórico de pontos GPS
    |       - reenvio futuro quando houver conexão
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
│       └── gps_fix_real_terminal.txt
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
* determinar se o veículo está parado ou em movimento;
* calcular velocidade atual;
* registrar velocidade máxima;
* calcular velocidade média;
* calcular distância percorrida;
* calcular tempo parado;
* calcular tempo em movimento;
* aplicar filtro contra drift do GPS;
* exibir o resumo da telemetria no monitor serial.

### `main.c`

Responsável por:

* inicializar o sistema;
* inicializar a UART do GPS;
* inicializar as estruturas de dados;
* receber linhas NMEA;
* chamar o parser;
* atualizar a telemetria;
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

## Tecnologias Utilizadas

* ESP32;
* ESP-IDF;
* GPS GY-NEO6MV2 / NEO-6M;
* Antena GPS externa;
* UART;
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

* microSD;
* MQTT no ESP32;
* SQLite;
* dashboard Node-RED;
* mapa;
* armazenamento offline;
* reenvio de dados.

---

## Hardware Utilizado

| Componente              | Função                             |
| ----------------------- | ---------------------------------- |
| ESP32 DevKit            | Microcontrolador principal         |
| GPS GY-NEO6MV2 / NEO-6M | Módulo GNSS                        |
| Antena GPS externa      | Melhoria de recepção dos satélites |
| Analisador lógico       | Validação física do sinal UART     |
| Notebook Windows + WSL2 | Ambiente de desenvolvimento        |
| Docker                  | Infraestrutura local               |
| Mosquitto               | Broker MQTT                        |
| Node-RED                | Dashboard e integração futura      |

---

## Ligações entre GPS e ESP32

| GPS | ESP32            |
| --- | ---------------- |
| VCC | 5V               |
| GND | GND              |
| TX  | GPIO16           |
| RX  | GPIO17, opcional |

Ligação principal para leitura:

```text
GPS TX -> ESP32 GPIO16
GPS GND -> ESP32 GND
```

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

## Serviços Docker

O projeto utiliza Docker Compose para subir os serviços locais.

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
* identificação de status `PARADO` e `EM MOVIMENTO`;
* aplicação de filtro de movimento com limite de 5 km/h;
* aplicação de distância mínima para reduzir drift;
* teste real caminhando e parado.

Status: concluída.

---

## Próximas Etapas

### Etapa 14 — Gravação em microSD

Implementar armazenamento local dos dados de telemetria.

Planejamento:

* criar arquivo CSV no microSD;
* salvar UTC, latitude, longitude, velocidade, distância e status;
* manter registro local mesmo sem rede;
* preparar base para reenvio futuro.

Exemplo planejado:

```csv
utc,latitude,longitude,speed_kmh,total_distance_m,status,satellites,hdop
192516.000,-21.540XXX,-49.841XXX,3.93,13.73,MOVING,9,1.10
```

### Etapa 15 — Integração MQTT no ESP32

Planejamento:

* conectar ESP32 ao Wi-Fi;
* publicar dados GPS em JSON;
* enviar telemetria para o Mosquitto;
* integrar com Node-RED.

### Etapa 16 — Dashboard no Node-RED

Planejamento:

* velocidade atual;
* velocidade máxima;
* distância percorrida;
* tempo parado;
* tempo em movimento;
* status do veículo;
* satélites;
* estado do fix GPS.

### Etapa 17 — Banco de Dados

Planejamento:

* SQLite;
* histórico de pontos GPS;
* histórico de telemetria;
* consulta das últimas leituras;
* preparação para mapa.

### Etapa 18 — Mapa e Rota

Planejamento:

* exibir última posição;
* exibir rota percorrida;
* visualizar deslocamento em mapa;
* integrar com dados armazenados.

---

## Exemplo de Saída Atual

```text
I TELEMETRY: ============== TELEMETRIA VEICULAR =============
I TELEMETRY: Status: EM MOVIMENTO
I TELEMETRY: UTC: 192516.000
I TELEMETRY: Latitude: -21.540XXX
I TELEMETRY: Longitude: -49.841XXX
I TELEMETRY: Velocidade atual: 3.93 km/h
I TELEMETRY: Velocidade maxima: 6.87 km/h
I TELEMETRY: Velocidade media: 4.12 km/h
I TELEMETRY: Distancia percorrida: 13.73 m
I TELEMETRY: Tempo parado: 97 s
I TELEMETRY: Tempo em movimento: 12 s
I TELEMETRY: Amostras validas: 70
I TELEMETRY: Satelites: 9
I TELEMETRY: HDOP: 1.10
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
* validação prática com movimentação real.

O projeto agora já demonstra competências importantes para firmware embarcado:

* integração de sensor real;
* leitura serial;
* parser de protocolo;
* organização modular;
* cálculo embarcado;
* análise de comportamento físico do sensor;
* documentação técnica baseada em testes reais.

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
* GPS;
* Parser NMEA;
* Telemetria veicular;
* Linux;
* Docker;
* MQTT;
* Validação de hardware com analisador lógico.
