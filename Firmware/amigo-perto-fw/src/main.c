/*
 * Amigo Perto - Sistema de Alerta de Proximidade
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Aplicação principal do firmware que implementa um sistema de alerta de proximidade
 * controlado remotamente via Bluetooth Low Energy.
 *
 * Hardware:
 * - XIAO nRF52840 (Nordic nRF52840)
 * - Buzzer piezoelétrico (PWM)
 * - Bateria LiPo 1S (3.0V-4.2V)
 * - LED Verde (conexão BLE)
 * - LED Azul (advertising)
 *
 * Funcionalidades:
 * - BLE advertising para descoberta do dispositivo
 * - Serviço GATT Buzzer customizado para controle remoto do alarme
 * - Serviço GATT Battery padrão (0x180F) para monitoramento de bateria
 * - LEDs de status para feedback visual
 * - HAL modular para abstração de hardware
 *
 * Arquitetura:
 * - src/main.c         : Aplicação principal (este arquivo)
 * - src/hal/           : Hardware Abstraction Layer (BLE, Buzzer, Battery)
 * - src/gatt/          : Serviços GATT BLE
 * - include/hal/       : APIs públicas HAL
 * - include/gatt/      : APIs públicas GATT
 */

// === INCLUDES DO ZEPHYR RTOS ===
#include <zephyr/kernel.h>         // Kernel: threads, timers, delays
#include <zephyr/logging/log.h>    // Sistema de logging
#include <zephyr/drivers/gpio.h>   // Controle de GPIO para LEDs

// === HARDWARE ABSTRACTION LAYER ===
#include "hal/ble.h"        // HAL BLE: advertising, conexões
#include "hal/buzzer.h"     // HAL Buzzer: controle PWM do alarme
#include "hal/battery.h"    // Battery lib: init, amostragem, callbacks

// === SERVIÇOS GATT ===
#include "gatt/buzzer_service.h"   // Serviço GATT customizado do buzzer
#include "gatt/battery_service.h"  // Battery Service padrão (0x180F)

// Registra módulo de logging
LOG_MODULE_REGISTER(MainApp, LOG_LEVEL_INF);

// === CONFIGURAÇÕES ===

// Nome do dispositivo BLE (definido em prj.conf via CONFIG_BT_DEVICE_NAME)
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME

// === CONFIGURAÇÕES DE ECONOMIA DE ENERGIA ===

// Duty-cycle dos LEDs: 50ms ON / 1950ms OFF = 2.5% (economiza ~97.5% de energia)
#define LED_BLINK_ON_MS   50
#define LED_BLINK_OFF_MS  1950

// Intervalo de advertising BLE em milissegundos
// Mais amostras de RSSI (melhor “estabilidade” no front-end), com maior consumo.
#define ADV_INTERVAL_MIN_MS 50
#define ADV_INTERVAL_MAX_MS 50

// Threshold de bateria crítica (percentual)
// Abaixo deste valor, um warning é exibido
#define BATTERY_CRITICAL_THRESHOLD 10

// Intervalo de amostragem da bateria (ms)
// Coleira BLE: prioriza autonomia e reduz wakeups/ADC.
#define BATTERY_SAMPLING_INTERVAL_MS 60000

// === CONFIGURAÇÃO DOS LEDS ===

// LED Verde: indica conexão BLE ativa
// Hardware: GPIO configurado via devicetree alias 'ledverde'
#define LED_VERDE_NODE DT_ALIAS(ledverde)
#if DT_NODE_HAS_STATUS(LED_VERDE_NODE, okay)
static const struct gpio_dt_spec led_verde = GPIO_DT_SPEC_GET(LED_VERDE_NODE, gpios);
#else
#error "LED verde não configurado no devicetree (alias 'ledverde' ausente)"
#endif

// LED Azul: indica advertising BLE ativo
// Hardware: GPIO configurado via devicetree alias 'ledazul'
#define LED_AZUL_NODE DT_ALIAS(ledazul)
#if DT_NODE_HAS_STATUS(LED_AZUL_NODE, okay)
static const struct gpio_dt_spec led_azul = GPIO_DT_SPEC_GET(LED_AZUL_NODE, gpios);
#else
#error "LED azul não configurado no devicetree (alias 'ledazul' ausente)"
#endif

// === TIMERS PARA PISCAR LEDS COM BAIXO CONSUMO ===
static struct k_timer led_azul_timer;
static struct k_timer led_verde_timer;
static bool led_azul_ativo = false;
static bool led_verde_ativo = false;

/**
 * Handler do timer do LED azul (advertising)
 * Alterna estado ON/OFF com duty-cycle de 2.5%
 */
