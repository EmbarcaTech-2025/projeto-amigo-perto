/*
 * HAL BLE - Hardware Abstraction Layer para Bluetooth Low Energy
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Este módulo encapsula o stack Bluetooth do Zephyr, fornecendo uma API
 * simplificada para inicialização, advertising e gerenciamento de conexões.
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

// Códigos de retorno das funções HAL BLE
typedef enum {
	HAL_BLE_SUCCESS = 0,         // Operação bem-sucedida
	HAL_BLE_ERROR_INIT = -1,     // Erro na inicialização
	HAL_BLE_ERROR_INVALID = -2,  // Parâmetro inválido
	HAL_BLE_ERROR_STATE = -3,    // Estado inválido
	HAL_BLE_ERROR_FAILED = -4,   // Operação falhou
} hal_ble_error_t;

// Parâmetros de configuração do advertising BLE
// Define como o dispositivo será anunciado para outros dispositivos
typedef struct {
	uint16_t interval_min_ms;  // Intervalo mínimo entre anúncios (20-10240 ms)
	uint16_t interval_max_ms;  // Intervalo máximo entre anúncios (20-10240 ms)
	bool connectable;          // true = aceita conexões, false = somente broadcasting
	bool use_identity;         // true = usa endereço MAC fixo, false = endereço aleatório
} hal_ble_adv_params_t;

// Informações sobre uma conexão BLE estabelecida
// Contém parâmetros negociados entre dispositivos durante a conexão
typedef struct {
	uint16_t interval_ms;  // Intervalo entre eventos de conexão (tempo entre trocas de dados)
	uint16_t latency;      // Número de eventos que o periférico pode pular (economia de energia)
	uint16_t timeout_ms;   // Tempo máximo sem comunicação antes de considerar conexão perdida
} hal_ble_conn_info_t;

// === CALLBACKS: Funções chamadas automaticamente quando eventos BLE ocorrem ===

// Chamado quando um dispositivo central (ex: smartphone) conecta ao nosso periférico
// conn_info: detalhes da conexão estabelecida (intervalo, latência, timeout)
typedef void (*hal_ble_connected_cb_t)(const hal_ble_conn_info_t *conn_info);

// Chamado quando a conexão BLE é encerrada (voluntariamente ou por timeout/erro)
// reason: código HCI indicando motivo da desconexão (ex: 0x13 = usuário encerrou)
typedef void (*hal_ble_disconnected_cb_t)(uint8_t reason);

// Chamado quando o advertising é iniciado com sucesso
typedef void (*hal_ble_adv_started_cb_t)(void);

// Chamado quando o advertising é parado (manualmente ou por conexão estabelecida)
typedef void (*hal_ble_adv_stopped_cb_t)(void);

// Estrutura que agrupa todos os callbacks BLE
// A aplicação fornece esta estrutura em hal_ble_init() para receber notificações
typedef struct {
	hal_ble_connected_cb_t connected;       // Notificação de nova conexão
	hal_ble_disconnected_cb_t disconnected; // Notificação de desconexão
	hal_ble_adv_started_cb_t adv_started;   // Notificação de advertising iniciado
	hal_ble_adv_stopped_cb_t adv_stopped;   // Notificação de advertising parado
} hal_ble_callbacks_t;

/*******************************************************************************
 * API PÚBLICA
 ******************************************************************************/

/**
 * Inicializa o subsistema BLE
 * 
 * Esta função deve ser chamada ANTES de qualquer outra operação BLE.
 * Ela inicializa o stack Bluetooth do Zephyr, registra callbacks de conexão
 * e prepara os dados de advertising.
 * 
 * Parâmetros:
 *   device_name: Nome que aparecerá para outros dispositivos (máx 29 caracteres)
 *   callbacks: Ponteiro para estrutura com funções callback (pode ser NULL)
 * 
 * Retorna:
 *   HAL_BLE_SUCCESS: Inicialização bem-sucedida
 *   HAL_BLE_ERROR_INIT: Falha ao habilitar Bluetooth
 *   HAL_BLE_ERROR_INVALID: device_name NULL ou muito longo
 * 
 * Exemplo:
 *   hal_ble_callbacks_t callbacks = { .connected = on_connect, ... };
 *   hal_ble_init("MeuDispositivo", &callbacks);
 */
int hal_ble_init(const char *device_name, const hal_ble_callbacks_t *callbacks);

/**
 * Inicia o advertising BLE (torna o dispositivo visível/conectável)
 * 
 * Após inicializar o BLE, chame esta função para começar a anunciar o dispositivo.
 * O advertising permite que outros dispositivos (smartphones, tablets) descubram
 * e conectem-se ao nosso periférico.
 * 
 * Parâmetros:
 *   adv_params: Configurações de advertising ou NULL para usar padrões (500ms)
 * 
 * Retorna:
 *   HAL_BLE_SUCCESS: Advertising iniciado com sucesso
 *   HAL_BLE_ERROR_STATE: BLE não inicializado ou já conectado
 *   HAL_BLE_ERROR_INVALID: Parâmetros inválidos
 *   HAL_BLE_ERROR_FAILED: Falha ao iniciar advertising no stack
 * 
 * Nota: O advertising para automaticamente quando uma conexão é estabelecida.
 *       Use o callback adv_stopped para detectar quando isso ocorre.
 */
int hal_ble_start_advertising(const hal_ble_adv_params_t *adv_params);


#ifdef __cplusplus
}
#endif

#endif /* HAL_BLE_H_ */
