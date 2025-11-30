/*
 * GATT Battery Service - Serviço BLE para monitoramento de bateria
 * 
 * @file battery_service.c
 * @brief Implementação do serviço GATT de bateria
 * Localização: src/gatt/battery_service.c
 * Header público: include/gatt/battery_service.h
 * 
 * Implementa o Battery Service padrão do Bluetooth (0x180F) com características
 * customizadas adicionais para informações detalhadas da bateria.
 * 
 * Características implementadas:
 * - Battery Level (0x2A19) - Padrão Bluetooth SIG (Read + Notify)
 * - Battery Voltage (custom 128-bit UUID) - Tensão em mV (Read)
 * - Battery State (custom 128-bit UUID) - Estado da bateria (Read)
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "gatt/battery_service.h"
#include "hal/battery.h"

// Zephyr includes
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>

// Registra módulo de logging
LOG_MODULE_REGISTER(gatt_battery, LOG_LEVEL_DBG);

/*******************************************************************************
 * DEFINIÇÕES DE UUIDs
 ******************************************************************************/

// Battery Service e Battery Level padrão já definidos em <zephyr/bluetooth/uuid.h>
// Usar BT_UUID_BAS e BT_UUID_BAS_BATTERY_LEVEL diretamente

// UUIDs customizados para informações adicionais
// Base UUID: 00001000-8e22-4541-9d4c-21edae82ed19
#define BT_UUID_BATTERY_VOLTAGE_VAL \
	BT_UUID_128_ENCODE(0x00001001, 0x8e22, 0x4541, 0x9d4c, 0x21edae82ed19)
#define BT_UUID_BATTERY_VOLTAGE \
	BT_UUID_DECLARE_128(BT_UUID_BATTERY_VOLTAGE_VAL)

#define BT_UUID_BATTERY_STATE_VAL \
	BT_UUID_128_ENCODE(0x00001002, 0x8e22, 0x4541, 0x9d4c, 0x21edae82ed19)
#define BT_UUID_BATTERY_STATE \
	BT_UUID_DECLARE_128(BT_UUID_BATTERY_STATE_VAL)

/*******************************************************************************
 * VARIÁVEIS PRIVADAS
 ******************************************************************************/

// Callbacks da aplicação
static const struct gatt_battery_service_cb *app_callbacks = NULL;

// Valores das características
static uint8_t battery_level = 0;      // Percentual (0-100%)
static uint16_t battery_voltage = 0;   // Tensão em mV
static uint8_t battery_state = 0;      // Estado (0-4)

// Conexão atual para notificações
static struct bt_conn *current_conn = NULL;

// Flag de notificação habilitada
static bool notify_enabled = false;

/*******************************************************************************
 * FUNÇÕES DE LEITURA DAS CARACTERÍSTICAS
 ******************************************************************************/

/**
 * @brief Lê o nível da bateria (padrão Bluetooth)
 */
static ssize_t read_battery_level(struct bt_conn *conn,
                                   const struct bt_gatt_attr *attr,
                                   void *buf, uint16_t len, uint16_t offset)
{
	// Atualiza valor lendo do HAL
	hal_battery_info_t info;
	int err = hal_battery_get_info(&info);
	
	if (err == HAL_BATTERY_SUCCESS) {
		battery_level = info.percentage;
		LOG_DBG("Leitura Battery Level: %d%%", battery_level);
		
		// Notifica aplicação
		if (app_callbacks && app_callbacks->battery_read_cb) {
			app_callbacks->battery_read_cb(battery_level);
		}
	} else {
		LOG_ERR("Erro ao ler bateria para GATT (err %d)", err);
	}
	
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
	                         &battery_level, sizeof(battery_level));
}

/**
 * @brief Lê a tensão da bateria (customizado)
 */
static ssize_t read_battery_voltage(struct bt_conn *conn,
                                     const struct bt_gatt_attr *attr,
                                     void *buf, uint16_t len, uint16_t offset)
{
	// Atualiza valor lendo do HAL
	hal_battery_info_t info;
	int err = hal_battery_get_info(&info);
	
	if (err == HAL_BATTERY_SUCCESS) {
		battery_voltage = info.voltage_mv;
		LOG_DBG("Leitura Battery Voltage: %d mV", battery_voltage);
	} else {
		LOG_ERR("Erro ao ler tensão da bateria (err %d)", err);
	}
	
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
	                         &battery_voltage, sizeof(battery_voltage));
}

/**
 * @brief Lê o estado da bateria (customizado)
 */
