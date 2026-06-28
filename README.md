# ESP32 GPS Telemetry IDF

Sistema de telemetria veicular utilizando **ESP32**, **GPS GY-NEO6MV2 / NEO-6M**, **ESP-IDF**, **MQTT**, **Docker**, **Node-RED** e armazenamento offline.

O objetivo do projeto é desenvolver uma solução embarcada capaz de coletar dados de localização e movimento de um veículo, interpretar dados GPS em formato NMEA, transmitir informações para um servidor local e futuramente exibir esses dados em dashboard, mapa e banco de dados.

Este projeto está sendo desenvolvido com foco em práticas profissionais de **sistemas embarcados**, **firmware**, **eletrônica aplicada** e **telemetria IoT**, incluindo:

* desenvolvimento com ESP-IDF;
* uso de Linux/WSL2;
* validação de comunicação UART;
* análise de sinais com analisador lógico;
* parser NMEA;
* validação de checksum;
* diagnóstico técnico do GPS;
* validação real com antena GPS externa;
* organização modular do firmware;
* infraestrutura local com Docker;
* comunicação MQTT;
* documentação técnica por etapas.

---

## Status do Projeto

**Em desenvolvimento**

### Status atual

A comunicação entre o **GPS GY-NEO6MV2 / NEO-6M** e o **ESP32** já foi validada por UART no ESP-IDF.

Também foi realizada a validação física do sinal UART utilizando analisador lógico e, posteriormente, a validação real do GPS com antena externa instalada.

Com a antena conectada, o módulo GPS obteve **fix válido por satélite**, confirmando:

* recepção real de satélites;
* funcionamento da antena externa;
* comunicação UART entre GPS e ESP32;
* recepção de mensagens NMEA válidas;
* validação de checksum;
* identificação de fix GPS;
* leitura de velocidade pela sentença `VTG`;
* leitura de horário UTC;
* funcionamento do diagnóstico técnico do firmware.

O diagnóstico técnico do GPS exibe no monitor serial:

* quantidade de mensagens `GGA`;
* quantidade de mensagens `RMC`;
* quantidade de mensagens `VTG`;
* quantidade de mensagens `GSA`;
* quantidade de mensagens ignoradas;
* quantidade de checksums inválidos;
* quantidade de mensagens válidas;
* status do GPS;
* quantidade de satélites;
* qualidade do fix;
* tempo desde a inicialização;
* horário UTC;
* última mensagem NMEA válida recebida.

---

## Validação com Antena GPS

A antena externa foi conectada ao módulo GPS GY-NEO6MV2 / NEO-6M e o teste foi realizado em área externa, com céu parcialmente aberto.

Mesmo com visada parcial do céu, o módulo obteve fix válido após alguns minutos.

### Resultado obtido

O firmware identificou:

```text
Status GPS: FIX 2D
Satélites: 8
GGA fix quality: 1
RMC: A
Checksums inválidos: 0
Mensagens válidas: 1130
```

Isso confirma que o GPS saiu do estado anterior:

```text
AGUARDANDO SATELITES
Satelites: 0
GGA fix quality: 0
RMC: V
```

para um estado válido:

```text
FIX 2D
Satelites: 8
GGA fix quality: 1
RMC: A
```

### Evidência do terminal

![Validação do fix GPS no terminal ESP-IDF](docs/images/gps-fix-terminal.jpeg)

### Montagem utilizada no teste

![Montagem ESP32 com GPS e antena externa](docs/images/gps-esp32-montagem-antena.jpeg)

### Ligação entre ESP32, GPS e antena

![Ligação do hardware ESP32 e módulo GPS](docs/images/gps-ligacao-hardware.jpeg)

---

## Interpretação do Resultado

Durante o teste com a antena, o GPS apresentou:

```text
Satelites: 8
GGA fix quality: 1
RMC: A
```

A interpretação técnica é:

