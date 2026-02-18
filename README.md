# Amigo Perto – Monitoramento de Proximidade BLE para PETs

![Status](https://img.shields.io/badge/Status-Conclu%C3%ADdo-green)
![Vers%C3%A3o](https://img.shields.io/badge/Vers%C3%A3o-1.0.0-blue)
![Plataforma](https://img.shields.io/badge/Plataforma-XIAO%20nRF52840-red)
![Firmware](https://img.shields.io/badge/Firmware-Zephyr%20%2B%20nRF%20Connect%20SDK-orange)
![Aplicativo](https://img.shields.io/badge/Aplicativo-Angular%20(PWA)-purple)

## 🌐 Web App (produção)

**Acesse o aplicativo web** no Google Chrome:

https://amigopertov2-49090345-fc8b8.web.app

## 📋 Sobre o Projeto

O **Amigo Perto** é um sistema (hardware + firmware + aplicativo) para **monitoramento de proximidade via Bluetooth Low Energy (BLE)**, voltado para **segurança de pets** em cenários urbanos, semiurbanos e rurais.

Em resumo, ele resolve o problema de “perdi a noção de distância do pet” com um **domo virtual de proximidade**:

- 📶 Monitora **RSSI** e classifica a proximidade em **Perto / Médio / Longe** (faixas)
- 🚨 Quando atinge **Longe**, o app alerta o tutor
- 🔊 O tutor pode **acionar manualmente** um beep na coleira para chamar a atenção do pet e reforçar o adestramento

### 🎯 Objetivo Principal

Desenvolver uma coleira eletrônica capaz de:

- ✅ **Estabelecer conexão BLE** entre tutor e coleira (BLE 5.x)
- 📶 **Monitorar RSSI continuamente** para detectar variações de proximidade
- 🧠 **Classificar proximidade por faixas** (Perto / Médio / Longe) com base em RSSI filtrado
- 🚨 **Acionar alerta no app** quando estiver em “Longe”
- 🔊 **Receber comandos remotos** para acionar o buzzer (beep) na coleira
- 🔋 **Reportar status de bateria** via BLE (Battery Service 0x180F)
- 🌙 **Otimizar consumo de energia** para operação contínua com bateria recarregável

### 👥 Equipe de Desenvolvimento

- **Eric Senne Roma**
- **Vitor Gomes**
- **Antônio Almeida**

**Data:** Fevereiro de 2026

---

## 🧭 Visão Geral do Sistema

- **Hardware:** XIAO nRF52840 + bateria Li‑Po recarregável (USB‑C) + buzzer + LEDs de status
- **Firmware (Zephyr / nRF Connect SDK):** advertising, conexão BLE, GATT (Battery + serviço custom) e controle do buzzer
- **Aplicativo (Angular PWA):** Web Bluetooth para scan/conexão, leitura de RSSI, **classificação por faixas** e envio de comandos

---

## 🏗️ Estrutura do Repositório

Este repositório está organizado por módulos do sistema:

### 📁 [Aplicativo/](Aplicativo/) — Aplicativo Web (PWA)
- Código do app em **Angular 21+**
- Conexão BLE via **Web Bluetooth API**
- Radar de proximidade por **RSSI** + alertas sonoros
- Documentos:
  - [README do App](Aplicativo/README.md)
  - [Blueprint/Especificação](Aplicativo/blueprint.md)

### 📁 [Firmware/](Firmware/) — Firmware (Zephyr RTOS)
- Projeto para **nRF Connect SDK** / **Zephyr**
- Implementa advertising, GATT e controle de buzzer
- Pasta principal: 📁 [Firmware/amigo-perto-fw/](Firmware/amigo-perto-fw/)
- Documentação do firmware (arquitetura, UUIDs/GATT, pinos/overlay, build/flash e troubleshooting):
  - [Firmware/amigo-perto-fw/README.md](Firmware/amigo-perto-fw/README.md)

### 📁 [Firmware/releases/](Firmware/releases/) — Release para Flash
- Arquivo `.uf2` da versão final para gravação rápida na placa
  - Ex.: [Firmware/releases/amigo-perto-v1.0.0.uf2](Firmware/releases/amigo-perto-v1.0.0.uf2)

### 📁 [Hardware/](Hardware/) — PCB e Projeto Eletrônico
Nesta pasta serão armazenados os artefatos de hardware para fabricação e edição:
- Gerbers para fabricação da PCB
- PDFs do esquemático e da PCB (exportados do Altium)
- Arquivos do projeto para abrir/editar no Altium
- Arquivos para fabricação da Case na impressora 3D

Documentação do hardware (arquivos Altium, pacote de Gerbers e modelos 3D):
- [Hardware/README.md](Hardware/README.md)

### 📁 [Documentação/](Documentação/) — Documentos e Apresentações (PDF)
- Compilado de entregas semanais, documentação final e apresentações por etapa
- Índice/descrição dos arquivos: [Documentação/README.md](Documentação/README.md)

---

## 🚀 Quick Start

### 1) 📋 Pré-requisitos

**Hardware**
- Placa: **XIAO nRF52840**
- Buzzer piezoelétrico (PWM)
- Bateria LiPo 1S (3.0V–4.2V)

**Firmware**
- VS Code + **nRF Connect for VS Code** (extensões Nordic)
- **nRF Connect SDK** instalado/configurado (Zephyr + west)

**Aplicativo**
- Node.js + npm
- Navegador com Web Bluetooth (na prática: **Chrome/Edge no Android/desktop**)
  - Observação: Web Bluetooth tem suporte limitado/variável em iOS

### 2) ⬇️ Clone do repositório

```bash
git clone <repository-url>
cd projeto-amigo-perto
```

### 3) ⚡ Firmware — Flash (recomendado via UF2)

Para flashar a versão final, use o arquivo `.uf2` em [Firmware/releases/](Firmware/releases/):

- Ex.: [Firmware/releases/amigo-perto-v1.0.0.uf2](Firmware/releases/amigo-perto-v1.0.0.uf2)

### 4) 🔧 Firmware — Build/Flash via west (opcional para desenvolvimento)

Dentro de [Firmware/amigo-perto-fw/](Firmware/amigo-perto-fw/):

```bash
cd Firmware/amigo-perto-fw
west build -b xiao_ble/nrf52840 --pristine
west flash
```

> Dica: também é possível abrir a pasta do firmware no VS Code com a extensão da Nordic e compilar/flashar pela UI.

### 5) 🌐 Aplicativo — Rodar localmente

Dentro de [Aplicativo/](Aplicativo/):

```bash
cd Aplicativo
npm install
npm run start
```

Ou, se preferir:

```bash
cd Aplicativo
npx ng serve
```

---

## 🧑‍💻 Como usar o dispositivo (passo a passo)

1. **Ligue o dispositivo**
  - Acione a **chave de alimentação** no módulo.
2. **Aguarde a inicialização**
  - Confirme que o **LED azul está piscando** (Bluetooth ativo e pronto para pareamento).
3. **(Primeiro uso) Habilite suporte no navegador**
  - No Chrome do celular, acesse: `chrome://flags`
  - Ative **Experimental Web Platform features**
  - Reinicie o navegador (se solicitado)
4. **Acesse o aplicativo web**
  - Abra no Chrome: https://amigopertov2-49090345-fc8b8.web.app
5. **Pareie o dispositivo**
  - Clique em **Procurar Amigo**
  - Selecione **Amigo Perto** na lista e confirme o pareamento
6. **Visualize a proximidade**
  - O radar exibe a faixa: **Perto / Médio / Longe**
7. **Conecte para funções completas**
  - Clique em **Conectar** para ver **bateria (%)** e habilitar o controle do buzzer
  - Quando conectado, o **LED verde** indica o estado conectado
8. **Use o alarme (buzzer)**
  - **▲ (seta para cima):** liga alarme intermitente
  - **● (botão central):** desliga o alarme imediatamente

---

## ✅ Requisitos (resumo)

**Funcionais (principais)**

- BLE 5.x para comunicação (coleira ↔ tutor)
- Monitoramento contínuo de RSSI
- Classificação em **faixas Perto/Médio/Longe** (com filtragem e histerese)
- Alerta no app quando estiver em **Longe**
- Comando remoto para **beep** no hardware
- Leitura de bateria via **Battery Service (0x180F)**

**Não funcionais (principais)**

- Baixo consumo e bateria recarregável
- Dispositivo compacto/leve e seguro ao pet
- Comunicação BLE robusta (interferências e perdas momentâneas)
- Evolução para **hardware dedicado** (PCB própria + integração mecânica)

---

## 📦 Lista de Materiais (Protótipo)

- Placa de desenvolvimento **XIAO nRF52840** (SoC BLE integrado)
- **Buzzer passivo** eletromagnético SMD 3,6 V (MLT-8530 ou equivalente)
- **Transistor NPN** SMD (BC817 ou equivalente)
- **Resistor 330 Ω** (limitador de corrente/base)
- **Chave deslizante (slide switch)** 3 pinos, pitch 2 mm (MSK-03EH ou equivalente)
- **Bateria Li‑Po 3,7 V – 100 mAh** (recarregável)
- **PCB dedicada** ao projeto
- **Carcaça impressa em 3D** para acomodação da PCB e bateria
- **Cabo USB‑C** (dados e alimentação) para gravação do firmware e recarga da bateria

## ⚙️ Características Técnicas

### 🛠️ Hardware Utilizado

| Componente | Função | Observações |
|---|---|---|
| **XIAO nRF52840** | MCU principal | BLE 5.x, baixo consumo |
| **Buzzer piezoelétrico** | Alerta sonoro | Controle via PWM |
| **Bateria LiPo 1S** | Alimentação | 3.0V–4.2V |
| **LED Verde** | Status de conexão | Pisca com duty-cycle reduzido |
| **LED Azul** | Status de advertising | Pisca com duty-cycle reduzido |

### 📊 Funcionalidades Principais

- **BLE 5.x**: advertising + conexão como periférico
- **Serviço GATT customizado (Buzzer Service)** para controle remoto do alarme
- **Battery Service padrão (0x180F)** para nível de bateria (característica 0x2A19)
- **Buzzer via PWM** com modo intermitente (economia de energia)
- **Monitoramento de bateria via ADC**
- **Power Management no Zephyr** (tickless + DC/DC via devicetree overlay)

---

## 🔬 Principais Decisões e Desafios Superados

- **RSSI em metros não é confiável:** ao avaliar o uso de RSSI para distância absoluta, observou-se baixa precisão por obstáculos, multipercurso e interferências.
- **Mudança de estratégia (validada na prática):** o sistema passou a operar por **faixas de proximidade** (Perto/Médio/Longe), com referência relacional ao comportamento do sinal no ambiente.
- **Estabilidade do sinal:** aplicação de técnicas de tratamento/filtragem e histerese para reduzir oscilações naturais e evitar “chattering” próximo ao limiar.
- **Robustez BLE:** ajuste de parâmetros (potência de TX e intervalos de advertising) e estratégias de reconexão para reduzir falsos alarmes e recuperar de perdas momentâneas.
- **Energia e ergonomia:** otimizações de baixo consumo (LEDs com duty-cycle reduzido, tickless, DC/DC) e evolução para hardware dedicado com PCB própria e case customizada.

---

## 🧩 Arquitetura

Camadas principais:

**Aplicação**
- Loop principal
- Callbacks BLE/GATT
- Lógica de controle e estados (advertising / conectado)

**HAL (Hardware Abstraction Layer)**
- BLE
- Buzzer (PWM)
- Bateria (ADC)

**Serviços GATT**
- Buzzer Service (custom)
- Battery Service (0x180F)

**Stack/Drivers**
- Zephyr RTOS + nRF Connect SDK
- Drivers (GPIO, PWM, ADC)
- Power management

Representação em blocos:

```
┌─────────────────────────────────────┐
│         Aplicação (main.c)          │
│  - Loop principal                   │
│  - Callbacks BLE/GATT               │
│  - Lógica de controle               │
├─────────────────────────────────────┤
│     HAL (Hardware Abstraction)      │
│  - hal/ble.c      (BLE)             │
│  - hal/buzzer.c   (PWM)             │
│  - hal/battery.c  (ADC)             │
├─────────────────────────────────────┤
│      Serviços GATT (gatt/)          │
│  - buzzer_service.c  (custom)       │
│  - battery_service.c (0x180F)       │
├─────────────────────────────────────┤
│        Zephyr RTOS + nRF SDK        │
│  - Bluetooth stack                  │
│  - Drivers (PWM, ADC, GPIO)         │
│  - Power management                 │
└─────────────────────────────────────┘
```

---

## 📱 Integração com o Aplicativo

Arquitetura do sistema:

```
┌──────────────┐         BLE          ┌──────────────┐
│  Smartphone  │◄────────────────────►│   Coleira    │
│ (PWA/Browser)│   RSSI (lido pelo    │  (Firmware)  │
│              │    aplicativo)       │              │
│  - Lê RSSI   │                      │  - Recebe    │
│  - Classifica│   Comandos GATT      │    comandos  │
│    por faixa ├─────────────────────►│  - Aciona    │
│  - Envia     │   (buzzer ON/OFF)    │    buzzer    │
│    comandos  │                      │  - Reporta   │
│  - Monitora  │◄─────────────────────┤    bateria   │
│    bateria   │   Battery (0x180F)   │              │
└──────────────┘                      └──────────────┘
```

---

## 🔄 Fluxo de Operação

Em termos de uso prático:

1. Ao ligar o dispositivo, o sistema inicia o boot.
2. O **LED azul** pisca indicando **modo advertising** (pronto para pareamento via BLE).
3. No aplicativo, ao iniciar a varredura (scan), o usuário já consegue **monitorar o RSSI** e a UI classifica a proximidade em **Perto/Médio/Longe**.
4. Se a faixa atingir **“Longe”**, o sistema emite **alerta no celular** (ex.: beep/vibração/feedback visual, conforme UI).
5. Ao clicar em **Conectar**, o usuário passa a ter acesso às características GATT:
  - leitura de **bateria (%)** via Battery Service
  - envio de comandos para **acionar/desligar** o buzzer no hardware
6. Caso o tutor deseje, pode **acionar manualmente** o beep para chamar a atenção do pet e reforçar o treinamento.

---

## 📏 Lógica de Distância (RSSI → Distância)

Durante o desenvolvimento, foi implementada leitura de RSSI com técnicas de estabilização. Entretanto, o projeto concluiu que **RSSI não é adequado para distância absoluta (em metros)** de forma consistente.

Assim, o sistema opera com **faixas de proximidade**:

- O app lê RSSI via BLE
- O app aplica tratamento/filtragem e classifica em **Perto / Médio / Longe**
- O app decide quando alertar e quando enviar comando de beep
- O firmware executa o comando (buzzer) e reporta status (bateria)

Essa abordagem torna o “domo virtual” mais adaptável a cenários abertos, urbanos e ambientes com obstáculos, já que a referência é relacional ao comportamento do sinal no ambiente.

---

## 🎯 Público-alvo e Evolução (curto)

- Tutores domésticos (urbano/semiurbano) e tutores em áreas rurais/propriedades amplas
- Expansões futuras possíveis (mesma base BLE): crianças, idosos e objetos

---

## 📁 Documentação e Recursos

- 📱 App (funcionalidades, comandos, build/test): [Aplicativo/README.md](Aplicativo/README.md)
- 📌 Blueprint do app (fases/decisões de UX): [Aplicativo/blueprint.md](Aplicativo/blueprint.md)
- 🧠 Firmware (documentação principal): [Firmware/amigo-perto-fw/README.md](Firmware/amigo-perto-fw/README.md)
- 🔎 Firmware (entrada do código): [Firmware/amigo-perto-fw/src/main.c](Firmware/amigo-perto-fw/src/main.c)
- 🧩 Hardware (documentação principal): [Hardware/README.md](Hardware/README.md)
- 📚 Documentação do projeto (PDFs): [Documentação/README.md](Documentação/README.md)

---

## 📜 Licença

Este projeto está licenciado conforme o arquivo [LICENSE](LICENSE).

---

## 🙏 Agradecimentos

Agradecemos ao **Programa EmbarcaTech** pela oportunidade de desenvolvimento deste projeto, aos mentores pelo suporte técnico e a todos que contribuíram para o sucesso desta iniciativa.

---

<div align="center">

![BLE](https://img.shields.io/badge/BLE-5.x-blue?style=for-the-badge)
![Nordic](https://img.shields.io/badge/Nordic-nRF52840-red?style=for-the-badge)
![Angular](https://img.shields.io/badge/Angular-PWA-purple?style=for-the-badge)

**Amigo Perto – Projeto Integrado (Hardware + Firmware + App) | 2026**

</div>
