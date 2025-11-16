/*
 * Firmware de Anúncio Bluetooth Low Energy (BLE) - Modo Não-Conectável
 * 
 * Descrição: Este firmware é projetado para placas nRF52840 ou nRF54L15 e implementa
 * um beacon BLE que transmite informações através de advertising packets. O dispositivo
 * opera em modo não-conectável, apenas transmitindo dados de identificação (nome) e
 * informações adicionais (URL) sem aceitar conexões de outros dispositivos.
 * 
 * Funcionalidades principais:
 * - Anuncia sua presença via Bluetooth em modo não-conectável
 * - Transmite nome do dispositivo nos pacotes de advertising
 * - Inclui URL na resposta de scan (scan response)
 * - Pisca LED para indicar funcionamento
 * 
 * Casos de uso: Beacons informativos, dispositivos de localização indoor,
 * transmissores de dados unidirecionais.
 * 
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

// Bibliotecas do Zephyr RTOS
#include <zephyr/kernel.h>              // Funções principais do kernel (threads, timers, sleep, etc)
#include <zephyr/logging/log.h>         // Sistema de logging para debug e informações

// Bibliotecas Bluetooth Low Energy
#include <zephyr/bluetooth/bluetooth.h> // Funções principais do Bluetooth (inicialização, advertising)
#include <zephyr/bluetooth/gap.h>       // GAP - Generic Access Profile (anúncios e descoberta)

// Biblioteca para controle de GPIO (LED)
#include <zephyr/drivers/gpio.h>        // Funções de controle de pinos GPIO

// Registra o módulo de logging com o nome "Lesson2_Exercise1"
LOG_MODULE_REGISTER(Lesson2_Exercise1, LOG_LEVEL_INF);

// Configurações do nome do dispositivo
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME      // Nome configurado via Kconfig (prj.conf)
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)  // Tamanho do nome (sem o caractere nulo '\0')

// Configuração do LED customizado
// O LED está configurado no devicetree como alias "led0"
// Para alterar o pino: edite boards/nrf54l15dk_nrf54l15.overlay ou o overlay da sua placa
#define LED_NODE DT_ALIAS(led0)

// Intervalo de piscar do LED
#define RUN_LED_BLINK_INTERVAL 1000     // Intervalo de piscar do LED (1000ms = 1 segundo)

// Especificação do GPIO do LED obtida do devicetree
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

/**
 * Dados de anúncio (advertising data)
 * Estrutura que define as informações básicas transmitidas continuamente pelo dispositivo.
 * Estes dados são enviados em todos os pacotes de advertising, permitindo que outros
 * dispositivos identifiquem e reconheçam este beacon sem precisar solicitar mais informações.
 */
static const struct bt_data ad[] = {
	// Flags do dispositivo BLE - indica que não suporta Bluetooth clássico (BR/EDR)
	// BT_LE_AD_NO_BREDR = dispositivo BLE puro, não é dual-mode
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	
	// Nome completo do dispositivo que aparecerá na varredura Bluetooth
	// Este é o nome visível quando outros dispositivos fazem scan
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

/**
 * Dados de URL para transmissão via BLE
 * Array contendo a URL "//academy.nordicsemi.com" em formato de beacon.
 * 
 * Estrutura:
 * - Byte 0 (0x17): Prefixo de encoding do URI conforme especificação
 *   (indica o esquema de URL usado - neste caso, formato customizado)
 * - Bytes seguintes: Caracteres ASCII da URL sem protocolo (http/https)
 * 
 * Nota: Este formato pode ser adaptado para Eddystone-URL ou outros padrões
 * de beacon, dependendo do caso de uso específico.
 */
static unsigned char url_data[] = { 0x17, '/', '/', 'a', 'c', 'a', 'd', 'e', 'm',
					'y',  '.', 'n', 'o', 'r', 'd', 'i', 'c', 's',
					'e',  'm', 'i', '.', 'c', 'o', 'm' };

/**
 * Dados de resposta de scan (scan response data)
 * Informações adicionais enviadas apenas quando um dispositivo solicita explicitamente
 * através de um scan request. Isso economiza energia, pois estes dados não são
 * transmitidos continuamente como os advertising data.
 * 
 * Neste caso, inclui a URL da Nordic Semiconductor Academy para referência.
 */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_URI, url_data, sizeof(url_data)),
};

