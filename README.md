# ESP32 GPS Telemetry IDF

Sistema de telemetria veicular utilizando **ESP32**, **GPS GY-NEO6MV2 / NEO-6M**, **ESP-IDF**, **MQTT**, **Docker**, **Node-RED** e armazenamento offline.

O objetivo do projeto é desenvolver uma solução embarcada capaz de coletar dados de localização e movimento de um veículo, interpretar os dados do GPS, transmitir informações para um servidor local e futuramente exibir esses dados em dashboard, mapa e banco de dados.

Este projeto está sendo desenvolvido com foco em práticas profissionais de sistemas embarcados, incluindo:

* desenvolvimento com ESP-IDF;
* uso de Linux/WSL2;
* validação de comunicação UART;
* análise de sinais com analisador lógico;
* organização modular do firmware;
* infraestrutura local com Docker;
* comunicação MQTT;
* documentação técnica por etapas.

---

## Status do Projeto

**Em desenvolvimento**

### Etapas já implementadas

* Ambiente Linux com WSL2 configurado;
* Projeto versionado no GitHub;
* Docker configurado;
* Mosquitto MQTT rodando em container;
* Node-RED rodando em container;
* Comunicação MQTT testada via terminal;
* Node-RED recebendo mensagens JSON via MQTT;
* Projeto ESP-IDF criado para teste de GPS via UART;
* ESP32 gravado com sucesso pelo WSL2 usando `usbipd`;
* GPS conectado ao ESP32 via UART2;
* Correção de ligação elétrica entre GPS e ESP32;
* Comunicação UART validada no ESP-IDF;
* Comunicação UART validada fisicamente com analisador lógico;
* Mensagens NMEA recebidas e documentadas;
* Parser NMEA implementado;
* Detecção de GPS sem fix funcionando corretamente.

### Etapa atual

Implementação e validação inicial do **parser NMEA**.

O ESP32 já recebe mensagens NMEA do GPS, interpreta sentenças dos tipos `GGA` e `RMC` e identifica corretamente quando o GPS ainda está sem fix.

Como o módulo está momentaneamente sem antena, o comportamento esperado é:

* receber mensagens NMEA;
* detectar `fix_quality = 0`;
* detectar `satellites = 0`;
* detectar status `V` nas mensagens RMC;
* exibir no monitor que o GPS ainda está sem fix.

### Próxima etapa

Implementar um **diagnóstico técnico do GPS**, com contadores e status mais detalhado, incluindo:

* quantidade de mensagens `GGA` recebidas;
* quantidade de mensagens `RMC` recebidas;
* quantidade de mensagens `VTG` recebidas;
* mensagens ignoradas;
* mensagens com checksum inválido;
* tempo de execução;
* último status válido do GPS.

---

## Objetivo Geral

Criar um sistema de telemetria veicular onde um ESP32 instalado no veículo lê dados de um módulo GPS e envia informações para um servidor local.

O sistema deverá permitir:

* Monitorar latitude e longitude;
* Calcular velocidade instantânea;
* Registrar velocidade máxima;
* Calcular velocidade média;
* Medir distância percorrida;
* Identificar tempo parado e tempo em movimento;
* Exibir rota em mapa;
* Armazenar dados offline em microSD quando não houver rede;
* Reenviar dados quando a conexão for restabelecida.

---

## Arquitetura Planejada do Sistema

```text
ESP32 no veículo
    |
    +--> GPS GY-NEO6MV2 / NEO-6M
    |
    +--> Parser NMEA
    |
    +--> Cálculo de telemetria
    |       - velocidade instantânea
    |       - velocidade máxima
    |       - velocidade média
    |       - distância percorrida
    |       - tempo parado
    |       - tempo em movimento
    |
    +--> MQTT via Wi-Fi
    |
    +--> microSD para armazenamento offline
            |
            v
Servidor local
    |
    +--> Mosquitto MQTT Broker
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

## Arquitetura Atual Implementada

```text
GPS GY-NEO6MV2 / NEO-6M
    |
    | UART 9600 bps
    v
ESP32 - UART2
    |
    | Leitura byte a byte
    v
Montagem de linhas NMEA
    |
    v
Parser NMEA
    |
    +--> GGA: fix, satélites, HDOP e altitude
    |
    +--> RMC: validade, latitude, longitude, velocidade e data
    |
    v
