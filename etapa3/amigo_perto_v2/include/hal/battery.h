/*
 * HAL Battery - Hardware Abstraction Layer para monitoramento de bateria
 */

/**
 * @file battery.h
 * @brief Interface HAL para leitura e monitoramento de bateria via ADC
 * 
 * Este módulo encapsula a lógica de leitura da tensão da bateria através do ADC,
 * gerenciamento de estados de carga e fornecimento de informações de nível de bateria
 * para a aplicação.
 * 
 * Funcionalidades:
 * - Inicialização do subsistema de monitoramento de bateria
 * - Leitura da tensão da bateria via ADC
 * - Cálculo do percentual de carga
 * - Gerenciamento de estados de carga (Critical, Low, Medium, Good)
 * - Otimizado para bateria tipo moeda (CR2032: 3.0V nominal, 2.0V mínimo)
 */

#ifndef HAL_BATTERY_H_
#define HAL_BATTERY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Códigos de erro do HAL Battery
 */
typedef enum {
	HAL_BATTERY_SUCCESS = 0,          /**< Operação bem-sucedida */
	HAL_BATTERY_ERROR_INIT = -1,      /**< Erro na inicialização */
	HAL_BATTERY_ERROR_READ = -2,      /**< Erro na leitura do ADC */
	HAL_BATTERY_ERROR_STATE = -3,     /**< Estado inválido (não inicializado) */
} hal_battery_error_t;

/**
 * @brief Estados de carga da bateria
 * 
 * Baseado em CR2032:
 * - 3.0V: 100% (tensão nominal)
 * - 2.8V: ~75% (good)
 * - 2.5V: ~25% (low)
 * - 2.0V: ~5% (critical)
 * - <2.0V: bateria esgotada
 */
typedef enum {
	HAL_BATTERY_STATE_CRITICAL = 0,   /**< Crítico: < 10% (< 2.2V) */
	HAL_BATTERY_STATE_LOW = 1,        /**< Baixo: 10-30% (2.2V - 2.5V) */
	HAL_BATTERY_STATE_MEDIUM = 2,     /**< Médio: 30-70% (2.5V - 2.8V) */
	HAL_BATTERY_STATE_GOOD = 3,       /**< Bom: > 70% (> 2.8V) */
	HAL_BATTERY_STATE_UNKNOWN = 4,    /**< Desconhecido (não inicializado ou erro) */
} hal_battery_state_t;

/**
 * @brief Estrutura com informações da bateria
 */
typedef struct {
	uint16_t voltage_mv;              /**< Tensão em milivolts */
	uint8_t percentage;               /**< Percentual de carga (0-100%) */
	hal_battery_state_t state;        /**< Estado de carga */
} hal_battery_info_t;

/**
 * @brief Inicializa o subsistema de monitoramento de bateria
 * 
 * Configura o ADC para leitura da tensão de bateria e prepara o módulo
 * para operação. Deve ser chamada antes de qualquer outra função do HAL.
 * 
 * @return HAL_BATTERY_SUCCESS em caso de sucesso
 * @return HAL_BATTERY_ERROR_INIT se houver erro na inicialização
 */
int hal_battery_init(void);

/**
 * @brief Lê a tensão atual da bateria
 * 
 * Realiza uma leitura do ADC e retorna a tensão em milivolts.
 * Esta função realiza múltiplas leituras e calcula a média para maior precisão.
 * 
 * @param voltage_mv Ponteiro para armazenar a tensão lida (em mV)
 * 
 * @return HAL_BATTERY_SUCCESS em caso de sucesso
 * @return HAL_BATTERY_ERROR_STATE se não inicializado
 * @return HAL_BATTERY_ERROR_READ se houver erro na leitura
 */
int hal_battery_read_voltage(uint16_t *voltage_mv);

/**
 * @brief Calcula o percentual de carga da bateria
 * 
 * Converte a tensão em mV para percentual de carga estimado (0-100%).
 * Utiliza interpolação linear baseada nas características de descarga
 * de bateria tipo moeda (CR2032).
 * 
 * @param voltage_mv Tensão da bateria em milivolts
 * 
 * @return Percentual de carga (0-100%)
 */
uint8_t hal_battery_voltage_to_percentage(uint16_t voltage_mv);

/**
 * @brief Determina o estado de carga da bateria
 * 
 * Converte o percentual de carga em estado categórico.
 * 
 * @param percentage Percentual de carga (0-100%)
 * 
 * @return Estado de carga da bateria
 */
hal_battery_state_t hal_battery_percentage_to_state(uint8_t percentage);

/**
 * @brief Obtém informações completas da bateria
 * 
 * Lê a tensão da bateria e calcula todas as informações derivadas
 * (percentual e estado) em uma única operação.
 * 
 * @param info Ponteiro para estrutura onde as informações serão armazenadas
 * 
 * @return HAL_BATTERY_SUCCESS em caso de sucesso
 * @return HAL_BATTERY_ERROR_STATE se não inicializado
 * @return HAL_BATTERY_ERROR_READ se houver erro na leitura
 */
int hal_battery_get_info(hal_battery_info_t *info);

/**
 * @brief Verifica se a bateria está em nível crítico
 * 
 * Função auxiliar para verificação rápida de bateria crítica.
 * 
 * @return true se bateria está crítica (< 10%)
 * @return false caso contrário ou se não inicializado
 */
bool hal_battery_is_critical(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_BATTERY_H_ */
