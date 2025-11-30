/*
 * GATT Battery Service - Serviço BLE para monitoramento de bateria
 */

/**
 * @file battery_service.h
 * @brief Interface do serviço GATT de bateria
 * 
 * Este módulo implementa o Battery Service padrão do Bluetooth (UUID 0x180F)
 * com a característica Battery Level (UUID 0x2A19), permitindo que aplicativos
 * móveis leiam o nível da bateria.
 * 
 * Características:
 * - Battery Level: Leitura e notificação do percentual de bateria (0-100%)
 * - Battery Voltage: Leitura da tensão em mV (customizado)
 * - Battery State: Leitura do estado (Critical, Low, Medium, Good) (customizado)
 * 
 * Compatível com Android, iOS e outros dispositivos BLE que suportam
 * o Battery Service padrão.
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
	 */
	void (*battery_read_cb)(uint8_t percentage);
};

/**
 * @brief Inicializa o serviço GATT de bateria
 * 
 * Registra o Battery Service e suas características no stack BLE.
 * Deve ser chamado após a inicialização do BLE e antes de iniciar advertising.
 * 
 * @param callbacks Estrutura de callbacks (pode ser NULL se não usar)
 * 
 * @return 0 em caso de sucesso
 * @return Código de erro negativo em caso de falha
 */
int gatt_battery_service_init(const struct gatt_battery_service_cb *callbacks);

/**
 * @brief Envia notificação de mudança no nível de bateria
 * 
 * Envia notificação BLE aos clientes conectados que habilitaram
 * notificações da característica Battery Level.
 * 
 * Esta função deve ser chamada quando o nível da bateria mudar
 * significativamente (ex: a cada 5% de mudança).
 * 
 * @param percentage Novo percentual de bateria (0-100%)
 * 
 * @return 0 em caso de sucesso
 * @return Código de erro negativo se não houver clientes conectados
 *         ou se notificações não estiverem habilitadas
 */
int gatt_battery_service_notify(uint8_t percentage);

/**
 * @brief Atualiza o valor da bateria sem enviar notificação
 * 
 * Atualiza o valor interno da característica Battery Level sem enviar
 * notificação aos clientes. Útil para atualizar o valor antes que
 * um cliente leia.
 * 
 * @param percentage Percentual de bateria (0-100%)
 * 
 * @return 0 em caso de sucesso
 */
int gatt_battery_service_update(uint8_t percentage);

#ifdef __cplusplus
}
#endif

#endif /* GATT_BATTERY_SERVICE_H_ */
