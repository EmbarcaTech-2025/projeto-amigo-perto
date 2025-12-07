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
#include "hal/battery.h"    // HAL Battery: leitura de percentual

// === SERVIÇOS GATT ===
#include "gatt/buzzer_service.h"   // Serviço GATT customizado do buzzer
#include "gatt/battery_service.h"  // Serviço GATT padrão de bateria (0x180F)

// === MÓDULO DE TESTE DE ENERGIA ===
#ifdef CONFIG_ENERGY_TEST
#include "test/energy_test.h"
#endif

// Registra módulo de logging
LOG_MODULE_REGISTER(MainApp, LOG_LEVEL_INF);

// === CONFIGURAÇÕES ===

// Nome do dispositivo BLE (definido em prj.conf via CONFIG_BT_DEVICE_NAME)
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME

// Intervalo de advertising BLE em milissegundos
// Menor intervalo = mais fácil descobrir, mas consome mais energia
#define ADV_INTERVAL_MS 500

// Threshold de bateria crítica (percentual)
// Abaixo deste valor, um warning é exibido
#define BATTERY_CRITICAL_THRESHOLD 10

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
	
	// Atualiza LEDs: azul OFF (para advertising), verde ON (conectado)
	gpio_pin_set_dt(&led_azul, 0);
	gpio_pin_set_dt(&led_verde, 1);
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
	
	// Atualiza LED: verde OFF (não conectado)
	gpio_pin_set_dt(&led_verde, 0);
}

/**
 * Callback: Advertising iniciado
 *
 * Chamado quando o dispositivo começa a anunciar sua presença.
 * Dispositivos BLE podem descobrir e conectar neste estado.
 */
static void on_ble_adv_started(void)
{
	LOG_INF("BLE Advertising iniciado");
	
	// Atualiza LED: azul ON (advertising ativo)
	gpio_pin_set_dt(&led_azul, 1);
}

/**
 * Callback: Advertising parado
 *
 * Chamado quando advertising é interrompido (conexão estabelecida ou erro).
 */
