/*
 * HAL BLE - Hardware Abstraction Layer para Bluetooth Low Energy
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file ble.h
 * @brief Interface HAL para funcionalidades Bluetooth Low Energy
 * 
 * Este módulo encapsula o stack Bluetooth do Zephyr, fornecendo uma API
 * simplificada para inicialização, advertising, conexão e leitura de RSSI.
 * 
 * Funcionalidades:
 * - Inicialização do stack BLE
 * - Controle de advertising (anúncio)
 * - Gerenciamento de conexões
 * - Leitura de RSSI
 * - Callbacks para eventos BLE
 */

#ifndef HAL_BLE_H_
#define HAL_BLE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * DEFINIÇÕES E TIPOS
 ******************************************************************************/

/**
 * @brief Códigos de erro do HAL BLE
 */
typedef enum {
	HAL_BLE_SUCCESS = 0,              /**< Operação bem-sucedida */
	HAL_BLE_ERROR_INIT = -1,          /**< Erro na inicialização */
	HAL_BLE_ERROR_INVALID = -2,       /**< Parâmetro inválido */
	HAL_BLE_ERROR_STATE = -3,         /**< Estado inválido */
	HAL_BLE_ERROR_NOT_CONNECTED = -4, /**< Não há conexão ativa */
	HAL_BLE_ERROR_FAILED = -5,        /**< Operação falhou */
} hal_ble_error_t;

/**
 * @brief Estados da conexão BLE
 */
typedef enum {
	HAL_BLE_STATE_IDLE = 0,           /**< Ocioso (não inicializado) */
	HAL_BLE_STATE_READY,              /**< Pronto mas não anunciando */
	HAL_BLE_STATE_ADVERTISING,        /**< Anunciando (advertising) */
	HAL_BLE_STATE_CONNECTED,          /**< Conectado a um dispositivo */
} hal_ble_state_t;

/**
 * @brief Parâmetros de advertising
 */
typedef struct {
	uint16_t interval_min_ms;         /**< Intervalo mínimo em ms (20-10240) */
	uint16_t interval_max_ms;         /**< Intervalo máximo em ms (20-10240) */
	bool connectable;                 /**< Permite conexões */
	bool use_identity;                /**< Usa endereço de identidade */
} hal_ble_adv_params_t;

/**
 * @brief Informações de conexão
 */
typedef struct {
	uint16_t interval_ms;             /**< Intervalo de conexão em ms */
	uint16_t latency;                 /**< Latência de conexão (eventos) */
	uint16_t timeout_ms;              /**< Timeout de supervisão em ms */
} hal_ble_conn_info_t;

/*******************************************************************************
 * CALLBACKS
 ******************************************************************************/

/**
 * @brief Callback chamado quando um dispositivo se conecta
 * 
 * @param conn_info Informações sobre a conexão estabelecida
 */
typedef void (*hal_ble_connected_cb_t)(const hal_ble_conn_info_t *conn_info);

/**
 * @brief Callback chamado quando um dispositivo se desconecta
 * 
 * @param reason Código do motivo da desconexão
 */
typedef void (*hal_ble_disconnected_cb_t)(uint8_t reason);

/**
 * @brief Callback chamado quando o advertising é iniciado
 */
typedef void (*hal_ble_adv_started_cb_t)(void);

/**
 * @brief Callback chamado quando o advertising é parado
 */
typedef void (*hal_ble_adv_stopped_cb_t)(void);

/**
 * @brief Estrutura de callbacks BLE
 * 
 * Registre estas funções para receber notificações de eventos BLE
 */
typedef struct {
	hal_ble_connected_cb_t connected;       /**< Callback de conexão */
	hal_ble_disconnected_cb_t disconnected; /**< Callback de desconexão */
	hal_ble_adv_started_cb_t adv_started;   /**< Callback advertising iniciado */
	hal_ble_adv_stopped_cb_t adv_stopped;   /**< Callback advertising parado */
} hal_ble_callbacks_t;

/*******************************************************************************
 * API PÚBLICA
 ******************************************************************************/

/**
 * @brief Inicializa o subsistema BLE
 * 
 * Inicializa o stack Bluetooth, configura os serviços GATT e prepara
 * o sistema para advertising e conexões.
 * 
 * @param device_name Nome do dispositivo para advertising (máx 29 caracteres)
 * @param callbacks Estrutura de callbacks para eventos BLE (pode ser NULL)
 * 
 * @return HAL_BLE_SUCCESS em caso de sucesso
 * @return HAL_BLE_ERROR_INIT se houver erro na inicialização
 * @return HAL_BLE_ERROR_INVALID se device_name for NULL ou muito longo
 */
int hal_ble_init(const char *device_name, const hal_ble_callbacks_t *callbacks);

/**
 * @brief Inicia o advertising (anúncio Bluetooth)
 * 
 * Torna o dispositivo visível e conectável para outros dispositivos BLE.
 * Use parâmetros padrão se adv_params for NULL.
 * 
 * @param adv_params Parâmetros de advertising (NULL usa valores padrão)
 * 
 * @return HAL_BLE_SUCCESS em caso de sucesso
 * @return HAL_BLE_ERROR_STATE se BLE não foi inicializado
 * @return HAL_BLE_ERROR_FAILED se falhar ao iniciar advertising
 */
int hal_ble_start_advertising(const hal_ble_adv_params_t *adv_params);

/**
 * @brief Para o advertising
 * 
 * Para de anunciar o dispositivo. Não afeta conexões já estabelecidas.
 * 
 * @return HAL_BLE_SUCCESS em caso de sucesso
 * @return HAL_BLE_ERROR_STATE se advertising não estava ativo
 */
int hal_ble_stop_advertising(void);

/**
 * @brief Desconecta do dispositivo conectado
 * 
 * Encerra a conexão BLE atual e retorna ao modo pronto.
 * 
 * @return HAL_BLE_SUCCESS em caso de sucesso
 * @return HAL_BLE_ERROR_NOT_CONNECTED se não há conexão ativa
 */
int hal_ble_disconnect(void);

/**
 * @brief Retorna o estado atual do BLE
 * 
 * @return Estado atual do subsistema BLE
 */
hal_ble_state_t hal_ble_get_state(void);

/**
 * @brief Verifica se há uma conexão ativa
 * 
 * @return true se conectado a um dispositivo
 * @return false se não há conexão
 */
bool hal_ble_is_connected(void);


#ifdef __cplusplus
}
#endif

#endif /* HAL_BLE_H_ */
