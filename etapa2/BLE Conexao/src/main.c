/*
 * Firmware para Coleira com Bluetooth Low Energy (BLE)
 * 
 * Descrição: Este firmware é projetado para ser gravado em placas nRF52840 ou nRF54L15
 * instaladas em coleiras. A função principal é estabelecer uma conexão Bluetooth com
 * um celular, que será responsável por ler e processar os valores de RSSI (Received
 * Signal Strength Indicator) para determinar a distância.
 * 
 * O firmware apenas:
 * - Anuncia sua presença via Bluetooth
 * - Aceita conexões de dispositivos móveis
 * - Pisca um LED para indicar que está funcionando
 * - Mantém a conexão ativa
 * 
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

// Bibliotecas do Zephyr RTOS
#include <zephyr/kernel.h>              // Funções principais do kernel (threads, timers, etc)
#include <zephyr/logging/log.h>         // Sistema de logging para debug

// Bibliotecas Bluetooth Low Energy
#include <zephyr/bluetooth/bluetooth.h> // Funções principais do Bluetooth
#include <zephyr/bluetooth/gap.h>       // GAP - Generic Access Profile (anúncios e descoberta)
#include <zephyr/bluetooth/gatt.h>      // GATT - Generic Attribute Profile (serviços e características)
#include <zephyr/bluetooth/uuid.h>      // UUIDs para identificação de serviços
#include <zephyr/bluetooth/addr.h>      // Gerenciamento de endereços Bluetooth
#include <zephyr/bluetooth/conn.h>      // Gerenciamento de conexões Bluetooth

// Biblioteca para controle de GPIO (LED customizado)
#include <zephyr/drivers/gpio.h>

// Configuração do LED customizado no pino 7
// O LED está configurado no devicetree como alias "led0"
#define LED_NODE DT_ALIAS(led0)

// Intervalo de piscar do LED
#define RUN_LED_BLINK_INTERVAL 1000     // Intervalo de piscar do LED (1000ms = 1 segundo)

// Especificação do GPIO do LED
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

// Variáveis globais
struct bt_conn *my_conn = NULL;         // Ponteiro para armazenar a conexão Bluetooth ativa
static struct k_work adv_work;          // Estrutura de trabalho para iniciar anúncios

// Parâmetros de anúncio Bluetooth (advertising)
static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	(BT_LE_ADV_OPT_CONN |                   // Permite que outros dispositivos se conectem
	 BT_LE_ADV_OPT_USE_IDENTITY),           // Usa o endereço de identidade do dispositivo
	BT_GAP_ADV_FAST_INT_MIN_1,              // Intervalo mínimo de anúncio: 30ms (conexão rápida)
	BT_GAP_ADV_FAST_INT_MAX_1,              // Intervalo máximo de anúncio: 60ms
	NULL);                                   // NULL = anúncio não direcionado (qualquer um pode conectar)

// Registra o módulo de logging com o nome "BLE_Coleira"
LOG_MODULE_REGISTER(BLE_Coleira, LOG_LEVEL_INF);

// Configurações do nome do dispositivo
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME  // Nome configurado no prj.conf: "BLE_Coleira"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)  // Tamanho do nome (sem o caractere nulo)

// Dados de anúncio (advertising data) - informações básicas transmitidas
static const struct bt_data ad[] = {
	// Flags indicando que é um dispositivo BLE de propósito geral, sem suporte a Bluetooth clássico
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	// Nome completo do dispositivo que aparecerá na busca Bluetooth
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

// Dados de resposta de scan (scan response data) - informações adicionais
static const struct bt_data sd[] = {
	// UUID customizado do serviço (usado para identificação específica)
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
			  BT_UUID_128_ENCODE(0x00001523, 0x1212, 0xefde, 0x1523, 0x785feabcd123)),
};

/**
 * Handler (manipulador) para iniciar o anúncio Bluetooth
 * Esta função é executada como uma tarefa de trabalho (work item)
 */