Monitor ESP-IDF
```

Validação física da comunicação:

```text
GPS TX
    |
    +--> ESP32 GPIO16
    |
    +--> CH1 / D0 do analisador lógico

GPS GND
    |
    +--> GND do ESP32
    |
    +--> GND do analisador lógico
```

Infraestrutura local já implementada:

```text
WSL2 Ubuntu
    |
    v
Docker Compose
    |
    +--> Mosquitto MQTT - porta 1883
    |
    +--> Node-RED - porta 1880
```

---

## Tecnologias Utilizadas

* ESP32;
* ESP-IDF;
* GPS GY-NEO6MV2 / NEO-6M;
* UART;
* Parser NMEA;
* Analisador lógico 24 MHz / 8 canais;
* Saleae Logic 2;
* MQTT;
* Mosquitto;
* Node-RED;
* Docker;
* Docker Compose;
* WSL2 Ubuntu;
* Git e GitHub;
* VSCode;
* SQLite, planejado;
* microSD, planejado.

---

## Estrutura Atual do Projeto

```text
telemetria-gps/
├── database/
├── docs/
│   └── images/
│       ├── gps_uart_saleae_logic.jpg
│       └── gps_uart_espidf_monitor.jpg
├── gps_uart_test/
│   ├── CMakeLists.txt
│   ├── main/
│   │   ├── CMakeLists.txt
│   │   ├── gps_uart_test.c
│   │   ├── gps_parser.c
│   │   └── gps_parser.h
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

Observação: a pasta `build/` do ESP-IDF não deve ser versionada no GitHub.

---

## Serviços Docker

O projeto utiliza Docker Compose para subir os serviços principais.

### Mosquitto MQTT

Responsável por receber e distribuir mensagens MQTT.

Porta utilizada:

```text
1883
```

### Node-RED

Responsável por processar, visualizar e futuramente armazenar os dados de telemetria.

Porta utilizada:

```text
1880
```

Acesso pelo navegador:

```text
http://localhost:1880
```

---

## Arquivo docker-compose.yml

```yaml
services:
  mosquitto:
    image: eclipse-mosquitto:2
    container_name: telemetria-mosquitto
    restart: unless-stopped
    ports:
      - "1883:1883"
    volumes:
      - ./mosquitto/config:/mosquitto/config
      - ./mosquitto/data:/mosquitto/data
      - ./mosquitto/log:/mosquitto/log

  nodered:
    image: nodered/node-red:latest
    container_name: telemetria-nodered
    restart: unless-stopped
    ports:
      - "1880:1880"
    volumes:
      - ./nodered/data:/data
    depends_on:
      - mosquitto
```

---

## Configuração do Mosquitto

Arquivo:

```text
mosquitto/config/mosquitto.conf
```

Configuração atual:

```conf
listener 1883
allow_anonymous true

persistence true
persistence_location /mosquitto/data/

log_dest file /mosquitto/log/mosquitto.log
log_dest stdout
```

Nesta fase inicial, o broker permite conexão anônima para facilitar os testes. Em uma etapa futura, poderá ser adicionada autenticação com usuário e senha.

---

## Como Executar a Infraestrutura Local

Entrar na pasta do projeto:

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

Resultado esperado:

```text
telemetria-mosquitto   Up
telemetria-nodered     Up
```

Acessar o Node-RED:

