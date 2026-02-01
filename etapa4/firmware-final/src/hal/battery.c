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
 * - Leitura de tensão via ADC (AIN7/P0.31) com divisor resistivo calibrado
 * - Detecção de estado de carregamento via GPIO (P0.17 active-low)
 * - Controle de velocidade de carga (P0.13: 50mA/100mA)
 * - Amostragem periódica com workqueue do Zephyr
 * - Filtros de estabilização: trimmed mean, IIR, spike rejection
 * - Sistema de callbacks para notificação de eventos
 * 
 * Hardware:
 * - Divisor resistivo: R1=1037kΩ (calibrado), R2=510kΩ
 * - ADC: 12-bit, VREF=0.6V, gain=1/6 (range 0-3.6V)
 * - Amostragem: 10 samples @ 500µs interval
 * 
 * Copyright 2024 Marcus Alexander Tjomsaas
 * SPDX-License-Identifier: Apache-2.0
 */

#include "battery.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <errno.h>

LOG_MODULE_REGISTER(battery, LOG_LEVEL_INF);

// === VALIDAÇÃO DO DEVICE TREE ===
// Garante em compile-time que o nó xiao_ble_battery_dev existe no DT
#if !DT_NODE_EXISTS(DT_NODELABEL(xiao_ble_battery_dev))
#error "Overlay for xiao_ble_battery_dev node not properly defined."
#endif

// Referência ao nó do Device Tree para bateria
#define BATTERY_NODE DT_NODELABEL(xiao_ble_battery_dev)
// Número máximo de callbacks permitidos (configurável via DT)
#define BATTERY_CALLBACK_MAX DT_PROP(BATTERY_NODE, battery_callbacks_max)

// Número de amostras ADC por leitura (padrão: 10)
// Aumentar melhora média mas aumenta tempo de bloqueio do ADC
#define ADC_TOTAL_SAMPLES DT_PROP(BATTERY_NODE, adc_total_samples)

// === CONFIGURAÇÃO DO ADC ===
// Parâmetros extraídos do Device Tree para leitura de tensão da bateria

#define ADC_RESOLUTION          DT_PROP(BATTERY_NODE, adc_resolution)  // Resolução em bits (padrão: 12-bit = 4096 níveis)
#define ADC_CHANNEL             DT_PROP(BATTERY_NODE, adc_channel_id)  // ID do canal ADC (7 = AIN7)
#define ADC_PORT                DT_PROP(BATTERY_NODE, adc_channel)     // Porta física do ADC (NRF_SAADC_AIN7)
#define ADC_REFERENCE           DT_PROP(BATTERY_NODE, adc_reference)   // Tensão de referência (0.6V)
#define ADC_GAIN                DT_PROP(BATTERY_NODE, adc_gain)        // Ganho (1/6 para range 0-3.6V)
#define ADC_SAMPLE_INTERVAL_US  DT_PROP(BATTERY_NODE, adc_sample_interval)  // Intervalo entre amostras (500µs)
#define ADC_ACQUISITION_TIME    DT_PROP(BATTERY_NODE, adc_acquisition_time) // Tempo de aquisição 

// Configuração do canal 7 do ADC para leitura de bateria
static struct adc_channel_cfg channel_7_cfg = {
    .gain = ADC_GAIN,                      // Ganho do ADC (1/6)
    .reference = ADC_REFERENCE,            // Tensão de referência (0.6V)
    .acquisition_time = ADC_ACQUISITION_TIME,  // Tempo de aquisição do sample
    .channel_id = ADC_CHANNEL,             // ID do canal (7)
#ifdef CONFIG_ADC_NRFX_SAADC
    .input_positive = ADC_PORT             // Entrada positiva (AIN7) - específico Nordic nRF
#endif
};

