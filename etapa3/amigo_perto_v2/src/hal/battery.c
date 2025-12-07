/*
 * HAL Battery - Hardware Abstraction Layer para monitoramento de bateria
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Implementação simplificada para leitura do percentual de carga da bateria LiPo 1S.
 * Usa o canal interno SAADC_VDD do nRF52840 para medir a tensão de alimentação.
 */

#include "hal/battery.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>

LOG_MODULE_REGISTER(hal_battery, LOG_LEVEL_DBG);

// === CONFIGURAÇÕES DO XIAO nRF52840 ===

// Pino P0.14 - READ_BAT / VBAT_ENABLE
// Deve ser LOW para habilitar o divisor de tensão da bateria
#define VBAT_ENABLE_PIN  14
#define VBAT_ENABLE_PORT DT_NODELABEL(gpio0)

// Pino P0.31 - PIN_VBAT (AIN7)
// Lê a tensão da bateria através do divisor resistivo R1=1MΩ, R2=510kΩ
#define VBAT_ADC_AIN     7

// === CONFIGURAÇÕES DO ADC ===

// Configuração do ADC para XIAO nRF52840
// Canal 7 = AIN7 = P0.31 = VBAT (através do divisor resistivo)
#define ADC_NODE DT_NODELABEL(adc)
#define ADC_CHANNEL 7

// Resolução do ADC (12 bits = 0-4095)
#define ADC_RESOLUTION 12

// Referência de tensão: nRF52840 usa 0.6V interna
// Com ganho 1/6, o range efetivo é 0-3.6V
// Divisor resistivo: R1≈1037Ω (VBAT) + R2=510kΩ (P0.14)
// V_adc = V_bat × (R2 / (R1 + R2)) = V_bat × (510k / 1547k) = V_bat × 0.3297
// Multiplicador: 1 / 0.3297 = 3.03 (usando 1547/510 para precisão - valor calibrado)
#define ADC_VREF_MV         600     // Referência interna 0.6V
#define ADC_GAIN_DIVISOR    6       // Ganho 1/6 = divisor 6
#define VBAT_DIVIDER_NUM    1547    // Numerador: R1 + R2 = 1037k + 510k (calibrado)
#define VBAT_DIVIDER_DEN    510     // Denominador: R2 = 510k
#define ADC_GAIN            ADC_GAIN_1_6
#define ADC_REFERENCE       ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)

// Número de amostras para oversampling (maior precisão, menos ruído)
#define ADC_SAMPLES 4

// === CARACTERÍSTICAS DA BATERIA LIPO 1S ===

// Tabela de conversão tensão -> percentual (curva de descarga LiPo 1S)
// Baseado em medições reais de bateria LiPo típica
typedef struct {
	uint16_t voltage_mv;
	uint8_t percentage;
} battery_state_t;

#define BATTERY_STATES_COUNT 11
static const battery_state_t battery_states[BATTERY_STATES_COUNT] = {
	{4200, 100}, // Totalmente carregada
	{4110, 90},
	{4020, 80},
	{3930, 70},
	{3840, 60},
	{3750, 50},
	{3660, 40},
	{3570, 30},
	{3480, 20},
	{3390, 10},
	{3300, 0}    // Tensão mínima segura
};

// === VARIÁVEIS PRIVADAS ===

// Device handle do ADC
static const struct device *adc_dev;

// Device handle do GPIO (para controle do VBAT_ENABLE)
static const struct device *gpio_dev;

// Flag de inicialização
static bool initialized = false;

// Configuração do canal ADC
static struct adc_channel_cfg channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_CHANNEL,
#ifdef CONFIG_ADC_NRFX_SAADC
	.input_positive = SAADC_CH_PSELP_PSELP_AnalogInput0 + VBAT_ADC_AIN,  // AIN7 = P0.31 (VBAT)
#endif
};

// Configuração da sequência de leitura
static struct adc_sequence sequence = {
	.channels = BIT(ADC_CHANNEL),
	.resolution = ADC_RESOLUTION,
};

// Buffer para armazenar amostras do ADC
static int16_t adc_sample_buffer[ADC_SAMPLES];

// === FUNÇÕES AUXILIARES PRIVADAS ===

/**
 * Converte valor bruto do ADC para tensão em milivolts
 * 
 * Usa a API do Zephyr adc_raw_to_millivolts() que já considera:
 * - Resolução do ADC (12 bits)
 * - Ganho configurado (1/6)
 * - Referência de tensão (0.6V)
 * - Saturação e faixa interna
 * 
 * Depois compensa o divisor resistivo R1≈1037Ω, R2=510kΩ:
 * V_bateria = V_adc_pin × (R1 + R2) / R2 = V_adc_pin × 1547 / 510 ≈ V_adc_pin × 3.03
 */