```text
http://localhost:1880
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

## Teste MQTT Realizado

Foi testada a comunicação entre o terminal WSL, Mosquitto e Node-RED.

### Tópico utilizado

```text
telemetria/gps/dados
```

### Payload de teste

```json
{
  "velocidade": 45.7,
  "latitude": -21.12345,
  "longitude": -49.12345,
  "status": "movimento"
}
```

### Publicação MQTT pelo terminal

```bash
docker exec -it telemetria-mosquitto mosquitto_pub -h localhost -t telemetria/gps/dados -m '{"velocidade":45.7,"latitude":-21.12345,"longitude":-49.12345,"status":"movimento"}'
```

### Resultado

O Node-RED recebeu corretamente a mensagem JSON no painel de debug.

---

## Configuração do Node-RED

Fluxo inicial utilizado:

```text
mqtt in -> debug
```

Configuração do broker MQTT no Node-RED:

```text
Nome: Mosquitto Telemetria
Servidor: mosquitto
Porta: 1883
Protocolo: MQTT V3.1.1
```

Importante: dentro do Docker, o Node-RED acessa o broker pelo nome do serviço:

```text
mosquitto
```

Não deve ser usado `localhost` dentro do Node-RED para acessar o Mosquitto, pois cada container possui seu próprio ambiente de rede.

---

# Firmware ESP-IDF — Teste UART GPS

Foi criado um projeto ESP-IDF separado para validar a comunicação UART entre o ESP32 e o módulo GPS.

Diretório:

```text
gps_uart_test/
```

## Configuração utilizada

* Microcontrolador: ESP32;
* Framework: ESP-IDF;
* Ambiente: Linux/WSL2;
* Comunicação: UART2;
* Baud rate: 9600 bps;
* Formato: 8N1;
* RX do ESP32: GPIO16;
* TX do ESP32: GPIO17.

## Ligações entre GPS e ESP32

| GPS | ESP32            |
| --- | ---------------- |
| VCC | 5V               |
| GND | GND              |
| TX  | GPIO16           |
| RX  | GPIO17, opcional |

Para leitura das mensagens NMEA, a ligação principal é:

```text
GPS TX -> ESP32 GPIO16
GPS GND -> ESP32 GND
```

Durante os testes, houve um erro de ligação inicial no GND, que foi corrigido. Após a correção, o ESP32 passou a receber as mensagens NMEA corretamente.

---

## Como Compilar e Gravar o Teste UART

Entrar na pasta do projeto:

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

Para sair do monitor:

```text
CTRL + ]
```

Caso a porta seja diferente, verificar com:

```bash
ls /dev/ttyUSB*
```

ou:

```bash
ls /dev/ttyACM*
```

---

# Validação com Analisador Lógico

Além da leitura pelo ESP32, a comunicação UART foi validada fisicamente com um analisador lógico 24 MHz / 8 canais.

## Objetivo

Validar o sinal elétrico e o protocolo UART antes da implementação das próximas etapas do sistema.

Essa validação permite confirmar que:

* O GPS está transmitindo dados pela UART;
* O baud rate está correto;
* O ESP32 está recebendo os mesmos dados;
* As mensagens seguem o padrão NMEA;
* A ausência de fix não é causada por erro de comunicação.

---

## Ligações do Analisador Lógico

O analisador foi conectado em paralelo no TX do GPS.

| Sinal             | Ligação                  |
| ----------------- | ------------------------ |
| GPS TX            | ESP32 GPIO16             |
| GPS TX            | CH1 do analisador lógico |
| GPS GND           | GND do ESP32             |
| GND do analisador | GND comum                |

Representação:

```text
GPS TX
   |
   +---- ESP32 GPIO16
   |
   +---- CH1 analisador lógico

GPS GND
   |
   +---- ESP32 GND
   |
   +---- GND analisador lógico
