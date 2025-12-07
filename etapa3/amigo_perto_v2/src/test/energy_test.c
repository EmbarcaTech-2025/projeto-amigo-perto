/*
 * Energy Consumption Test Module - Implementation
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "test/energy_test.h"
#include "hal/buzzer.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include <string.h>

LOG_MODULE_REGISTER(energy_test, LOG_LEVEL_INF);

// === ARMAZENAMENTO DE RESULTADOS (RAM) ===
typedef struct {
	energy_test_results_t baseline;
	energy_test_results_t buzzer;
	bool valid;
} stored_results_t;

static stored_results_t stored_results = {.valid = false};

// === CONFIGURAÇÕES DO ADC DIFERENCIAL ===

// Usaremos dois canais ADC adjacentes para medição diferencial
// AIN0 (P0.02) = lado positivo do shunt (BAT+)
// AIN1 (P0.03) = lado negativo do shunt (conectado à bateria)
#define ADC_NODE              DT_NODELABEL(adc)
#define ADC_CHANNEL_POSITIVE  0    // AIN0
#define ADC_CHANNEL_NEGATIVE  1    // AIN1

#define ADC_RESOLUTION        14   // 14 bits para maior precisão
#define ADC_GAIN              ADC_GAIN_1   // Ganho 1 (±0.6V)
#define ADC_REFERENCE         ADC_REF_INTERNAL  // Ref 0.6V
#define ADC_ACQUISITION_TIME  ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40)

// Resistor shunt: 1 Ω
#define SHUNT_RESISTANCE_OHM  1.0f

// Taxa de amostragem: 10 Hz (100ms entre leituras)
#define SAMPLING_PERIOD_MS    100

// Número de amostras para oversampling
#define OVERSAMPLING_COUNT    4

// Tensão de referência interna do nRF52840
#define ADC_VREF_MV           600

// === VARIÁVEIS PRIVADAS ===

static const struct device *adc_dev;
static bool initialized = false;
static bool measurement_running = false;

// Acumuladores para integração temporal
static double energy_accumulator_mws = 0.0;  // mW·s
static uint32_t measurement_start_time = 0;
static uint32_t sample_count = 0;

// Configuração dos canais ADC
static struct adc_channel_cfg channel_cfg_pos = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_CHANNEL_POSITIVE,
	.differential = 1,  // Modo diferencial
#ifdef CONFIG_ADC_NRFX_SAADC
	.input_positive = SAADC_CH_PSELP_PSELP_AnalogInput0 + ADC_CHANNEL_POSITIVE,
	.input_negative = SAADC_CH_PSELN_PSELN_AnalogInput0 + ADC_CHANNEL_NEGATIVE,
#endif
};

// Buffer para amostras ADC
static int16_t adc_sample_buffer[OVERSAMPLING_COUNT];

// Configuração da sequência de leitura
static struct adc_sequence sequence = {
	.channels = BIT(ADC_CHANNEL_POSITIVE),
	.resolution = ADC_RESOLUTION,
};

// Work item para amostragem periódica
static struct k_work_delayable sampling_work;
static void sampling_work_handler(struct k_work *work);

// Forward declarations
static void energy_test_save_baseline(const energy_test_results_t *results);
static void energy_test_save_buzzer(const energy_test_results_t *results);

// === FUNÇÕES AUXILIARES PRIVADAS ===

/**
 * @brief Converte valor bruto do ADC diferencial para tensão em mV
 * 
 * Em modo diferencial com ganho 1 e referência 0.6V:
 * - Resolução: 14 bits = 16384 valores (-8192 a +8191)
 * - Range: ±0.6V
 * - Conversão: V = (raw / 8192) × 600mV
 */
static inline float adc_raw_to_mv(int16_t adc_value)
{
	// Com 14 bits: full scale = 2^13 = 8192
	const int32_t full_scale = (1 << (ADC_RESOLUTION - 1));
	
	// Tensão em mV: (valor / full_scale) × referência
	float voltage_mv = ((float)adc_value / (float)full_scale) * (float)ADC_VREF_MV;
	
	return voltage_mv;
}

