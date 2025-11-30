/*
 * HAL Battery - Hardware Abstraction Layer para monitoramento de bateria
 * 
 * @file battery.c
 * @brief Implementação do HAL Battery
 * Localização: src/hal/battery.c
 * Header público: include/hal/battery.h
 * 
 * Este módulo implementa o monitoramento de bateria através de ADC para
 * leitura de tensão e gerenciamento de estados de carga.
 * 
 * Características:
 * - Leitura via ADC com oversampling para maior precisão
 * - Otimizado para bateria tipo moeda CR2032 (3.0V nominal, 2.0V mínimo)
 * - Baixo consumo: ADC ativado apenas durante leitura
 * - Suporte a divider resistivo para leitura de tensão
 * - Interpolação linear por segmentos para cálculo de percentual
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "hal/battery.h"

// Zephyr includes
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>

// Registra módulo de logging
LOG_MODULE_REGISTER(hal_battery, LOG_LEVEL_DBG);

/*******************************************************************************
 * CONFIGURAÇÕES E CONSTANTES
 ******************************************************************************/

// Configuração do ADC a partir do devicetree
#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
    !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#warning "ADC não configurado no devicetree, usando configuração padrão"
#define ADC_NODE DT_NODELABEL(adc)
#define ADC_CHANNEL 0
#define ADC_RESOLUTION 12
#else
#define ADC_NODE DT_IO_CHANNELS_CTLR(DT_PATH(zephyr_user))
#define ADC_CHANNEL DT_IO_CHANNELS_INPUT(DT_PATH(zephyr_user))
#define ADC_RESOLUTION 12
#endif

// Referência de tensão do ADC (nRF52840: 0.6V com ganho 1/6 = 3.6V range)
#define ADC_VREF_MV         3600   /**< Tensão de referência em mV */
#define ADC_GAIN            ADC_GAIN_1_6
#define ADC_REFERENCE       ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)

// Número de leituras para média
#define ADC_SAMPLES         4

// Características da bateria CR2032
#define BATTERY_VOLTAGE_MAX_MV   3000   /**< 3.0V - tensão máxima (100%) */
#define BATTERY_VOLTAGE_GOOD_MV  2800   /**< 2.8V - bom (70%) */
#define BATTERY_VOLTAGE_LOW_MV   2500   /**< 2.5V - baixo (30%) */
#define BATTERY_VOLTAGE_CRIT_MV  2200   /**< 2.2V - crítico (10%) */
#define BATTERY_VOLTAGE_MIN_MV   2000   /**< 2.0V - mínimo (0%) */

// Divisor resistivo (se aplicável)
// Se houver divisor resistivo R1/(R1+R2), ajustar:
// Exemplo: R1=1M, R2=1M -> DIVIDER_RATIO = 2.0
#define BATTERY_DIVIDER_RATIO    1.0f   /**< Razão do divisor (1.0 = sem divisor) */

/*******************************************************************************
 * VARIÁVEIS PRIVADAS
 ******************************************************************************/

// Device handle do ADC
static const struct device *adc_dev;

// Configuração do canal ADC
static struct adc_channel_cfg channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_CHANNEL,
#ifdef CONFIG_ADC_NRFX_SAADC
	.input_positive = SAADC_CH_PSELP_PSELP_VDD,  // VDD para nRF52
#endif
};

// Sequence de leitura
static struct adc_sequence sequence = {
	.channels = BIT(ADC_CHANNEL),
	.resolution = ADC_RESOLUTION,
};

// Buffer para leitura
static int16_t adc_sample_buffer[ADC_SAMPLES];

// Estado do módulo
static bool initialized = false;
static hal_battery_info_t last_reading = {
	.voltage_mv = 0,
	.percentage = 0,
	.state = HAL_BATTERY_STATE_UNKNOWN,
};

/*******************************************************************************
 * FUNÇÕES PRIVADAS - ADC
 ******************************************************************************/

/**
 * @brief Converte valor ADC raw para milivolts
 * 
 * @param adc_value Valor raw do ADC
 * @return Tensão em milivolts
 */
static uint16_t adc_raw_to_mv(int16_t adc_value)
{
	// Conversão: (adc_value / adc_max) * vref * divider_ratio
	uint32_t adc_max = (1 << ADC_RESOLUTION) - 1;
	uint32_t voltage_mv = ((uint32_t)adc_value * ADC_VREF_MV) / adc_max;
	
	// Aplica razão do divisor resistivo
	voltage_mv = (uint32_t)((float)voltage_mv * BATTERY_DIVIDER_RATIO);
	
	return (uint16_t)voltage_mv;
}

