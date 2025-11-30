/*
 * GATT Buzzer Service - Serviço BLE para controle remoto do buzzer
 * 
 * @file buzzer_service.c
 * @brief Implementação do Serviço Customizado GATT Buzzer Service
 * Localização: src/gatt/buzzer_service.c
 * Header público: include/gatt/buzzer_service.h
 * 
 * Este arquivo implementa um serviço GATT customizado que permite controlar
 * o buzzer remotamente via Bluetooth Low Energy.
 * 
 * Componentes principais:
 * - Serviço GATT primário com UUID customizado (128-bit)
 * - Característica de escrita para buzzer intermitente (Write without response)
 * - Callbacks para comunicação com a aplicação principal
 * - Validação de dados recebidos (0x00=OFF, 0x01=ON)
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

// Bibliotecas padrão C
#include <zephyr/types.h>               // Tipos de dados padrão
#include <stddef.h>                     // Definições padrão (NULL, size_t, etc)
#include <string.h>                     // Funções de manipulação de strings
#include <errno.h>                      // Códigos de erro

// Bibliotecas do Zephyr
#include <zephyr/sys/printk.h>          // Função printk para debug
#include <zephyr/sys/byteorder.h>       // Conversão de endianness
#include <zephyr/kernel.h>              // Funções do kernel Zephyr

// Bibliotecas Bluetooth
#include <zephyr/bluetooth/bluetooth.h> // Stack Bluetooth principal
#include <zephyr/bluetooth/hci.h>       // Host Controller Interface
#include <zephyr/bluetooth/conn.h>      // Gerenciamento de conexões
#include <zephyr/bluetooth/uuid.h>      // UUIDs
#include <zephyr/bluetooth/gatt.h>      // GATT (Generic Attribute Profile)

// Header do serviço
#include "gatt/buzzer_service.h"

// Sistema de logging
#include <zephyr/logging/log.h>

// Declara que este arquivo usa o módulo de log
LOG_MODULE_REGISTER(gatt_buzzer, LOG_LEVEL_DBG);

// UUID do descritor de nome de usuário (User Description)
#define BT_UUID_GATT_CHRC_USER_DESC BT_UUID_DECLARE_16(0x2901)

// Formato de apresentação booleano para descritores GATT (0x2904)
// Format: 0x01 (Boolean), Exponent: 0x01, Unit: 0x2701, Namespace: 0x01, Description: 0x0000
static uint8_t gatt_cpf_format_boolean[] = {0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};

// Variável global estática: armazena os callbacks registrados pela aplicação
static struct gatt_buzzer_service_cb buzzer_cb;

/**
 * Callback de escrita da característica Buzzer Intermitente
 * 
 * Esta função é chamada automaticamente pelo stack Bluetooth quando um dispositivo
 * conectado escreve um valor na característica de buzzer intermitente.
 * 
 * @param conn - Ponteiro para a conexão BLE ativa
 * @param attr - Ponteiro para o atributo GATT sendo escrito
 * @param buf - Buffer contendo os dados recebidos
 * @param len - Tamanho dos dados recebidos
 * @param offset - Offset de escrita (deve ser 0 para esta característica)
 * @param flags - Flags de escrita
 * @return Número de bytes escritos ou código de erro
 */
static ssize_t write_buzzer_intermittent(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Escrita no atributo buzzer intermitente, handle: %u, conn: %p", attr->handle, (void *)conn);

	if (len != 1U) 
	{
		LOG_DBG("Write buzzer intermitente: Tamanho de dado incorreto");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (offset != 0) 
	{
		LOG_DBG("Write buzzer intermitente: Offset de dado incorreto");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (buzzer_cb.buzzer_intermittent_cb) 
	{
		uint8_t val = *((uint8_t *)buf);

		if (val == 0x00 || val == 0x01) 
		{
			buzzer_cb.buzzer_intermittent_cb(val ? true : false);
		} 
		else 
		{
			LOG_DBG("Write buzzer intermitente: Valor incorreto");
			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
	}

	return len;
}

/**
 * Declaração do Serviço GATT Buzzer Service
 * 
 * Esta macro define e registra automaticamente o serviço GATT no stack Bluetooth.
 * O serviço será descoberto por dispositivos conectados e suas características
 * poderão ser acessadas via comandos BLE.
 * 
 * Estrutura:
 * - Serviço Primário: Buzzer Service (identificado por BT_UUID_BUZZER_SERVICE)
 *   - Característica Buzzer Intermitente: Permite escrita de 1 byte (0x00 ou 0x01)
 *     - Propriedade: WRITE (escrita sem resposta)
 *     - Permissão: WRITE (qualquer dispositivo conectado pode escrever)
 *     - Callback de escrita: write_buzzer_intermittent()
 */
BT_GATT_SERVICE_DEFINE(buzzer_svc,
	// Define o serviço primário com UUID customizado
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BUZZER_SERVICE),
	
	// Define a característica de Buzzer Intermitente
	BT_GATT_CHARACTERISTIC(
		BT_UUID_BUZZER_INTERMITTENT_CHAR,  // UUID da característica
		BT_GATT_CHRC_WRITE,               // Propriedade: permite escrita
		BT_GATT_PERM_WRITE,               // Permissão: escrita permitida
		NULL,                             // Callback de leitura (não usado)
		write_buzzer_intermittent,        // Callback de escrita
		NULL                              // Ponteiro para valor armazenado (não usado)
	),
	// Adiciona descritor de usuário para nome legível
	BT_GATT_DESCRIPTOR(BT_UUID_GATT_CHRC_USER_DESC, BT_GATT_PERM_READ, NULL, NULL, "Buzzer Intermitente"),
	// Adiciona descritor de formato booleano (Presentation Format Descriptor)
	BT_GATT_DESCRIPTOR(BT_UUID_DECLARE_16(0x2904), BT_GATT_PERM_READ, NULL, NULL, gatt_cpf_format_boolean)
);

/**
 * Função de inicialização do serviço GATT Buzzer
 * 
 * Registra os callbacks da aplicação que serão chamados quando
 * eventos do serviço ocorrerem (ex: escrita nas características de buzzer).
 * 
 * @param callbacks - Ponteiro para estrutura contendo os callbacks da aplicação
 * @return 0 em caso de sucesso
 */
int gatt_buzzer_service_init(const struct gatt_buzzer_service_cb *callbacks)
{
	// Verifica se o ponteiro de callbacks é válido
	if (callbacks) 
	{
		// Armazena os callbacks na variável global
		buzzer_cb.buzzer_intermittent_cb = callbacks->buzzer_intermittent_cb;
	}

	// Retorna sucesso (0)
	return 0;
}
