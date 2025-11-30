/*
 * Amigo Perto - Sistema de Alerta de Proximidade
 * 
 * @file main.c
 * @brief Aplicação principal do firmware
 * Localização: src/main.c
 * 
 * Descrição: Este firmware implementa um sistema de alerta de proximidade
 * controlado remotamente via Bluetooth Low Energy.
 * 
 * Funcionalidades:
 * - Advertising BLE para descoberta do dispositivo
 * - Serviço GATT Buzzer customizado (controle remoto de alarme)
 * - Serviço GATT Battery padrão (monitoramento de bateria CR2032)
 * - LEDs de status (verde=conexão, azul=advertising)
 * - HAL modular (Buzzer, Battery, BLE)
 * 
 * Arquitetura:
 * - src/main.c           - Aplicação principal
 * - src/hal/             - Hardware Abstraction Layer
 * - src/gatt/            - Serviços GATT BLE
 * - include/hal/         - Headers públicos HAL
 * - include/gatt/        - Headers públicos GATT
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

// Bibliotecas do Zephyr RTOS
#include <zephyr/kernel.h>              // Funções principais do kernel (threads, timers, delays)
#include <zephyr/logging/log.h>         // Sistema de logging para debug e mensagens

// Biblioteca para controle de GPIO
#include <zephyr/drivers/gpio.h>       // Controle de GPIO para LEDs

// Hardware Abstraction Layer
#include "hal/buzzer.h"
#include "hal/battery.h"
#include "hal/ble.h"

// GATT Services
#include "gatt/buzzer_service.h"
#include "gatt/battery_service.h"

// Registra o módulo de logging com o nome "MainApp" e nível INFO
LOG_MODULE_REGISTER(MainApp, LOG_LEVEL_INF);

// Configurações do nome do dispositivo
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME      		// Nome do dispositivo definido no prj.conf

// LED verde: GPIO 30 - indica status de conexão
#define LED_VERDE_NODE DT_ALIAS(ledverde)
#if DT_NODE_HAS_STATUS(LED_VERDE_NODE, okay)
static const struct gpio_dt_spec led_verde = GPIO_DT_SPEC_GET(LED_VERDE_NODE, gpios);
#else
#error "Unsupported board: ledverde devicetree alias is not defined"
#endif

// LED azul: GPIO 6 - indica eventos do sistema
#define LED_AZUL_NODE DT_ALIAS(ledazul)
#if DT_NODE_HAS_STATUS(LED_AZUL_NODE, okay)
static const struct gpio_dt_spec led_azul = GPIO_DT_SPEC_GET(LED_AZUL_NODE, gpios);
#else
#error "Unsupported board: ledazul devicetree alias is not defined"
#endif

/**
 * Callbacks HAL BLE - Eventos de conexão Bluetooth
 */

/**
 * Callback chamado quando um dispositivo se conecta via BLE
 */
static void on_ble_connected(const hal_ble_conn_info_t *conn_info)
{
	LOG_INF("Dispositivo conectado");
	LOG_INF("  Intervalo: %u ms", conn_info->interval_ms);
	LOG_INF("  Latência: %u", conn_info->latency);
	LOG_INF("  Timeout: %u ms", conn_info->timeout_ms);
	// Apaga o LED azul e acende o LED verde ao conectar
	gpio_pin_set_dt(&led_azul, 0);
	gpio_pin_set_dt(&led_verde, 1);
}

/**
 * Callback chamado quando um dispositivo se desconecta
 */
static void on_ble_disconnected(uint8_t reason)
{
	LOG_INF("Dispositivo desconectado (motivo %u)", reason);
	// Desativa o buzzer intermitente ao desconectar
	hal_buzzer_set_intermittent(false, 0);
	// Apaga o LED verde ao desconectar
	gpio_pin_set_dt(&led_verde, 0);
}

/**
 * Callback chamado quando advertising é iniciado
 */
static void on_ble_adv_started(void)
{
	LOG_INF("Advertising iniciado");
	// Mantém o LED azul aceso durante advertising
	gpio_pin_set_dt(&led_azul, 1);
}

/**
 * Callback chamado quando advertising é parado
 */
static void on_ble_adv_stopped(void)
{
	LOG_INF("Advertising parado");
}

/**
 * Estrutura de callbacks BLE
 */
static const hal_ble_callbacks_t ble_callbacks = {
	.connected = on_ble_connected,
	.disconnected = on_ble_disconnected,
	.adv_started = on_ble_adv_started,
	.adv_stopped = on_ble_adv_stopped,
};

/**
 * Callbacks GATT Buzzer Service - Eventos de escrita nas características
 */

/**
 * Callback chamado quando o buzzer intermitente é acionado via BLE
 */
static void on_buzzer_intermittent_write(const bool buzzer_state)
{
	LOG_INF("Buzzer Intermitente via BLE: %s", buzzer_state ? "ATIVADO" : "DESATIVADO");
	int err;
	err = hal_buzzer_set_intermittent(buzzer_state, HAL_BUZZER_INTENSITY_MEDIUM);
	if (err != HAL_BUZZER_SUCCESS) 
	{
		LOG_ERR("Falha ao controlar buzzer intermitente (err %d)", err);
	}
}

/**
 * Estrutura de callbacks GATT Buzzer
 */