// Opções de sequenciamento do ADC (múltiplas amostras)
static struct adc_sequence_options options = {
    .extra_samplings = ADC_TOTAL_SAMPLES - 1,  // Amostras adicionais (total = 1 + extra)
    .interval_us = ADC_SAMPLE_INTERVAL_US,     // Intervalo entre cada amostra (500µs)
};

// Buffer para armazenar as amostras do ADC (10 amostras de 16-bit)
static int16_t sample_buffer[ADC_TOTAL_SAMPLES];

// Configuração da sequência de leitura do ADC
static struct adc_sequence sequence = {
    .options = &options,               // Opções de sequenciamento
    .channels = BIT(ADC_CHANNEL),      // Máscara de bits indicando canal ativo
    .buffer = sample_buffer,           // Buffer onde amostras serão armazenadas
    .buffer_size = sizeof(sample_buffer),  // Tamanho do buffer em bytes
    .resolution = ADC_RESOLUTION,      // Resolução da conversão (12-bit)
};

// === VARIÁVEIS LOCAIS ===

// Periféricos do MCU para gerenciamento de bateria
// Dispositivo ADC para leitura de tensão
static const struct device *adc_battery_dev = DEVICE_DT_GET(DT_NODELABEL(adc));

// GPIO P0.17 (entrada, active-low): detecta se carregador está conectado
static const struct gpio_dt_spec charging_enable = GPIO_DT_SPEC_GET_OR(BATTERY_NODE, charging_enable_gpios, {0});
// GPIO P0.14 (saída, active-low): habilita divisor resistivo para leitura
static const struct gpio_dt_spec read_enable = GPIO_DT_SPEC_GET_OR(BATTERY_NODE, read_enable_gpios, {0});
// GPIO P0.13 (saída, active-low): controla velocidade de carga (0=50mA, 1=100mA)
static const struct gpio_dt_spec charge_speed = GPIO_DT_SPEC_GET_OR(BATTERY_NODE, charge_speed_gpios, {0});

// Workers do Zephyr para amostragem periódica e única
static struct k_work_delayable sample_periodic_work;  // Trabalho delayable para amostragem contínua
static struct k_work sample_once_work;                // Trabalho imediato para amostragem única

// Sistema de interrupção para detecção de mudança no estado de carga
static struct gpio_callback charging_callback;        // Estrutura de callback GPIO
static struct k_work charging_interrupt_work;         // Trabalho executado quando interrupção dispara

// Array de callbacks registrados para mudança de estado de carregamento
static battery_charging_callback_t charging_callbacks[BATTERY_CALLBACK_MAX];
static size_t charging_callbacks_registered = 0;      // Contador de callbacks registrados

// Array de callbacks registrados para quando nova amostra de tensão está pronta
static battery_sample_callback_t sample_ready_callback[BATTERY_CALLBACK_MAX];
static size_t sample_ready_callbacks_registered = 0;  // Contador de callbacks registrados

// Intervalo de amostragem periódica (em ms)
static uint32_t sampling_interval_ms;
// Flag indicando se o módulo foi inicializado
static uint8_t is_initialized = false;

// Mutex para proteger leitura ADC de acessos concorrentes
static K_MUTEX_DEFINE(battery_mut);

// Estrutura que mapeia tensão para percentual de carga
typedef struct
{
    uint16_t voltage;      // Tensão em milivolts
    uint8_t percentage;    // Percentual de carga (0-100%)
} BatteryState;

#define BATTERY_STATES_COUNT 11
// Tabela de referência para bateria LiPo típica (3.7V nominal)
// Valores calibrados para interpolação linear entre pontos
// AJUSTE CONFORME DATASHEET da sua bateria para maior precisão
static BatteryState battery_states[BATTERY_STATES_COUNT] = {
    {4000, 100},  // Totalmente carregada (4.0V)
    {3930, 90},   // 90%
    {3860, 80},   // 80%
    {3790, 70},   // 70%
    {3720, 60},   // 60%
    {3650, 50},   // 50% (tensão nominal)
    {3580, 40},   // 40%
    {3510, 30},   // 30%
    {3440, 20},   // 20%
    {3370, 10},   // 10% (bateria baixa)
    {3300, 0}     // 0% - Tensão mínima segura (proteger de descarga profunda)
};