static void on_ble_adv_stopped(void)
{
	LOG_DBG("BLE Advertising parado");
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
	
	// Controla o buzzer através do HAL com intensidade média
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
// Funções chamadas quando características GATT são lidas remotamente

/**
 * Callback: Característica Battery Level lida via BLE
 *
 * Chamado quando o dispositivo central (smartphone) lê a característica
 * Battery Level (0x2A19) do serviço GATT padrão Battery (0x180F).
 *
 * @param percentage Percentual de bateria lido (0-100%)
 * @param voltage_mv Tensão da bateria em milivolts
 */
static void on_battery_read(uint8_t percentage, uint16_t voltage_mv)
{
	LOG_INF("Bateria lida via BLE: %d%% (%dmV)", percentage, voltage_mv);
}

// Registra callbacks Battery Service
static const struct gatt_battery_service_cb battery_callbacks = {
	.battery_read_cb = on_battery_read,
};

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

	LOG_INF("=============================================");
	LOG_INF("   Amigo Perto - Alerta de Proximidade");
	LOG_INF("=============================================");
	LOG_INF("");

	// === ETAPA 1: Inicialização dos LEDs ===
	
	LOG_INF("[1/6] Inicializando LEDs...");
	
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

	LOG_INF("LEDs configurados com sucesso");
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
	
	// === ETAPA 3: Inicialização da Bateria ===
	
	LOG_INF("[3/6] Inicializando HAL Battery...");
	
	err = hal_battery_init();
	if (err != HAL_BATTERY_SUCCESS) 
	{
		LOG_ERR("Erro ao inicializar HAL Battery: %d", err);
		return -1;
	}

	// Lê nível inicial da bateria
	uint8_t battery_level = hal_battery_get_percentage();
	LOG_INF("HAL Battery inicializado - Nível: %d%%", battery_level);
	
	// Alerta se bateria estiver crítica
	if (battery_level < BATTERY_CRITICAL_THRESHOLD) 
	{
		LOG_WRN("ATENÇÃO: Bateria crítica! (%d%%) - Recarregue em breve", battery_level);
	}
	LOG_INF("");
	
	// === ETAPA 4: Inicialização do BLE ===
	
	LOG_INF("[4/6] Inicializando HAL BLE...");
	
	err = hal_ble_init(DEVICE_NAME, &ble_callbacks);
	if (err != HAL_BLE_SUCCESS) 
	{
		LOG_ERR("Erro ao inicializar HAL BLE: %d", err);
		return -1;
	}

	LOG_INF("HAL BLE inicializado - Device: %s", DEVICE_NAME);
	LOG_INF("");
	
	// === ETAPA 5: Inicialização dos Serviços GATT ===
	
	LOG_INF("[5/6] Inicializando serviços GATT...");
	
	// Serviço customizado: Buzzer Service
	err = gatt_buzzer_service_init(&buzzer_callbacks);
	if (err != 0) 
	{
		LOG_ERR("Erro ao inicializar GATT Buzzer Service: %d", err);
		return -1;
	}
	LOG_INF("  - Buzzer Service (customizado)");
	
	// Serviço padrão: Battery Service (UUID 0x180F)
	err = gatt_battery_service_init(&battery_callbacks);
	if (err != 0) 
	{
		LOG_ERR("Erro ao inicializar GATT Battery Service: %d", err);
		return -1;
	}
	LOG_INF("  - Battery Service (0x180F)");
	
	LOG_INF("Serviços GATT inicializados");
	LOG_INF("");
	
	// === ETAPA 6: Inicia Advertising BLE ===
	
	LOG_INF("[6/6] Iniciando BLE Advertising...");
	
	// Configura parâmetros de advertising
	hal_ble_adv_params_t adv_params = {
		.interval_min_ms = ADV_INTERVAL_MS,   // Intervalo entre anúncios
		.interval_max_ms = ADV_INTERVAL_MS,   // Intervalo fixo
		.connectable = true,                  // Aceita conexões
		.use_identity = true,                 // Usa MAC fixo (rastreável)
	};
	
	err = hal_ble_start_advertising(&adv_params);
	if (err != HAL_BLE_SUCCESS) 
	{
		LOG_ERR("Erro ao iniciar advertising: %d", err);
		return -1;
	}
	
	LOG_INF("Advertising iniciado - Intervalo: %d ms", ADV_INTERVAL_MS);
	LOG_INF("");
	
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
	LOG_INF("LEDs:");
	LOG_INF("  - Azul: Advertising ativo");
	LOG_INF("  - Verde: Conectado");
	LOG_INF("=============================================");
	
#ifdef CONFIG_ENERGY_TEST
	// === MODO DE TESTE DE ENERGIA ===
	LOG_INF("");
	LOG_INF("========================================");
	LOG_INF("  MODO DE TESTE DE ENERGIA ATIVADO");
	LOG_INF("========================================");
	
	// Inicializa módulo de teste
	err = energy_test_init();
	if (err != ENERGY_TEST_SUCCESS)
	{
		LOG_ERR("Falha ao inicializar teste de energia: %d", err);
	}
	else
	{
		// Aguarda 5 segundos antes de iniciar
		LOG_INF("Iniciando testes em 5 segundos...");
		k_sleep(K_SECONDS(5));
		
		// Teste 1: Baseline (sem buzzer)
		energy_test_results_t baseline_results;
		energy_test_run_baseline(&baseline_results);
		
		// Aguarda 10 segundos entre testes
		LOG_INF("");
		LOG_INF("Aguardando 10 segundos...");
		k_sleep(K_SECONDS(10));
		
		// Teste 2: Com buzzer
		energy_test_results_t buzzer_results;
		energy_test_run_with_buzzer(&buzzer_results);
		
		// Análise comparativa
		LOG_INF("");
		LOG_INF("========================================");
		LOG_INF("  ANÁLISE COMPARATIVA");
		LOG_INF("========================================");
		
		float buzzer_overhead_mwh = buzzer_results.energy_mwh - baseline_results.energy_mwh;
		float buzzer_overhead_pct = (baseline_results.energy_mwh > 0) ? 
		                            (buzzer_overhead_mwh / baseline_results.energy_mwh * 100.0f) : 0.0f;
		
		LOG_INF("Consumo baseline: %.6f mWh/min", (double)baseline_results.energy_mwh);
		LOG_INF("Consumo com buzzer: %.6f mWh/min", (double)buzzer_results.energy_mwh);
		LOG_INF("Overhead do buzzer: %.6f mWh/min (+%.1f%%)", 
		        (double)buzzer_overhead_mwh, (double)buzzer_overhead_pct);
		LOG_INF("");
		LOG_INF("Projeção horária:");
		LOG_INF("  - Baseline: %.3f mWh/h", (double)baseline_results.energy_mwh * 60.0);
		LOG_INF("  - Com buzzer: %.3f mWh/h", (double)buzzer_results.energy_mwh * 60.0);
		LOG_INF("  - Overhead: %.3f mWh/h", (double)buzzer_overhead_mwh * 60.0);
		
		LOG_INF("");
		LOG_INF("=== TESTES CONCLUÍDOS ===");
		LOG_INF("Retornando ao modo normal...");
		LOG_INF("");
	}
#endif
	
	// Loop infinito - sistema controlado por eventos via callbacks
	while (1) 
	{
		// Suspende thread principal indefinidamente
		// Toda a lógica é controlada por callbacks assíncronos:
		// - BLE: on_ble_connected, on_ble_disconnected
		// - GATT: on_buzzer_intermittent_write, on_battery_read
		k_sleep(K_FOREVER);
	}
	
	return 0;
}