/**
 * @brief Realiza leitura do ADC com oversampling
 * 
 * @param voltage_mv Ponteiro para armazenar tensão em mV
 * @return 0 em sucesso, negativo em erro
 */
static int adc_read_differential(float *voltage_mv)
{
	int32_t sum = 0;
	uint8_t valid_samples = 0;
	
	sequence.buffer = adc_sample_buffer;
	sequence.buffer_size = sizeof(adc_sample_buffer);
	
	for (int i = 0; i < OVERSAMPLING_COUNT; i++)
	{
		int ret = adc_read(adc_dev, &sequence);
		if (ret < 0)
		{
			LOG_ERR("Erro na leitura ADC: %d", ret);
			continue;
		}
		
		int16_t raw_value = adc_sample_buffer[0];
		sum += raw_value;
		valid_samples++;
		
		k_msleep(1);  // Pequeno delay entre amostras
	}
	
	if (valid_samples == 0)
	{
		LOG_ERR("Nenhuma leitura ADC válida");
		return -EIO;
	}
	
	// Calcula média
	int16_t avg_raw = sum / valid_samples;
	*voltage_mv = adc_raw_to_mv(avg_raw);
	
	LOG_DBG("ADC diferencial: raw=%d, voltage=%.3f mV", avg_raw, (double)*voltage_mv);
	
	return 0;
}

/**
 * @brief Handler periódico para amostragem durante medição
 */
static void sampling_work_handler(struct k_work *work)
{
	if (!measurement_running)
	{
		return;
	}
	
	// Lê tensão no shunt
	float voltage_shunt_mv;
	int ret = adc_read_differential(&voltage_shunt_mv);
	if (ret < 0)
	{
		LOG_WRN("Falha na leitura, ignorando amostra");
		goto reschedule;
	}
	
	// Calcula corrente: I = V / R (mA)
	// V em mV, R em Ω → I em mA
	float current_ma = voltage_shunt_mv / SHUNT_RESISTANCE_OHM;
	
	// Calcula potência: P = V × I (mW)
	// Tensão da bateria é aproximadamente 3.7V (típico LiPo)
	// Para maior precisão, poderia ler do ADC de bateria
	const float battery_voltage_v = 3.7f;
	float power_mw = battery_voltage_v * current_ma;
	
	// Integra energia: E += P × Δt
	// Δt em segundos = SAMPLING_PERIOD_MS / 1000
	float delta_t_s = (float)SAMPLING_PERIOD_MS / 1000.0f;
	energy_accumulator_mws += (double)power_mw * (double)delta_t_s;
	
	sample_count++;
	
	LOG_DBG("Amostra #%u: V=%.3f mV, I=%.3f mA, P=%.3f mW, E=%.3f mWs",
	        sample_count, (double)voltage_shunt_mv, (double)current_ma, 
	        (double)power_mw, energy_accumulator_mws);

reschedule:
	// Reagenda próxima amostragem
	k_work_schedule(&sampling_work, K_MSEC(SAMPLING_PERIOD_MS));
}

// === API PÚBLICA ===

int energy_test_init(void)
{
	if (initialized)
	{
		LOG_WRN("Módulo de teste de energia já inicializado");
		return ENERGY_TEST_SUCCESS;
	}
	
	// Obtém device do ADC
	adc_dev = DEVICE_DT_GET(ADC_NODE);
	if (!device_is_ready(adc_dev))
	{
		LOG_ERR("ADC device não está pronto");
		return ENERGY_TEST_ERROR_INIT;
	}
	
	// Configura canal diferencial
	int ret = adc_channel_setup(adc_dev, &channel_cfg_pos);
	if (ret < 0)
	{
		LOG_ERR("Falha ao configurar canal ADC diferencial: %d", ret);
		return ENERGY_TEST_ERROR_INIT;
	}
	
	// Inicializa work item
	k_work_init_delayable(&sampling_work, sampling_work_handler);
	
	initialized = true;
	
	LOG_INF("=== Módulo de Teste de Energia Inicializado ===");
	LOG_INF("Configuração:");
	LOG_INF("  - Shunt: %.1f Ω", (double)SHUNT_RESISTANCE_OHM);
	LOG_INF("  - ADC: %d bits, diferencial", ADC_RESOLUTION);
	LOG_INF("  - Taxa de amostragem: %d Hz", 1000 / SAMPLING_PERIOD_MS);
	LOG_INF("  - Oversampling: %d amostras", OVERSAMPLING_COUNT);
	
	return ENERGY_TEST_SUCCESS;
}