// === ESTABILIZAÇÃO DE LEITURA ADC ===
//
// O AIN7 pode mostrar jitter considerável mesmo com VBAT estável.
// Aplicamos três camadas de filtros para estabilizar a leitura:
// 1. Média aparada (trimmed mean): descarta amostra mínima e máxima
// 2. Filtro IIR passa-baixa: suavização temporal com alpha=1/8
// 3. Rejeição de spikes: ignora variações bruscas >150mV

#define VBAT_SPIKE_REJECT_DELTA_MV 150  // Threshold para rejeição de spikes (mV)
#define VBAT_IIR_ALPHA_SHIFT 3          // Shift para divisão por 8 (alpha = 1/8)

// Valor filtrado da tensão da bateria (resultado do IIR)
static uint16_t vbat_filtered_mv;
// Flag indicando se vbat_filtered_mv já foi inicializado
static bool vbat_filtered_valid;

/**
 * Verifica se um pino GPIO está ativo, respeitando polaridade do DT
 * 
 * Lê o nível físico do pino e interpreta conforme configuração active-high/active-low.
 * Para active-low: nível 0 = ativo, nível 1 = inativo
 * Para active-high: nível 1 = ativo, nível 0 = inativo
 * 
 * @param spec Especificação do GPIO do Device Tree
 * @return true se o pino está ativo, false caso contrário ou em erro
 */
static bool gpio_dt_is_active(const struct gpio_dt_spec *spec)
{
    // Lê o nível lógico do pino GPIO (0 ou 1)
    int level = gpio_pin_get_dt(spec);
    // Se a leitura falhou (retorno negativo), considera inativo
    if (level < 0)
    {
        return false;
    }

    // Interpreta o nível: diferente de zero = alto
    bool active = (level != 0);
    // Se o pino for configurado como active-low no DT, inverte a lógica
    if ((spec->dt_flags & GPIO_ACTIVE_LOW) != 0U)
    {
        active = !active;  // Nível baixo (0) passa a ser ativo
    }

    return active;
}

// === FUNÇÕES PRIVADAS ===

/**
 * Habilita o divisor resistivo para leitura de tensão da bateria
 * 
 * Ativa o pino read_enable (nível alto) para conectar o divisor ao ADC.
 * Deve ser chamado uma vez na inicialização.
 */
static int battery_enable_read()
{
    // Define pino read_enable em nível alto (ativo)
    return gpio_pin_set_dt(&read_enable, 1);
}

/**
 * Executa todos os callbacks registrados para mudança de estado de carga
 * 
 * Chamada pelo workqueue quando interrupção de charging_enable dispara.
 * Lê o estado atual do pino e notifica todos os interessados.
 * 
 * @param work Ponteiro para estrutura de trabalho (não usado)
 */
static void run_charging_callbacks(struct k_work *work)
{
    // Lê estado atual do pino (considerando active-low)
    bool is_charging = gpio_dt_is_active(&charging_enable);
    LOG_DBG("Charger %s", is_charging ? "connected" : "disconnected");

    // Notifica todos os callbacks registrados
    for (uint8_t callback = 0; callback < charging_callbacks_registered; callback++)
    {
        charging_callbacks[callback](is_charging);
    }
}

/**
 * Notifica todos os callbacks quando uma nova amostra de tensão está pronta
 * 
 * @param millivolt Tensão da bateria em milivolts (já filtrada)
 */
static void run_sample_ready_callbacks(uint32_t millivolt)
{
    // Percorre array de callbacks e notifica cada um
    for (uint8_t callback = 0; callback < sample_ready_callbacks_registered; callback++)
    {
        sample_ready_callback[callback](millivolt);
    }
}

