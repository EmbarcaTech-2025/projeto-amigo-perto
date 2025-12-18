# Amigo Perto - Blueprint

## Visão Geral

O "Amigo Perto" é um aplicativo da web que se conecta a um dispositivo de controle remoto via Bluetooth de baixa energia (BLE). Ele permite que o usuário envie comandos para o dispositivo e monitore a proximidade do mesmo.

## Funcionalidades e Design (Fase 20)

*   **Leitura de RSSI Pré-Conexão:** O aplicativo lê a força do sinal (RSSI) do dispositivo *antes* de conectar.
*   **Fluxo de UI em Três Estágios:** A interface agora tem estados claros para "Desconectado", "Escaneando" e "Conectado".
*   **Descoberta Manual Funcional:** O aplicativo força a exibição do seletor de dispositivos do navegador (`acceptAllDevices: true`), permitindo que o usuário se conecte ao dispositivo com sucesso.
*   **Alerta Háptico e Sonoro:** Quando o dispositivo está no estado "Longe", a tela do radar agora treme e emite um bipe intermitente para alertar o usuário.

## Plano de Desenvolvimento

### Fases 1-20 (Concluídas)

*   Estrutura, lógica, simulação, UI/UX, implantação, configuração do Bluetooth, mapeamento de comandos, refinamento do layout, implementação do radar de proximidade, refatoração para leitura de RSSI real, diagnóstico de UUID, implementação da descoberta manual e alertas hápticos/sonoros.
