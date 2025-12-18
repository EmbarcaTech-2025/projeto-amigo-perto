# AmigoPerto - Visão Geral do Projeto

`AmigoPerto` é um aplicativo de segurança pessoal e rastreamento de dispositivos. Ele permite que os usuários se conectem a um dispositivo Bluetooth (como um microcontrolador Zephyr) para controlá-lo e monitorá-lo remotamente.

## Funcionalidades Implementadas

- **Conexão Bluetooth:**
  - Procura e conecta a um dispositivo Bluetooth específico que adverte o serviço `19B10000-E8F2-537E-4F6C-D104768A1214`.
  - Exibe o status da conexão e o nome do dispositivo conectado.
- **Painel de Controle do Dispositivo:**
  - **Controle de Atuadores:** Permite ligar/desligar um LED e acionar um buzzer no dispositivo conectado.
  - **Monitoramento de Bateria:** Lê e exibe o nível da bateria do dispositivo usando o serviço padrão de bateria do Bluetooth (BAS).
  - **Comandos Adicionais:** Inclui botões para solicitar a atualização do nível da bateria e para reiniciar o dispositivo.
- **Interface Responsiva:**
    - Um layout moderno e limpo com cabeçalho, conteúdo principal e rodapé.
    - Uso de ícones (Font Awesome) e um esquema de cores coeso para uma experiência de usuário intuitiva.
- **Feedback em Tempo Real:**
    - Exibe a última resposta recebida do dispositivo em um mini terminal.
    - Fornece mensagens de status e erro no rodapé.

## Design e Estilo

- **Tema:** Moderno e escuro (dark mode), com um esquema de cores centrado em roxos e azuis (`#8a74ff`).
- **Layout:** Estrutura de página única com cabeçalho e rodapé fixos para fácil acesso aos controles de conexão e status.
- **Componentes:**
    - Cards estilizados para agrupar informações e ações do dispositivo.
    - Botões interativos com feedback visual (hover, active, disabled).
    - Barra de progresso para o nível da bateria.
- **Fontes:** Utiliza as fontes `Roboto` para o corpo do texto e `Fira Code` para o terminal de respostas, ambas carregadas via Google Fonts.

---

# Plano de Ação: Refatoração da Interface e Novas Funcionalidades

**Objetivo:** Redesenhar completamente a interface do usuário para focar no controle de um único dispositivo e adicionar novas funcionalidades, como controle de buzzer, leitura de bateria e reinicialização.

**Passos Executados:**

1.  **Refatoração do Serviço Bluetooth (`BluetoothService`):
    - **Característica de Comando Única:** Simplificada a lógica para usar uma única característica (`19b10001-e8f2-537e-4f6c-d104768a1214`) para enviar todos os comandos de controle (LED, buzzer, etc.).
    - **Leitura de Bateria:** Adicionado suporte para o Serviço de Bateria Bluetooth (BAS) para ler o `battery_level` (característica `0x2A19`). Um novo sinal `batteryLevel` foi criado para armazenar esse valor.
    - **Novos Comandos:** Implementados métodos para enviar comandos específicos: `toggleBuzzer`, `toggleLed`, `requestBattery`, `reboot`.
    - **Gerenciamento de Estado:** Mantidos e aprimorados os sinais para `connectionStatus`, `device`, `error`, `lastResponse` e `isLoading`.

2.  **Criação do `DeviceInfoComponent`:**
    - Um novo componente standalone foi criado para encapsular toda a interface de controle do dispositivo conectado.
    - O template (`device-info.component.html`) foi construído com um layout de grid para os botões de ação e exibe dinamicamente as informações do dispositivo e o nível da bateria.
    - O CSS (`device-info.component.css`) foi criado para estilizar o card, os botões e a barra de bateria, seguindo a nova identidade visual.

3.  **Atualização da Estrutura Principal (`AppComponent`):
    - O template (`app.component.html`) foi redesenhado para ter um cabeçalho fixo com o botão de conectar/desconectar e um rodapé para mensagens de status.
    - O `AppComponent` agora importa e renderiza o novo `DeviceInfoComponent`.
    - Os estilos (`app.component.css`) foram atualizados para refletir o novo layout de três seções (cabeçalho, conteúdo, rodapé).

4.  **Configuração Global:**
    - **Ícones e Fontes:** O `index.html` foi atualizado para incluir a biblioteca Font Awesome (para ícones) e as fontes `Roboto` e `Fira Code` do Google Fonts.

5.  **Correção de Compilação:**
    - O `DeviceInfoComponent` foi adicionado ao array de `imports` do `AppComponent` para resolver o erro de compilação `NG8001`, garantindo que o componente filho seja reconhecido.