| Campo                    | Significado                                                  |
| ------------------------ | ------------------------------------------------------------ |
| `Satelites: 8`           | O módulo está recebendo satélites suficientes para navegação |
| `GGA fix quality: 1`     | Existe fix GPS válido                                        |
| `RMC: A`                 | Os dados de navegação são válidos                            |
| `Checksums inválidos: 0` | As mensagens NMEA recebidas não apresentaram corrupção       |
| `Status GPS: FIX 2D`     | O firmware identificou posição GPS válida                    |

A sentença `VTG` também passou a apresentar velocidade válida:

```text
$GNVTG,348.69,T,,M,1.49,N,2.75,K,A*2F
```

Interpretação:

| Campo              | Valor       |
| ------------------ | ----------- |
| Curso verdadeiro   | `348.69°`   |
| Velocidade em nós  | `1.49 N`    |
| Velocidade em km/h | `2.75 K`    |
| Status             | `A`, válido |

Como o teste foi feito parado ou com pouca movimentação, pequenas velocidades como `1 km/h`, `2 km/h` ou `3 km/h` podem aparecer devido à oscilação natural do GPS. Em etapas futuras, será implementado filtro para considerar o veículo parado abaixo de um limite mínimo de velocidade.

Exemplo planejado:

```c
if (velocidade_kmh < 2.0) {
    veiculo_parado = true;
}
```

---

## Observação Sobre Mensagens GSA

Durante o teste, o firmware apresentou:

```text
Mensagens GSA: 0
GSA fix type: 1
```

Isso não indica falha de hardware.

O GPS foi validado com sucesso usando as sentenças `GGA`, `RMC` e `VTG`. O módulo pode simplesmente não estar enviando sentenças `GSA` no ciclo atual, ou o firmware ainda pode não estar tratando todas as variações possíveis, como:

```text
$GPGSA
$GNGSA
$GLGSA
$BDGSA
```

Para a validação principal do projeto, o firmware considera como critérios mais importantes:

```text
GGA fix quality > 0
RMC = A
Satélites >= 4
```

Portanto, mesmo com `Mensagens GSA: 0`, o teste da antena e do GPS foi considerado aprovado.

Em uma etapa futura, o diagnóstico poderá ser ajustado para exibir:

```text
GSA fix type: N/A
```

quando nenhuma sentença `GSA` tiver sido recebida.

---

## Etapas já implementadas

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
* Parser NMEA inicial implementado;
* Validação de checksum implementada;
* Diagnóstico técnico do GPS implementado;
* Detecção de GPS sem fix funcionando corretamente;
* Antena GPS externa instalada;
* Primeiro fix GPS obtido com sucesso;
* GPS validado com satélites reais;
* Recepção de mensagens `GGA`, `RMC` e `VTG` com dados válidos;
* Registro fotográfico da montagem e do terminal.

---

## Objetivo Geral

Criar um sistema de telemetria veicular onde um ESP32 instalado no veículo lê dados de um módulo GPS e envia informações para um servidor local.

O sistema deverá permitir:

* monitorar latitude e longitude;
* calcular velocidade instantânea;
* registrar velocidade máxima;
* calcular velocidade média;
* medir distância percorrida;
* identificar tempo parado e tempo em movimento;
* exibir rota em mapa;
* armazenar dados offline em microSD quando não houver rede;
* reenviar dados quando a conexão for restabelecida.

---

## Arquitetura Planejada do Sistema