/**
 * Função principal do programa
 * 
 * Fluxo de execução:
 * 1. Inicializa o hardware (GPIO do LED)
 * 2. Inicializa a pilha Bluetooth
 * 3. Inicia o advertising em modo não-conectável
 * 4. Entra em loop infinito piscando o LED para indicar que o sistema está ativo
 * 
 * @return 0 em caso de sucesso (nunca alcançado devido ao loop infinito)
 * @return -1 em caso de erro na inicialização
 */
int main(void)
{
	int blink_status = 0;  // Variável não utilizada (mantida para compatibilidade)
	int err;               // Variável para armazenar códigos de erro das funções

	// Mensagem inicial de log indicando o início da aplicação
	LOG_INF("Starting Lesson 2 - Exercise 1 \n");

	// Verifica se o dispositivo GPIO do LED está pronto para uso
	// Esta verificação é importante antes de qualquer operação com GPIO
	if (!gpio_is_ready_dt(&led)) 
	{
		LOG_ERR("LED GPIO device not ready\n");
		return -1;  // Retorna erro se o GPIO não estiver disponível
	}

	// Configura o pino do LED como saída e inicializa em estado LOW (desligado)
	// GPIO_OUTPUT_INACTIVE garante que o LED inicia apagado
	err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (err) 
	{
		LOG_ERR("Failed to configure LED GPIO (err %d)\n", err);
		return -1;  // Retorna erro se a configuração falhar
	}

	LOG_INF("LED initialized on pin P1.07\n");

	// Inicializa e habilita a pilha Bluetooth
	// O parâmetro NULL indica que não há callback de inicialização personalizado
	// (usa configuração padrão do Zephyr)
	err = bt_enable(NULL);
	if (err) 
	{
		// Se a pilha Bluetooth não inicializar, registra o erro e encerra
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return -1;  // Retorna erro se a inicialização do Bluetooth falhar
	}

	LOG_INF("Bluetooth initialized\n");

	/**
	 * Inicia o advertising Bluetooth em modo não-conectável
	 * 
	 * Parâmetros:
	 * - BT_LE_ADV_NCONN: Modo não-conectável - o dispositivo apenas anuncia,
	 *   não aceita solicitações de conexão de outros dispositivos
	 * - ad: Ponteiro para os dados de advertising (nome e flags)
	 * - ARRAY_SIZE(ad): Quantidade de elementos no array de advertising data
	 * - sd: Ponteiro para os dados de scan response (URL)
	 * - ARRAY_SIZE(sd): Quantidade de elementos no array de scan response
	 * 
	 * Nota: Para aceitar conexões, substitua BT_LE_ADV_NCONN por BT_LE_ADV_CONN
	 * ou use BT_LE_ADV_PARAM() para parâmetros personalizados.
	 */
	err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) 
	{
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return -1;  // Retorna erro se o advertising não iniciar
	}

	LOG_INF("Advertising successfully started\n");

	/**
	 * Loop principal - indicador de vida do sistema
	 * 
	 * Este loop infinito pisca o LED a cada segundo para fornecer feedback visual
	 * de que o firmware está executando corretamente. O LED piscando indica:
	 * - Sistema operacional (Zephyr RTOS) funcionando
	 * - Bluetooth ativo e anunciando
	 * - Firmware não travado
	 * 
	 * k_sleep() coloca a thread em sleep mode, economizando energia enquanto aguarda
	 */
	for (;;) 
	{
		// Alterna o estado do LED (se está aceso, apaga; se está apagado, acende)
		gpio_pin_toggle_dt(&led);
		
		// Aguarda 1 segundo antes da próxima iteração
		// K_MSEC converte milissegundos para a unidade de tempo do kernel
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}

	// Código nunca alcançado devido ao loop infinito
	// Mantido apenas para manter a função com retorno compatível
	return 0;
}
