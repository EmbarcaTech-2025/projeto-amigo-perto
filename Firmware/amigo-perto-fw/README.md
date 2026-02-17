# Amigo Perto — Firmware (Zephyr / nRF Connect SDK)

![Plataforma](https://img.shields.io/badge/Plataforma-XIAO%20nRF52840-red)
![RTOS](https://img.shields.io/badge/RTOS-Zephyr-blue)
![BLE](https://img.shields.io/badge/BLE-Perif%C3%A9rico%20(GATT%20Server)-brightgreen)
![Build](https://img.shields.io/badge/Build-west%20%2B%20CMake-orange)

Firmware do dispositivo **Amigo Perto** (coleira/tag BLE). Implementa **advertising BLE**, **GATT Server** (bateria + buzzer) e controle de **LEDs**, **buzzer PWM** e **telemetria de bateria via ADC** com foco em **baixo consumo**.

- Entrada: `src/main.c`
- Board/overlay: `boards/xiao_ble.overlay`
- Config: `prj.conf`

---

## ✅ O que o firmware faz

- BLE Peripheral (GAP): anuncia como **Amigo Perto** e aceita conexão
- GATT Server:
  - **Battery Service (0x180F)** + **Battery Level (0x2A19)** (leitura de %)
  - **Battery Voltage (custom)**: leitura adicional em **mV (uint16)** (diagnóstico)
  - **Buzzer Service (custom)**: escrita **0x00=OFF / 0x01=ON**
- LEDs de status (baixo duty-cycle via `k_timer`):
  - **Azul**: advertising ativo
  - **Verde**: conectado
- Buzzer (HAL PWM): **18 kHz** + modo intermitente **20ms ON / 80ms OFF**
- Bateria (HAL ADC): leitura VBAT com filtros (trimmed mean + IIR + rejeição de spikes) e callbacks

---

## 🧩 Arquitetura (camadas)

A arquitetura segue o diagrama em blocos e separa responsabilidades em 6 camadas:

1) **Aplicação (Firmware)** — orquestra boot, LEDs e reação a eventos
- Inicialização e timers dos LEDs
- Callbacks BLE (`connected/disconnected/adv_*`)
- Callback de escrita GATT do buzzer (liga/desliga)
- Referência: `src/main.c`

2) **GATT (Serviços BLE)** — interface consumida pelo cliente
- Battery Service (0x180F) + Battery Level (0x2A19)
- Battery Voltage (custom uint16 mV)
- Buzzer Service (custom) com write 0/1 + validações
- Referências:
  - `src/gatt/battery_service.c`, `include/gatt/battery_service.h`
  - `src/gatt/buzzer_service.c`, `include/gatt/buzzer_service.h`

3) **HAL (Hardware Abstraction Layer)** — APIs simples para Zephyr/hardware
- `hal_ble`: advertising + callbacks, start assíncrono via `k_work`
- `hal_battery`: ADC + filtros + amostragem periódica + callbacks
- `hal_buzzer`: PWM 18kHz + burst 20/80ms via `k_work_delayable`
- Referências:
  - `src/hal/ble.c`, `include/hal/ble.h`
  - `src/hal/battery.c`, `include/hal/battery.h`
  - `src/hal/buzzer.c`, `include/hal/buzzer.h`

4) **Zephyr (RTOS + drivers + stack BLE)**
- Bluetooth stack (GAP/GATT), drivers (GPIO/PWM/ADC), kernel (`k_work`, `k_timer`) e logging

5) **Config (Kconfig + Devicetree)**
- Kconfig: `prj.conf`
- Devicetree overlay: `boards/xiao_ble.overlay`
- Binding DT (bateria): `dts/bindings/xiao-ble-battery.yaml`

6) **Hardware**
- nRF52840 SoC + LEDs, buzzer (PWM) e bateria (ADC + sinais do carregador)

---

## 🔌 Mapeamento de pinos (do overlay)

Definido em `boards/xiao_ble.overlay`:

- **LED Verde**: P0.30 (alias `ledverde`)
- **LED Azul**: P0.06 (alias `ledazul`)
- **Buzzer (PWM0 OUT0)**: P0.28 (alias `pwm-led0` / `pwm-led0`)
- **Bateria (ADC)**: canal 7 (AIN7; típico P0.31)
- **Sinais do carregador / medição** (active-low)
  - `charging-enable`: P0.17
  - `read-enable`: P0.14
  - `charge-speed`: P0.13

---

## 📡 Contrato BLE (GATT) — serviços e características

### Battery Service (Bluetooth SIG)

| Item | UUID | Tipo | Propriedades | Valor |
|---|---:|---|---|---|
| Battery Service | `0x180F` | Service | - | - |
| Battery Level | `0x2A19` | Characteristic | Read | `uint8` (0–100) |

Implementação: `src/gatt/battery_service.c`

### Battery Voltage (custom)

- **UUID (128-bit derivado)**: usa `BT_UUID_128_ENCODE(0x00002B19, 0x0000, 0x1000, 0x8000, 0x00805F9B34FB)`
- **Formato**: `uint16` little-endian
- **Unidade (CPF)**: volt com expoente `-3` (valor em mV)
- **Cache TTL**: 750 ms (evita leituras repetidas do ADC em sequência)

Implementação: `src/gatt/battery_service.c`

### Buzzer Service (custom)

| Item | UUID | Tipo | Propriedades | Valor |
|---|---|---|---|---|
| Buzzer Service | `12345678-ABCD-EFAB-CDEF-123456789ABC` | Service | - | - |
| Buzzer Intermittent | `12345679-ABCD-EFAB-CDEF-123456789ABC` | Characteristic | Write | `0x00` OFF / `0x01` ON |

Validações do write:
- `len == 1`
- `offset == 0`
- valor permitido: `0x00` ou `0x01`

Implementação: `src/gatt/buzzer_service.c`

---

## 🔄 Fluxogramas

Legenda rápida (termos usados abaixo):

- **GATT**: tabela de serviços/characteristics BLE exposta pelo dispositivo.
- **Characteristic (característica)**: “campo” do BLE que pode ser lido/escrito.
- **Write handler**: função no firmware chamada quando o cliente escreve numa característica.
- **HAL**: camada do firmware que simplifica acesso a drivers (PWM/ADC/BLE) do Zephyr.
- **Workqueue (`k_work`)**: mecanismo do Zephyr para rodar tarefas assíncronas com segurança.

### 1) Boot e inicialização (alto nível)

```text
[Power ON]
  |
  v
[Init LEDs + timers]
  |
  v
[Init HAL Buzzer]
  |
  v
[Init HAL BLE]
  |
  v
[Init GATT services]
  |
  v
[Start Advertising]
  |
  v
[Init Battery (best-effort)]
  |
  v
[Register battery callbacks]
  |
  v
[Sample once + start periodic sampling]
  |
  v
[Idle: wait for callbacks]
```

Notas relevantes (implementação atual em `src/main.c`):
- O **advertising é iniciado antes** do `battery_init()` (bateria é *best-effort*).
- Battery Service é inicializado antes da bateria; se a bateria falhar, leituras podem retornar **0**.

### 2) Máquina de estados BLE (status + LEDs)

```text
                 +-------------------+
                 |       Boot        |
                 +-------------------+
                           |
                           | hal_ble_start_advertising
                           v
                 +-------------------+
                 |    Advertising    |
                 |  LED Azul pisca   |
                 +-------------------+
                           |
                           | on_connected
                           v
                 +-------------------+
                 |     Connected     |
                 | LED Verde pisca   |
                 | (Buzzer OFF ao    |
                 |  desconectar)     |
                 +-------------------+
                           |
                           | on_disconnected / on_recycled
                           v
                 +-------------------+
                 |    Advertising    |
                 |  LED Azul pisca   |
                 +-------------------+
```

### 3) Fluxo do comando do buzzer (GATT → App → HAL)