static void led_azul_timer_handler(struct k_timer *timer)
{
	// Alterna estado
	led_azul_ativo = !led_azul_ativo;
	gpio_pin_set_dt(&led_azul, led_azul_ativo ? 1 : 0);
	
	// Próximo timeout: curto se aceso, longo se apagado
	k_timer_start(&led_azul_timer, 
	              K_MSEC(led_azul_ativo ? LED_BLINK_ON_MS : LED_BLINK_OFF_MS), 
	              K_NO_WAIT);
}

/**
 * Handler do timer do LED verde (conectado)
 * Alterna estado ON/OFF com duty-cycle de 2.5%
 */
static void led_verde_timer_handler(struct k_timer *timer)
{
	// Alterna estado
	led_verde_ativo = !led_verde_ativo;
	gpio_pin_set_dt(&led_verde, led_verde_ativo ? 1 : 0);
	
	// Próximo timeout: curto se aceso, longo se apagado
	k_timer_start(&led_verde_timer, 
	              K_MSEC(led_verde_ativo ? LED_BLINK_ON_MS : LED_BLINK_OFF_MS), 
	              K_NO_WAIT);
}

// === CALLBACKS BLE ===
// Funções chamadas pelo HAL BLE para notificar eventos de conexão

/**
 * Callback: Dispositivo conectado via BLE
 *
 * Chamado quando um dispositivo central (ex: smartphone) estabelece conexão.
 * Atualiza LEDs de status e exibe parâmetros de conexão negociados.
 *
 * @param conn_info Informações da conexão (intervalo, latência, timeout)
 */
static void on_ble_connected(const hal_ble_conn_info_t *conn_info)
{
	LOG_INF("=== BLE CONECTADO ===");
	LOG_INF("Intervalo: %u ms", conn_info->interval_ms);
	LOG_INF("Latência: %u conexões", conn_info->latency);
	LOG_INF("Timeout: %u ms", conn_info->timeout_ms);
	
	// Para timer do LED azul e apaga
	k_timer_stop(&led_azul_timer);
	gpio_pin_set_dt(&led_azul, 0);
	led_azul_ativo = false;
	
	// Inicia timer do LED verde (piscar lento)
	led_verde_ativo = false;
	k_timer_start(&led_verde_timer, K_MSEC(LED_BLINK_OFF_MS), K_NO_WAIT);
}

/**
 * Callback: Dispositivo desconectado
 *
 * Chamado quando a conexão BLE é encerrada (timeout, comando remoto, etc).
 * Desliga o buzzer (segurança) e atualiza status dos LEDs.
 *
 * @param reason Código HCI do motivo da desconexão
 */
static void on_ble_disconnected(uint8_t reason)
{
	LOG_INF("=== BLE DESCONECTADO ===");
	LOG_INF("Motivo: 0x%02X", reason);
	
	// Segurança: desliga buzzer ao desconectar
	hal_buzzer_set_intermittent(false, 0);
	
	// Para timer do LED verde e apaga
	k_timer_stop(&led_verde_timer);
	gpio_pin_set_dt(&led_verde, 0);
	led_verde_ativo = false;
	
	// Reinicia timer do LED azul (piscar lento)
	led_azul_ativo = false;
	k_timer_start(&led_azul_timer, K_MSEC(LED_BLINK_OFF_MS), K_NO_WAIT);
}

/**
 * Callback: Advertising iniciado
 *
 * Chamado quando o dispositivo começa a anunciar sua presença.
 * Dispositivos BLE podem descobrir e conectar neste estado.
 */
static void on_ble_adv_started(void)
{
	LOG_INF("BLE Advertising iniciado (modo baixo consumo)");
	
	// Inicia timer do LED azul (piscar lento)
	led_azul_ativo = false;
	k_timer_start(&led_azul_timer, K_MSEC(LED_BLINK_OFF_MS), K_NO_WAIT);
}

/**
 * Callback: Advertising parado
 *
 * Chamado quando advertising é interrompido (conexão estabelecida ou erro).
 */
static void on_ble_adv_stopped(void)
{
	LOG_DBG("BLE Advertising parado");
	
	// Para timer do LED azul e apaga
	k_timer_stop(&led_azul_timer);
	gpio_pin_set_dt(&led_azul, 0);
	led_azul_ativo = false;
}

// Registra callbacks BLE no HAL
static const hal_ble_callbacks_t ble_callbacks = {
	.connected = on_ble_connected,
	.disconnected = on_ble_disconnected,
	.adv_started = on_ble_adv_started,
	.adv_stopped = on_ble_adv_stopped,
};

// === CALLBACKS GATT BUZZER SERVICE ===
// Funções chamadas quando características GATT são escritas remotamente

/**
 * Callback: Característica Buzzer Intermitente escrita via BLE
 *
 * Chamado quando o dispositivo central (smartphone) escreve na característica
 * Buzzer Intermittent do serviço GATT customizado.
 *
 * Controla o modo intermitente do alarme com intensidade média.
 *
 * @param buzzer_state true=ativar alarme intermitente, false=desligar
 */
