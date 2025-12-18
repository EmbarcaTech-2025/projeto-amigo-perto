# Amigo Perto - Blueprint

## Visão Geral

O "Amigo Perto" é um aplicativo da web que se conecta a um dispositivo de controle remoto via Bluetooth de baixa energia (BLE). Ele permite que o usuário envie comandos para o dispositivo e monitore a proximidade do mesmo.

## Funcionalidades e Design (Fase 18)

*   **Leitura de RSSI Pré-Conexão:** O aplicativo agora lê a força do sinal (RSSI) do dispositivo *antes* de conectar.
*   **Fluxo de UI em Três Estágios:** A interface agora tem estados claros para "Desconectado", "Escaneando" e "Conectado".
*   **UUIDs Personalizados:** O aplicativo foi atualizado para usar os UUIDs de serviço e característica corretos para o dispositivo do usuário.
*   **Tentativa de Filtro Automático:** Foi feita uma tentativa de usar um filtro de serviço com os UUIDs corretos, mas o dispositivo não foi encontrado, provando que ele não anuncia seus serviços publicamente.

## Plano de Desenvolvimento

### Fases 1-18 (Concluídas)

*   Estrutura, lógica, simulação, UI/UX, implantação, configuração do Bluetooth, mapeamento de comandos, refinamento do layout, implementação do radar de proximidade, refatoração para leitura de RSSI real, diagnóstico de UUID e tentativa de filtro com UUIDs corretos.

