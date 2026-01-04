/*
 * GATT Buzzer Service - Controle remoto do buzzer via BLE
 * 
 * Implementa serviço customizado com 1 característica de escrita
 * para ligar/desligar o buzzer remotamente (0x00=OFF, 0x01=ON).
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "gatt/buzzer_service.h"
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gatt_buzzer, LOG_LEVEL_DBG);

// === ESTADO INTERNO ===

// Armazena o callback registrado pela aplicação (main.c)
static struct gatt_buzzer_service_cb buzzer_cb;

// === CALLBACK GATT ===

/**
 * @brief Handler de escrita da característica Buzzer Intermitente
 * 
 * Chamado automaticamente quando dispositivo remoto escreve na característica.
 * Valida dados (tamanho, offset, valor) e chama callback da aplicação.
 * 
 * @param conn Conexão BLE ativa
 * @param attr Atributo GATT sendo escrito
 * @param buf Dados recebidos (1 byte: 0x00 ou 0x01)
 * @param len Tamanho dos dados (deve ser 1)
 * @param offset Offset de escrita (deve ser 0)
 * @param flags Flags de escrita
 * @return Bytes escritos ou código de erro GATT
 */
static ssize_t write_buzzer_intermittent(struct bt_conn *conn,
                                          const struct bt_gatt_attr *attr,
                                          const void *buf,
                                          uint16_t len,
                                          uint16_t offset,
                                          uint8_t flags)
{
	LOG_DBG("Write buzzer - handle: %u, len: %u", attr->handle, len);

	// Valida tamanho: deve ser exatamente 1 byte
	if (len != 1U) 
	{
		LOG_WRN("Tamanho inválido: %u (esperado: 1)", len);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	// Valida offset: deve começar no byte 0
	if (offset != 0) 
	{
		LOG_WRN("Offset inválido: %u (esperado: 0)", offset);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	// Lê valor recebido
	uint8_t val = *((uint8_t *)buf);

	// Valida valor: apenas 0x00 (OFF) ou 0x01 (ON) permitidos
	if (val != 0x00 && val != 0x01) 
	{
		LOG_WRN("Valor inválido: 0x%02X (permitido: 0x00 ou 0x01)", val);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	// Chama callback da aplicação se registrado
	if (buzzer_cb.buzzer_intermittent_cb) 
	{
		buzzer_cb.buzzer_intermittent_cb(val ? true : false);
	}

	return len;
}

// === DEFINIÇÃO DO SERVIÇO GATT ===

/**
 * Estrutura do serviço:
 * - Serviço primário: Buzzer Service (UUID customizado)
 *   └─ Característica: Buzzer Intermitente (Write only)
 *      - Propriedade: WRITE (sem resposta)
 *      - Permissão: WRITE
 *      - Valor: 1 byte (0x00=OFF, 0x01=ON)
 */
BT_GATT_SERVICE_DEFINE(buzzer_svc,
	// Declaração do serviço primário
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BUZZER_SERVICE),
	
	// Característica: Buzzer Intermitente
	BT_GATT_CHARACTERISTIC(
		BT_UUID_BUZZER_INTERMITTENT_CHAR,  // UUID da característica
		BT_GATT_CHRC_WRITE,                // Propriedade: Write without response
		BT_GATT_PERM_WRITE,                // Permissão: Write
		NULL,                              // Sem leitura
		write_buzzer_intermittent,         // Handler de escrita
		NULL                               // Sem armazenamento interno
	),
);

// === API PÚBLICA ===

/**
 * @brief Inicializa o serviço GATT Buzzer
 * 
 * Registra callbacks para receber comandos remotos.
 * O serviço é automaticamente exposto após hal_ble_start_advertising().
 * 
 * @param callbacks Estrutura com callback ou NULL
 * @return 0 em sucesso
 */
int gatt_buzzer_service_init(const struct gatt_buzzer_service_cb *callbacks)
{
	// Registra callback se fornecido
	if (callbacks) 
	{
		buzzer_cb.buzzer_intermittent_cb = callbacks->buzzer_intermittent_cb;
		LOG_INF("Buzzer Service registrado com callback");
	} 
	else 
	{
		LOG_WRN("Buzzer Service sem callback");
	}

	return 0;
}