```text
[Cliente BLE (WebApp/App)]
  |
  | Escreve na característica do buzzer (1 byte): 0x00=DESLIGA / 0x01=LIGA
  v
[GATT: Característica BLE do Buzzer (WRITE)]
  |
  | (Write handler no firmware)
  v
{ Valida dados recebidos }
  - tamanho (len) == 1
  - offset == 0
  - valor ∈ {0x00, 0x01}
  | ok                               | inválido
  v                                  v
[Callback da aplicação (main.c)]         [Erro GATT (valor/tamanho inválido)]
  |
  v
[HAL do Buzzer: liga/desliga modo intermitente]
  |
  v
[PWM: 18 kHz + padrão 20ms ON / 80ms OFF]
```

### 4) Bateria (ADC + filtros + callbacks)

```text
[Inicia amostragem periódica da bateria]
  (battery_start_sampling(interval_ms))
              |
              v
[Workqueue do Zephyr: tarefa periódica]
  (k_work_delayable: sample_periodic_work)
              |
              v
[Lê tensão VBAT em mV]
  (battery_get_millivolt(&mv))
              |
              v
[ADC: coleta N amostras]
              |
              v
[Filtro 1: trimmed mean]
  (descarta a menor e a maior amostra)
              |
              v
[Filtro 2: IIR passa-baixa]
  (suaviza no tempo, alpha=1/8)
              |
              v
[Filtro 3: rejeição de spikes]
  (ignora saltos >150mV)
              |
              v
[Notifica callbacks]
  (sample_ready(mv))
              |
              v
[Reagenda a tarefa periódica]
```

---

## 🧪 Como testar (rápido)

### Teste com nRF Connect (Android)

- Verifique se o dispositivo anuncia como **Amigo Perto**
- Conecte
- Leia:
  - **Battery Level (0x2A19)**
  - **Battery Voltage** (custom)
- Escreva no **Buzzer Intermittent**:
  - `01` para ligar
  - `00` para desligar

### Teste com cliente Web Bluetooth (PWA)

- O cliente BLE do projeto usa **Web Bluetooth**
- Implementação no app: `Aplicativo/src/app/bluetooth.service.ts`

---

## 🧱 Build e Flash

### Via `west`

```bash
cd Firmware/amigo-perto-fw
west build -b xiao_ble/nrf52840 --pristine
west flash
```

### Via VS Code (nRF Connect)

- Abra a pasta `Firmware/amigo-perto-fw/`
- Use as ações de Build/Flash da extensão **nRF Connect for VS Code**

---

## ⚡ Configurações relevantes (Kconfig)

Em `prj.conf`:

- `CONFIG_BT=y`, `CONFIG_BT_PERIPHERAL=y`
- `CONFIG_BT_DEVICE_NAME="Amigo Perto"`
- `CONFIG_BT_CTLR_TX_PWR_PLUS_8=y` (potência TX do controlador)
- `CONFIG_ADC=y`, `CONFIG_PWM=y`, `CONFIG_GPIO=y`
- `CONFIG_PM=y`, `CONFIG_PM_DEVICE=y`, `CONFIG_TICKLESS_KERNEL=y`

---

## 🧯 Troubleshooting (curto)

- **Não aparece no scan**
  - Confirme `LED azul` piscando (advertising)
  - Verifique se o nome do device está correto (`CONFIG_BT_DEVICE_NAME`)
- **Escrita no buzzer não funciona**
  - Verifique se está escrevendo **1 byte** (`00`/`01`) e `offset=0`
  - Confirme que o PWM está mapeado (alias `pwm-led0` no overlay)
- **Bateria sempre 0% / 0mV**
  - Se `battery_init()` falhar, o BAS continua ativo (best-effort) e pode retornar 0
  - Verifique o nó `xiao_ble_battery_dev` e o ADC (`&adc status = "okay"`)

---

## Licenças / créditos

- O módulo de bateria é adaptado de uma biblioteca pública (ver cabeçalho em `src/hal/battery.c`).
- Os demais módulos seguem os headers de licença presentes no projeto.
