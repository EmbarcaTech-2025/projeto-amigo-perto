/*
 * HAL Buzzer - Interface para controle de buzzer/LED via PWM
 *
 * Fornece funções para inicializar e controlar o buzzer piezo passivo.
 * Otimizado para cachorros (18 kHz) com baixo consumo energético (duty cycle 20-40%).
 * Padrão burst: 20ms ON / 80ms OFF (reduz consumo em ~75%).
 */

#ifndef HAL_BUZZER_H_
#define HAL_BUZZER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Códigos de retorno das funções do HAL Buzzer
 */
typedef enum {
	HAL_BUZZER_SUCCESS = 0,        // Operação bem-sucedida
	HAL_BUZZER_ERROR_INIT = -1,    // Erro na inicialização
	HAL_BUZZER_ERROR_INVALID = -2, // Parâmetro inválido
	HAL_BUZZER_ERROR_STATE = -3,   // Estado inválido
} hal_buzzer_error_t;

/**
 * @brief Níveis de intensidade (duty cycle) para o buzzer
 * 
 * Para buzzer piezo a 18 kHz:
 * - Valores 20-40: Baixo consumo, audível para cachorros, quase inaudível para humanos
 * - Valores 50-75: Médio consumo, mais audível
 * - Valor 100: Alto consumo, máxima intensidade
 */
typedef enum {
	HAL_BUZZER_INTENSITY_OFF = 0,    // Desligado
	HAL_BUZZER_INTENSITY_LOW = 20,   // Baixa intensidade (recomendado, baixo consumo)
	HAL_BUZZER_INTENSITY_MEDIUM = 30,// Média intensidade (balanceado)
	HAL_BUZZER_INTENSITY_HIGH = 40,  // Alta intensidade (ainda econômico)
	HAL_BUZZER_INTENSITY_MAX = 100,  // Máxima intensidade (alto consumo)
} hal_buzzer_intensity_t;

/**
 * @brief Inicializa o subsistema de buzzer
 * 
 * Configura PWM a 18 kHz (ideal para cachorros, quase inaudível para humanos).
 * Duty cycle padrão: 20% (baixo consumo).
 * Deve ser chamada antes de qualquer outra função do HAL Buzzer.
 * 
 * @return HAL_BUZZER_SUCCESS ou erro
 */
int hal_buzzer_init(void);

/**
 * @brief Ativa/desativa padrão burst intermitente (alarme sonoro)
 * 
 * Padrão burst: 20ms ligado / 80ms desligado (reduz consumo em ~75%).
 * Frequência: 18 kHz (audível para cachorros, quase inaudível para humanos).
 * 
 * @param active true para ativar, false para desativar
 * @param intensity Duty cycle desejado (0-100)
 *                  Recomendado: 20-40 para baixo consumo
 *                  Use HAL_BUZZER_INTENSITY_LOW/MEDIUM/HIGH ou valor customizado
 * @return HAL_BUZZER_SUCCESS ou erro
 */
int hal_buzzer_set_intermittent(bool active, uint8_t intensity);

#ifdef __cplusplus
}
#endif

#endif /* HAL_BUZZER_H_ */
