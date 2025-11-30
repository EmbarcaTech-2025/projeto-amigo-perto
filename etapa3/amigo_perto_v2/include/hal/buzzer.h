/*
 * HAL Buzzer - Hardware Abstraction Layer para controle de Buzzer/LED via PWM
 */

/**
 * @file buzzer.h
 * @brief Interface HAL para controle de buzzer através de PWM
 * 
 * Este módulo encapsula a lógica de controle do buzzer (LED simulando buzzer)
 * via PWM, fornecendo uma API simples e independente da implementação específica
 * do driver do Zephyr.
 * 
 * Funcionalidades:
 * - Inicialização do subsistema de buzzer
 * - Controle intermitente liga/desliga com diferentes intensidades
 */

#ifndef HAL_BUZZER_H_
#define HAL_BUZZER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Códigos de erro do HAL Buzzer
 */
typedef enum {
	HAL_BUZZER_SUCCESS = 0,          /**< Operação bem-sucedida */
	HAL_BUZZER_ERROR_INIT = -1,      /**< Erro na inicialização */
	HAL_BUZZER_ERROR_INVALID = -2,   /**< Parâmetro inválido */
	HAL_BUZZER_ERROR_STATE = -3,     /**< Estado inválido */
} hal_buzzer_error_t;

/**
 * @brief Níveis de intensidade do buzzer
 */
typedef enum {
	HAL_BUZZER_INTENSITY_OFF = 0,    /**< Desligado (0%) */
	HAL_BUZZER_INTENSITY_LOW = 25,   /**< Baixa intensidade (25%) */
	HAL_BUZZER_INTENSITY_MEDIUM = 50,/**< Média intensidade (50%) */
	HAL_BUZZER_INTENSITY_HIGH = 75,  /**< Alta intensidade (75%) */
	HAL_BUZZER_INTENSITY_MAX = 100,  /**< Máxima intensidade (100%) */
} hal_buzzer_intensity_t;

/**
 * @brief Inicializa o subsistema de buzzer
 * 
 * Configura o hardware PWM, inicializa os timers e prepara o buzzer
 * para operação. Deve ser chamada antes de qualquer outra função do HAL.
 * 
 * @return HAL_BUZZER_SUCCESS em caso de sucesso
 * @return HAL_BUZZER_ERROR_INIT se houver erro na inicialização
 */
int hal_buzzer_init(void);

/**
 * @brief Liga/desliga o buzzer intermitente
 * 
 * Ativa ou desativa o padrão intermitente (500ms on/off) na intensidade fornecida.
 * 
 * @param active true para ativar, false para desativar
 * @param intensity Intensidade do buzzer (0-100)
 * 
 * @return HAL_BUZZER_SUCCESS em caso de sucesso
 * @return HAL_BUZZER_ERROR_INVALID se intensidade > 100
 * @return HAL_BUZZER_ERROR_STATE se buzzer não foi inicializado
 */
int hal_buzzer_set_intermittent(bool active, uint8_t intensity);


#ifdef __cplusplus
}
#endif

#endif /* HAL_BUZZER_H_ */
