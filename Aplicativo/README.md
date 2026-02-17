# Amigo Perto — Aplicativo (Angular PWA + Web Bluetooth)

![Framework](https://img.shields.io/badge/Framework-Angular%2021%2B-red)
![Plataforma](https://img.shields.io/badge/Plataforma-PWA-blue)
![BLE](https://img.shields.io/badge/BLE-Web%20Bluetooth%20API-brightgreen)
![Build](https://img.shields.io/badge/Build-Angular%20CLI-orange)
![Hosting](https://img.shields.io/badge/Hosting-Firebase-purple)

Aplicativo web progressivo (PWA) do **Amigo Perto**, focado em **monitoramento de proximidade via RSSI** e **controle remoto** de um periférico **BLE**.

- Entrada do app: [`main.ts`](Aplicativo/src/main.ts)
- UI principal: [`app.component.html`](Aplicativo/src/app/app.component.html)
- Cliente BLE (Web Bluetooth): [`BluetoothService`](Aplicativo/src/app/bluetooth.service.ts)
- Especificação/decisões de UX: [blueprint.md](Aplicativo/blueprint.md)

---

## ✅ O que o aplicativo faz

- **Scan BLE (Central)** via **Web Bluetooth API**
  - Descobre o periférico anunciado como “Amigo Perto”
- **Conexão BLE** iniciada pelo usuário (requisito do navegador)
- **Leitura de proximidade em tempo real**
  - Mede **RSSI** e classifica em faixas (ex.: *Perto / Médio / Longe*)
  - Exibe **radar** com feedback visual
- **Alertas sensoriais quando “Longe”**
  - **Visual** (tremor/efeito de alerta na UI)
  - **Auditivo** via **Web Audio API** (bipe intermitente)
- **Controle remoto do dispositivo**
  - Envia comandos via GATT para **ligar/desligar** o buzzer do hardware
- **Telemetria**
  - Lê **Battery Level** do serviço padrão (0x180F)

---

## 🧩 Arquitetura (camadas)

A arquitetura do app separa responsabilidades em blocos, no mesmo espírito do firmware:

1) **UI (Componentes Angular)**
- Renderização de estados (desconectado/escaneando/conectado)
- Composição da tela (status, bateria e controle)
- Referência: [`app.component.html`](Aplicativo/src/app/app.component.html)

2) **Cliente BLE (Central)**
- Fluxo Web Bluetooth: request device → connect → get services → read/write characteristics
- Estado reativo (Signals) para refletir conexão/scan/erros
- Referência: [`BluetoothService`](Aplicativo/src/app/bluetooth.service.ts)

3) **Proximidade (RSSI → faixa)**
- Coleta/atualização de RSSI
- Classificação por faixas e atualização do “radar”
- Detalhes e regras: [blueprint.md](Aplicativo/blueprint.md)

4) **Alertas**
- Geração de beep intermitente via Web Audio API
- Feedback visual no radar quando estado “Longe”
- Referência funcional (alto nível): [README do app](Aplicativo/README.md)

5) **Build/Config/Deploy**
- Angular CLI: build/test
- Firebase Hosting: configs em [`firebase.json`](Aplicativo/firebase.json) e [`/.firebaserc`](Aplicativo/.firebaserc)

---

## 📡 Contrato BLE (GATT) consumido pelo app

Os UUIDs usados no cliente BLE estão definidos em [`BluetoothService`](Aplicativo/src/app/bluetooth.service.ts):

### Battery Service (Bluetooth SIG)

| Item | UUID | Tipo | Propriedades | Uso no app |
|---|---:|---|---|---|
| Battery Service | `0x180F` | Service | - | Descoberta |
| Battery Level | `0x2A19` | Characteristic | Read | Exibição do nível de bateria |

Constantes: [`BATTERY_SERVICE_UUID`](Aplicativo/src/app/bluetooth.service.ts), [`BATTERY_LEVEL_CHARACTERISTIC_UUID`](Aplicativo/src/app/bluetooth.service.ts)