/**
 * @brief Realiza leitura do ADC com oversampling
 * 
 * @param voltage_mv Ponteiro para armazenar tensão lida
 * @return 0 em sucesso, < 0 em erro
 */
static int adc_read_with_oversampling(uint16_t *voltage_mv)
{
	int ret;
	int32_t sum = 0;
	uint8_t valid_samples = 0;
	
	// Configura buffer no sequence
	sequence.buffer = adc_sample_buffer;
	sequence.buffer_size = sizeof(adc_sample_buffer);
	
	// Realiza múltiplas leituras
	for (int i = 0; i < ADC_SAMPLES; i++) 
	{
		ret = adc_read(adc_dev, &sequence);
		if (ret < 0) 
		{
			LOG_ERR("Erro na leitura ADC: %d", ret);
			continue;
		}
		
		int16_t raw_value = adc_sample_buffer[0];
		
		// Valida leitura (ignora valores negativos ou saturados)
		if (raw_value >= 0 && raw_value < (1 << ADC_RESOLUTION)) 
		{
			sum += raw_value;
			valid_samples++;
		}
		
		// Pequeno delay entre leituras
		k_msleep(1);
	}
	
	if (valid_samples == 0) 
	{
		LOG_ERR("Nenhuma leitura ADC válida");
		return -EIO;
	}
	
	// Calcula média
	int16_t avg_raw = sum / valid_samples;
	*voltage_mv = adc_raw_to_mv(avg_raw);
	
	LOG_DBG("ADC raw avg: %d, voltage: %d mV (%d samples)", 
	        avg_raw, *voltage_mv, valid_samples);
	
	return 0;
}

/*******************************************************************************
 * FUNÇÕES PRIVADAS - CÁLCULOS
 ******************************************************************************/

/**
 * @brief Interpola linearmente entre dois pontos
 * 
 * @param x Valor de entrada
 * @param x0 Ponto inicial X
 * @param y0 Ponto inicial Y
 * @param x1 Ponto final X
 * @param y1 Ponto final Y
 * @return Valor interpolado Y
 */
static int32_t linear_interpolate(int32_t x, int32_t x0, int32_t y0, 
                                   int32_t x1, int32_t y1)
{
	if (x <= x0) return y0;
	if (x >= x1) return y1;
	
	return y0 + ((x - x0) * (y1 - y0)) / (x1 - x0);
}

/*******************************************************************************
 * API PÚBLICA
 ******************************************************************************/

int hal_battery_init(void)
{
	if (initialized) 
	{
		LOG_WRN("HAL Battery já inicializado");
		return HAL_BATTERY_SUCCESS;
	}
	
	// Obtém device do ADC
	adc_dev = DEVICE_DT_GET(ADC_NODE);
	if (!device_is_ready(adc_dev)) 
	{
		LOG_ERR("ADC device não está pronto");
		return HAL_BATTERY_ERROR_INIT;
	}
	
	// Configura canal ADC
	int ret = adc_channel_setup(adc_dev, &channel_cfg);

	if (ret < 0) 
	{
		LOG_ERR("Falha ao configurar canal ADC: %d", ret);
		return HAL_BATTERY_ERROR_INIT;
	}
	
	initialized = true;
	
	// Realiza primeira leitura
	hal_battery_info_t info;
	ret = hal_battery_get_info(&info);
	if (ret == HAL_BATTERY_SUCCESS) 
	{
		LOG_INF("HAL Battery inicializado: %d mV (%d%%, estado: %d)",
		        info.voltage_mv, info.percentage, info.state);
	} 
	else 
	{
		LOG_WRN("HAL Battery inicializado, mas leitura inicial falhou");
	}
	
	return HAL_BATTERY_SUCCESS;
}

int hal_battery_read_voltage(uint16_t *voltage_mv)
{
	if (!initialized) 
	{
		LOG_ERR("HAL Battery não inicializado");
		return HAL_BATTERY_ERROR_STATE;
	}
	
	if (voltage_mv == NULL)
	{
		LOG_ERR("Ponteiro voltage_mv é NULL");
		return HAL_BATTERY_ERROR_READ;
	}
	
	int ret = adc_read_with_oversampling(voltage_mv);
	if (ret < 0) 
	{
		LOG_ERR("Erro ao ler tensão da bateria");
		return HAL_BATTERY_ERROR_READ;
	}
	
	return HAL_BATTERY_SUCCESS;
}

