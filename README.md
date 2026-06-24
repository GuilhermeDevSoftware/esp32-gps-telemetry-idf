# ESP32 GPS Telemetry IDF

Sistema de telemetria veicular utilizando **ESP32**, **GPS GY-NEO6MV2**, **ESP-IDF**, **MQTT**, **Docker**, **Node-RED** e armazenamento offline.

O objetivo do projeto é desenvolver uma solução embarcada capaz de coletar dados de localização e movimento de um veículo, transmitir os dados via MQTT para um servidor local e exibir informações em dashboard, mapa e banco de dados.

---

## Status do Projeto

**Em desenvolvimento**

Etapa atual concluída:

* Ambiente Linux com WSL2 configurado
* Docker instalado e validado
* Mosquitto MQTT rodando em container
* Node-RED rodando em container
* Comunicação MQTT testada com sucesso
* Node-RED recebendo mensagens JSON via MQTT

---

## Objetivo Geral

Criar um sistema de telemetria veicular onde um ESP32 instalado no veículo lê dados de um módulo GPS e envia informações para um servidor local.

O sistema deverá permitir:

* Monitorar latitude e longitude
* Calcular velocidade instantânea
* Registrar velocidade máxima
* Calcular velocidade média
* Medir distância percorrida
* Identificar tempo parado e tempo em movimento
* Exibir rota em mapa
* Armazenar dados offline em microSD quando não houver rede
* Reenviar dados quando a conexão for restabelecida

---

## Arquitetura Planejada

```text
ESP32 + GPS GY-NEO6MV2
        |
        | MQTT via Wi-Fi
        v
Mosquitto MQTT Broker
        |
        v
Node-RED
        |
        +--> Dashboard
        +--> Banco de dados
        +--> Mapa
        +--> Análise de telemetria
```

---

## Arquitetura Atual Implementada

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

Nesta etapa, os dados ainda são simulados pelo terminal. O ESP32 será integrado nas próximas fases.

---

## Tecnologias Utilizadas

* ESP32
* ESP-IDF
* GPS GY-NEO6MV2
* MQTT
* Mosquitto
* Node-RED
* Docker
* Docker Compose
* WSL2 Ubuntu
* Git e GitHub
* SQLite
* VSCode

---

## Estrutura Atual do Projeto

```text
telemetria-gps/
├── database/
├── docker-compose.yml
├── mosquitto/
│   ├── config/
│   │   └── mosquitto.conf
│   ├── data/
│   └── log/
├── nodered/
│   └── data/
├── .gitignore
└── README.md
```

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

Nesta fase inicial, o broker está permitindo conexão anônima para facilitar os testes. Em uma etapa futura, será adicionada autenticação com usuário e senha.

---

## Como Executar o Projeto

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

## Etapas Concluídas

### Etapa 1 — Preparação do Ambiente Linux

* Instalação e configuração do WSL2
* Criação do usuário Linux
* Atualização do Ubuntu
* Instalação de ferramentas básicas:

  * Git
  * Python
  * SQLite
  * Curl
  * Wget
  * Build-essential

Status: concluída.

---

### Etapa 2 — Integração VSCode com WSL2

* Projeto criado em `/home/guilherme/projetos/telemetria-gps`
* Pasta aberta pelo VSCode usando WSL
* Terminal Linux validado dentro do VSCode

Status: concluída.

---

### Etapa 3 — Instalação e Validação do Docker

* Docker Desktop instalado no Windows
* Integração com Ubuntu WSL2 habilitada
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

### Etapa 4 — Criação da Infraestrutura Docker

* Criado `docker-compose.yml`
* Criadas pastas persistentes para Mosquitto, Node-RED e banco de dados
* Subidos containers com:

```bash
docker compose up -d
```

Serviços iniciados:

* `telemetria-mosquitto`
* `telemetria-nodered`

Status: concluída.

---

### Etapa 5 — Teste de Comunicação MQTT

* Teste de publicação e assinatura MQTT realizado dentro do container Mosquitto
* Teste de envio JSON para o Node-RED
* Node-RED recebeu a mensagem corretamente no debug

Status: concluída.

---

## Próximas Etapas

### Etapa 6 — Simulador de Telemetria GPS em Python

Criar um script Python para simular dados de GPS antes de integrar o ESP32 real.

O simulador deverá publicar mensagens MQTT com:

* Latitude
* Longitude
* Velocidade
* Status do veículo
* Timestamp

---

### Etapa 7 — Dashboard Inicial no Node-RED

Criar uma interface para visualizar:

* Velocidade atual
* Status do veículo
* Última latitude
* Última longitude
* Indicador de comunicação MQTT

---

### Etapa 8 — Banco de Dados

Adicionar armazenamento dos dados recebidos.

Inicialmente planejado:

* SQLite
* Tabela de leituras GPS
* Registro de data/hora
* Registro de velocidade
* Registro de latitude e longitude

---

### Etapa 9 — Integração com ESP32

Criar firmware em ESP-IDF para:

* Conectar o ESP32 ao Wi-Fi
* Ler dados do GPS via UART
* Interpretar dados NMEA
* Publicar JSON via MQTT

---

### Etapa 10 — Armazenamento Offline

Adicionar microSD ao ESP32 para:

* Salvar dados localmente quando não houver conexão
* Reenviar dados quando a rede voltar
* Evitar perda de telemetria

---

## Comandos Git Utilizados

Inicialização do repositório:

```bash
git init
git add .
git commit -m "Configura ambiente Docker com Mosquitto e Node-RED"
git branch -M main
git remote add origin https://github.com/GuilhermeDevSoftware/esp32-gps-telemetry-idf.git
git push -u origin main
```

---

## Autor

Guilherme Costa

Projeto desenvolvido com foco em aprendizado e portfólio profissional nas áreas de:

* Sistemas embarcados
* ESP32
* ESP-IDF
* IoT
* Redes
* MQTT
* Linux
* Docker
* Telemetria veicular
# esp32-gps-telemetry-idf
