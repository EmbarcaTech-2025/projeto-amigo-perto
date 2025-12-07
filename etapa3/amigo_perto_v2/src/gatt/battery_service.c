/*
 * GATT Battery Service - Serviço BLE padrão para monitoramento de bateria
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Implementação simplificada do Battery Service padrão Bluetooth SIG (UUID 0x180F).
 * Fornece apenas a característica Battery Level (UUID 0x2A19) para leitura de percentual.
 *
 * Características:
 * - Battery Level (0x2A19): Read-only, retorna percentual 0-100%
 *
 * Integração:
 * - Usa hal_battery_get_percentage() para obter o valor atual
 * - Notifica aplicação via callback quando cliente lê o valor
 * - Valor é sempre atualizado em tempo real a cada leitura
 */

// === INCLUDES ===
#include "gatt/battery_service.h"
#include "hal/battery.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

LOG_MODULE_REGISTER(gatt_battery, LOG_LEVEL_DBG);

// === UUIDs PADRÃO BLUETOOTH SIG ===
// Battery Service (0x180F) e Battery Level (0x2A19)
// Definidos em <zephyr/bluetooth/uuid.h>:
// - BT_UUID_BAS: Battery Service
// - BT_UUID_BAS_BATTERY_LEVEL: Battery Level Characteristic

// === VARIÁVEIS PRIVADAS ===

// Callbacks da aplicação (opcional)
static const struct gatt_battery_service_cb *app_callbacks = NULL;

// Cache do último valor lido (para evitar leituras ADC repetidas muito próximas)
static uint8_t battery_level_cache = 0;
static uint16_t battery_voltage_cache = 0;

// === CALLBACKS GATT ===

/**
 * Callback: Leitura da característica Battery Level
 *
 * Chamado automaticamente pelo stack BLE quando um cliente remoto (smartphone)
 * lê a característica Battery Level (0x2A19).
 *
 * Fluxo:
 * 1. Lê percentual atual via hal_battery_get_percentage()
 * 2. Lê tensão em mV via hal_battery_get_millivolt()
 * 3. Atualiza cache local
 * 4. Notifica aplicação via callback (se registrado)
 * 5. Retorna valor ao cliente BLE
 *
 * @param conn Conexão BLE que solicitou a leitura
 * @param attr Atributo GATT sendo lido
 * @param buf Buffer para armazenar o valor
 * @param len Tamanho do buffer
 * @param offset Offset de leitura (sempre 0 para Battery Level)
 *
 * @return Número de bytes lidos, ou código de erro negativo
 */
static ssize_t read_battery_level(struct bt_conn *conn,
                                   const struct bt_gatt_attr *attr,
                                   void *buf, uint16_t len, uint16_t offset)
{
	// Lê valor atual da bateria via HAL
	// Esta função já faz oversampling do ADC e conversão para percentual
	battery_level_cache = hal_battery_get_percentage();
	
	// Lê tensão da bateria em milivolts
	hal_battery_get_millivolt(&battery_voltage_cache);
	
	LOG_DBG("Cliente leu Battery Level: %d%% (%dmV)", battery_level_cache, battery_voltage_cache);
	
	// Notifica aplicação se callback foi registrado
	if (app_callbacks && app_callbacks->battery_read_cb) {
		app_callbacks->battery_read_cb(battery_level_cache, battery_voltage_cache);
	}
	
	// Retorna valor ao cliente BLE (1 byte: 0-100%)
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
	                         &battery_level_cache, sizeof(battery_level_cache));
}

/**
 * Callback: Leitura da característica Battery Voltage (customizada)
 *
 * Fornece a tensão da bateria em milivolts como uint16_t (2 bytes).
 * Útil para diagnóstico e monitoramento mais preciso do que apenas o percentual.
 *
 * @param conn Conexão BLE que solicitou a leitura
 * @param attr Atributo GATT sendo lido
 * @param buf Buffer para armazenar o valor
 * @param len Tamanho do buffer
 * @param offset Offset de leitura
 *
 * @return Número de bytes lidos, ou código de erro negativo
 */
static ssize_t read_battery_voltage(struct bt_conn *conn,
                                     const struct bt_gatt_attr *attr,
                                     void *buf, uint16_t len, uint16_t offset)
{
	LOG_DBG("Cliente leu Battery Voltage: %dmV", battery_voltage_cache);
	
	// Retorna tensão ao cliente BLE (2 bytes: milivolts)
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
	                         &battery_voltage_cache, sizeof(battery_voltage_cache));
}

