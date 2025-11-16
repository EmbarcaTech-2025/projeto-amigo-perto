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
*   **Monitoramento de Proximidade:** Após encontrar um dispositivo, o app monitora a força do sinal (RSSI) e calcula a distância aproximada.
*   **Sistema de Alerta:** Dispara um ciclo de 5 alertas sonoros (beeps) e visuais (tela piscando) quando o dispositivo ultrapassa um limite de distância. O ciclo de alerta é ininterrupto, garantindo que o usuário seja notificado.
*   **Tratamento de Erros:** Lida com erros de busca e cancelamento pelo usuário, reiniciando o estado da aplicação de forma consistente.

## Plano Atual

### Fase 3: Melhoria de UX/UI - Polimento Visual

*   **Visão Geral:** Modernizar a aparência do aplicativo para criar uma experiência de usuário mais sofisticada, intuitiva e visualmente agradável.
*   **Implementação:**
    1.  **Paleta de Cores e Fundo:** Substituir o fundo branco por um fundo com textura sutil (`#f0f4f8`) e introduzir uma paleta de cores mais rica e moderna.
    2.  **Botão de Ação Principal (`DeviceScannerComponent`):**
        *   Aplicar um gradiente linear (de `#6a11cb` para `#2575fc`) como cor de fundo.
        *   Adicionar uma sombra de caixa (`box-shadow`) para um efeito "lifted".
        *   Implementar um efeito de "brilho" no hover, aumentando a interatividade.
        *   Mudar a cor do estado de "busca" (`scanning`) para um gradiente de vermelho (`#d53369` para `#daae51`).
    3.  **Cartão de Detalhes (`DeviceDetailsComponent`):**
        *   Redesenhar o cartão com um fundo branco, cantos arredondados e uma sombra profunda para se destacar do fundo.
    4.  **Tipografia e Mensagens:**
        *   Refinar a hierarquia do texto (títulos, subtítulos, status) para melhorar a legibilidade.
        *   Estilizar as mensagens de status com cores específicas para sucesso, erro e informação.