static void adv_work_handler(struct k_work *work)
{
	// Inicia o anúncio Bluetooth com os parâmetros e dados configurados
	int err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	// Verifica se houve erro ao iniciar o anúncio
	if (err) 
	{
		LOG_ERR("Advertising failed to start (err %d)", err);
		return;
	}

	// Log de sucesso
	LOG_INF("Advertising successfully started");
}

/**
 * Função para submeter a tarefa de iniciar anúncio ao sistema
 * Isso permite que o anúncio seja iniciado de forma assíncrona
 */
static void advertising_start(void)
{
	k_work_submit(&adv_work);
}

/**
 * Callback chamado quando um dispositivo se conecta
 * @param conn - Ponteiro para a estrutura de conexão
 * @param err - Código de erro (0 se conexão bem-sucedida)
 */
void on_connected(struct bt_conn *conn, uint8_t err)
{
	// Verifica se houve erro na conexão
	if (err) 
	{
		LOG_ERR("Connection error %d", err);
		return;
	}
	
	// Log de sucesso
	LOG_INF("Connected");
	
	// Incrementa a referência da conexão e armazena o ponteiro
	// Isso mantém a conexão ativa e permite gerenciá-la
	my_conn = bt_conn_ref(conn);
}

/**
 * Callback chamado quando um dispositivo se desconecta
 * @param conn - Ponteiro para a estrutura de conexão
 * @param reason - Código do motivo da desconexão
 */
void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	// Log informando a desconexão e o motivo
	LOG_INF("Disconnected. Reason %d", reason);
	
	// Decrementa a referência da conexão (libera recursos)
	bt_conn_unref(my_conn);
}

/**
 * Callback chamado quando a conexão é reciclada
 * Isso acontece após uma desconexão, permitindo reutilização da estrutura
 */
/**
 * Callback chamado quando a conexão é reciclada
 * Isso acontece após uma desconexão, permitindo reutilização da estrutura
 */
void on_recycled(void)
{
	// Reinicia o anúncio para aceitar novas conexões
	advertising_start();
}

/**
 * Estrutura de callbacks para eventos de conexão Bluetooth
 * Registra as funções que serão chamadas em cada evento
 */
struct bt_conn_cb connection_callbacks = {
	.connected              = on_connected,      // Quando conecta
	.disconnected           = on_disconnected,   // Quando desconecta
	.recycled               = on_recycled,       // Quando a conexão é reciclada
};

/**
 * Função principal do programa
 * Inicializa todos os componentes e entra em loop infinito piscando o LED
 */
int main(void)
{
	int err;               // Variável para armazenar códigos de erro

	// Mensagem inicial de log
	LOG_INF("Starting BLE Coleira - Simple Bluetooth Connection\n");

	// Verifica se o dispositivo GPIO do LED está pronto
	if (!gpio_is_ready_dt(&led)) 
	{
		LOG_ERR("LED GPIO device not ready");
		return -1;
	}

	// Configura o pino do LED como saída e inicializa em LOW (desligado)
	err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (err) 
	{
		LOG_ERR("Failed to configure LED GPIO (err %d)", err);
		return -1;
	}

	LOG_INF("LED initialized on GPIO pin 7");

	// Registra os callbacks de conexão Bluetooth
	err = bt_conn_cb_register(&connection_callbacks);
	if (err) 
	{
		LOG_ERR("Connection callback register failed (err %d)", err);
	}

	// Inicializa e habilita o Bluetooth
	err = bt_enable(NULL);
	if (err) 
	{
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return -1;  // Retorna erro se a inicialização falhar
	}

	// Log de sucesso da inicialização do Bluetooth
	LOG_INF("Bluetooth initialized");
	
	// Inicializa a estrutura de trabalho para anúncio
	k_work_init(&adv_work, adv_work_handler);
	
	// Inicia o anúncio Bluetooth (torna o dispositivo visível)
	advertising_start();

	// Loop infinito - mantém o programa rodando
	for (;;) 
	{
		// Alterna o estado do LED (pisca o LED)
		gpio_pin_toggle_dt(&led);
		
		// Aguarda 1 segundo antes da próxima iteração
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
