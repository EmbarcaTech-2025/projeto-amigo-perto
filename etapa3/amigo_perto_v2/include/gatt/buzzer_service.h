/*
 * GATT Buzzer Service - Controle remoto do buzzer via BLE
 * 
 * Serviço customizado para ligar/desligar o buzzer remotamente.
 * Estrutura: 1 serviço + 1 característica de escrita (0x00=OFF, 0x01=ON)
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef GATT_BUZZER_SERVICE_H_
#define GATT_BUZZER_SERVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <stdbool.h>

// === UUIDs CUSTOMIZADOS (128-bit) ===

// UUID do serviço Buzzer: 12345678-ABCD-EFAB-CDEF-123456789ABC
#define BT_UUID_BUZZER_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0xABCD, 0xEFAB, 0xCDEF, 0x123456789ABC)

// UUID da característica Buzzer Intermitente: 12345679-ABCD-EFAB-CDEF-123456789ABC
#define BT_UUID_BUZZER_INTERMITTENT_CHAR_VAL \
	BT_UUID_128_ENCODE(0x12345679, 0xABCD, 0xEFAB, 0xCDEF, 0x123456789ABC)

// Macros para usar nos serviços GATT
#define BT_UUID_BUZZER_SERVICE \
	BT_UUID_DECLARE_128(BT_UUID_BUZZER_SERVICE_VAL)
#define BT_UUID_BUZZER_INTERMITTENT_CHAR \
	BT_UUID_DECLARE_128(BT_UUID_BUZZER_INTERMITTENT_CHAR_VAL)

// === CALLBACK ===

/**
 * @brief Callback chamado quando comando BLE é recebido
 * @param buzzer_state true=ligar buzzer, false=desligar
 */
typedef void (*buzzer_intermittent_cb_t)(const bool buzzer_state);

/**
 * @brief Estrutura de callbacks do serviço
 */
struct gatt_buzzer_service_cb {
	buzzer_intermittent_cb_t buzzer_intermittent_cb;
};

// === INICIALIZAÇÃO ===

/**
 * @brief Inicializa o serviço GATT Buzzer
 * 
 * Registra o callback para receber comandos remotos.
 * Chamar após hal_ble_init() e antes de hal_ble_start_advertising().
 * 
 * @param callbacks Estrutura com callback (ou NULL)
 * @return 0 em sucesso, negativo em erro
 */
int gatt_buzzer_service_init(const struct gatt_buzzer_service_cb *callbacks);

#ifdef __cplusplus
}
#endif

#endif /* GATT_BUZZER_SERVICE_H_ */