static void on_buzzer_intermittent_write(const bool buzzer_state)
{
	LOG_INF("=== COMANDO BUZZER ===");
	LOG_INF("Estado: %s", buzzer_state ? "ATIVADO" : "DESATIVADO");
	
	// Controla o buzzer através do HAL com intensidade média (30%, otimizado para baixo consumo)
	// Frequência: 18 kHz (audível para cachorros, quase inaudível para humanos)
	// Padrão burst: 20ms ON / 80ms OFF (reduz consumo em ~75%)
	int err = hal_buzzer_set_intermittent(buzzer_state, HAL_BUZZER_INTENSITY_MEDIUM);
	if (err != HAL_BUZZER_SUCCESS) 
	{
		LOG_ERR("Erro ao controlar buzzer: %d", err);
	}
}

// Registra callbacks Buzzer Service
static const struct gatt_buzzer_service_cb buzzer_callbacks = {
	.buzzer_intermittent_cb = on_buzzer_intermittent_write,
};

// === CALLBACKS GATT BATTERY SERVICE ===
// O serviço GATT de bateria lê os valores on-demand via battery_get_millivolt().
// Aqui registramos callbacks da *biblioteca* de bateria só para logging/debug.

static void log_battery_voltage(uint16_t millivolt)
{
	uint8_t battery_percentage = 0;

	int ret = battery_get_percentage(&battery_percentage, millivolt);
	if (ret)
	{
		LOG_ERR("Falha ao calcular percentual de bateria (%d)", ret);
		return;
	}

	LOG_INF("Battery: %u mV (%u%%)", millivolt, battery_percentage);
	if (battery_percentage < BATTERY_CRITICAL_THRESHOLD)
	{
		LOG_WRN("ATENÇÃO: Bateria crítica! (%u%%) - Recarregue em breve", battery_percentage);
	}
}

static void log_charging_state(bool is_charging)
{
	LOG_INF("Charger %s", is_charging ? "connected" : "disconnected");
}

// === FUNÇÃO PRINCIPAL ===

