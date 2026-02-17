# Amigo Perto

O "Amigo Perto" é um aplicativo da web progressivo (PWA) construído com as versões mais recentes do Angular. Ele foi projetado para se conectar e interagir com dispositivos Bluetooth de Baixa Energia (BLE). O foco principal do aplicativo é permitir que o usuário controle um dispositivo remotamente e monitore sua proximidade em tempo real através de uma interface de radar intuitiva.

## Principais Funcionalidades

*   **Conexão Bluetooth (BLE):** Utiliza a **Web Bluetooth API** para escanear e se conectar a periféricos BLE. A conexão é iniciada pelo usuário para garantir a compatibilidade e segurança.
*   **Controle Remoto:** Apresenta um D-Pad interativo que envia comandos para o dispositivo conectado através de características Bluetooth específicas.
*   **Radar de Proximidade:**
    *   Mede a força do sinal recebido (RSSI) em tempo real para estimar a distância do dispositivo.
    *   Apresenta um radar visual que classifica a proximidade em "Perto", "Média" e "Longe".
*   **Alertas Sensoriais:**
    *   **Visual:** A tela do radar treme quando o dispositivo entra no estado "Longe", fornecendo um feedback tátil imediato.
    *   **Auditivo:** Um bipe intermitente, gerado com a **Web Audio API**, é acionado quando o dispositivo está "Longe".
*   **Interface Reativa:** Construído com **Angular Signals** para um gerenciamento de estado moderno, eficiente e sem o uso de RxJS para o estado local.
*   **Arquitetura Moderna:**
    *   **Componentes Standalone:** Arquitetura 100% baseada em componentes, diretivas e pipes standalone.
    *   **OnPush Change Detection:** Todos os componentes usam `ChangeDetectionStrategy.OnPush` para um desempenho otimizado.
    *   **Novo Control Flow:** Utiliza as diretivas `@if`, `@for` e `@switch` nativas para uma lógica de template mais limpa e declarativa.

## Tecnologias Utilizadas

*   **Framework:** Angular 21+
*   **State Management:** Angular Signals
*   **Estilização:** CSS nativo com Variáveis CSS para um design coeso e fácil de manter.
*   **APIs do Navegador:** Web Bluetooth API, Web Audio API
*   **Build:** Angular CLI
*   **Hospedagem:** Firebase Hosting

---

## Servidor de Desenvolvimento

Para iniciar um servidor de desenvolvimento local, execute:

```bash
ng serve
```

Navegue para `http://localhost:4200/`. O aplicativo será recarregado automaticamente se você alterar qualquer um dos arquivos de origem.

## Build

Para compilar o projeto para produção, execute:

```bash
ng build
```

Os artefatos da compilação serão armazenados no diretório `dist/`.

## Testes Unitários

Para executar os testes unitários com o Vitest, use o seguinte comando:

```bash
ng test
```
