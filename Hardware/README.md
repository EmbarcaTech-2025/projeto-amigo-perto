# Hardware — Amigo Perto

Esta pasta contém os arquivos do **hardware (esquemático, PCB e modelos 3D da carcaça)** do projeto Amigo Perto.

## 📁 Arquivos nesta pasta

### 🧩 Projeto Altium (fontes)

- [main.SchDoc](main.SchDoc)
  - Esquemático principal (Altium Designer).
- [pcb_amigo_perto.PcbDoc](pcb_amigo_perto.PcbDoc)
  - Layout da placa (PCB) + roteamento (Altium Designer).

> Para editar/visualizar esses arquivos, use **Altium Designer** (ou um visualizador compatível com `SchDoc`/`PcbDoc`, quando disponível).

### 🏭 Pacote de fabricação (outputs)

- [pcb_amigo_perto_v3.0.0_11-01-26.zip](pcb_amigo_perto_v3.0.0_11-01-26.zip)
  - Export do Altium em formato **“Project Outputs”** com os arquivos para fabricação da PCB.
  - Inclui um conjunto de **Gerbers** (camadas), **contorno da placa**, **furação (drill)** e relatórios.

Arquivos típicos dentro desse `.zip`:
- Gerbers de cobre/silk/máscara (ex.: `.GTL`, `.GBL`, `.GTO`, `.GBO`, `.GTS`, `.GBS`)
- Contorno/recorte da placa (ex.: `.GKO`)
- Furação/drill (ex.: `.TXT`)
- Relatórios do Altium (ex.: DRC `.drc` / `.html`, status, etc.)

Como usar (fabricação):
- Envie o `.zip` para o fabricante (JLCPCB/PCBWay/etc.) como **Gerber package**.
- Se o fabricante pedir confirmação de camadas, use um **Gerber viewer** para checar o empilhamento e o contorno.

### 🧱 Modelos 3D (carcaça)

- [base_pcb_v3.3MF.zip](base_pcb_v3.3MF.zip)
  - Parte **base** da carcaça em formato **3MF** (empacotado como `.zip`).
- [frente_v3.3MF.zip](frente_v3.3MF.zip)
  - Parte **frontal/tampa** da carcaça em formato **3MF** (empacotado como `.zip`).

Como usar (impressão 3D):
- Em geral, você pode **renomear** o arquivo de `.zip` para `.3mf` e abrir no fatiador (Cura/PrusaSlicer/Bambu Studio).
- Alternativamente, extraia o `.zip` e importe o modelo pelo fatiador (se ele suportar o pacote 3MF extraído).

## 🔗 Links úteis

- Documentação do firmware (para pinagem/overlay e comportamento do dispositivo):
  - [Firmware/amigo-perto-fw/README.md](../Firmware/amigo-perto-fw/README.md)
- Visão geral do projeto: [README.md](../README.md)