void energy_test_start(void)
{
	if (!initialized)
	{
		LOG_ERR("Módulo não inicializado");
		return;
	}
	
	LOG_INF("=== INICIANDO MEDIÇÃO DE ENERGIA ===");
	
	// Reseta acumuladores
	energy_accumulator_mws = 0.0;
	sample_count = 0;
	measurement_start_time = k_uptime_get_32();
	measurement_running = true;
	
	// Inicia amostragem periódica
	k_work_schedule(&sampling_work, K_MSEC(SAMPLING_PERIOD_MS));
}

int energy_test_stop(energy_test_results_t *results)
{
	if (!initialized)
	{
		LOG_ERR("Módulo não inicializado");
		return ENERGY_TEST_ERROR_INIT;
	}
	
	if (!measurement_running)
	{
		LOG_WRN("Nenhuma medição em andamento");
		return ENERGY_TEST_ERROR_READ;
	}
	
	// Para amostragem
	measurement_running = false;
	k_work_cancel_delayable(&sampling_work);
	
	// Calcula duração
	uint32_t duration_ms = k_uptime_get_32() - measurement_start_time;
	
	// Converte energia para mWh: 1 mWh = 3600 mWs
	float energy_mwh = (float)(energy_accumulator_mws / 3600.0);
	
	// Calcula potência média
	double duration_s = (double)duration_ms / 1000.0;
	float avg_power_mw = (duration_s > 0) ? (float)(energy_accumulator_mws / duration_s) : 0.0f;
	
	// Preenche estrutura de resultados
	if (results != NULL)
	{
		results->energy_mwh = energy_mwh;
		results->duration_ms = duration_ms;
		results->avg_power_mw = avg_power_mw;
		
		// Calcula valores médios
		if (sample_count > 0)
		{
			results->current_ma = avg_power_mw / 3.7f;  // Aproximação com Vbat=3.7V
			results->voltage_shunt_mv = results->current_ma * SHUNT_RESISTANCE_OHM;
		}
		else
		{
			results->current_ma = 0.0f;
			results->voltage_shunt_mv = 0.0f;
		}
	}
	
	LOG_INF("=== MEDIÇÃO CONCLUÍDA ===");
	LOG_INF("Duração: %u.%03u s", duration_ms / 1000, duration_ms % 1000);
	LOG_INF("Amostras: %u", sample_count);
	LOG_INF("Energia: %.6f mWh", (double)energy_mwh);
	LOG_INF("Potência média: %.3f mW", (double)avg_power_mw);
	LOG_INF("Corrente média: %.3f mA", (double)results->current_ma);
	
	return ENERGY_TEST_SUCCESS;
}

int energy_test_get_instant(float *current_ma, float *power_mw)
{
	if (!initialized)
	{
		return ENERGY_TEST_ERROR_INIT;
	}
	
	// Lê tensão no shunt
	float voltage_shunt_mv;
	int ret = adc_read_differential(&voltage_shunt_mv);
	if (ret < 0)
	{
		return ENERGY_TEST_ERROR_READ;
	}
	
	// Calcula corrente
	float i_ma = voltage_shunt_mv / SHUNT_RESISTANCE_OHM;
	
	// Calcula potência (usando Vbat típico)
	float p_mw = 3.7f * i_ma;
	
	if (current_ma != NULL)
	{
		*current_ma = i_ma;
	}
	
	if (power_mw != NULL)
	{
		*power_mw = p_mw;
	}
	
	return ENERGY_TEST_SUCCESS;
}