/**
 * Handler de interrupção GPIO para detecção de mudança no estado de carga
 * 
 * Executado em contexto de ISR (interrupt service routine).
 * Apenas agenda trabalho para execução em contexto de thread.
 * 
 * @param dev Dispositivo GPIO que gerou a interrupção
 * @param cb Estrutura de callback
 * @param pins Máscara de bits indicando quais pinos dispararam
 */
static void charging_callback_handler(const struct device *dev,
                                      struct gpio_callback *cb,
                                      uint32_t pins)
{
    // Agenda trabalho para processar mudança de estado fora da ISR
    k_work_submit(&charging_interrupt_work);
}

/**
 * Handler de trabalho para amostragem periódica da bateria
 * 
 * Executado periodicamente pela workqueue do Zephyr.
 * Lê tensão, notifica callbacks e reagenda próxima execução.
 * 
 * @param work Ponteiro para estrutura de trabalho delayable
 */
static void sample_periodic_handler(struct k_work *work)
{
    uint16_t millivolt;
    // Lê tensão da bateria (com todos os filtros aplicados)
    int ret = battery_get_millivolt(&millivolt);
    if (ret)
    {
        LOG_ERR("Failed to get battery voltage");
        goto reschedule;  // Mesmo em erro, continua amostragem
    }

    // Notifica todos os callbacks com o valor lido
    run_sample_ready_callbacks(millivolt);

reschedule:
    // Reagenda próxima amostragem após intervalo configurado
    k_work_reschedule(&sample_periodic_work, K_MSEC(sampling_interval_ms));
}

/**
 * Handler de trabalho para amostragem única da bateria
 * 
 * Executado uma vez quando battery_sample_once() é chamada.
 * Não reagenda após execução.
 * 
 * @param work Ponteiro para estrutura de trabalho
 */
static void sample_once_handler(struct k_work *work)
{
    uint16_t millivolt;
    // Lê tensão da bateria
    int ret = battery_get_millivolt(&millivolt);
    if (ret)
    {
        LOG_ERR("Failed to get battery voltage");
        return;  // Em erro, apenas retorna (não reagenda)
    }

    // Notifica callbacks com o valor lido
    run_sample_ready_callbacks(millivolt);
}

// === FUNÇÕES PÚBLICAS (API) ===

/**
 * Registra callback para receber notificações de mudança de estado de carga
 * 
 * Permite que módulos externos sejam notificados quando carregador é
 * conectado/desconectado (via interrupção no pino charging_enable).
 */
int battery_register_charging_callback(battery_charging_callback_t callback)
{
    // Verifica se atingiu o limite máximo de callbacks
    if (charging_callbacks_registered == BATTERY_CALLBACK_MAX)
    {
        LOG_ERR("Maximum number of callbacks reached, operation aborted");
        return -ENOMEM;  // Erro: sem memória disponível
    }

    // Adiciona callback ao array e incrementa contador
    charging_callbacks[charging_callbacks_registered++] = callback;

    return 0;
}

/**
 * Registra callback para receber notificações quando amostra está pronta
 * 
 * Callback é chamado após cada leitura de tensão (periódica ou sob demanda),
 * recebendo o valor já filtrado em milivolts.
 */
int battery_register_sample_callback(battery_sample_callback_t callback)
{
    // Verifica se atingiu o limite máximo de callbacks
    if (sample_ready_callbacks_registered == BATTERY_CALLBACK_MAX)
    {
        LOG_ERR("Maximum number of callbacks reached, operation aborted");
        return -ENOMEM;
    }

    // Adiciona callback ao array e incrementa contador
    sample_ready_callback[sample_ready_callbacks_registered++] = callback;
    return 0;
}

/**
 * Configura carregador para modo rápido (100mA)
 * 
 * Ativa pino charge_speed para selecionar corrente de carga alta.
 */
