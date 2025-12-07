/*
 * GATT Battery Service - Serviço BLE padrão para monitoramento de bateria
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Implementação simplificada do Battery Service padrão Bluetooth SIG (UUID 0x180F)
 * com apenas a característica Battery Level (UUID 0x2A19) para leitura de percentual.
 *
 * Características implementadas:
 * - Battery Level (0x2A19): Leitura e notificação do percentual (0-100%)
 *
 * Compatível com Android, iOS e qualquer dispositivo que suporte o Battery Service.
 * O percentual é obtido diretamente do HAL Battery (hal_battery_get_percentage).
 */

#ifndef GATT_BATTERY_SERVICE_H_
#define GATT_BATTERY_SERVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Estrutura de callbacks do serviço de bateria
 * 
 * Permite que a aplicação seja notificada quando o cliente solicita
 * informações da bateria.
 */
struct gatt_battery_service_cb {
	/**
	 * @brief Callback chamado quando o cliente lê o nível da bateria
	 * 
	 * Opcional. Permite que a aplicação faça logging ou outras ações
	 * quando a bateria é consultada.
	 * 
	 * @param percentage Percentual de bateria lido (0-100%)
	 * @param voltage_mv Tensão da bateria em milivolts
	 */
	void (*battery_read_cb)(uint8_t percentage, uint16_t voltage_mv);
};

// === API PÚBLICA ===

/**
 * Inicializa o Battery Service GATT
 *
 * Registra o Battery Service padrão (0x180F) no stack BLE com a característica
 * Battery Level (0x2A19) configurada para leitura.
 *
 * Deve ser chamado após hal_ble_init() e antes de hal_ble_start_advertising().
 *
 * Comportamento:
 * - Lê valor inicial da bateria via hal_battery_get_percentage()
 * - Registra callback para leitura remota via BLE
 * - Permite que clientes BLE leiam o percentual sob demanda
 *
 * @param callbacks Estrutura de callbacks (pode ser NULL se não precisar de notificações)
 *
 * @return 0 em caso de sucesso
 * @return Código de erro negativo em caso de falha
 *
 * Exemplo:
 *   gatt_battery_service_init(&battery_callbacks);
 */
int gatt_battery_service_init(const struct gatt_battery_service_cb *callbacks);

#ifdef __cplusplus
}
#endif

#endif /* GATT_BATTERY_SERVICE_H_ */