int energy_test_run_baseline(energy_test_results_t *results)
{
	LOG_INF("========================================");
	LOG_INF("  TESTE DE CONSUMO BASELINE (60s)");
	LOG_INF("========================================");
	LOG_INF("Sistema sem buzzer - medindo consumo base");
	LOG_INF("");
	
	// Inicia medição
	energy_test_start();
	
	// Aguarda 60 segundos
	for (int i = 1; i <= 60; i++)
	{
		k_sleep(K_SECONDS(1));
		
		// Atualiza a cada 10 segundos
		if (i % 10 == 0)
		{
			float current_ma, power_mw;
			if (energy_test_get_instant(&current_ma, &power_mw) == ENERGY_TEST_SUCCESS)
			{
				LOG_INF("[%02ds] I=%.3f mA, P=%.3f mW", 
				        i, (double)current_ma, (double)power_mw);
			}
		}
	}
	
	// Para e obtém resultados
	int ret = energy_test_stop(results);
	
	if (ret == ENERGY_TEST_SUCCESS && results != NULL)
	{
		LOG_INF("");
		LOG_INF("=== RESULTADOS BASELINE ===");
		LOG_INF("Energia consumida: %.6f mWh", (double)results->energy_mwh);
		LOG_INF("Taxa horária: %.3f mWh/h", (double)results->energy_mwh);
		LOG_INF("Potência média: %.3f mW", (double)results->avg_power_mw);
		LOG_INF("Corrente média: %.3f mA", (double)results->current_ma);
	}
	
	// Salva resultados baseline
	energy_test_save_baseline(results);
	
	return ret;
}

int energy_test_run_with_buzzer(energy_test_results_t *results)
{
	LOG_INF("========================================");
	LOG_INF("  TESTE DE CONSUMO COM BUZZER (60s)");
	LOG_INF("========================================");
	LOG_INF("Buzzer ativo - medindo consumo total");
	LOG_INF("");
	
	// Liga buzzer
	int ret = hal_buzzer_set_intermittent(true, HAL_BUZZER_INTENSITY_MEDIUM);
	if (ret != HAL_BUZZER_SUCCESS)
	{
		LOG_ERR("Falha ao ligar buzzer: %d", ret);
		return ENERGY_TEST_ERROR_INIT;
	}
	
	// Inicia medição
	energy_test_start();
	
	// Aguarda 60 segundos
	for (int i = 1; i <= 60; i++)
	{
		k_sleep(K_SECONDS(1));
		
		// Atualiza a cada 10 segundos
		if (i % 10 == 0)
		{
			float current_ma, power_mw;
			if (energy_test_get_instant(&current_ma, &power_mw) == ENERGY_TEST_SUCCESS)
			{
				LOG_INF("[%02ds] I=%.3f mA, P=%.3f mW", 
				        i, (double)current_ma, (double)power_mw);
			}
		}
	}
	
	// Desliga buzzer
	hal_buzzer_set_intermittent(false, 0);
	
	// Para e obtém resultados
	ret = energy_test_stop(results);
	
	if (ret == ENERGY_TEST_SUCCESS && results != NULL)
	{
		LOG_INF("");
		LOG_INF("=== RESULTADOS COM BUZZER ===");
		LOG_INF("Energia consumida: %.6f mWh", (double)results->energy_mwh);
		LOG_INF("Taxa horária: %.3f mWh/h", (double)results->energy_mwh);
		LOG_INF("Potência média: %.3f mW", (double)results->avg_power_mw);
		LOG_INF("Corrente média: %.3f mA", (double)results->current_ma);
	}
	
	// Salva resultados com buzzer
	energy_test_save_buzzer(results);
	
	return ret;
}

// === COMANDOS SHELL PARA VISUALIZAÇÃO ===

#ifdef CONFIG_SHELL

