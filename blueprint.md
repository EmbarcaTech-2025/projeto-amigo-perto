# Amigo Perto - Blueprint

## Visão Geral

O "Amigo Perto" é um aplicativo da web que se conecta a um dispositivo Bluetooth de baixa energia (BLE) e ajuda o usuário a não o perder. Ele monitora a força do sinal (RSSI) e fornece feedback visual e auditivo para indicar a proximidade do dispositivo.

## Funcionalidades Implementadas

*   **Conexão Bluetooth:** Conecta-se a um dispositivo BLE próximo.
*   **Visualização da Força do Sinal:** Um "radar" exibe a força do sinal em tempo real.
*   **Feedback de Proximidade:** O aplicativo exibe textos como "Muito Perto", "Distância Média" ou "Sinal Fraco - Risco de Perda".
*   **Controles do Dispositivo:** Permite ligar/desligar uma campainha e um LED no dispositivo.
*   **Controle de Bateria:** Solicita a atualização do nível da bateria.
*   **Reinicialização do Dispositivo:** Permite reiniciar o dispositivo.

## Design e Estilo

*   **Paleta de Cores:** A paleta de cores atual não está bem definida. O plano é usar uma paleta moderna e agradável.
*   **Tipografia:** A tipografia atual não é consistente. O plano é usar fontes que melhorem a legibilidade e a estética.
*   **Layout:** O layout atual é muito simples. O plano é criar um layout baseado em cartões para exibir as informações do dispositivo.

## Plano de Modernização

### Fase 1: Refatoração da Fundação

1.  **Analisar o código existente:** (Concluído)
2.  **Criar um `blueprint.md`:** (Concluído)
3.  **Refatorar o `app.component`:** Transformá-lo no componente principal, que gerenciará o layout e a lógica central.
4.  **Criar um `BluetoothService`:** Encapsular toda a lógica da API Web Bluetooth em um serviço dedicado para limpar os componentes e reutilizar a lógica.
5.  **Atualizar o `main.ts`:** Fazer o bootstrap do `AppComponent` standalone.

### Fase 2: Modernização dos Componentes

1.  Refatorar `device-info.component` e `rssi-radar.component` para serem componentes standalone, usarem `OnPush` e os novos `input()` signals.

### Fase 3: Melhorias de UI/UX

1.  Redesenhar a interface do usuário para ser mais moderna, com um layout baseado em cartões para os dispositivos.
2.  Adicionar um botão "Escanear" claro.
3.  Exibir o status da conexão e fornecer um feedback melhor ao usuário.