uint8_t hal_battery_voltage_to_percentage(uint16_t voltage_mv)
{
	int32_t percentage;
	
	// Interpolação por segmentos para melhor precisão
	if (voltage_mv >= BATTERY_VOLTAGE_MAX_MV) 
	{
		percentage = 100;
	}
	
	else if (voltage_mv >= BATTERY_VOLTAGE_GOOD_MV) 
	{
		// 70% - 100%: 2.8V - 3.0V
		percentage = linear_interpolate(voltage_mv,
		                                BATTERY_VOLTAGE_GOOD_MV, 70,
		                                BATTERY_VOLTAGE_MAX_MV, 100);
	}
	
	else if (voltage_mv >= BATTERY_VOLTAGE_LOW_MV) 
	{
		// 30% - 70%: 2.5V - 2.8V
		percentage = linear_interpolate(voltage_mv,
		                                BATTERY_VOLTAGE_LOW_MV, 30,
		                                BATTERY_VOLTAGE_GOOD_MV, 70);
	}
	
	else if (voltage_mv >= BATTERY_VOLTAGE_CRIT_MV) 
	{
		// 10% - 30%: 2.2V - 2.5V
		percentage = linear_interpolate(voltage_mv,
		                                BATTERY_VOLTAGE_CRIT_MV, 10,
		                                BATTERY_VOLTAGE_LOW_MV, 30);
	}
	
	else if (voltage_mv >= BATTERY_VOLTAGE_MIN_MV) 
	{
		// 0% - 10%: 2.0V - 2.2V
		percentage = linear_interpolate(voltage_mv,
		                                BATTERY_VOLTAGE_MIN_MV, 0,
		                                BATTERY_VOLTAGE_CRIT_MV, 10);
	}
	
	else 
	{
		percentage = 0;
	}
	
	// Garante range 0-100
	if (percentage > 100) percentage = 100;
	if (percentage < 0) percentage = 0;
	
	return (uint8_t)percentage;
}

hal_battery_state_t hal_battery_percentage_to_state(uint8_t percentage)
{
	if (percentage > 70) 
	{
		return HAL_BATTERY_STATE_GOOD;
	}
	
	else if (percentage > 30) 
	{
		return HAL_BATTERY_STATE_MEDIUM;
	}
	
	else if (percentage > 10) 
	{
		return HAL_BATTERY_STATE_LOW;
	}
	
	else 
	{
		return HAL_BATTERY_STATE_CRITICAL;
	}
}

int hal_battery_get_info(hal_battery_info_t *info)
{
	if (!initialized) 
	{
		LOG_ERR("HAL Battery não inicializado");
		return HAL_BATTERY_ERROR_STATE;
	}
	
	if (info == NULL) 
	{
		LOG_ERR("Ponteiro info é NULL");
		return HAL_BATTERY_ERROR_READ;
	}
	
	// Lê tensão
	uint16_t voltage_mv;
	int ret = hal_battery_read_voltage(&voltage_mv);
	
	if (ret != HAL_BATTERY_SUCCESS) 
	{
		return ret;
	}
	
	// Calcula informações derivadas
	uint8_t percentage = hal_battery_voltage_to_percentage(voltage_mv);
	hal_battery_state_t state = hal_battery_percentage_to_state(percentage);
	
	// Atualiza estrutura
	info->voltage_mv = voltage_mv;
	info->percentage = percentage;
	info->state = state;
	
	// Salva última leitura
	last_reading = *info;
	
	LOG_DBG("Bateria: %d mV, %d%%, estado: %d", 
	        voltage_mv, percentage, state);
	
	return HAL_BATTERY_SUCCESS;
}

bool hal_battery_is_critical(void)
{
	if (!initialized) 
	{
		return false;
	}
	
	hal_battery_info_t info;
	int ret = hal_battery_get_info(&info);
	
	if (ret != HAL_BATTERY_SUCCESS) 
	{
		return false;
	}
	
	return (info.state == HAL_BATTERY_STATE_CRITICAL);
}
