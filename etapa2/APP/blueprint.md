
# AmigoPerto - Visão Geral do Projeto

`AmigoPerto` é um aplicativo de segurança pessoal e rastreamento de dispositivos. Ele permite que os usuários encontrem um dispositivo Bluetooth perdido e, em caso de emergência, acionem um botão de pânico para enviar sua localização.

## Funcionalidades Implementadas

- **Modo de Busca (Radar):**
  - Procura por um dispositivo Bluetooth específico.
  - Exibe informações do dispositivo encontrado (nome, status).
- **Modo de Alerta:**
  - Permite enviar alertas sonoros (suave, forte) para o dispositivo pareado.
  - Monitora se o dispositivo está fora de alcance e exibe um alerta visual.
- **Botão de Pânico:**
  - Ao ser acionado, solicita a geolocalização do usuário.
  - Simula o envio das coordenadas para um serviço de emergência.
  - Fornece feedback visual claro na tela sobre o sucesso ou a falha da operação.
- **Feedback de Carregamento:**
  - Exibe um indicador visual (spinner) durante operações demoradas, como a conexão com o dispositivo no "Modo Alerta", melhorando o feedback para o usuário.

## Design e Estilo

- **Tema:** Moderno e escuro (dark mode), com gradientes e sombras para profundidade.
- **Cores:** Paleta focada em tons de azul e ciano para tecnologia, com vermelho e laranja para alertas e pânico.
- **Interatividade:** Animações sutis em botões e alertas para criar uma experiência de usuário mais dinâmica e responsiva.
- **Feedback Visual:** Uso de cores e ícones para comunicar status importantes, como "fora de alcance", "sucesso", "erro" e "carregando".

---

# Plano de Ação Atual: Feedback de Carregamento na Conexão

**Objetivo:** Adicionar um feedback visual claro para o usuário enquanto o aplicativo tenta se conectar ao dispositivo Bluetooth para entrar no "Modo Alerta".

**Passos Executados:**

1.  **Adição de Estado no Serviço:**
    - Foi adicionado um novo `signal` `isLoading` ao `BluetoothService` para gerenciar o estado de conexão.

2.  **Lógica de Ativação:**
    - O método `switchToAlertMode()` no serviço agora define `isLoading` como `true` no início da operação e como `false` ao final, tanto em caso de sucesso quanto de falha, garantindo que o estado seja sempre consistente.

3.  **Integração no Template HTML:**
    - Utilizando o controle de fluxo `@if`, o botão "Ativar Alertas" é substituído por um componente de spinner e um texto informativo ("Conectando ao dispositivo...") quando `bluetoothService.isLoading()` é verdadeiro.
    - Isso impede cliques múltiplos durante a conexão e mantém o usuário informado sobre o andamento do processo.

4.  **Estilização do Carregamento:**
    - Foram adicionados e ajustados estilos CSS para o container do spinner e o texto de carregamento, garantindo uma integração visual coesa com o restante do design da aplicação.
