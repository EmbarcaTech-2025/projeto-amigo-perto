/*
 * GATT Buzzer Service - Serviço BLE para controle remoto do buzzer
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file buzzer_service.h
 * @brief API do Serviço Customizado GATT Buzzer Service
 * 
 * Este arquivo define a interface do serviço BLE customizado para controle de buzzer.
 * Contém os UUIDs do serviço e características, tipos de callback e função de inicialização.
 * 
 * Estrutura do serviço:
 * - 1 Serviço primário com UUID customizado
 * - 1 Característica de escrita para controlar o buzzer
 */

#ifndef GATT_BUZZER_SERVICE_H_
#define GATT_BUZZER_SERVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <stdbool.h>

/**
 * Definição dos UUIDs de 128 bits
 * UUIDs customizados identificam unicamente o serviço e suas características
 */
/** @brief Serviço Buzzer UUID - UUID completamente customizado. */
#define BT_UUID_BUZZER_SERVICE_VAL BT_UUID_128_ENCODE(0x12345678, 0xABCD, 0xEFAB, 0xCDEF, 0x123456789ABC)

/** @brief Buzzer Intermitente Characteristic UUID. */
#define BT_UUID_BUZZER_INTERMITTENT_CHAR_VAL BT_UUID_128_ENCODE(0x12345679, 0xABCD, 0xEFAB, 0xCDEF, 0x123456789ABC)

#define BT_UUID_BUZZER_SERVICE BT_UUID_DECLARE_128(BT_UUID_BUZZER_SERVICE_VAL)
#define BT_UUID_BUZZER_INTERMITTENT_CHAR BT_UUID_DECLARE_128(BT_UUID_BUZZER_INTERMITTENT_CHAR_VAL)

/**
 * Tipo de callback para controle do buzzer intermitente
 * 
 * Este tipo define a assinatura da função callback que será chamada
 * quando um comando de buzzer intermitente for recebido via BLE
 * 
 * @param buzzer_state - true para ativar, false para desativar
 */
typedef void (*buzzer_intermittent_cb_t)(const bool buzzer_state);

/**
 * Estrutura de callbacks usada pelo serviço GATT Buzzer
 * 
 * Contém ponteiros para as funções callback da aplicação
 * que serão chamadas quando eventos específicos ocorrerem
 */
struct gatt_buzzer_service_cb {
	/** Ponteiro para a função callback de controle do buzzer intermitente */
	buzzer_intermittent_cb_t buzzer_intermittent_cb;
};

/**
 * Inicializa o serviço GATT Buzzer
 * 
 * Esta função registra os callbacks da aplicação com o serviço GATT Buzzer.
 * Deve ser chamada após a inicialização do Bluetooth e antes de iniciar
 * o advertising.
 * 
 * @param[in] callbacks Ponteiro para estrutura contendo os callbacks da aplicação.
 *                      Pode ser NULL se nenhum callback for definido.
 * 
 * @retval 0 Se a operação foi bem-sucedida
 * @retval <0 Código de erro negativo em caso de falha
 */
int gatt_buzzer_service_init(const struct gatt_buzzer_service_cb *callbacks);

#ifdef __cplusplus
}
#endif

#endif /* GATT_BUZZER_SERVICE_H_ */