```

No software Saleae Logic 2, o canal físico CH1 foi identificado como:

```text
D0
```

---

## Configuração no Saleae Logic 2

Decoder utilizado:

```text
Async Serial
```

Configuração:

```text
Input Channel: D0
Bit Rate: 9600
Bits per Frame: 8
Stop Bits: 1
Parity: None
Significant Bit: Least Significant Bit first
Signal: Non-inverted
```

---

## Captura com Analisador Lógico

O analisador lógico capturou o sinal UART no canal D0 e decodificou corretamente as mensagens NMEA.

![Captura UART GPS no Saleae Logic](docs/images/gps_uart_saleae_logic.jpg)

---

## Recepção no ESP32

As mesmas mensagens capturadas pelo analisador lógico também foram recebidas pelo ESP32 via UART2 e exibidas no monitor serial do ESP-IDF.

![Monitor ESP-IDF recebendo mensagens NMEA](docs/images/gps_uart_espidf_monitor.jpg)

---

# Parser NMEA

Nesta etapa foi implementado um parser NMEA para transformar as linhas recebidas do GPS em dados estruturados.

## Arquivos implementados

```text
main/
├── gps_uart_test.c
├── gps_parser.c
└── gps_parser.h
```

O arquivo `gps_parser.c` é responsável por interpretar as sentenças NMEA, enquanto o arquivo `gps_parser.h` define a estrutura de dados utilizada pelo firmware.

## Dados extraídos

O parser foi preparado para extrair:

* status de fix;
* latitude;
* longitude;
* velocidade em km/h;
* curso em graus;
* horário UTC;
* data;
* quantidade de satélites;
* qualidade do fix;
* HDOP;
* altitude.

## Sentenças tratadas

### GGA

A sentença `GGA` fornece informações como:

* qualidade do fix;
* quantidade de satélites;
* HDOP;
* altitude;
* latitude e longitude, quando houver fix.

Exemplo recebido:

```text
$GNGGA,,,,,,0,00,25.5,,,,,,*64
```

Interpretação:

```text
Fix quality: 0
Satélites: 0
HDOP: 25.5
```

O valor `fix_quality = 0` indica que o GPS ainda não possui posição válida.

### RMC

A sentença `RMC` fornece informações como:

* validade dos dados;
* latitude;
* longitude;
* velocidade;
* curso;
* data.

Exemplo recebido:

```text
$GNRMC,,V,,,,,,,,,,M*4E
```

Interpretação:

```text
Status: V
Dados inválidos
GPS sem fix
```

O campo `V` indica que os dados de navegação ainda não são válidos.

Quando o GPS possuir fix, esse campo deverá aparecer como `A`.

### VTG

A sentença `VTG` está relacionada à direção e velocidade sobre o solo.

Exemplo recebido:

```text
$GNVTG,,,,,,,,,M*2D
```

Como ainda não há fix válido, os campos de velocidade aparecem vazios.

---

## Saída Atual do Firmware

Exemplo real obtido no monitor do ESP-IDF:

```text
W (3329) GPS_APP: GPS sem fix ainda
W (3329) GPS_APP: Satélites: 0 | Qualidade fix: 0 | UTC:
I (3539) GPS_APP: NMEA: $GNVTG,,,,,,,,,M*2D
I (5079) GPS_APP: NMEA: $GNGGA,,,,,,0,00,25.5,,,,,,*64
I (5329) GPS_APP: NMEA: $GNRMC,,V,,,,,,,,,,M*4E
I (5539) GPS_APP: NMEA: $GNVTG,,,,,,,,,M*2D
I (7079) GPS_APP: NMEA: $GNGGA,,,,,,0,00,25.5,,,,,,*64
```

Essa saída confirma que:

* o ESP32 está recebendo dados do GPS;
* o baud rate está correto;
* as linhas NMEA estão sendo montadas corretamente;
* o parser reconhece mensagens `GGA` e `RMC`;
* o sistema identifica corretamente que ainda não há fix válido;
* o firmware continua estável, sem reinicializações.

---

# Etapas Concluídas

## Etapa 1 — Preparação do Ambiente Linux

* Instalação e configuração do WSL2;
* Criação do usuário Linux;
* Atualização do Ubuntu;
* Instalação de ferramentas básicas;
* Integração com VSCode.

Status: concluída.

---

## Etapa 2 — Instalação e Validação do Docker

* Docker Desktop instalado no Windows;
* Integração com Ubuntu WSL2 habilitada;
* Teste executado com:

```bash
docker run hello-world
```

Resultado obtido:

```text
Hello from Docker!
```

Status: concluída.

---

## Etapa 3 — Infraestrutura Docker

* Criado `docker-compose.yml`;
* Criadas pastas persistentes para Mosquitto, Node-RED e banco de dados;
* Subidos containers com:

```bash
docker compose up -d
```

Serviços iniciados:

* `telemetria-mosquitto`;
* `telemetria-nodered`.

Status: concluída.

---

## Etapa 4 — Teste de Comunicação MQTT

* Teste de publicação e assinatura MQTT realizado dentro do container Mosquitto;
* Teste de envio JSON para o Node-RED;
* Node-RED recebeu a mensagem corretamente no debug.

Status: concluída.

---

## Etapa 5 — Projeto ESP-IDF para Teste UART

* Projeto `gps_uart_test` criado;
* ESP32 configurado para UART2;
* RX em GPIO16;
* TX em GPIO17;
* Baud rate de 9600 bps;
* Firmware compilado e gravado no ESP32.

Status: concluída.

---

## Etapa 6 — Validação UART com GPS

* GPS conectado ao ESP32 via UART;
* Correção da ligação elétrica realizada;
* Mensagens NMEA recebidas no monitor serial;
* Comunicação entre GPS e ESP32 validada.

Status: concluída.

---

## Etapa 7 — Validação com Analisador Lógico

* Analisador lógico conectado ao TX do GPS;
* GND comum entre ESP32, GPS e analisador;
* Sinal UART capturado no canal D0;
* Decoder Async Serial configurado em 9600 bps;
* Mensagens NMEA visualizadas no Saleae Logic 2;
* Mensagens comparadas com o monitor do ESP-IDF.

Status: concluída.

---

## Etapa 8 — Parser NMEA

* Criado arquivo `gps_parser.h`;
* Criado arquivo `gps_parser.c`;
* Estrutura `gps_data_t` implementada;
* Parser para sentenças `GGA` implementado;
* Parser para sentenças `RMC` implementado;
* Conversão de coordenadas NMEA para decimal preparada;
* Conversão de velocidade de nós para km/h preparada;
* Validação de checksum implementada;
* Detecção de GPS sem fix funcionando.

Status: concluída.

---

# Próximas Etapas

## Etapa 9 — Diagnóstico Técnico do GPS

Adicionar contadores e informações técnicas para acompanhar a qualidade da comunicação com o GPS.

Planejamento:

* Contador de mensagens `GGA`;
* Contador de mensagens `RMC`;
* Contador de mensagens `VTG`;
* Contador de mensagens ignoradas;
* Contador de checksums inválidos;
* Tempo desde a inicialização;
* Última sentença NMEA recebida;
* Status textual do GPS: `SEM FIX`, `COM FIX`, `AGUARDANDO SATÉLITES`.

---

## Etapa 10 — Organização Modular do Firmware

Separar melhor as responsabilidades do firmware.

Estrutura planejada:

```text
main/
├── app_main.c
├── gps_uart.c
├── gps_uart.h
├── nmea_parser.c
├── nmea_parser.h
├── gps_data.c
└── gps_data.h
```

Responsabilidades planejadas:

```text
gps_uart.c
- Inicializar a UART
- Ler bytes do GPS
- Montar linhas NMEA brutas

