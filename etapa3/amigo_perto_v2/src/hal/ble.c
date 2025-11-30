/*
 * HAL BLE - Hardware Abstraction Layer para Bluetooth Low Energy
 * 
 * @file ble.c
 * @brief Implementação do HAL BLE
 * Localização: src/hal/ble.c
 * Header público: include/hal/ble.h
 * 
 * Este módulo implementa o controle do stack Bluetooth Low Energy,
 * encapsulando as APIs do Zephyr e fornecendo interface simplificada.
 * 
 * Funcionalidades:
 * - Inicialização e configuração do stack BLE
 * - Controle de advertising (start/stop, parâmetros customizados)
 * - Gerenciamento de conexões (callbacks de eventos)
 * - Atualização de parâmetros de conexão
 * - Encapsulamento das APIs Zephyr para facilitar uso
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "hal/ble.h"

// Zephyr includes
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

// Bluetooth includes
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>

// Serviço GATT customizado
#include "gatt/buzzer_service.h"

// Registra módulo de logging
LOG_MODULE_REGISTER(hal_ble, LOG_LEVEL_DBG);

/*******************************************************************************
 * CONFIGURAÇÕES E CONSTANTES
 ******************************************************************************/

// Valores padrão para advertising
#define DEFAULT_ADV_INTERVAL_MIN_MS     500   /**< 500ms */
#define DEFAULT_ADV_INTERVAL_MAX_MS     500   /**< 500ms */

// Limites dos parâmetros de advertising (conforme spec Bluetooth)
#define ADV_INTERVAL_MIN_MS             20
#define ADV_INTERVAL_MAX_MS             10240

// Tamanho máximo do nome do dispositivo
#define MAX_DEVICE_NAME_LEN             29

// Conversão de milissegundos para unidades BLE (0.625ms por unidade)
#define MS_TO_BLE_UNITS(ms)             ((ms) * 8 / 5)

/*******************************************************************************
 * VARIÁVEIS PRIVADAS
 ******************************************************************************/

// Estado do módulo
static bool initialized = false;
static hal_ble_state_t current_state = HAL_BLE_STATE_IDLE;
static hal_ble_callbacks_t user_callbacks = {0};

// Nome do dispositivo
static char device_name[MAX_DEVICE_NAME_LEN + 1] = {0};

// Conexão atual
static struct bt_conn *current_conn = NULL;

// Work item para iniciar advertising de forma assíncrona
static struct k_work adv_work;

// Dados de advertising
static struct bt_data ad_data[2];
static struct bt_data sd_data[1];
static size_t ad_data_count = 0;
static size_t sd_data_count = 0;

// Parâmetros de advertising
static struct bt_le_adv_param adv_param_storage;
static const struct bt_le_adv_param *adv_param = NULL;

/*******************************************************************************
 * FUNÇÕES PRIVADAS - CALLBACKS DO STACK BLUETOOTH
 ******************************************************************************/

/**
 * @brief Callback chamado quando uma conexão é estabelecida
 */
static void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) 
	{
		LOG_ERR("Conexão falhou (err %u)", err);
		
		// Reinicia advertising
		k_work_submit(&adv_work);
		return;
	}
	
	// Armazena referência da conexão
	if (current_conn) 
	{
		bt_conn_unref(current_conn);
	}
	current_conn = bt_conn_ref(conn);
	current_state = HAL_BLE_STATE_CONNECTED;
	
	// Lê informações da conexão
	struct bt_conn_info info;
	if (bt_conn_get_info(conn, &info) == 0) 
	{
		LOG_INF("Conectado - Intervalo: %u, Latência: %u, Timeout: %u", info.le.interval, info.le.latency, info.le.timeout);
	}
	
	// Notifica aplicação
	if (user_callbacks.connected) 
	{
		hal_ble_conn_info_t conn_info = {0};
		
		if (bt_conn_get_info(conn, &info) == 0) 
		{
			conn_info.interval_ms = info.le.interval * 1250 / 1000; // 1.25ms por unidade
			conn_info.latency = info.le.latency;
			conn_info.timeout_ms = info.le.timeout * 10; // 10ms por unidade
		}
		
		user_callbacks.connected(&conn_info);
	}
}

/**
 * @brief Callback chamado quando uma conexão é encerrada
 */
static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Desconectado (motivo %u)", reason);
	
	// Libera referência da conexão
	if (current_conn) 
	{
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}
	
	current_state = HAL_BLE_STATE_READY;
	
	// Notifica aplicação
	if (user_callbacks.disconnected) 
	{
		user_callbacks.disconnected(reason);
	}
}

/**
 * @brief Callback chamado quando a conexão é reciclada
 */
