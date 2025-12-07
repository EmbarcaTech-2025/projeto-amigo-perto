/*
 * Energy Consumption Test Module
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Módulo de teste para medição de consumo energético usando resistor shunt.
 * Este arquivo pode ser removido após os testes sem afetar o código principal.
 */

#ifndef ENERGY_TEST_H_
#define ENERGY_TEST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Códigos de erro do módulo de teste de energia
 */
typedef enum {
	ENERGY_TEST_SUCCESS = 0,
	ENERGY_TEST_ERROR_INIT = -1,
	ENERGY_TEST_ERROR_READ = -2,
} energy_test_error_t;

/**
 * @brief Estrutura com resultados da medição
 */
typedef struct {
	float voltage_shunt_mv;     // Tensão no shunt (mV)
	float current_ma;           // Corrente instantânea (mA)
	float energy_mwh;           // Energia consumida (mWh)
	uint32_t duration_ms;       // Duração da medição (ms)
	float avg_power_mw;         // Potência média (mW)
} energy_test_results_t;

/**
 * @brief Inicializa o módulo de teste de energia
 * 
 * Configura o ADC em modo diferencial para ler a tensão no shunt.
 * 
 * @return ENERGY_TEST_SUCCESS em sucesso, código de erro caso contrário
 */
int energy_test_init(void);

/**
 * @brief Inicia uma nova medição de energia
 * 
 * Reseta contadores e inicia integração temporal.
 */
void energy_test_start(void);

/**
 * @brief Para a medição e calcula resultados finais
 * 
 * @param results Ponteiro para estrutura onde serão armazenados os resultados
 * @return ENERGY_TEST_SUCCESS em sucesso, código de erro caso contrário
 */
int energy_test_stop(energy_test_results_t *results);

/**
 * @brief Obtém medição instantânea sem parar o teste
 * 
 * @param current_ma Ponteiro para armazenar corrente instantânea (mA)
 * @param power_mw Ponteiro para armazenar potência instantânea (mW)
 * @return ENERGY_TEST_SUCCESS em sucesso, código de erro caso contrário
 */
int energy_test_get_instant(float *current_ma, float *power_mw);

/**
 * @brief Executa teste completo sem buzzer (1 minuto)
 * 
 * Mede consumo base do sistema por 60 segundos.
 * 
 * @param results Ponteiro para armazenar resultados
 * @return ENERGY_TEST_SUCCESS em sucesso, código de erro caso contrário
 */
int energy_test_run_baseline(energy_test_results_t *results);

/**
 * @brief Executa teste completo com buzzer (1 minuto)
 * 
 * Liga o buzzer e mede consumo por 60 segundos.
 * 
 * @param results Ponteiro para armazenar resultados
 * @return ENERGY_TEST_SUCCESS em sucesso, código de erro caso contrário
 */
int energy_test_run_with_buzzer(energy_test_results_t *results);

#ifdef __cplusplus
}
#endif

#endif /* ENERGY_TEST_H_ */
