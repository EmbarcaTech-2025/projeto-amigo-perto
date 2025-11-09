# Amigo Perto – Monitoramento de Proximidade BLE para Cães

Firmware embarcado em C para microcontroladores da Nordic Semiconductor, com comunicação Bluetooth Low Energy (BLE). O sistema monitora a proximidade entre o cão e o tutor, acionando um alerta sonoro na coleira quando o animal ultrapassa um limite configurado.

## Objetivo do Projeto

Desenvolver uma coleira eletrônica capaz de:

- Estabelecer conexão BLE segura com o smartphone do tutor
- Monitorar continuamente o RSSI
- Estimar a distância e comparar com um raio configurável
- Acionar alerta físico (buzzer) quando o limite for ultrapassado
- Manter consumo de energia reduzido para operação contínua

## Funcionalidades Principais

- BLE 5.x (advertising e/ou GATT)
- Medição periódica de RSSI
- Estimativa de distância com filtragem e histerese
- Controle de buzzer via PWM
- Armazenamento de parâmetros (limiar, emparelhamento, intensidade)
- Aplicativo móvel para configuração e monitoramento

## Arquitetura Resumida

Camadas de software utilizadas:

**Aplicação**
- Loop principal
- Leitura de RSSI
- Cálculo de distância
- Lógica do dome virtual
- Acionamento do buzzer

**HAL (Hardware Abstraction Layer)**
- BLE
- Buzzer
- Botão
- Bateria

**Drivers e Stack BLE**
- Zephyr RTOS ou bare metal com nRF Connect SDK

**Hardware**
- nRF52840 ou nRF54L15
- Buzzer
- Bateria compacta
- Botão de controle

## Exemplos de Estrutura de Diretórios

```
amigo_perto/
├── src/
│ ├── main.c
│ ├── proximity.c
│ └── hal/
│ ├── ble_hal.c
│ ├── ble_hal.h
│ ├── buzzer_hal.c
│ ├── buzzer_hal.h
│ ├── battery_hal.c
│ ├── battery_hal.h
│ └── button_hal.c
├── boards/
│ └── nrf52840_collar.overlay
├── prj.conf
├── CMakeLists.txt
└── README.md
```

## Hardware Utilizado

- **Microcontrolador**: nRF52840 ou nRF54L15
- **Conexão**: BLE 5.x
- **Buzzer** para alerta
- **Bateria** tipo Li-Po ou CR

## Como Compilar

**Requisitos:**
- nRF Connect SDK
- VSCODE com extensões Nordic

## Próximas Etapas
- Iniciar o desenvolvimento dos módulos de hardware e software
- Realizar testes unitários nos módulos individuais (Leitura de RSSI, comunicação BLE e etc)
- Testar implementação leitura periódica de RSSI no app móvel (modo Advertising) ou no firmware (modo conexão - GATT);
- Implementar HAL
- Aplicativo Android simples para configuração

## Contato

Equipe: Eric Senne Roma, Vitor Gomes e Antônio Almeida