int battery_set_fast_charge()
{
    // Verifica se módulo foi inicializado
    if (!is_initialized)
    {
        return -ECANCELED;
    }

    // Define pino em nível alto = 100mA
    return gpio_pin_set_dt(&charge_speed, 1);
}

/**
 * Configura carregador para modo lento (50mA)
 * 
 * Desativa pino charge_speed para selecionar corrente de carga baixa.
 */
int battery_set_slow_charge()
{
    // Verifica se módulo foi inicializado
    if (!is_initialized)
    {
        return -ECANCELED;
    }

    // Define pino em nível baixo = 50mA
    return gpio_pin_set_dt(&charge_speed, 0);
}

/**
 * Lê tensão da bateria com múltiplos filtros de estabilização
 * 
 * Processo:
 * 1. Adquire mutex para acesso exclusivo ao ADC
 * 2. Lê múltiplas amostras do ADC (10 por padrão)
 * 3. Calcula média aparada (descarta min/max)
 * 4. Converte ADC raw para milivolts
 * 5. Aplica fator do divisor resistivo (R1+R2)/R2
 * 6. Aplica filtro IIR passa-baixa e rejeição de spikes
 * 7. Libera mutex e retorna valor estabilizado
 */
int battery_get_millivolt(uint16_t *battery_millivolt)
{

    int ret = 0;

    // Parâmetros do divisor resistivo (calibrado empiricamente)
    const uint16_t R1 = 1037;  // Originalmente 1MΩ, ajustado após medições reais
                                // Variações devido a tolerância dos resistores (~5%) e temperatura
    const uint16_t R2 = 510;   // 510kΩ (tolerância típica)

    // Obtém tensão de referência interna do ADC
    uint16_t adc_vref = adc_ref_internal(adc_battery_dev);

    // Trava mutex para garantir acesso exclusivo ao ADC (timeout 10s)
    ret = k_mutex_lock(&battery_mut, K_SECONDS(10));
    if (ret < 0)
    {
        LOG_ERR("Cannot get battery voltage as mutex is locked");
        return ret;
    }

    // Executa sequência de leitura do ADC (múltiplas amostras)
    ret |= adc_read(adc_battery_dev, &sequence);

    if (ret)
    {
        LOG_WRN("ADC read failed (error %d)", ret);
    }

    // === FILTRO 1: MÉDIA APARADA (TRIMMED MEAN) ===
    // Remove outliers descartando valores mínimo e máximo antes de calcular média
    int32_t adc_sum = 0;
    int16_t adc_min = INT16_MAX;  // Inicializa com maior valor possível
    int16_t adc_max = INT16_MIN;  // Inicializa com menor valor possível
    
    // Primeira passada: soma tudo e identifica min/max
    for (uint8_t sample = 0; sample < ADC_TOTAL_SAMPLES; sample++)
    {
        int16_t v = sample_buffer[sample];
        adc_sum += v;
        if (v < adc_min)
        {
            adc_min = v;  // Atualiza mínimo
        }
        if (v > adc_max)
        {
            adc_max = v;  // Atualiza máximo
        }
    }

    uint32_t adc_average;
    // Se temos pelo menos 3 amostras, descarta min e max
    if (ADC_TOTAL_SAMPLES >= 3)
    {
        adc_sum -= adc_min;  // Remove amostra mínima
        adc_sum -= adc_max;  // Remove amostra máxima
        adc_average = (uint32_t)(adc_sum / (ADC_TOTAL_SAMPLES - 2));  // Média das restantes
    }
    else
    {
        // Com poucas amostras, calcula média simples
        adc_average = (uint32_t)(adc_sum / ADC_TOTAL_SAMPLES);
    }

    // Converte valor ADC raw (0-4095) para milivolts
    uint32_t adc_mv = adc_average;
    ret |= adc_raw_to_millivolts(adc_vref, ADC_GAIN, ADC_RESOLUTION, &adc_mv);

    // Aplica fator de escala do divisor resistivo para obter tensão real da bateria
    // VBAT = VADC * (R1 + R2) / R2
    // Com R1=1037k e R2=510k: scale_factor ≈ 3.033
    float scale_factor = ((float)(R1 + R2)) / R2;
    uint16_t vbat_mv = (uint16_t)(adc_mv * scale_factor);

    // === FILTROS 2 e 3: IIR PASSA-BAIXA + REJEIÇÃO DE SPIKES ===
    // Aplica suavização temporal para eliminar ruído e transições bruscas
    
    if (!vbat_filtered_valid)
    {
        // Primeira leitura: inicializa filtro IIR com valor atual
        vbat_filtered_mv = vbat_mv;
        vbat_filtered_valid = true;
    }
    else
    {
        // Calcula diferença absoluta entre leitura atual e valor filtrado
        int32_t diff = (int32_t)vbat_mv - (int32_t)vbat_filtered_mv;
        if (diff < 0)
        {
            diff = -diff;  // Valor absoluto
        }

        // REJEIÇÃO DE SPIKE: se variação muito grande, mantém valor anterior
        if (diff > VBAT_SPIKE_REJECT_DELTA_MV)
        {
            // Spike detectado (>150mV): descarta leitura, mantém valor estável
            vbat_mv = vbat_filtered_mv;
        }
        else
        {
            // FILTRO IIR: filtered += (new - filtered) / 8
            // Suaviza mudanças gradualmente (alpha = 1/8 = 12.5%)
            int32_t delta = (int32_t)vbat_mv - (int32_t)vbat_filtered_mv;
            vbat_filtered_mv = (uint16_t)((int32_t)vbat_filtered_mv + (delta >> VBAT_IIR_ALPHA_SHIFT));
            vbat_mv = vbat_filtered_mv;  // Usa valor filtrado
        }
    }

    // Armazena resultado final no ponteiro fornecido
    *battery_millivolt = vbat_mv;
    // Libera mutex para permitir novas leituras
    k_mutex_unlock(&battery_mut);

    LOG_DBG("%d mV", *battery_millivolt);
    return ret;
}