static const struct gatt_buzzer_service_cb buzzer_callbacks = {
	.buzzer_intermittent_cb = on_buzzer_intermittent_write,
};

/**
 * Callbacks GATT Battery Service - Eventos de leitura da bateria
 */

/**
 * Callback chamado quando a bateria é lida via BLE
 */
static void on_battery_read(uint8_t percentage)
{
	LOG_INF("Bateria lida via BLE: %d%%", percentage);
}

/**
 * Estrutura de callbacks GATT Battery
 */
static const struct gatt_battery_service_cb battery_callbacks = {
	.battery_read_cb = on_battery_read,
};

/**
 * Função principal do programa
 * Inicializa todos os componentes usando HAL e entra em loop infinito
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int main(void)
{
	int err;

	LOG_INF("==================================================");
	LOG_INF("  Amigo Perto - Sistema de Alerta de Proximidade");
	LOG_INF("==================================================");

	// ========== Inicialização dos LEDs de status ==========
	
	if (!gpio_is_ready_dt(&led_verde)) 
	{
		LOG_ERR("GPIO do LED verde não está pronto");
		return -1;
	}
	
	if (!gpio_is_ready_dt(&led_azul)) 
	{
		LOG_ERR("GPIO do LED azul não está pronto");
		return -1;
	}

	err = gpio_pin_configure_dt(&led_verde, GPIO_OUTPUT_INACTIVE);

	if (err) 
	{
		LOG_ERR("Falha ao configurar LED verde (err %d)", err);
		return -1;
	}
	
	err = gpio_pin_configure_dt(&led_azul, GPIO_OUTPUT_INACTIVE);

	if (err) 
	{
		LOG_ERR("Falha ao configurar LED azul (err %d)", err);
		return -1;
	}

	LOG_INF("LEDs de status configurados");
	
	// Não acende LED verde durante inicialização

	// ========== Inicialização HAL Buzzer ==========
	
	err = hal_buzzer_init();
	if (err != HAL_BUZZER_SUCCESS) 
	{
		LOG_ERR("Falha ao inicializar HAL Buzzer (err %d)", err);
		return -1;
	}

	LOG_INF("HAL Buzzer inicializado");
	
	// ========== Inicialização HAL Battery ==========
	
	err = hal_battery_init();
	if (err != HAL_BATTERY_SUCCESS) 
	{
		LOG_ERR("Falha ao inicializar HAL Battery (err %d)", err);
		return -1;
	}

	LOG_INF("HAL Battery inicializado");
	
	// Lê informações da bateria
	hal_battery_info_t battery_info;
	err = hal_battery_get_info(&battery_info);
	
	if (err == HAL_BATTERY_SUCCESS) 
	{
		LOG_INF("Bateria: %d mV (%d%%), Estado: %d",
		        battery_info.voltage_mv,
		        battery_info.percentage,
		        battery_info.state);
		
		// Alerta se bateria crítica
		if (battery_info.state == HAL_BATTERY_STATE_CRITICAL) 
		{
			LOG_WRN("BATERIA CRÍTICA! Substituir bateria em breve");
		}
	}
	
	// ========== Inicialização HAL BLE ==========
	
	err = hal_ble_init(DEVICE_NAME, &ble_callbacks);
	if (err != HAL_BLE_SUCCESS) 
	{
		LOG_ERR("Falha ao inicializar HAL BLE (err %d)", err);
		return -1;
	}

	LOG_INF("HAL BLE inicializado");
	
	// ========== Inicialização Serviço GATT Buzzer ==========
	
	err = gatt_buzzer_service_init(&buzzer_callbacks);
	if (err != 0) 
	{
		LOG_ERR("Falha ao inicializar serviço GATT Buzzer (err %d)", err);
		return -1;
	}

	LOG_INF("Serviço GATT Buzzer inicializado");
	
	// ========== Inicialização Serviço GATT Battery ==========
	
	err = gatt_battery_service_init(&battery_callbacks);
	if (err != 0) 
	{
		LOG_ERR("Falha ao inicializar serviço GATT Battery (err %d)", err);
		return -1;
	}

	LOG_INF("Serviço GATT Battery inicializado");
	
	// ========== Inicia Advertising ==========
	
	// Parâmetros customizados de advertising
	hal_ble_adv_params_t adv_params = {
		.interval_min_ms = 500,
		.interval_max_ms = 500,
		.connectable = true,
		.use_identity = true,
	};
	
	err = hal_ble_start_advertising(&adv_params);

	if (err != HAL_BLE_SUCCESS) 
	{
		LOG_ERR("Falha ao iniciar advertising (err %d)", err);
		return -1;
	}
	
	// ========== Sistema Pronto ==========
	
	// Não apaga LED verde após inicialização
	
	LOG_INF("==================================================");
	LOG_INF("  Sistema inicializado com sucesso!");
	LOG_INF("  Aguardando conexão BLE...");
	LOG_INF("");
	LOG_INF("  Controle remoto disponível via BLE:");
	LOG_INF("    - Buzzer Intermitente (0x00=OFF, 0x01=ON)");
	LOG_INF("    - Battery Service (0x180F) - Leitura sob demanda");
	LOG_INF("==================================================");
	
	for (;;) 
	{
		// O sistema responde via callbacks BLE
		k_sleep(K_FOREVER);
	}
	
	return 0;
}