// UUID customizado (128-bit) para Battery Voltage em milivolts
// Base UUID: 00000000-0000-1000-8000-00805F9B34FB (Bluetooth SIG base)
// Customizado: 00002A19-0000-1000-8000-00805F9B34FB (derivado de Battery Level)
// Modificado: 00002B19-0000-1000-8000-00805F9B34FB (offset para voltage)
#define BT_UUID_BATTERY_VOLTAGE \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x00002B19, 0x0000, 0x1000, 0x8000, 0x00805F9B34FB))

// Presentation Format Descriptor para Battery Voltage
// Define o formato de exibição: uint16, unidade = volt (com expoente -3 = milivolts)
static struct bt_gatt_cpf voltage_format = {
	.format = 0x06,        // uint16
	.exponent = -3,        // 10^-3 = mili (valor × 0.001 = volts)
	.unit = 0x2728,        // UUID da unidade: Electric Potential Difference (volt)
	.name_space = 0x01,    // Bluetooth SIG namespace
	.description = 0x0000, // Sem descrição adicional
};

// === DEFINIÇÃO DO SERVIÇO GATT ===

/**
 * Battery Service GATT Definition
 *
 * Serviço padrão Bluetooth SIG para monitoramento de bateria.
 * Reconhecido automaticamente por Android, iOS e outros sistemas operacionais.
 *
 * Estrutura:
 * - Service UUID: 0x180F (Battery Service)
 *   - Characteristic UUID: 0x2A19 (Battery Level)
 *     - Properties: Read
 *     - Permissions: Read
 *     - Format: uint8 (0-100%)
 *     - Callback: read_battery_level()
 *   - Characteristic UUID: 00002B19-...-34FB (Battery Voltage - customizada)
 *     - Properties: Read
 *     - Permissions: Read
 *     - Format: uint16 (milivolts)
 *     - Callback: read_battery_voltage()
 *
 * Nota: Não implementa notificações (CCC) pois a leitura sob demanda
 * é suficiente para a maioria dos casos de uso e economiza energia.
 */
BT_GATT_SERVICE_DEFINE(battery_svc,
	// Primary Service: Battery Service (UUID 0x180F)
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
	
	// Characteristic: Battery Level (UUID 0x2A19)
	// Propriedades: Read (leitura sob demanda pelo cliente)
	// Permissões: Read (qualquer cliente conectado pode ler)
	// Callbacks: read_battery_level (read), NULL (write), NULL (value)
	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
	                       BT_GATT_CHRC_READ,
	                       BT_GATT_PERM_READ,
	                       read_battery_level, NULL, NULL),
	
	// Characteristic: Battery Voltage (UUID 0x2B19 - customizada)
	// Propriedades: Read (leitura sob demanda pelo cliente)
	// Permissões: Read (qualquer cliente conectado pode ler)
	// Callbacks: read_battery_voltage (read), NULL (write), NULL (value)
	// Descriptor: Presentation Format (indica formato uint16 + unidade volt com expoente -3)
	BT_GATT_CHARACTERISTIC(BT_UUID_BATTERY_VOLTAGE,
	                       BT_GATT_CHRC_READ,
	                       BT_GATT_PERM_READ,
	                       read_battery_voltage, NULL, NULL),
	BT_GATT_CPF(&voltage_format),
);



// === API PÚBLICA ===

/**
 * Inicializa o Battery Service GATT
 *
 * Registra o serviço no stack BLE e faz leitura inicial da bateria.
 * O serviço fica disponível automaticamente após inicialização do BLE.
 *
 * @param callbacks Estrutura de callbacks (opcional, pode ser NULL)
 * @return 0 sempre (sucesso)
 */
int gatt_battery_service_init(const struct gatt_battery_service_cb *callbacks)
{
	// Armazena callbacks da aplicação (se fornecidos)
	app_callbacks = callbacks;
	
	// Lê valor inicial da bateria para popular o cache
	battery_level_cache = hal_battery_get_percentage();
	hal_battery_get_millivolt(&battery_voltage_cache);
	
	LOG_INF("Battery Service inicializado");
	LOG_INF("  Nível inicial: %d%% (%dmV)", battery_level_cache, battery_voltage_cache);
	LOG_INF("  UUID Service: 0x180F");
	LOG_INF("  UUID Characteristics:");
	LOG_INF("    - Battery Level: 0x2A19 (percentual)");
	LOG_INF("    - Battery Voltage: 00002B19-...-34FB (milivolts)");
	
	return 0;
}