static int cmd_energy_show_results(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	
	if (!stored_results.valid)
	{
		shell_print(sh, "");
		shell_print(sh, "Nenhum resultado disponível na memória.");
		shell_print(sh, "Execute o teste com a placa alimentada por bateria primeiro,");
		shell_print(sh, "então conecte o USB para ver os resultados.");
		shell_print(sh, "");
		shell_print(sh, "IMPORTANTE: Não reinicie a placa antes de conectar o USB!");
		shell_print(sh, "");
		return 0;
	}
	
	shell_print(sh, "");
	shell_print(sh, "========================================");
	shell_print(sh, "  RESULTADOS DO TESTE DE ENERGIA");
	shell_print(sh, "========================================");
	shell_print(sh, "");
	
	shell_print(sh, "=== TESTE BASELINE (60s sem buzzer) ===");
	shell_print(sh, "Energia: %.6f mWh", (double)stored_results.baseline.energy_mwh);
	shell_print(sh, "Potência média: %.3f mW", (double)stored_results.baseline.avg_power_mw);
	shell_print(sh, "Corrente média: %.3f mA", (double)stored_results.baseline.current_ma);
	shell_print(sh, "Duração: %u ms", stored_results.baseline.duration_ms);
	shell_print(sh, "");
	
	shell_print(sh, "=== TESTE COM BUZZER (60s ativo) ===");
	shell_print(sh, "Energia: %.6f mWh", (double)stored_results.buzzer.energy_mwh);
	shell_print(sh, "Potência média: %.3f mW", (double)stored_results.buzzer.avg_power_mw);
	shell_print(sh, "Corrente média: %.3f mA", (double)stored_results.buzzer.current_ma);
	shell_print(sh, "Duração: %u ms", stored_results.buzzer.duration_ms);
	shell_print(sh, "");
	
	float overhead_mwh = stored_results.buzzer.energy_mwh - stored_results.baseline.energy_mwh;
	float overhead_pct = (stored_results.baseline.energy_mwh > 0) ? 
	                     (overhead_mwh / stored_results.baseline.energy_mwh * 100.0f) : 0.0f;
	
	shell_print(sh, "=== ANÁLISE COMPARATIVA ===");
	shell_print(sh, "Consumo baseline: %.6f mWh/min", (double)stored_results.baseline.energy_mwh);
	shell_print(sh, "Consumo com buzzer: %.6f mWh/min", (double)stored_results.buzzer.energy_mwh);
	shell_print(sh, "Overhead do buzzer: %.6f mWh/min (+%.1f%%)", (double)overhead_mwh, (double)overhead_pct);
	shell_print(sh, "");
	
	shell_print(sh, "=== PROJEÇÃO HORÁRIA ===");
	shell_print(sh, "Baseline: %.3f mWh/h", (double)stored_results.baseline.energy_mwh * 60.0);
	shell_print(sh, "Com buzzer: %.3f mWh/h", (double)stored_results.buzzer.energy_mwh * 60.0);
	shell_print(sh, "Overhead: %.3f mWh/h", (double)overhead_mwh * 60.0);
	shell_print(sh, "");
	
	// Estimativa de autonomia com bateria de 100mAh @ 3.7V = 370mWh
	const float battery_capacity_mwh = 370.0f;
	float hours_baseline = battery_capacity_mwh / (stored_results.baseline.energy_mwh * 60.0f);
	float hours_buzzer = battery_capacity_mwh / (stored_results.buzzer.energy_mwh * 60.0f);
	
	shell_print(sh, "=== ESTIMATIVA DE AUTONOMIA (bateria 100mAh @ 3.7V) ===");
	shell_print(sh, "Modo standby: %.1f horas (%.1f dias)", (double)hours_baseline, (double)hours_baseline / 24.0);
	shell_print(sh, "Buzzer contínuo: %.1f horas", (double)hours_buzzer);
	shell_print(sh, "========================================");
	shell_print(sh, "");
	
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_energy,
	SHELL_CMD(show, NULL, "Mostra resultados do teste de energia", cmd_energy_show_results),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(energy, &sub_energy, "Comandos de teste de energia", NULL);

#endif /* CONFIG_SHELL */

// === FUNÇÕES AUXILIARES PARA SALVAR RESULTADOS ===

static void energy_test_save_baseline(const energy_test_results_t *results)
{
	if (results != NULL)
	{
		stored_results.baseline = *results;
	}
}

static void energy_test_save_buzzer(const energy_test_results_t *results)
{
	if (results != NULL)
	{
		stored_results.buzzer = *results;
		stored_results.valid = true;  // Marca como válido após ambos os testes
	}
}
