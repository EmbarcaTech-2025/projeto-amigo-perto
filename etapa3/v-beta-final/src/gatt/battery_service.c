/*
 * Serviço BLE GATT de Bateria
 *
 * Implementa o serviço padrão Bluetooth SIG Battery Service (UUID 0x180F) e expõe:
 * - Battery Level (UUID 0x2A19): percentual 0-100 (uint8)
 * - Battery Voltage (UUID custom 128-bit): tensão em milivolts (uint16)
 *
 * Observações:
 * - A leitura de ADC pode levar alguns ms; por isso há um cache curto.
 * - Se a bateria não estiver disponível, retorna valores 0.
 */

// === INCLUSÕES ===
#include "gatt/battery_service.h"
#include "hal/battery.h"

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

LOG_MODULE_REGISTER(gatt_battery, LOG_LEVEL_INF);

// === FAIXA DE TENSÃO DA BATERIA ===
/* Faixa típica Li-Po 1 célula: ~3,0 V a ~4,25 V */
#define BATTERY_MV_MIN 3000
#define BATTERY_MV_MAX 4250

/*
 * Ler o ADC pode ser relativamente caro e pode bloquear por alguns ms.
 * Para evitar leituras repetidas quando um cliente BLE faz leituras em sequência,
 * mantemos um cache curto (TTL).
 */
#ifndef BATTERY_SERVICE_CACHE_TTL_MS
#define BATTERY_SERVICE_CACHE_TTL_MS 750
#endif

/**
 * Valida se a tensão lida está dentro da faixa esperada para bateria Li-Po.
 * Valores fora dessa faixa indicam leitura flutuante (sem bateria) ou erro.
 */
static bool battery_mv_valid(uint16_t mv)
{
	return (mv >= BATTERY_MV_MIN) && (mv <= BATTERY_MV_MAX);
}

// === UUIDs PADRÃO BLUETOOTH SIG ===
// Battery Service (0x180F) e Battery Level (0x2A19)
// Definidos em <zephyr/bluetooth/uuid.h>:
// - BT_UUID_BAS: Battery Service
// - BT_UUID_BAS_BATTERY_LEVEL: Battery Level

// === DADOS PRIVADOS ===

struct battery_cache {
	uint8_t percent;
	uint16_t mv;
	uint32_t timestamp_ms;
};

/* Cache do último valor (evita leituras repetidas do ADC em sequência) */
static struct battery_cache cache;
static struct k_mutex cache_lock;

static void cache_set(uint8_t percent, uint16_t mv)
{
	cache.percent = percent;
	cache.mv = mv;
	cache.timestamp_ms = k_uptime_get_32();
}

static int cache_refresh_locked(void)
{
	uint16_t mv = 0;
	uint8_t percent = 0;
	int ret = battery_get_millivolt(&mv);

	if (ret) {
		cache_set(0, 0);
		return ret;
	}

	if (!battery_mv_valid(mv)) {
		cache_set(0, 0);
		return -ENODATA;
	}

	ret = battery_get_percentage(&percent, mv);
	if (ret) {
		/* Mantém tensão válida, mas zera percentual em caso de falha */
		percent = 0;
	}

	cache_set(percent, mv);
	return 0;
}

static void battery_service_get_cached(uint8_t *percent, uint16_t *mv)
{
	(void)k_mutex_lock(&cache_lock, K_FOREVER);

	const uint32_t now = k_uptime_get_32();
	const bool cache_empty = (cache.timestamp_ms == 0U);
	const uint32_t age_ms = now - cache.timestamp_ms;
	if (cache_empty || age_ms >= BATTERY_SERVICE_CACHE_TTL_MS) {
		(void)cache_refresh_locked();
	}

	*percent = cache.percent;
	*mv = cache.mv;

	k_mutex_unlock(&cache_lock);
}

// === CALLBACKS DO GATT ===

static ssize_t read_battery_level(struct bt_conn *conn,
                                   const struct bt_gatt_attr *attr,
                                   void *buf, uint16_t len, uint16_t offset)
{
	uint8_t battery_level;
	uint16_t battery_voltage;

	battery_service_get_cached(&battery_level, &battery_voltage);
	LOG_DBG("Cliente leu Battery Level: %u%% (%umV)", battery_level, battery_voltage);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
	                         &battery_level, sizeof(battery_level));
}

/*
 * Leitura da característica de tensão (customizada).
 * Retorna a tensão em mV como uint16 (2 bytes, little-endian).
 */
static ssize_t read_battery_voltage(struct bt_conn *conn,
                                     const struct bt_gatt_attr *attr,
                                     void *buf, uint16_t len, uint16_t offset)
{
	uint8_t battery_level;
	uint16_t battery_voltage;

	battery_service_get_cached(&battery_level, &battery_voltage);
	uint16_t battery_voltage_le = sys_cpu_to_le16(battery_voltage);
	LOG_DBG("Cliente leu Battery Voltage: %umV", battery_voltage);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
	                         &battery_voltage_le, sizeof(battery_voltage_le));
}

// UUID customizado (128-bit) para Battery Voltage (tensão em mV)
// Base UUID (SIG): 00000000-0000-1000-8000-00805F9B34FB
// Aqui usamos um UUID derivado para manter consistência com o BAS.
#define BT_UUID_BATTERY_VOLTAGE BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x00002B19, 0x0000, 0x1000, 0x8000, 0x00805F9B34FB))

// Descriptor CPF (Characteristic Presentation Format) para Battery Voltage.
// Formato: uint16, unidade = volt, expoente -3 (valor em milivolts).
static struct bt_gatt_cpf voltage_format = {
	.format = 0x06,        // uint16
	.exponent = -3,        // 10^-3 = mili (valor × 0.001 = volts)
	.unit = 0x2728,        // UUID da unidade: Electric Potential Difference (volt)
	.name_space = 0x01,    // Bluetooth SIG namespace
	.description = 0x0000, // Sem descrição adicional
};

// === DEFINIÇÃO DO SERVIÇO GATT ===

/*
 * Battery Service (0x180F)
 * - Battery Level (0x2A19): uint8 (0-100%)
 * - Battery Voltage (custom): uint16 (mV)
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

/* Inicializa o serviço e tenta preencher o cache (best-effort). */
int battery_service_init(void)
{
	k_mutex_init(&cache_lock);

	/* Best-effort: preenche cache se a camada de bateria já estiver inicializada */
	(void)k_mutex_lock(&cache_lock, K_FOREVER);
	(void)cache_refresh_locked();
	k_mutex_unlock(&cache_lock);

	LOG_INF("Battery Service inicializado");
	return 0;
}