/**
 * Converte tensão da bateria para percentual de carga (0-100%)
 * 
 * Usa interpolação linear entre 11 pontos de referência calibrados
 * para bateria LiPo típica. Se tensão estiver fora da tabela, retorna
 * 100% (acima de 4.0V) ou 0% (abaixo de 3.3V).
 */
int battery_get_percentage(uint8_t *battery_percentage, uint16_t battery_millivolt)
{
    // Verifica limites superior e inferior da tabela
    if (battery_millivolt >= battery_states[0].voltage)
    {
        *battery_percentage = 100;  // Acima do máximo conhecido
        return 0;
    }
    else if (battery_millivolt <= battery_states[BATTERY_STATES_COUNT - 1].voltage)
    {
        *battery_percentage = 0;  // Abaixo do mínimo seguro
        return 0;
    }

    // Busca os dois pontos de referência que envolvem a tensão atual
    for (uint16_t i = 0; i < BATTERY_STATES_COUNT - 1; i++)
    {
        uint16_t voltage_high = battery_states[i].voltage;      // Ponto superior
        uint16_t voltage_low = battery_states[i + 1].voltage;   // Ponto inferior

        // Verifica se tensão atual está entre esses dois pontos
        if (battery_millivolt <= voltage_high && battery_millivolt >= voltage_low)
        {
            uint8_t percentage_high = battery_states[i].percentage;      // % do ponto superior
            uint8_t percentage_low = battery_states[i + 1].percentage;   // % do ponto inferior

            // Calcula ranges para interpolação linear
            int32_t voltage_range = voltage_high - voltage_low;          // Diferença de tensão
            int32_t percentage_range = percentage_high - percentage_low; // Diferença de %
            int32_t voltage_diff = battery_millivolt - voltage_low;      // Distância do ponto inferior

            // Previne divisão por zero (caso dois pontos tenham mesma tensão)
            if (voltage_range == 0)
            {
                *battery_percentage = percentage_high;
            }
            else
            {
                // Interpolação linear: % = %_low + (diff * range_%) / range_V
                *battery_percentage = percentage_low + (voltage_diff * percentage_range) / voltage_range;
            }

            LOG_DBG("%d %%", *battery_percentage);
            return 0;
        }
    }

    // Se chegou aqui, tensão não está em nenhum intervalo conhecido
    return -ESPIPE;
}

