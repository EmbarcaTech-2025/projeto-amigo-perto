/*
 * HAL Battery - Hardware Abstraction Layer para monitoramento de bateria
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Interface simplificada para leitura do percentual de carga da bateria LiPo.
 * Usa o canal interno SAADC_VDD do nRF52840 para medir a tensão de alimentação.
 */

#ifndef HAL_BATTERY_H_
#define HAL_BATTERY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// Códigos de retorno das funções HAL Battery
typedef enum {
	HAL_BATTERY_SUCCESS = 0,     // Operação bem-sucedida
	HAL_BATTERY_ERROR_INIT = -1, // Erro na inicialização do ADC
	HAL_BATTERY_ERROR_READ = -2, // Erro ao ler ADC
} hal_battery_error_t;

// === API PÚBLICA ===

/**
 * Inicializa o módulo de monitoramento de bateria
 * 
 * Configura o ADC do nRF52840 para ler o canal interno VDD (tensão de alimentação).
 * Esta função deve ser chamada antes de usar hal_battery_get_percentage().
 * 
 * Retorna:
 *   HAL_BATTERY_SUCCESS: Inicialização bem-sucedida
 *   HAL_BATTERY_ERROR_INIT: Falha ao configurar ADC
 * 
 * Exemplo:
 *   if (hal_battery_init() == HAL_BATTERY_SUCCESS) {
 *       uint8_t level = hal_battery_get_percentage();
 *   }
 */
int hal_battery_init(void);

/**
 * Lê o percentual de carga da bateria LiPo
 * 
 * Realiza leitura do ADC com oversampling (múltiplas leituras para maior precisão),
 * converte a tensão medida em percentual de carga baseado na curva de descarga
 * da bateria LiPo 1S usando interpolação linear em 11 pontos.
 * 
 * Retorna:
 *   0-100: Percentual de carga da bateria
 *   0: Se não inicializado ou erro na leitura
 * 
 * Curva de conversão (11 pontos):
 *   - 4.20V = 100% | 4.11V = 90% | 4.02V = 80% | 3.93V = 70%
 *   - 3.84V = 60%  | 3.75V = 50% | 3.66V = 40% | 3.57V = 30%
 *   - 3.48V = 20%  | 3.39V = 10% | 3.30V = 0%
 * 
 * Nota: A leitura leva ~20ms devido ao oversampling e estabilização do circuito.
 */
uint8_t hal_battery_get_percentage(void);

/**
 * Lê a tensão da bateria em milivolts
 * 
 * Realiza leitura do ADC com oversampling e retorna a tensão bruta em mV.
 * Útil para depuração ou quando é necessário o valor exato de tensão.
 * 
 * Parâmetros:
 *   battery_millivolt: Ponteiro para armazenar a tensão em mV
 * 
 * Retorna:
 *   0: Sucesso
 *   Negativo: Código de erro (ver hal_battery_error_t)
 * 
 * Exemplo:
 *   uint16_t voltage_mv;
 *   if (hal_battery_get_millivolt(&voltage_mv) == 0) {
 *       printk("Bateria: %d mV\n", voltage_mv);
 *   }
 */
int hal_battery_get_millivolt(uint16_t *battery_millivolt);

#ifdef __cplusplus
}
#endif

#endif /* HAL_BATTERY_H_ */