nmea_parser.c
- Receber uma linha NMEA
- Identificar sentenças GNGGA, GNRMC e GNVTG
- Extrair campos relevantes
- Validar dados

gps_data.c
- Armazenar dados tratados
- Informar fix válido ou inválido
- Disponibilizar latitude, longitude, velocidade e satélites

app_main.c
- Inicializar o sistema
- Criar tasks
- Integrar os módulos
```

---

## Etapa 11 — Integração MQTT com ESP32

Criar firmware para:

* Conectar o ESP32 ao Wi-Fi;
* Publicar dados GPS em JSON;
* Enviar mensagens para o broker Mosquitto;
* Integrar com o Node-RED.

---

## Etapa 12 — Dashboard Inicial no Node-RED

Criar uma interface para visualizar:

* Velocidade atual;
* Status do veículo;
* Última latitude;
* Última longitude;
* Indicador de comunicação MQTT;
* Estado do fix GPS;
* Quantidade de satélites.

---

## Etapa 13 — Banco de Dados

Adicionar armazenamento dos dados recebidos.

Planejamento inicial:

* SQLite;
* Tabela de leituras GPS;
* Registro de data/hora;
* Registro de velocidade;
* Registro de latitude;
* Registro de longitude;
* Registro de status do GPS.

---

## Etapa 14 — Armazenamento Offline em microSD

Adicionar microSD ao ESP32 para:

* Salvar dados localmente quando não houver conexão;
* Reenviar dados quando a rede voltar;
* Evitar perda de telemetria durante o uso no veículo.

---

## Comandos Git Recomendados

Verificar alterações:

```bash
git status
```

Adicionar alterações:

```bash
git add README.md gps_uart_test docs/images .gitignore
```

Criar commit:

```bash
git commit -m "documenta parser nmea e leitura gps sem fix"
```

Enviar para o GitHub:

```bash
git push
```

---

## Autor

Guilherme Costa

Projeto desenvolvido com foco em aprendizado e portfólio profissional nas áreas de:

* Sistemas embarcados;
* ESP32;
* ESP-IDF;
* IoT;
* Redes;
* MQTT;
* Linux;
* Docker;
* Telemetria veicular;
* Validação de hardware com analisador lógico.