static inline uint16_t adc_raw_to_mv(int16_t adc_value) {
	// Converte usando API do Zephyr (já aplica ganho, referência e resolução)
	int32_t adc_mv = adc_value;
	adc_raw_to_millivolts(ADC_VREF_MV, ADC_GAIN, ADC_RESOLUTION, &adc_mv);
	
	// Compensa divisor resistivo: V_bat = V_pin × (R1+R2)/R2 = V_pin × 1547/510
	uint16_t voltage_bat = (uint16_t)((adc_mv * VBAT_DIVIDER_NUM) / VBAT_DIVIDER_DEN);
	
	LOG_DBG("ADC raw=%d -> PIN=%dmV -> BAT=%dmV", adc_value, (int)adc_mv, voltage_bat);
	return voltage_bat;
}

/**
 * Realiza leitura do ADC com oversampling
 * 
 * Faz múltiplas leituras e calcula a média para reduzir ruído e aumentar precisão.
 * Ignora amostras inválidas (negativas ou saturadas).
 */
static int adc_read_with_oversampling(uint16_t *voltage_mv) 
{
	int32_t sum = 0;
	uint8_t valid_samples = 0;
	
	// Habilita circuito de leitura da bateria (ativa o divisor resistivo)
	// Como configuramos GPIO_ACTIVE_LOW, gpio_pin_set(1) = LOW físico = habilita
	gpio_pin_set(gpio_dev, VBAT_ENABLE_PIN, 1);
	
	// Aguarda estabilização do circuito
	k_msleep(5);
	
	// Configura buffer para receber amostras
	sequence.buffer = adc_sample_buffer;
	sequence.buffer_size = sizeof(adc_sample_buffer);
	
	// Realiza múltiplas leituras
	for (int i = 0; i < ADC_SAMPLES; i++) 
	{
		int ret = adc_read(adc_dev, &sequence);
		if (ret < 0) 
		{
			LOG_ERR("Erro na leitura ADC: %d", ret);
			continue;
		}
		
		// Lê a primeira amostra do buffer (uma leitura por vez)
		int16_t raw_value = adc_sample_buffer[0];
		
		// Valida amostra (ignora valores anormais)
		if (raw_value >= 0 && raw_value < (1 << ADC_RESOLUTION)) 
		{
			sum += raw_value;
			valid_samples++;
			LOG_DBG("Amostra %d: raw=%d", i+1, raw_value);
		}
		
		// Pequeno delay entre amostras
		k_msleep(1);
	}
	
	// Verifica se obtivemos amostras válidas
	if (valid_samples == 0) 
	{
		LOG_ERR("Nenhuma leitura ADC válida");
		return -EIO;
	}
	
	// Calcula média e converte para mV
	int16_t avg_raw = sum / valid_samples;
	*voltage_mv = adc_raw_to_mv(avg_raw);
	
	// Desabilita circuito de leitura para economia de corrente
	// Como configuramos GPIO_ACTIVE_LOW, gpio_pin_set(0) = HIGH físico = desabilita
	gpio_pin_set(gpio_dev, VBAT_ENABLE_PIN, 0);
	
	LOG_DBG("ADC: raw=%d, voltage=%d mV (%d amostras)", avg_raw, *voltage_mv, valid_samples);
	
	return 0;
}

/**
 * Converte tensão em mV para percentual de carga (0-100%)
 * 
 * Usa interpolação linear entre pontos da tabela de estados da bateria.
 * A tabela possui 11 pontos que descrevem a curva de descarga real da LiPo 1S.
 * 
 * Algoritmo:
 * 1. Verifica limites (>= 4200mV = 100%, <= 3300mV = 0%)
 * 2. Encontra dois pontos adjacentes que englobam a tensão medida
 * 3. Interpola linearmente: % = %low + (V - Vlow) × (%high - %low) / (Vhigh - Vlow)
 */