/**
 * Inicia amostragem periódica da bateria
 * 
 * Agenda trabalho delayable que executa continuamente a cada interval_ms.
 * Callbacks registrados são notificados a cada nova leitura.
 */
int battery_start_sampling(uint32_t interval_ms)
{
    // Valida parâmetro: intervalo deve ser maior que zero
    if (interval_ms == 0)
    {
        LOG_ERR("Sampling interval must be greater than zero");
        return -EINVAL;
    }

    // Armazena intervalo para uso nos reagendamentos
    sampling_interval_ms = interval_ms;
    // Agenda primeira execução do trabalho
    k_work_schedule(&sample_periodic_work, K_MSEC(interval_ms));

    LOG_INF("Start sampling battery voltage at %d ms", interval_ms);
    return 0;
}

/**
 * Para a amostragem periódica da bateria
 * 
 * Cancela o trabalho delayable agendado, interrompendo o ciclo de leituras.
 */
int battery_stop_sampling(void)
{
    // Cancela qualquer execução agendada
    k_work_cancel_delayable(&sample_periodic_work);
    LOG_INF("Stopped periodic sampling of battery voltage");
    return 0;
}

/**
 * Realiza uma única leitura de tensão sob demanda
 * 
 * Agenda trabalho imediato (não periódico) para uma leitura.
 * Útil quando não se quer amostragem contínua.
 */
int battery_sample_once(void)
{
    // Submete trabalho para execução imediata
    k_work_submit(&sample_once_work);
    return 0;
}

/**
 * Inicializa todo o sistema de gerenciamento de bateria
 * 
 * Etapas:
 * 1. Configura ADC para leitura de tensão
 * 2. Configura GPIOs (entrada: charging_enable, saídas: read_enable, charge_speed)
 * 3. Inicializa workers para amostragem
 * 4. Configura interrupção para detecção de carga
 * 5. Habilita leitura e carga rápida
 * 
 * Deve ser chamada antes de usar qualquer outra função do módulo.
 */