static void on_recycled(void)
{
	LOG_DBG("Conexão reciclada - reiniciando advertising");
	
	// Reinicia advertising
	k_work_submit(&adv_work);
}

// Estrutura de callbacks de conexão
static struct bt_conn_cb conn_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
	.recycled = on_recycled,
};

/*******************************************************************************
 * FUNÇÕES PRIVADAS - ADVERTISING
 ******************************************************************************/

/**
 * @brief Handler do work item para iniciar advertising
 */
static void adv_work_handler(struct k_work *work)
{
	if (current_state == HAL_BLE_STATE_CONNECTED) 
	{
		LOG_WRN("Já conectado, não inicia advertising");
		return;
	}
	
	// Usa parâmetros padrão se não foram configurados
	const struct bt_le_adv_param *param = adv_param;
	if (!param) 
	{
		param = BT_LE_ADV_PARAM(
			(BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_USE_IDENTITY),
			MS_TO_BLE_UNITS(DEFAULT_ADV_INTERVAL_MIN_MS),
			MS_TO_BLE_UNITS(DEFAULT_ADV_INTERVAL_MAX_MS),
			NULL
		);
	}
	
	// Inicia advertising
	int err = bt_le_adv_start(param, ad_data, ad_data_count, sd_data, sd_data_count);
	if (err) 
	{
		LOG_ERR("Advertising falhou (err %d)", err);
		return;
	}
	
	current_state = HAL_BLE_STATE_ADVERTISING;
	LOG_INF("Advertising iniciado");
	
	// Notifica aplicação
	if (user_callbacks.adv_started) 
	{
		user_callbacks.adv_started();
	}
}

/**
 * @brief Prepara os dados de advertising
 */
static void prepare_adv_data(void)
{
	size_t name_len = strlen(device_name);
	
	// Advertising data
	ad_data_count = 0;
	
	// Flags
	ad_data[ad_data_count].type = BT_DATA_FLAGS;
	ad_data[ad_data_count].data_len = 1;
	static const uint8_t flags = BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR;
	ad_data[ad_data_count].data = &flags;
	ad_data_count++;
	
	// Nome do dispositivo
	if (name_len > 0) 
	{
		ad_data[ad_data_count].type = BT_DATA_NAME_COMPLETE;
		ad_data[ad_data_count].data_len = name_len;
		ad_data[ad_data_count].data = (const uint8_t *)device_name;
		ad_data_count++;
	}
	
	// Scan response data
	sd_data_count = 0;
	
	// UUID do serviço customizado Buzzer Service (declaração estática)
	static const struct bt_uuid_128 buzzer_uuid = BT_UUID_INIT_128(BT_UUID_BUZZER_SERVICE_VAL);
	
	sd_data[sd_data_count].type = BT_DATA_UUID128_ALL;
	sd_data[sd_data_count].data_len = 16;
	sd_data[sd_data_count].data = buzzer_uuid.val;
	sd_data_count++;
}

/*******************************************************************************
 * API PÚBLICA
 ******************************************************************************/

int hal_ble_init(const char *device_name_param, const hal_ble_callbacks_t *callbacks)
{
	if (initialized) 
	{
		LOG_WRN("HAL BLE já inicializado");
		return HAL_BLE_SUCCESS;
	}
	
	// Valida parâmetros
	if (!device_name_param) 
	{
		LOG_ERR("Nome do dispositivo não pode ser NULL");
		return HAL_BLE_ERROR_INVALID;
	}
	
	size_t name_len = strlen(device_name_param);
	if (name_len > MAX_DEVICE_NAME_LEN) 
	{
		LOG_ERR("Nome do dispositivo muito longo (max %d caracteres)", MAX_DEVICE_NAME_LEN);
		return HAL_BLE_ERROR_INVALID;
	}
	
	// Armazena nome do dispositivo
	strncpy(device_name, device_name_param, MAX_DEVICE_NAME_LEN);
	device_name[MAX_DEVICE_NAME_LEN] = '\0';
	
	// Armazena callbacks
	if (callbacks) 
	{
		user_callbacks = *callbacks;
	}
	
	// Habilita Bluetooth
	int err = bt_enable(NULL);
	if (err) 
	{
		LOG_ERR("Falha ao habilitar Bluetooth (err %d)", err);
		return HAL_BLE_ERROR_INIT;
	}
	
	LOG_INF("Bluetooth habilitado");
	
	// Registra callbacks de conexão
	bt_conn_cb_register(&conn_callbacks);
	
	// Inicializa serviço customizado MY LBS
	// (callbacks do serviço serão registrados pela aplicação)
	
	// Prepara dados de advertising
	prepare_adv_data();
	
	// Inicializa work item para advertising
	k_work_init(&adv_work, adv_work_handler);
	
	// Estado pronto
	current_state = HAL_BLE_STATE_READY;
	initialized = true;
	
	LOG_INF("HAL BLE inicializado - Device: %s", device_name);
	return HAL_BLE_SUCCESS;
}

