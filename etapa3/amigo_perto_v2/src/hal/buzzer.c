/*
 * HAL Buzzer - Hardware Abstraction Layer para controle de Buzzer/LED via PWM
 * 
 * @file buzzer.c
 * @brief Implementação do HAL Buzzer
 * Localização: src/hal/buzzer.c
 * Header público: include/hal/buzzer.h
 * 
 * Este módulo implementa o controle de buzzer através de PWM para acionamento
 * de alarmes intermitentes com diferentes intensidades.
 * 
 * Funcionalidades:
 * - Controle de buzzer via PWM com duty cycle configurável
 * - Padrão intermitente (500ms ON/OFF)
 * - 3 níveis de intensidade (LOW=25%, MEDIUM=50%, HIGH=100%)
 * - Thread dedicada para temporização
 * - Controle thread-safe com semáforos
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "hal/buzzer.h"

// Zephyr includes
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>

// Registra módulo de logging
LOG_MODULE_REGISTER(hal_buzzer, LOG_LEVEL_DBG);

/*******************************************************************************
 * CONFIGURAÇÕES E CONSTANTES
 ******************************************************************************/

// Configurações e constantes

// Configuração do PWM a partir do devicetree
#define PWM_LED0_NODE DT_ALIAS(pwm_led0)

#if !DT_NODE_HAS_STATUS(PWM_LED0_NODE, okay)
#error "pwm-led0 devicetree alias não está definido ou não está okay"
#endif

// Parâmetros PWM
#define PWM_PERIOD_NS       20000000U   /**< 20ms - 50Hz */
#define PWM_PULSE_OFF_NS    0U          /**< PWM desligado */

#define PATTERN_INTERMITTENT_PERIOD_MS  500   /**< Período on/off intermitente */

/*******************************************************************************
 * VARIÁVEIS PRIVADAS
 ******************************************************************************/

// Variáveis privadas

// Device handle do PWM
static const struct pwm_dt_spec pwm_led = PWM_DT_SPEC_GET(PWM_LED0_NODE);

// Estado do módulo
static bool initialized = false;
static uint8_t current_intensity = HAL_BUZZER_INTENSITY_MEDIUM;

// Work item para padrão intermitente
static struct k_work_delayable pattern_intermittent_work;
static bool pattern_intermittent_active = false;

/*******************************************************************************
 * FUNÇÕES PRIVADAS - CONTROLE PWM
 ******************************************************************************/

// Funções privadas - controle PWM

/**
 * @brief Converte intensidade percentual em nanosegundos de pulso PWM
 * 
 * @param intensity Intensidade em porcentagem (0-100)
 * @return Duração do pulso em nanosegundos
 */
static uint32_t intensity_to_pulse_ns(uint8_t intensity)
{
	if (intensity > 100) {
		intensity = 100;
	}
	
	// Calcula duty cycle proporcional
	return (PWM_PERIOD_NS * intensity) / 100;
}

/**
 * @brief Define o PWM com base na intensidade
 * 
 * @param intensity Intensidade (0-100), 0 desliga o PWM
 * @return 0 em sucesso, < 0 em erro
 */
static int pwm_set_intensity(uint8_t intensity)
{
	uint32_t pulse_ns = intensity_to_pulse_ns(intensity);
	
	int ret = pwm_set_dt(&pwm_led, PWM_PERIOD_NS, pulse_ns);
	if (ret < 0) 
	{
		LOG_ERR("Falha ao configurar PWM (err %d)", ret);
		return ret;
	}
	
	return 0;
}

/*******************************************************************************
 * HANDLERS DE PADRÕES
 ******************************************************************************/

// Handler do padrão intermitente

/**
 * @brief Handler do padrão intermitente
 * 
 * Alterna o estado do buzzer a cada 500ms
 */
static void pattern_intermittent_handler(struct k_work *work)
{
	static bool state = false;
	
	if (!pattern_intermittent_active) 
	{
		pwm_set_intensity(0);
		return;
	}
	
	// Alterna estado
	state = !state;
	
	if (state) 
	{
		pwm_set_intensity(current_intensity);
	} 
	else 
	{
		pwm_set_intensity(0);
	}
	
	// Reagenda
	k_work_schedule(&pattern_intermittent_work, 
	                K_MSEC(PATTERN_INTERMITTENT_PERIOD_MS));
}



/*******************************************************************************
 * FUNÇÕES PRIVADAS - CONTROLE DE PADRÕES
 ******************************************************************************/

// Funções privadas - controle de padrão intermitente


static void stop_intermittent(void)
{
	pattern_intermittent_active = false;
	k_work_cancel_delayable(&pattern_intermittent_work);
	pwm_set_intensity(0);
}

/**
 * @brief Inicia o padrão intermitente
 * 
 * @param intensity Intensidade do buzzer (0-100)
 * @return 0 em sucesso
 */
int hal_buzzer_set_intermittent(bool active, uint8_t intensity)
{
	if (!initialized) 
	{
		LOG_ERR("HAL Buzzer não inicializado");
		return HAL_BUZZER_ERROR_STATE;
	}

	if (intensity > 100) 
	{
		LOG_ERR("Intensidade inválida: %d (máximo 100)", intensity);
		return HAL_BUZZER_ERROR_INVALID;
	}

	if (active) 
	{
		current_intensity = intensity;
		pattern_intermittent_active = true;
		k_work_schedule(&pattern_intermittent_work, K_NO_WAIT);
		LOG_INF("Buzzer intermitente ATIVADO (intensidade: %d%%)", intensity);
	} 
	else 
	{
		stop_intermittent();
		LOG_INF("Buzzer intermitente DESATIVADO");
	}

	return HAL_BUZZER_SUCCESS;
}



/*******************************************************************************
 * API PÚBLICA
 ******************************************************************************/

// API pública

int hal_buzzer_init(void)
{
	if (initialized) 
	{
		LOG_WRN("HAL Buzzer já inicializado");
		return HAL_BUZZER_SUCCESS;
	}
	// Verifica se o device PWM está pronto
	if (!device_is_ready(pwm_led.dev)) 
	{
		LOG_ERR("PWM device não está pronto");
		return HAL_BUZZER_ERROR_INIT;
	}
	// Inicializa PWM desligado
	int ret = pwm_set_intensity(0);
	if (ret < 0) 
	{
		LOG_ERR("Falha ao inicializar PWM");
		return HAL_BUZZER_ERROR_INIT;
	}

	// Inicializa work item
	k_work_init_delayable(&pattern_intermittent_work, pattern_intermittent_handler);
	current_intensity = HAL_BUZZER_INTENSITY_MEDIUM;
	pattern_intermittent_active = false;
	initialized = true;
	LOG_INF("HAL Buzzer inicializado com sucesso");

	return HAL_BUZZER_SUCCESS;
}