### Buzzer Service (custom)

| Item | UUID | Tipo | Propriedades | Valor |
|---|---|---|---|---|
| Buzzer Service | `12345678-ABCD-EFAB-CDEF-123456789ABC` | Service | - | - |
| Buzzer Intermittent | `12345679-ABCD-EFAB-CDEF-123456789ABC` | Characteristic | Write | `0x00` OFF / `0x01` ON |

Constantes: [`CUSTOM_SERVICE_UUID`](Aplicativo/src/app/bluetooth.service.ts), [`CUSTOM_RX_CHARACTERISTIC_UUID`](Aplicativo/src/app/bluetooth.service.ts)

---

## 🔄 Fluxo de operação (alto nível)

```text
[Usuário abre o PWA]
  |
  v
[Iniciar scan (Web Bluetooth)]
  |
  v
[Device encontrado]
  |
  v
[Conectar]
  |
  v
[Conectado]
  |         \
  |          \  (Opcional)
  |           -> [Write GATT buzzer: 0x01/0x00]
  |
  v
[Ler bateria (0x2A19)]
  |
  v
[Monitorar RSSI + atualizar radar + alertas]
```

---

## 🚀 Quick Start

### Pré-requisitos

- **Node.js + npm**
- **Angular CLI** (ou via `npx`)
- Navegador com **Web Bluetooth**
  - Na prática: **Chrome/Edge**
  - Observação: Web Bluetooth requer **contexto seguro** (HTTPS). `localhost` é aceito como exceção em dev.

### (Primeiro uso) Habilitar suporte no navegador (Chrome no celular)

- No Google Chrome do celular, acesse: `chrome://flags`
- Ative a opção **Experimental Web Platform features**
- Reinicie o navegador, se solicitado

### Rodar em desenvolvimento

Dentro de [Aplicativo/](Aplicativo/):

```bash
cd Aplicativo
npm install
ng serve
```

Acesse `http://localhost:4200/`.

### Build (produção)

```bash
cd Aplicativo
ng build
```

Saída em `dist/`.

### Testes unitários (Vitest via Angular CLI)

```bash
cd Aplicativo
ng test
```

---

## 🔥 Deploy (Firebase Hosting)

Arquivos de configuração:
- [Aplicativo/firebase.json](Aplicativo/firebase.json)
- [Aplicativo/.firebaserc](Aplicativo/.firebaserc)

Fluxo típico:

```bash
cd Aplicativo
ng build
firebase deploy
```

> Ajuste o “public”/diretório de saída conforme o que estiver definido no [`firebase.json`](Aplicativo/firebase.json).

---

## 🧪 Como testar (rápido)

### Teste com o dispositivo (firmware)

1. Ligue o dispositivo e confirme que está em advertising (LED azul no hardware).
2. No app, inicie scan e conecte.
3. Valide:
   - leitura de bateria (0x2A19)
   - comando do buzzer (write `01` liga / `00` desliga)

### Teste com nRF Connect (Android) para isolar problemas

Se o app não conectar/ler/escrever, teste primeiro com nRF Connect:
- O periférico deve expor Battery Service (0x180F) e o serviço custom do buzzer (UUID 128-bit)

---

## 🧯 Troubleshooting (curto)

- **“Procurando dispositivo…” e não encontra**
  - Confirme que o periférico está anunciando e próximo
  - No Android/desktop, confirme Bluetooth ligado e permissões concedidas ao navegador
- **Conecta, mas não lê bateria**
  - Verifique se o firmware está expondo o BAS (0x180F) e se o device está pareado corretamente
- **Comando do buzzer não funciona**
  - Confirme que está escrevendo exatamente **1 byte**: `0x00` ou `0x01`
  - Confirme que o UUID do serviço/characteristic bate com as constantes em [`BluetoothService`](Aplicativo/src/app/bluetooth.service.ts)
- **iOS**
  - Suporte a Web Bluetooth é limitado/variável; priorize Chrome/Edge no Android/desktop

---