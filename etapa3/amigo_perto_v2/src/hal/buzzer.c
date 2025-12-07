/*
 * HAL Buzzer - Controle de buzzer piezo via PWM
 * 
 * Frequência: 18 kHz (audível para cães, quase inaudível para humanos)
 * Padrão burst: 20ms ON / 80ms OFF (economia de ~75% energia)
 * Duty cycle: 20-40% recomendado para baixo consumo
 *
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "hal/buzzer.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>

LOG_MODULE_REGISTER(hal_buzzer, LOG_LEVEL_DBG);

// === CONFIGURAÇÃO HARDWARE ===
#define PWM_LED0_NODE DT_ALIAS(pwm_led0)
#if !DT_NODE_HAS_STATUS(PWM_LED0_NODE, okay)
#error "pwm-led0 devicetree alias não está definido"
#endif

// === PARÂMETROS PWM E BURST ===
#define PWM_PERIOD_NS    55555U  // 18 kHz (~55.5µs)
#define BURST_ON_MS      20      // Burst: 20ms ligado
#define BURST_OFF_MS     80      // Burst: 80ms desligado (economia 75%)

// === ESTADO INTERNO ===
static const struct pwm_dt_spec pwm_led = PWM_DT_SPEC_GET(PWM_LED0_NODE);
static bool initialized = false;
static uint8_t current_intensity = HAL_BUZZER_INTENSITY_LOW;  // Duty cycle padrão: 20%
static struct k_work_delayable intermittent_work;
static bool intermittent_active = false;

// === FUNÇÕES AUXILIARES ===

// Converte duty cycle (0-100%) em pulso PWM (nanosegundos)
static inline uint32_t intensity_to_pulse_ns(uint8_t intensity) 
{
	// Limita intensidade ao máximo de 100%
	if (intensity > HAL_BUZZER_INTENSITY_MAX) intensity = HAL_BUZZER_INTENSITY_MAX;
	
	// Calcula largura do pulso proporcional ao período
	return (PWM_PERIOD_NS * intensity) / 100;
}

// Aplica duty cycle ao PWM (0 = desligado)
static int set_pwm(uint8_t intensity) 
{
	// Configura o PWM com o período e pulso calculado
	int ret = pwm_set_dt(&pwm_led, PWM_PERIOD_NS, intensity_to_pulse_ns(intensity));
	// Registra erro se a configuração falhar
	if (ret < 0) LOG_ERR("Erro ao configurar PWM: %d", ret);

	return ret;
}

// === HANDLER BURST INTERMITENTE ===
// Alterna 20ms ON / 80ms OFF para economia de energia
static void intermittent_handler(struct k_work *work) 
{
	// Mantém o estado atual do burst (ligado/desligado)
	static bool state = false;
	// Se o modo intermitente foi desativado, desliga e retorna
	if (!intermittent_active) 
	{
		set_pwm(HAL_BUZZER_INTENSITY_OFF);
		return;
	}

	// Alterna entre ligado e desligado
	state = !state;
	// Aplica intensidade quando ligado, 0 quando desligado
	set_pwm(state ? current_intensity : HAL_BUZZER_INTENSITY_OFF);
	// Agenda próxima alternância (20ms ON ou 80ms OFF)
	k_work_schedule(&intermittent_work, K_MSEC(state ? BURST_ON_MS : BURST_OFF_MS));
}



// Interrompe burst e desliga buzzer
static void stop_intermittent(void) 
{
	// Marca o modo intermitente como inativo
	intermittent_active = false;
	// Cancela qualquer trabalho agendado
	k_work_cancel_delayable(&intermittent_work);
	// Desliga o PWM
	set_pwm(HAL_BUZZER_INTENSITY_OFF);
}

// === API PÚBLICA ===

/**
 * Ativa/desativa padrão burst: 20ms ON / 80ms OFF
 * @param active true=ativar, false=desativar
 * @param intensity Duty cycle 0-100 (recomendado: 20-40)
 */
int hal_buzzer_set_intermittent(bool active, uint8_t intensity) 
{
	// Verifica se o HAL foi inicializado
	if (!initialized) 
	{
		LOG_ERR("HAL Buzzer não inicializado");
		return HAL_BUZZER_ERROR_STATE;
	}

	// Valida o parâmetro de intensidade
	if (intensity > HAL_BUZZER_INTENSITY_MAX) 
	{
		LOG_ERR("Intensidade inválida: %d", intensity);
		return HAL_BUZZER_ERROR_INVALID;
	}

	// Ativa ou desativa o modo intermitente
	if (active) 
	{
		// Armazena a intensidade configurada
		current_intensity = intensity;
		// Marca o modo intermitente como ativo
		intermittent_active = true;
		// Inicia o trabalho de burst imediatamente
		k_work_schedule(&intermittent_work, K_NO_WAIT);
		LOG_INF("Buzzer ATIVADO (%d%%)", intensity);
	} 
	else 
	{
		// Para o modo intermitente e desliga o buzzer
		stop_intermittent();
		LOG_INF("Buzzer DESATIVADO");
	}

	return HAL_BUZZER_SUCCESS;
}


/**
 * Inicializa buzzer: 18 kHz, duty cycle 20%, burst 20ms/80ms
 */
int hal_buzzer_init(void) 
{
	// Evita reinicialização
	if (initialized) 
	{
		LOG_WRN("HAL Buzzer já inicializado");
		return HAL_BUZZER_SUCCESS;
	}
	// Verifica se o dispositivo PWM está pronto
	if (!device_is_ready(pwm_led.dev)) 
	{
		LOG_ERR("PWM device não está pronto");
		return HAL_BUZZER_ERROR_INIT;
	}
	// Inicializa o PWM desligado
	if (set_pwm(HAL_BUZZER_INTENSITY_OFF) < 0) 
	{
		LOG_ERR("Falha ao inicializar PWM");
		return HAL_BUZZER_ERROR_INIT;
	}

	// Inicializa o trabalho delayable para o modo intermitente
	k_work_init_delayable(&intermittent_work, intermittent_handler);
	// Define intensidade padrão
	current_intensity = HAL_BUZZER_INTENSITY_LOW;
	// Modo intermitente começa desativado
	intermittent_active = false;
	// Marca como inicializado
	initialized = true;
	LOG_INF("HAL Buzzer OK: 18kHz, 20%%, burst 20/80ms");

	return HAL_BUZZER_SUCCESS;
}