static uint8_t voltage_to_percentage(uint16_t voltage_mv) 
{
	// Verifica limites superior e inferior
	if (voltage_mv >= battery_states[0].voltage_mv) 
	{
		return 100;
	}

	if (voltage_mv <= battery_states[BATTERY_STATES_COUNT - 1].voltage_mv) 
	{
		return 0;
	}
	
	// Procura os dois pontos entre os quais a tensão se encontra
	for (uint8_t i = 0; i < BATTERY_STATES_COUNT - 1; i++) 
	{
		uint16_t voltage_high = battery_states[i].voltage_mv;
		uint16_t voltage_low = battery_states[i + 1].voltage_mv;
		
		// Encontrou o intervalo correto
		if (voltage_mv <= voltage_high && voltage_mv >= voltage_low) 
		{
			uint8_t percentage_high = battery_states[i].percentage;
			uint8_t percentage_low = battery_states[i + 1].percentage;
			
			int32_t voltage_range = voltage_high - voltage_low;
			int32_t percentage_range = percentage_high - percentage_low;
			int32_t voltage_diff = voltage_mv - voltage_low;
			
			// Proteção contra divisão por zero (improvável, mas seguro)
			if (voltage_range == 0) {
				return percentage_high;
			}
			
			// Interpolação linear
			uint8_t percentage = percentage_low + 
			                     (voltage_diff * percentage_range) / voltage_range;
			
			LOG_DBG("Interpolação: %dmV entre [%d,%d]mV -> %d%% entre [%d,%d]%%",
			        voltage_mv, voltage_low, voltage_high,
			        percentage, percentage_low, percentage_high);
			
			return percentage;
		}
	}
	
	// Não deveria chegar aqui (segurança)
	LOG_WRN("Tensão %dmV fora da tabela de estados", voltage_mv);
	return 0;
}

// === API PÚBLICA ===

/**
 * Inicializa o módulo de monitoramento de bateria
 */
int hal_battery_init(void) 
{
	// Previne reinicialização
	if (initialized) 
	{
		LOG_WRN("HAL Battery já inicializado");
		return HAL_BATTERY_SUCCESS;
	}
	
	// Obtém device do GPIO para controle do VBAT_ENABLE
	gpio_dev = DEVICE_DT_GET(VBAT_ENABLE_PORT);
	if (!device_is_ready(gpio_dev)) 
	{
		LOG_ERR("GPIO device não está pronto");
		return HAL_BATTERY_ERROR_INIT;
	}
	
	// Configura P0.14 como saída ACTIVE_LOW (0=habilita divisor, 1=desabilita)
	// Inicialmente em estado inativo (HIGH lógico = LOW físico = desabilitado)
	int ret = gpio_pin_configure(gpio_dev, VBAT_ENABLE_PIN, 
	                             GPIO_OUTPUT_INACTIVE | GPIO_ACTIVE_LOW);
	if (ret < 0) 
	{
		LOG_ERR("Falha ao configurar pino VBAT_ENABLE: %d", ret);
		return HAL_BATTERY_ERROR_INIT;
	}
	
	// Obtém device do ADC
	adc_dev = DEVICE_DT_GET(ADC_NODE);
	if (!device_is_ready(adc_dev)) 
	{
		LOG_ERR("ADC device não está pronto");
		return HAL_BATTERY_ERROR_INIT;
	}
	
	// Configura canal ADC
	ret = adc_channel_setup(adc_dev, &channel_cfg);
	if (ret < 0) 
	{
		LOG_ERR("Falha ao configurar canal ADC: %d", ret);
		return HAL_BATTERY_ERROR_INIT;
	}
	
	initialized = true;
	
	// Faz leitura inicial para validar
	uint8_t initial_level = hal_battery_get_percentage();
	LOG_INF("HAL Battery inicializado - Nível: %d%%", initial_level);
	
	return HAL_BATTERY_SUCCESS;
}

/**
 * Lê o percentual de carga da bateria
 */
uint8_t hal_battery_get_percentage(void) 
{
	// Verifica se foi inicializado
	if (!initialized) 
	{
		LOG_ERR("HAL Battery não inicializado");
		return 0;
	}
	
	// Lê tensão com oversampling
	uint16_t voltage_mv;
	int ret = adc_read_with_oversampling(&voltage_mv);
	if (ret < 0) 
	{
		LOG_ERR("Erro ao ler tensão da bateria");
		return 0;
	}
	
	// Converte para percentual
	uint8_t percentage = voltage_to_percentage(voltage_mv);
	
	LOG_DBG("Bateria: %d mV = %d%%", voltage_mv, percentage);
	
	return percentage;
}

/**
 * Lê a tensão da bateria em milivolts
 */
int hal_battery_get_millivolt(uint16_t *battery_millivolt) 
{
	// Verifica se foi inicializado
	if (!initialized) 
	{
		LOG_ERR("HAL Battery não inicializado");
		return HAL_BATTERY_ERROR_INIT;
	}
	
	// Valida ponteiro
	if (battery_millivolt == NULL) 
	{
		LOG_ERR("Ponteiro nulo fornecido");
		return HAL_BATTERY_ERROR_READ;
	}
	
	// Lê tensão com oversampling
	int ret = adc_read_with_oversampling(battery_millivolt);
	if (ret < 0) 
	{
		LOG_ERR("Erro ao ler tensão da bateria");
		return HAL_BATTERY_ERROR_READ;
	}
	
	LOG_DBG("Tensão da bateria: %d mV", *battery_millivolt);
	
	return HAL_BATTERY_SUCCESS;
}
