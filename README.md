# Amigo Perto – Monitoramento de Proximidade BLE para PETs

Firmware embarcado em C para microcontroladores da Nordic Semiconductor, com comunicação Bluetooth Low Energy (BLE). O sistema monitora a proximidade entre o cão e o tutor, acionando um alerta sonoro na coleira quando o animal ultrapassa um limite configurado.

## Objetivo do Projeto

Desenvolver uma coleira eletrônica capaz de:

- Estabelecer conexão BLE segura com o smartphone do tutor
- Fornecer leitura de RSSI para o aplicativo móvel
- Receber comandos remotos para acionamento do buzzer
- Reportar status de bateria em tempo real
- Manter consumo de energia reduzido para operação contínua

## Funcionalidades Principais

- BLE 5.x (advertising e GATT)
- Serviço GATT customizado para controle do buzzer
- Serviço GATT padrão Battery (0x180F)
- Controle de buzzer via PWM com modo intermitente
- Monitoramento de bateria via ADC
- Aplicativo móvel para leitura de RSSI, cálculo de distância e controle remoto

## Arquitetura Resumida

Camadas de software utilizadas:

**Aplicação**
- Loop principal
- Callbacks BLE
- Lógica de controle

**HAL (Hardware Abstraction Layer)**
- BLE
- Buzzer
- Bateria

**Serviços GATT**
- Buzzer Service (customizado)
- Battery Service (padrão 0x180F)

**Drivers e Stack BLE**
- Zephyr RTOS com nRF Connect SDK

**Hardware**
- XIAO nRF52840
- Buzzer piezoelétrico
- Bateria Li-Po
- LEDs de status

## Exemplo de Estrutura de Diretórios

```
amigo_perto_v2/
├── src/
│   ├── main.c
│   ├── hal/
│   │   ├── ble.c
│   │   ├── buzzer.c
│   │   └── battery.c
│   └── gatt/
│       ├── buzzer_service.c
│       └── battery_service.c
├── include/
│   ├── hal/
│   └── gatt/
├── boards/
│   ├── xiao_ble.overlay
│   └── nrf52840dongle_nrf52840.overlay
├── prj.conf
├── CMakeLists.txt
└── README.md
```

## Hardware Utilizado

- **Microcontrolador**: XIAO nRF52840 (Nordic nRF52840)
- **Conexão**: BLE 5.3
- **Buzzer** piezoelétrico para alerta (18kHz)
- **Bateria** tipo Li-Po (3.0V - 4.2V)
- **LEDs** de status (verde e azul)

## Como Compilar

**Requisitos:**
- nRF Connect SDK v3.1.0+
- VS Code com extensões Nordic (nRF Connect for VS Code)
- Toolchain ARM GCC

**Passos:**
```bash
# Clone o repositório
git clone <repository-url>
cd amigo_perto_v2

# Compile o projeto
west build -b xiao_ble/nrf52840 --pristine

# Flash no hardware
west flash
```

## Lógica de Distância

**A lógica de conversão RSSI → distância é implementada no aplicativo móvel**, não no firmware:

1. **Aplicativo** lê RSSI periodicamente via BLE
2. **Aplicativo** calcula distância usando modelo de propagação log-distance
3. **Aplicativo** compara com limiar configurado
4. **Se ultrapassar**: aplicativo envia comando para ativar buzzer via GATT
5. **Firmware** aciona buzzer em modo intermitente (economia de energia)


## Próximas Etapas

- [x] Implementar HAL (BLE, Buzzer, Battery)
- [x] Implementar serviços GATT
- [x] Otimizar consumo de energia - Primeira versão
- [x] Desenvolver aplicativo Android
- [x] Testes de consumo de energia
- [ ] Integração com o aplicativo
- [ ] Testes avançados
- [ ] Integração com o case


## Contato

Equipe: Eric Senne Roma, Vitor Gomes e Antônio Almeida
