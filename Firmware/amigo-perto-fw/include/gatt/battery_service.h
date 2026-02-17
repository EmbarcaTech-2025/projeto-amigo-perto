/*
 * Serviço BLE GATT de Bateria.
 *
 * Expõe o serviço padrão 0x180F (Battery Service) com:
 * - 0x2A19 Battery Level: percentual (0-100)
 * - Battery Voltage (custom): tensão em mV (uint16) + CPF
 */

#ifndef BATTERY_SERVICE_H_
#define BATTERY_SERVICE_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>

// === API PÚBLICA ===

/** Inicializa o serviço GATT de bateria (best-effort). */
int battery_service_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_SERVICE_H_ */