```text
ESP32 no veículo
    |
    +--> GPS GY-NEO6MV2 / NEO-6M
    |       |
    |       +--> Antena GPS externa
    |
    +--> Parser NMEA
    |
    +--> Diagnóstico GPS
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
    +--> Antena GPS externa
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
Validação de checksum
    |
    v
Parser NMEA inicial
    |
    v
Diagnóstico técnico GPS
    |
    +--> Contador GGA
    +--> Contador RMC
    +--> Contador VTG
    +--> Contador GSA
    +--> Contador de mensagens válidas
    +--> Contador de checksums inválidos
    +--> Contador de mensagens ignoradas
    +--> Status do fix
    +--> Satélites
    +--> UTC
    +--> Última mensagem válida
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
* Antena GPS externa;
* UART;
* Parser NMEA;
* Validação de checksum;
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
│       ├── gps_uart_espidf_monitor.jpg
│       ├── gps-fix-terminal.jpeg
│       ├── gps-esp32-montagem-antena.jpeg
│       └── gps-ligacao-hardware.jpeg
├── gps_uart_test/
│   ├── CMakeLists.txt
│   ├── main/
│   │   ├── CMakeLists.txt
│   │   ├── main.c
│   │   ├── gps_diagnostic.c
│   │   └── gps_diagnostic.h
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

## Como Compilar e Gravar o Firmware

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

* o GPS está transmitindo dados pela UART;
* o baud rate está correto;
* o ESP32 está recebendo os mesmos dados;
* as mensagens seguem o padrão NMEA;
* a ausência de fix inicial não era causada por erro de comunicação.

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

# Parser NMEA e Diagnóstico Técnico

Nesta etapa foi implementada uma camada de diagnóstico para acompanhar a qualidade das mensagens NMEA recebidas pelo ESP32.

## Arquivos implementados

```text
gps_uart_test/main/
├── main.c
├── gps_diagnostic.c
└── gps_diagnostic.h
```

O arquivo `gps_diagnostic.c` é responsável por:

* validar checksum NMEA;
* identificar o tipo da sentença;
* contar mensagens recebidas por tipo;
* armazenar a última mensagem válida;
* atualizar o status do GPS;
* imprimir o diagnóstico técnico no monitor ESP-IDF.

O arquivo `gps_diagnostic.h` define a estrutura de dados do diagnóstico e os estados possíveis do GPS.

---

## Sentenças monitoradas

### GGA

A sentença `GGA` fornece informações como:

* qualidade do fix;
* quantidade de satélites;
* HDOP;
* altitude;
* latitude e longitude, quando houver fix.

Exemplo de teste sem fix:

```text
$GNGGA,,,,,,0,00,25.5,,,,,,*64
```

Interpretação:

```text
Fix quality: 0
Satélites: 0
GPS sem posição válida
```

Com antena instalada e fix válido, o firmware passou a identificar:

```text
GGA fix quality: 1
Satélites: 8
```

---

### RMC

A sentença `RMC` fornece informações como:

* validade dos dados;
* latitude;
* longitude;
* velocidade;
* curso;
* data.

Exemplo de teste sem fix:

```text
$GNRMC,,V,,,,,,,,,,M*4E
```

Interpretação:

```text
Status: V
Dados inválidos
GPS sem fix
```

Com antena instalada e fix válido, o status passou para:

```text
RMC: A
```

O campo `A` indica que os dados de navegação estão válidos.

---

### VTG

A sentença `VTG` está relacionada à direção e velocidade sobre o solo.

Exemplo com dados válidos após fix:

```text
$GNVTG,348.69,T,,M,1.49,N,2.75,K,A*2F
```

Interpretação:

```text
Curso verdadeiro: 348.69 graus
Velocidade: 1.49 nós
Velocidade: 2.75 km/h
Status: A
```

Essa sentença será importante para cálculo de:

* velocidade instantânea;
* velocidade máxima;
* velocidade média;
* detecção de veículo parado;
* detecção de veículo em movimento.

---

### GSA

A sentença `GSA` pode indicar se o GPS possui fix 2D ou 3D.

Status possíveis considerados:

```text
1 - Sem fix
2 - Fix 2D
3 - Fix 3D
```

No teste atual, o módulo não enviou mensagens `GSA`, mas isso não impediu a validação do fix GPS, pois o firmware conseguiu confirmar dados válidos por meio das sentenças `GGA`, `RMC` e `VTG`.

---

## Diagnóstico implementado

O diagnóstico técnico exibe:

* tempo desde a inicialização;
* status do GPS;
* contador de mensagens `GGA`;
* contador de mensagens `RMC`;
* contador de mensagens `VTG`;
* contador de mensagens `GSA`;
* contador de mensagens ignoradas;
* contador de checksums inválidos;
* contador de mensagens válidas;
* quantidade de satélites;
* qualidade do fix;
* tipo de fix informado por GSA;
* status da sentença RMC;
* horário UTC;
* última mensagem válida recebida.

---

## Saída Inicial Sem Fix

Exemplo real obtido no monitor do ESP-IDF durante teste inicial sem fix:

```text
I (6338) GPS_APP: ================ DIAGNOSTICO GPS ================
I (6338) GPS_APP: Tempo desde inicializacao: 6 s
I (6338) GPS_APP: Status GPS: AGUARDANDO SATELITES
I (6338) GPS_APP: Mensagens GGA: 3
I (6338) GPS_APP: Mensagens RMC: 3
I (6348) GPS_APP: Mensagens VTG: 3
I (6348) GPS_APP: Mensagens GSA: 0
I (6348) GPS_APP: Mensagens ignoradas: 0
I (6348) GPS_APP: Checksums invalidos: 0
I (6358) GPS_APP: Mensagens validas: 9
I (6358) GPS_APP: Satelites: 0 | GGA fix quality: 0 | GSA fix type: 1 | RMC: V
I (6368) GPS_APP: UTC: --
I (6368) GPS_APP: Ultima mensagem valida: $GNVTG,,,,,,,,,M*2D
I (6378) GPS_APP: =================================================
```

Essa saída confirmou que:

* o ESP32 estava recebendo dados do GPS;
* o baud rate estava correto;
* as linhas NMEA estavam sendo montadas corretamente;
* as mensagens estavam passando pela validação de checksum;
* o firmware identificava corretamente que o GPS ainda estava aguardando satélites.

---

## Saída Atual Com Antena e Fix GPS

Exemplo real obtido no monitor do ESP-IDF após instalação da antena GPS externa:

```text
I (750227) GPS_APP: ================ DIAGNOSTICO GPS ================
I (750227) GPS_APP: Tempo desde inicializacao: 749 s
I (750227) GPS_APP: Status GPS: FIX 2D
I (750227) GPS_APP: Mensagens GGA: 374
I (750227) GPS_APP: Mensagens RMC: 374
I (750237) GPS_APP: Mensagens VTG: 374
I (750237) GPS_APP: Mensagens GSA: 0
I (750237) GPS_APP: Mensagens ignoradas: 8
I (750247) GPS_APP: Checksums invalidos: 0
I (750247) GPS_APP: Mensagens validas: 1130
I (750247) GPS_APP: Satelites: 8 | GGA fix quality: 1 | GSA fix type: 1 | RMC: A
I (750257) GPS_APP: UTC: 183256.000
I (750257) GPS_APP: Ultima mensagem valida: $GNVTG,348.69,T,,M,1.49,N,2.75,K,A*2F
I (750267) GPS_APP: =================================================
```

Essa saída confirma que:

* o GPS obteve fix válido;
* a antena externa está funcionando;
* o módulo recebeu 8 satélites;
* a sentença `GGA` indicou `fix quality = 1`;
* a sentença `RMC` indicou status `A`;
* a sentença `VTG` apresentou velocidade válida;
* não houve checksum inválido;
* o firmware está interpretando corretamente o estado do GPS.

---

## Status do GPS no Firmware

O firmware trabalha com os seguintes estados:

```text
AGUARDANDO SATELITES
SEM FIX
FIX 2D
FIX 3D
```

No estado atual do projeto, após a instalação da antena externa, o status exibido é:

```text
FIX 2D
```

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

## Etapa 8 — Parser NMEA Inicial

* Leitura de sentenças NMEA;
* Identificação de mensagens `GGA`;
* Identificação de mensagens `RMC`;
* Identificação de mensagens `VTG`;
* Detecção de GPS sem fix;
* Preparação para extração de dados de posição, velocidade e horário.

Status: concluída.

---

## Etapa 9 — Diagnóstico Técnico do GPS

* Contador de mensagens `GGA`;
* Contador de mensagens `RMC`;
* Contador de mensagens `VTG`;
* Contador de mensagens `GSA`;
* Contador de mensagens ignoradas;
* Contador de checksums inválidos;
* Contador de mensagens válidas;
* Status textual do GPS;
* Tempo desde a inicialização;
* Última sentença NMEA válida recebida;
* Diagnóstico periódico no monitor serial.

Status: concluída.

---

## Etapa 10 — Validação com Antena GPS

* Antena GPS externa conectada ao módulo;
* Teste realizado em área externa com céu parcialmente aberto;
* GPS saiu do estado `AGUARDANDO SATELITES`;
* Fix válido obtido;
* 8 satélites identificados;
* `GGA fix quality = 1`;
* `RMC = A`;
* `VTG` com velocidade válida;
* Nenhum checksum inválido;
* Montagem documentada com fotos;
* Log do terminal documentado.

Status: concluída.

---

# Próximas Etapas

## Etapa 11 — Extração de Latitude, Longitude e Velocidade

Implementar o tratamento completo dos dados úteis para telemetria.

Planejamento:

* converter latitude NMEA para decimal;
* converter longitude NMEA para decimal;
* converter velocidade de nós para km/h;
* armazenar último ponto válido;
* exibir latitude e longitude no monitor;
* diferenciar dados válidos e inválidos;
* tratar ruído de velocidade quando o GPS estiver parado;
* definir limite mínimo para considerar movimento real.

---

## Etapa 12 — Organização Modular do Firmware

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

## Etapa 13 — Integração MQTT com ESP32

Criar firmware para:

* conectar o ESP32 ao Wi-Fi;
* publicar dados GPS em JSON;
* enviar mensagens para o broker Mosquitto;
* integrar com o Node-RED.

---

## Etapa 14 — Dashboard Inicial no Node-RED

Criar uma interface para visualizar:

* velocidade atual;
* status do veículo;
* última latitude;
* última longitude;
* indicador de comunicação MQTT;
* estado do fix GPS;
* quantidade de satélites.

---

## Etapa 15 — Banco de Dados

Adicionar armazenamento dos dados recebidos.

Planejamento inicial:

* SQLite;
* tabela de leituras GPS;
* registro de data/hora;
* registro de velocidade;
* registro de latitude;
* registro de longitude;
* registro de status do GPS.

---

## Etapa 16 — Armazenamento Offline em microSD

Adicionar microSD ao ESP32 para:

* salvar dados localmente quando não houver conexão;
* reenviar dados quando a rede voltar;
* evitar perda de telemetria durante o uso no veículo.

---

# Comandos Git Recomendados

Verificar alterações:

```bash
git status
```

Adicionar README e imagens:

```bash
git add README.md docs/images/gps-fix-terminal.jpeg docs/images/gps-esp32-montagem-antena.jpeg docs/images/gps-ligacao-hardware.jpeg
```

Criar commit:

```bash
git commit -m "docs: documentar validacao do GPS com antena"
```

Enviar para o GitHub:

```bash
git push
```

---

# Observações Técnicas

A etapa atual comprova pontos importantes do sistema embarcado:

* comunicação UART funcional;
* alimentação e GND corretos;
* baud rate correto;
* recepção de sentenças NMEA;
* validação de checksum funcionando;
* ausência de mensagens corrompidas no teste com antena;
* antena GPS externa funcionando;
* fix GPS obtido;
* recepção real de satélites;
* firmware estável;
* diagnóstico técnico periódico implementado.

A próxima etapa será transformar os dados NMEA válidos em informações úteis para telemetria, principalmente:

* latitude em decimal;
* longitude em decimal;
* velocidade em km/h;
* status parado/em movimento;
* distância percorrida;
* velocidade máxima;
* velocidade média.

---

# Autor

Guilherme Costa

Projeto desenvolvido com foco em aprendizado e portfólio profissional nas áreas de:

* Sistemas embarcados;
* Firmware;
* ESP32;
* ESP-IDF;
* IoT;
* Redes;
* MQTT;
* Linux;
* Docker;
* Telemetria veicular;
* Validação de hardware com analisador lógico.
