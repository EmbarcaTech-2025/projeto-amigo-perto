# AmigoPerto - Blueprint

## Visão Geral

AmigoPerto é um aplicativo da web que utiliza a API Web Bluetooth para descobrir e interagir com dispositivos Bluetooth próximos. O objetivo é fornecer uma interface simples para que os usuários encontrem seus dispositivos e, futuramente, sejam alertados caso se afastem deles.

## Estilo, Design e Recursos (Versão Atual)

### Estilo e Design:

*   **Mascote:** A foto do Tico, um cachorro amigável, é usada como mascote principal do aplicativo.
*   **Tipografia:** A fonte "Poppins" do Google Fonts é usada para uma aparência moderna e limpa.
*   **Layout:** O layout é centralizado e responsivo.
*   **Cores:** Paleta de cores principal em azul amigável (`#007bff`) com cinza para texto.
*   **Componentes:**
    *   `HeaderComponent`: Exibe o mascote, nome do aplicativo e slogan.
    *   `DeviceScannerComponent`: Contém o botão principal de ação para iniciar a busca.
    *   `DeviceDetailsComponent`: Exibe os detalhes do dispositivo encontrado em um cartão.

### Recursos:

*   **Busca por Dispositivos:** O aplicativo busca e encontra dispositivos Bluetooth próximos.
*   **Exibição de Dispositivos:** Exibe o nome e o ID do dispositivo encontrado.
*   **Tratamento de Erros:** Lida com erros de busca e cancelamento pelo usuário.

## Plano Atual

### Fase 2: Identificação de Amigo e Força do Sinal

*   **Visão Geral:** Transformar o aplicativo em um "radar de amigos", associando um dispositivo específico (Tico) à sua foto e exibindo a força do sinal (RSSI) para indicar a proximidade.
*   **Implementação:**
    1.  **Modelo de Dados:** Adicionar propriedades `rssi` e `photoUrl` à interface `Device`.
    2.  **Serviço Bluetooth:**
        *   Implementar a lógica para "observar" os anúncios de um dispositivo após a descoberta para obter o RSSI.
        *   Criar uma associação (mapa) entre IDs de dispositivos conhecidos e suas fotos (ex: ID do "Holy-IOT" -> foto do Tico).
        *   Quando um dispositivo for encontrado, verificar se ele é um "amigo conhecido" e, em caso afirmativo, adicionar sua foto e iniciar a observação do RSSI.
    3.  **UI de Detalhes do Dispositivo:**
        *   Modificar o `DeviceDetailsComponent` para exibir a foto do amigo.
        *   Adicionar um campo para mostrar o valor do RSSI, indicando a "Força do Sinal".