static ssize_t read_battery_state(struct bt_conn *conn,
                                   const struct bt_gatt_attr *attr,
                                   void *buf, uint16_t len, uint16_t offset)
{
	// Atualiza valor lendo do HAL
	hal_battery_info_t info;
	int err = hal_battery_get_info(&info);
	
	if (err == HAL_BATTERY_SUCCESS) {
		battery_state = (uint8_t)info.state;
		LOG_DBG("Leitura Battery State: %d", battery_state);
	} else {
		LOG_ERR("Erro ao ler estado da bateria (err %d)", err);
	}
	
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
	                         &battery_state, sizeof(battery_state));
}

/*******************************************************************************
 * FUNÇÕES DE CCC (Client Characteristic Configuration)
 ******************************************************************************/

/**
 * @brief Callback quando cliente habilita/desabilita notificações
 */
static void battery_level_ccc_changed(const struct bt_gatt_attr *attr,
                                       uint16_t value)
{
	notify_enabled = (value == BT_GATT_CCC_NOTIFY);
	
	LOG_INF("Notificações de bateria %s",
	        notify_enabled ? "HABILITADAS" : "DESABILITADAS");
}

/*******************************************************************************
 * DEFINIÇÃO DO SERVIÇO GATT
 ******************************************************************************/

// Definição do Battery Service
BT_GATT_SERVICE_DEFINE(battery_svc,
	// Primary Service: Battery Service (0x180F)
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
	
	// Characteristic: Battery Level (0x2A19)
	// Propriedades: Read + Notify
	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
	                       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
	                       BT_GATT_PERM_READ,
	                       read_battery_level, NULL, NULL),
	
	// CCC Descriptor para notificações
	BT_GATT_CCC(battery_level_ccc_changed,
	            BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	
	// Characteristic: Battery Voltage (customizado)
	// Propriedades: Read
	BT_GATT_CHARACTERISTIC(BT_UUID_BATTERY_VOLTAGE,
	                       BT_GATT_CHRC_READ,
	                       BT_GATT_PERM_READ,
	                       read_battery_voltage, NULL, NULL),
	
	// Characteristic: Battery State (customizado)
	// Propriedades: Read
	BT_GATT_CHARACTERISTIC(BT_UUID_BATTERY_STATE,
	                       BT_GATT_CHRC_READ,
	                       BT_GATT_PERM_READ,
	                       read_battery_state, NULL, NULL),
);

/*******************************************************************************
 * CALLBACKS DE CONEXÃO
 ******************************************************************************/

/**
 * @brief Callback de conexão BLE
 */
static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Falha na conexão (err %u)", err);
		return;
	}
	
	// Armazena conexão para notificações
	if (current_conn) {
		bt_conn_unref(current_conn);
	}
	current_conn = bt_conn_ref(conn);
	
	LOG_DBG("Cliente BLE conectado");
}

/**
 * @brief Callback de desconexão BLE
 */
static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	LOG_DBG("Cliente BLE desconectado (razão %u)", reason);
	
	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}
	
	notify_enabled = false;
}

// Estrutura de callbacks de conexão
BT_CONN_CB_DEFINE(battery_conn_callbacks) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
};

/*******************************************************************************
 * API PÚBLICA
 ******************************************************************************/

int gatt_battery_service_init(const struct gatt_battery_service_cb *callbacks)
{
	app_callbacks = callbacks;
	
	// Lê valor inicial da bateria
	hal_battery_info_t info;
	int err = hal_battery_get_info(&info);
	
	if (err == HAL_BATTERY_SUCCESS) {
		battery_level = info.percentage;
		battery_voltage = info.voltage_mv;
		battery_state = (uint8_t)info.state;
		
		LOG_INF("Battery Service inicializado");
		LOG_INF("  Nível: %d%%", battery_level);
		LOG_INF("  Tensão: %d mV", battery_voltage);
		LOG_INF("  Estado: %d", battery_state);
	} else {
		LOG_WRN("Battery Service inicializado, mas leitura inicial falhou");
	}
	
	return 0;
}

int gatt_battery_service_notify(uint8_t percentage)
{
	if (!current_conn) {
		LOG_DBG("Nenhum cliente conectado para notificar");
		return -ENOTCONN;
	}
	
	if (!notify_enabled) {
		LOG_DBG("Notificações não habilitadas pelo cliente");
		return -EACCES;
	}
	
	battery_level = percentage;
	
	int err = bt_gatt_notify(current_conn, &battery_svc.attrs[1],
	                         &battery_level, sizeof(battery_level));
	
	if (err) {
		LOG_ERR("Falha ao enviar notificação (err %d)", err);
		return err;
	}
	
	LOG_DBG("Notificação de bateria enviada: %d%%", percentage);
	
	return 0;
}

int gatt_battery_service_update(uint8_t percentage)
{
	battery_level = percentage;
	LOG_DBG("Valor de bateria atualizado: %d%%", percentage);
	return 0;
}