int battery_init()
{
    int ret = 0;

    // === ETAPA 1: CONFIGURAÇÃO DO ADC ===
    // Verifica se dispositivo ADC está pronto para uso
    if (!device_is_ready(adc_battery_dev))
    {
        LOG_ERR("ADC device not found!");
        return -EIO;  // Erro de I/O
    }

    // Configura canal 7 do ADC com parâmetros definidos
    ret |= adc_channel_setup(adc_battery_dev, &channel_7_cfg);
    if (ret)
    {
        LOG_ERR("ADC setup failed (error %d)", ret);
        return ret;
    }

    // === ETAPA 2: VERIFICAÇÃO DE GPIOs ===
    // Verifica se todos os GPIOs necessários estão disponíveis
    
    // GPIO P0.17 - Detecção de estado de carregamento (entrada)
    if (!gpio_is_ready_dt(&charging_enable))
    {
        LOG_ERR("GPIO charging_enable not found!");
        return -EIO;
    }

    // GPIO P0.14 - Habilitação de leitura do divisor (saída)
    if (!gpio_is_ready_dt(&read_enable))
    {
        LOG_ERR("GPIO read_enable not found!");
        return -EIO;
    }

    // GPIO P0.13 - Controle de velocidade de carga (saída)
    if (!gpio_is_ready_dt(&charge_speed))
    {
        LOG_ERR("GPIO charging_enable not found!");  // [BUG: mensagem deveria ser charge_speed]
        return -EIO;
    }

    // === ETAPA 3: CONFIGURAÇÃO DOS PINOS GPIO ===
    
    // Configura charging_enable como ENTRADA com polaridade active-low
    // Pino monitora estado do carregador (0=carregando, 1=não carregando)
    ret |= gpio_pin_configure_dt(&charging_enable, GPIO_INPUT | GPIO_ACTIVE_LOW);
    if (ret)
    {
        LOG_ERR("Failed to configure GPIO_BATTERY_CHARGING_ENABLE pin (error %d)", ret);
        return ret;
    }

    // Habilita interrupção em AMBAS as bordas (subida E descida)
    // Detecta tanto conexão quanto desconexão do carregador
    ret |= gpio_pin_interrupt_configure_dt(&charging_enable, GPIO_INT_EDGE_BOTH);
    if (ret)
    {
        LOG_ERR("Failed to configure GPIO_BATTERY_CHARGING_ENABLE pin interrupt (error %d)", ret);
        return ret;
    }

    // Configura read_enable como SAÍDA active-low
    // Controla habilitaƧão do divisor resistivo para leitura ADC
    ret |= gpio_pin_configure_dt(&read_enable, GPIO_OUTPUT | GPIO_ACTIVE_LOW);
    if (ret)
    {
        LOG_ERR("Failed to configure GPIO_BATTERY_READ_ENABLE pin (error %d)", ret);
        return ret;
    }
    
    // Configura charge_speed como SAÍDA active-low
    // Controla velocidade de carga: 0=50mA (lento), 1=100mA (rápido)
    ret |= gpio_pin_configure_dt(&charge_speed, GPIO_OUTPUT | GPIO_ACTIVE_LOW);
    if (ret)
    {
        LOG_ERR("Failed to configure GPIO_BATTERY_CHARGE_SPEED pin (error %d)", ret);
        return ret;
    }

    // === ETAPA 4: INICIALIZAÇÃO DE WORKERS E CALLBACKS ===
    
    // Inicializa trabalho delayable para amostragem periódica
    k_work_init_delayable(&sample_periodic_work, sample_periodic_handler);
    // Inicializa trabalho imediato para amostragem única
    k_work_init(&sample_once_work, sample_once_handler);

    // Inicializa trabalho para processar mudanças de estado de carga
    k_work_init(&charging_interrupt_work, run_charging_callbacks);
    // Configura callback GPIO para interrupção do pino charging_enable
    gpio_init_callback(&charging_callback, charging_callback_handler, BIT(charging_enable.pin));
    // Registra callback no driver GPIO
    gpio_add_callback_dt(&charging_enable, &charging_callback);

    // === ETAPA 5: VERIFICAÇÃO DE ESTADO INICIAL ===
    // Lê estado atual do carregador e faz log
    bool is_charging = gpio_dt_is_active(&charging_enable);
    LOG_INF("Charger %s", is_charging ? "connected" : "disconnected");

    // Marca módulo como inicializado (habilita outras funções)
    is_initialized = true;
    LOG_INF("Initialized");

    // Inicializa filtro IIR como inválido (primeira leitura irá popular)
    vbat_filtered_valid = false;

    // === ETAPA 6: CONFIGURAÇÃO INICIAL DE CARGA ===
    // Habilita carga rápida (100mA) por padrão
    ret |= battery_set_fast_charge();
    if (ret)
    {
        LOG_ERR("Failed to set fast charging (error %d)", ret);
        return ret;
    }

    // Habilita leitura de tensão da bateria (ativa divisor resistivo)
    ret |= battery_enable_read();
    if (ret)
    {
        LOG_ERR("Failed to enable battery reading (error %d)", ret);
        return ret;
    }

    return 0;  // Sucesso!
}
