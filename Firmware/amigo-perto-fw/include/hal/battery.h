/*
 * HAL Battery - Gerenciamento de bateria LiPo via ADC e GPIO
 * 
 * Adaptado da biblioteca pública:
 * https://github.com/Tjoms99/xiao_sense_nrf52840_battery_lib
 * Autores originais: Tjoms99 (Marcus Alexander Tjomsaas),
 *                    DBS06 (Philipp Steiner),
 *                    bringert (Bjorn Bringert)
 * 
 * Recursos:
 * - Leitura de tensão via ADC (AIN7) com divisor resistivo
 * - Detecção de estado de carregamento via GPIO (active-low)
 * - Controle de velocidade de carga (50mA slow / 100mA fast)
 * - Amostragem periódica com filtros de estabilização
 * - Callbacks para mudança de estado e novas amostras
 * 
 * Filtros aplicados:
 * - Média aparada (trimmed mean) - remove min/max
 * - Filtro IIR passa-baixa (alpha = 1/8)
 * - Rejeição de spikes (>150mV)
 *
 * Copyright 2024 Marcus Alexander Tjomsaas
 * Licensed under the Apache License, Version 2.0
 */

#ifndef __BATTERY_H__
#define __BATTERY_H__

#include <stdint.h>
#include <stdbool.h>

// === TIPOS DE CALLBACK ===
// Callback chamado quando o estado de carregamento muda (conectado/desconectado)
typedef void (*battery_charging_callback_t)(bool is_charging);
// Callback chamado quando uma nova amostra de tensão está disponível
typedef void (*battery_sample_callback_t)(uint16_t millivolt);

/**
 * @brief Registra callback executado quando o estado de carregamento muda
 * 
 * O callback é chamado automaticamente quando o pino charging_enable detecta
 * uma transição (carregador conectado/desconectado). A detecção ocorre via
 * interrupção em ambas as bordas (GPIO_INT_EDGE_BOTH).
 *
 * @param callback Função a ser chamada com o novo estado (true=carregando)
 * @return 0 se bem-sucedido, -ENOMEM se limite de callbacks atingido
 * @note Se erro -12, aumente BATTERY_CALLBACK_MAX no Device Tree
 */
int battery_register_charging_callback(battery_charging_callback_t callback);

/**
 * @brief Registra callback executado quando uma nova amostra de tensão está pronta
 * 
 * O callback recebe a tensão em milivolts após aplicação de todos os filtros:
 * - Média aparada (descarta min/max das amostras)
 * - Filtro IIR passa-baixa (suavização temporal)
 * - Rejeição de spikes (variações >150mV)
 *
 * @param callback Função a ser chamada com a tensão em mV
 * @return 0 se bem-sucedido, -ENOMEM se limite de callbacks atingido
 * @note Se erro -12, aumente BATTERY_CALLBACK_MAX no Device Tree
 */
int battery_register_sample_callback(battery_sample_callback_t callback);

/**
 * @brief Configura carga rápida (100mA)
 * 
 * Ativa o pino charge_speed (nível alto) para selecionar corrente de 100mA.
 * Recomendado quando há energia estável disponível (USB conectado).
 *
 * @return 0 se bem-sucedido, -ECANCELED se não inicializado
 */
int battery_set_fast_charge(void);

/**
 * @brief Configura carga lenta (50mA)
 * 
 * Desativa o pino charge_speed (nível baixo) para selecionar corrente de 50mA.
 * Recomendado para prolongar vida útil da bateria ou quando fonte é limitada.
 *
 * @return 0 se bem-sucedido, -ECANCELED se não inicializado
 */
int battery_set_slow_charge(void);

/**
 * @brief Obtém a tensão atual da bateria em milivolts
 * 
 * Realiza leitura do ADC (múltiplas amostras), aplica divisor resistivo
 * (R1=1037kΩ, R2=510kΩ calibrado) e filtros de estabilização.
 * Usa mutex para proteção contra leituras concorrentes.
 *
 * @param[out] battery_millivolt Ponteiro onde a tensão será armazenada
 * @return 0 se bem-sucedido, código de erro negativo em falha
 */
int battery_get_millivolt(uint16_t *battery_millivolt);

/**
 * @brief Calcula percentual da bateria baseado na tensão
 * 
 * Usa interpolação linear entre 11 pontos de referência (4000mV=100% até
 * 3300mV=0%) otimizados para bateria LiPo típica. Ajuste battery_states[]
 * no .c conforme datasheet da sua bateria.
 *
 * @param[out] battery_percentage Ponteiro onde o percentual (0-100) será armazenado
 * @param[in] battery_millivolt Tensão em mV para calcular o percentual
 * @return 0 se bem-sucedido, -ESPIPE se fora dos limites conhecidos
 */
int battery_get_percentage(uint8_t *battery_percentage, uint16_t battery_millivolt);

/**
 * @brief Inicia amostragem periódica da tensão da bateria
 * 
 * Agenda trabalho delayable que executa a cada interval_ms. A cada ciclo,
 * lê a tensão via ADC e notifica todos os callbacks registrados.
 *
 * @param[in] interval_ms Intervalo de amostragem em milissegundos (deve ser >0)
 * @return 0 se bem-sucedido, -EINVAL se interval_ms for zero
 * @note Callbacks registrados são chamados automaticamente a cada amostra
 */
int battery_start_sampling(uint32_t interval_ms);

/**
 * @brief Para a amostragem periódica da tensão da bateria
 * 
 * Cancela o trabalho delayable agendado. Não afeta callbacks já registrados.
 *
 * @return 0 sempre (sucesso)
 */
int battery_stop_sampling(void);

/**
 * @brief Realiza uma única leitura de tensão da bateria
 * 
 * Agenda trabalho imediato (não periódico) para ler tensão uma vez.
 * Útil para leituras sob demanda sem iniciar amostragem contínua.
 *
 * @return 0 sempre (trabalho agendado com sucesso)
 * @note Callbacks registrados são chamados quando a amostra estiver pronta
 */
int battery_sample_once(void);

/**
 * @brief Inicializa o sistema de gerenciamento de bateria
 * 
 * Configura:
 * - ADC canal 7 (AIN7/P0.31) para leitura de tensão
 * - GPIO charging_enable como entrada com interrupção (detecção de carga)
 * - GPIO read_enable como saída (habilita leitura do divisor)
 * - GPIO charge_speed como saída (controle 50mA/100mA)
 * - Callbacks e workers para amostragem e notificações
 * 
 * Deve ser chamada antes de qualquer outra função deste módulo.
 *
 * @return 0 se bem-sucedido, código de erro negativo em falha
 */
int battery_init(void);

#endif