/**
 * Função principal do firmware
 *
 * Sequência de inicialização:
 * 1. LEDs de status (verde e azul)
 * 2. HAL Buzzer (controle PWM do alarme)
 * 3. HAL Battery (leitura ADC da bateria LiPo)
 * 4. HAL BLE (stack Bluetooth)
 * 5. Serviços GATT (Buzzer e Battery)
 * 6. Inicia advertising BLE
 * 7. Loop infinito aguardando eventos via callbacks
 *
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int main(void)
{
	int err;
	bool battery_ok = false;

	LOG_INF("=============================================");
	LOG_INF("   Amigo Perto - Alerta de Proximidade");
	LOG_INF("   Modo: Ultra Baixo Consumo");
	LOG_INF("=============================================");
	LOG_INF("");

	// === ETAPA 1: Inicialização dos LEDs ===
	
	LOG_INF("[1/6] Inicializando LEDs (modo economia)...");
	
	// Verifica se GPIOs estão prontos
	if (!gpio_is_ready_dt(&led_verde)) 
	{
		LOG_ERR("GPIO LED verde não está pronto");
		return -1;
	}
	
	if (!gpio_is_ready_dt(&led_azul)) 
	{
		LOG_ERR("GPIO LED azul não está pronto");
		return -1;
	}

	// Configura LEDs como saída, inicialmente desligados
	err = gpio_pin_configure_dt(&led_verde, GPIO_OUTPUT_INACTIVE);
	if (err) 
	{
		LOG_ERR("Erro ao configurar LED verde: %d", err);
		return -1;
	}
	
	err = gpio_pin_configure_dt(&led_azul, GPIO_OUTPUT_INACTIVE);
	if (err) 
	{
		LOG_ERR("Erro ao configurar LED azul: %d", err);
		return -1;
	}

	// Inicializa timers dos LEDs
	k_timer_init(&led_azul_timer, led_azul_timer_handler, NULL);
	k_timer_init(&led_verde_timer, led_verde_timer_handler, NULL);

	LOG_INF("LEDs configurados (duty-cycle: 2.5%%)");
	LOG_INF("");

	// === ETAPA 2: Inicialização do Buzzer ===
	
	LOG_INF("[2/6] Inicializando HAL Buzzer...");
	
	err = hal_buzzer_init();
	if (err != HAL_BUZZER_SUCCESS) 
	{
		LOG_ERR("Erro ao inicializar HAL Buzzer: %d", err);
		return -1;
	}

	LOG_INF("HAL Buzzer inicializado");
	LOG_INF("");
	
	// === ETAPA 3: Inicialização do BLE ===
	
	LOG_INF("[3/6] Inicializando HAL BLE...");
	
	err = hal_ble_init(DEVICE_NAME, &ble_callbacks);
	if (err != HAL_BLE_SUCCESS) 
	{
		LOG_ERR("Erro ao inicializar HAL BLE: %d", err);
		return -1;
	}

	LOG_INF("HAL BLE inicializado - Device: %s", DEVICE_NAME);
	LOG_INF("");
	
	// === ETAPA 4: Inicialização dos Serviços GATT ===
	
	LOG_INF("[4/6] Inicializando serviços GATT...");
	
	// Serviço customizado: Buzzer Service
	err = gatt_buzzer_service_init(&buzzer_callbacks);
	if (err != 0) 
	{
		LOG_ERR("Erro ao inicializar GATT Buzzer Service: %d", err);
		return -1;
	}
	LOG_INF("  - Buzzer Service (customizado)");
	
	// Serviço padrão: Battery Service (UUID 0x180F)
	err = battery_service_init();
	if (err != 0)
	{
		// Best-effort: BLE deve continuar mesmo que a bateria/ADC não esteja pronto
		LOG_WRN("Battery Service init retornou erro (%d). BLE seguirá ativo", err);
	}
	LOG_INF("  - Battery Service (0x180F)");
	
	LOG_INF("Serviços GATT inicializados");
	LOG_INF("");
	
	// === ETAPA 5: Inicia Advertising BLE ===
	
	LOG_INF("[5/6] Iniciando BLE Advertising...");
	
	// Configura parâmetros de advertising otimizados
	hal_ble_adv_params_t adv_params = {
		.interval_min_ms = ADV_INTERVAL_MIN_MS,
		.interval_max_ms = ADV_INTERVAL_MAX_MS,
		.connectable = true,                  // Aceita conexões
		.use_identity = true,                 // Usa MAC fixo (rastreável)
	};
	
	err = hal_ble_start_advertising(&adv_params);
	if (err != HAL_BLE_SUCCESS) 
	{
		LOG_ERR("Erro ao iniciar advertising: %d", err);
		return -1;
	}
	
	LOG_INF("Advertising iniciado - Intervalo: %u-%u ms", ADV_INTERVAL_MIN_MS, ADV_INTERVAL_MAX_MS);
	LOG_INF("");

	// === ETAPA 6: Inicialização da Bateria (best-effort) ===
	// Igual ao exemplo: mantém BLE/advertising ativo mesmo se a bateria falhar.

	LOG_INF("[6/6] Inicializando Battery library...");
	err = battery_init();
	if (err)
	{
		LOG_WRN("Battery init falhou (%d). BLE seguirá ativo com valores=0", err);
		battery_ok = false;
	}
	else
	{
		battery_ok = true;
	}

	if (battery_ok)
	{
		int ret = battery_register_charging_callback(log_charging_state);
		if (ret)
		{
			LOG_WRN("Falha ao registrar charging callback (%d)", ret);
		}

		ret = battery_register_sample_callback(log_battery_voltage);
		if (ret)
		{
			LOG_WRN("Falha ao registrar sample callback (%d)", ret);
		}

		// Amostra inicial (1x) + amostragem periódica
		ret = battery_sample_once();
		if (ret)
		{
			LOG_WRN("Falha ao amostrar bateria uma vez (%d)", ret);
		}
		k_sleep(K_SECONDS(3));

		ret = battery_start_sampling(BATTERY_SAMPLING_INTERVAL_MS);
		if (ret)
		{
			LOG_WRN("Falha ao iniciar amostragem periódica (%d)", ret);
		}
	}
	
	// === SISTEMA PRONTO ===
	
	LOG_INF("=============================================");
	LOG_INF("   SISTEMA INICIALIZADO COM SUCESSO");
	LOG_INF("=============================================");
	LOG_INF("");
	LOG_INF("Status:");
	LOG_INF("  - BLE Advertising: ATIVO");
	LOG_INF("  - Aguardando conexão...");
	LOG_INF("");
	LOG_INF("Serviços BLE disponíveis:");
	LOG_INF("  - Buzzer Service (customizado)");
	LOG_INF("      Intermittent: write 0x01=ON, 0x00=OFF");
	LOG_INF("  - Battery Service (0x180F)");
	LOG_INF("      Battery Level (0x2A19): read 0-100%%");
	LOG_INF("      Battery Voltage (customizado): read milivolts");
	LOG_INF("");
	LOG_INF("LEDs (piscantes):");
	LOG_INF("  - Azul: Advertising ativo");
	LOG_INF("  - Verde: Conectado");
	LOG_INF("=============================================");
	
	// Loop infinito - sistema controlado por eventos via callbacks
	k_sleep(K_FOREVER);
	
	return 0;
}