int hal_ble_start_advertising(const hal_ble_adv_params_t *adv_params)
{
	if (!initialized) 
	{
		LOG_ERR("HAL BLE não inicializado");
		return HAL_BLE_ERROR_STATE;
	}
	
	if (current_state == HAL_BLE_STATE_CONNECTED) 
	{
		LOG_WRN("Já conectado, não pode iniciar advertising");
		return HAL_BLE_ERROR_STATE;
	}
	
	if (current_state == HAL_BLE_STATE_ADVERTISING) 
	{
		LOG_WRN("Advertising já está ativo");
		return HAL_BLE_SUCCESS;
	}
	
	// Configura parâmetros de advertising
	if (adv_params) 
	{
		// Valida parâmetros
		if (adv_params->interval_min_ms < ADV_INTERVAL_MIN_MS ||
		    adv_params->interval_min_ms > ADV_INTERVAL_MAX_MS ||
		    adv_params->interval_max_ms < ADV_INTERVAL_MIN_MS ||
		    adv_params->interval_max_ms > ADV_INTERVAL_MAX_MS ||
		    adv_params->interval_min_ms > adv_params->interval_max_ms) {
			LOG_ERR("Parâmetros de advertising inválidos");
			return HAL_BLE_ERROR_INVALID;
		}
		
		// Prepara parâmetros
		uint32_t options = 0;
		if (adv_params->connectable) 
		{
			options |= BT_LE_ADV_OPT_CONN;
		}
		if (adv_params->use_identity) 
		{
			options |= BT_LE_ADV_OPT_USE_IDENTITY;
		}
		
		adv_param_storage.id = 0;
		adv_param_storage.sid = 0;
		adv_param_storage.secondary_max_skip = 0;
		adv_param_storage.options = options;
		adv_param_storage.interval_min = MS_TO_BLE_UNITS(adv_params->interval_min_ms);
		adv_param_storage.interval_max = MS_TO_BLE_UNITS(adv_params->interval_max_ms);
		adv_param_storage.peer = NULL;
		
		adv_param = &adv_param_storage;
		
		LOG_DBG("Parâmetros de advertising configurados: %u-%u ms",
		        adv_params->interval_min_ms, adv_params->interval_max_ms);
	} 
	else 
	{
		// Usa parâmetros padrão
		adv_param = NULL;
	}
	
	// Inicia advertising via work item (assíncrono)
	k_work_submit(&adv_work);
	
	return HAL_BLE_SUCCESS;
}

int hal_ble_stop_advertising(void)
{
	if (!initialized) 
	{
		LOG_ERR("HAL BLE não inicializado");
		return HAL_BLE_ERROR_STATE;
	}
	
	if (current_state != HAL_BLE_STATE_ADVERTISING) 
	{
		LOG_WRN("Advertising não está ativo");
		return HAL_BLE_ERROR_STATE;
	}
	
	int err = bt_le_adv_stop();
	if (err) 
	{
		LOG_ERR("Falha ao parar advertising (err %d)", err);
		return HAL_BLE_ERROR_FAILED;
	}
	
	current_state = HAL_BLE_STATE_READY;
	LOG_INF("Advertising parado");
	
	// Notifica aplicação
	if (user_callbacks.adv_stopped) 
	{
		user_callbacks.adv_stopped();
	}
	
	return HAL_BLE_SUCCESS;
}

int hal_ble_disconnect(void)
{
	if (!initialized) 
	{
		LOG_ERR("HAL BLE não inicializado");
		return HAL_BLE_ERROR_STATE;
	}
	
	if (!current_conn) 
	{
		LOG_ERR("Não há conexão ativa");
		return HAL_BLE_ERROR_NOT_CONNECTED;
	}
	
	int err = bt_conn_disconnect(current_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	if (err) 
	{
		LOG_ERR("Falha ao desconectar (err %d)", err);
		return HAL_BLE_ERROR_FAILED;
	}
	
	LOG_INF("Desconexão solicitada");
	return HAL_BLE_SUCCESS;
}

hal_ble_state_t hal_ble_get_state(void)
{
	return current_state;
}

bool hal_ble_is_connected(void)
{
	return (current_state == HAL_BLE_STATE_CONNECTED && current_conn != NULL);
}






