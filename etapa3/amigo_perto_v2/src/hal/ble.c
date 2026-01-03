/*
 * HAL BLE - Hardware Abstraction Layer para Bluetooth Low Energy
 * Copyright (c) 2025
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Este módulo implementa o controle do stack Bluetooth Low Energy,
 * encapsulando as APIs do Zephyr e fornecendo interface simplificada.
 *
 * Arquitetura:
 * - Inicialização do stack BLE e registro de callbacks
 * - Controle de advertising (start com parâmetros customizados)
 * - Gerenciamento automático de conexões via callbacks do Zephyr
 * - Notificação de eventos para a aplicação via callbacks
 */

#include "hal/ble.h"

// Zephyr includes
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

// Bluetooth includes
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>

// Serviço GATT customizado
#include "gatt/buzzer_service.h"

// Registra módulo de logging
LOG_MODULE_REGISTER(hal_ble, LOG_LEVEL_DBG);

// === CONSTANTES E CONFIGURAÇÕES ===

// Valores padrão para advertising quando adv_params é NULL
#define DEFAULT_ADV_INTERVAL_MIN_MS     500   // 500ms entre anúncios
#define DEFAULT_ADV_INTERVAL_MAX_MS     500   // 500ms entre anúncios

// Limites dos parâmetros de advertising (conforme especificação Bluetooth)
#define ADV_INTERVAL_MIN_MS             20    // Mínimo permitido pela spec
#define ADV_INTERVAL_MAX_MS             10240 // Máximo permitido pela spec

// Tamanho máximo do nome do dispositivo no advertising data
#define MAX_DEVICE_NAME_LEN             29

// Conversão: BLE usa unidades de 0.625ms, precisamos converter de milissegundos
// Fórmula: unidades_ble = ms * (1000 / 625) = ms * 8 / 5
#define MS_TO_BLE_UNITS(ms)             ((ms) * 8 / 5)

// === VARIÁVEIS PRIVADAS (Estado interno do módulo) ===

// Flag indicando se o módulo foi inicializado com sucesso
static bool initialized = false;

// Callbacks fornecidos pela aplicação para notificação de eventos
static hal_ble_callbacks_t user_callbacks = {0};

// Nome do dispositivo usado no advertising (armazenado localmente)
static char device_name[MAX_DEVICE_NAME_LEN + 1] = {0};

// Referência à conexão BLE atual (NULL = desconectado)
// O Zephyr usa reference counting, precisamos chamar bt_conn_ref/unref
static struct bt_conn *current_conn = NULL;

// Work item para iniciar advertising de forma assíncrona
// Usado para evitar bloqueios e permitir chamadas de contexto de callback
static struct k_work adv_work;

// Arrays com os dados de advertising (AD) e scan response (SD)
// AD: dados enviados no pacote principal de advertising
// SD: dados adicionais enviados quando um scanner solicita mais informações
static struct bt_data ad_data[2];  // [0]=Flags, [1]=Nome
static struct bt_data sd_data[2];  // [0]=UUID16 (Battery), [1]=UUID128 (Buzzer)
static size_t ad_data_count = 0;
static size_t sd_data_count = 0;

// Armazenamento para parâmetros customizados de advertising
static struct bt_le_adv_param adv_param_storage; //onde os parâmetros customizados ficam armazenados
static const struct bt_le_adv_param *adv_param = NULL; //qual configuração será usada (um ponteiro que pode apontar para o storage ou ser NULL para usar padrão)

// === CALLBACKS DO STACK BLUETOOTH (chamados pelo Zephyr) ===

/**
 * Callback chamado pelo stack Zephyr quando uma conexão BLE é estabelecida
 * 
 * Este callback é invocado automaticamente quando um dispositivo central
 * (ex: smartphone) completa o processo de conexão com nosso periférico.
 * 
 * Fluxo:
 * 1. Verifica se houve erro na conexão
 * 2. Se erro, reinicia advertising para aceitar nova tentativa
 * 3. Se sucesso, armazena referência da conexão
 * 4. Lê parâmetros negociados (intervalo, latência, timeout)
 * 5. Notifica a aplicação via callback user_callbacks.connected
 * 
 * Parâmetros:
 *   conn: ponteiro para estrutura de conexão do Zephyr
 *   err: 0=sucesso, outro valor=código de erro
 */
static void on_connected(struct bt_conn *conn, uint8_t err)
{
	// Se houve erro ao estabelecer conexão
	if (err) 
	{
		LOG_ERR("Conexão falhou (err %u)", err);
		// Reinicia advertising para aceitar novas tentativas
		k_work_submit(&adv_work);
		return;
	}
	
	// Armazena referência da conexão (incrementa ref count)
	// Se já tínhamos uma conexão anterior, libera ela primeiro
	if (current_conn) 
	{
		bt_conn_unref(current_conn);
	}
	current_conn = bt_conn_ref(conn);
	
	// Lê informações da conexão estabelecida
	struct bt_conn_info info;
	if (bt_conn_get_info(conn, &info) == 0) 
	{
		LOG_INF("Conectado - Intervalo: %u, Latência: %u, Timeout: %u", 
		        info.le.interval, info.le.latency, info.le.timeout);
	}
	
	// Notifica a aplicação se callback foi registrado
	if (user_callbacks.connected) 
	{
		// Converte informações do Zephyr para formato do HAL
		hal_ble_conn_info_t conn_info = {0};
		
		if (bt_conn_get_info(conn, &info) == 0) 
		{
			// Conversões de unidades:
			// - Intervalo: unidades de 1.25ms -> milissegundos
			// - Timeout: unidades de 10ms -> milissegundos
			conn_info.interval_ms = info.le.interval * 1250 / 1000;
			conn_info.latency = info.le.latency;
			conn_info.timeout_ms = info.le.timeout * 10;
		}
		
		user_callbacks.connected(&conn_info);
	}
}

/**
 * Callback chamado pelo stack Zephyr quando uma conexão BLE é encerrada
 * 
 * Motivos comuns de desconexão:
 * - 0x13: Usuário encerrou a conexão remotamente
 * - 0x08: Timeout de conexão (dispositivos muito distantes)
 * - 0x16: Conexão encerrada pelo host local
 * 
 * Parâmetros:
 *   conn: ponteiro para estrutura de conexão do Zephyr
 *   reason: código HCI indicando motivo da desconexão
 */
static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Desconectado (motivo 0x%02X)", reason);
	
	// Libera referência da conexão (decrementa ref count)
	if (current_conn) 
	{
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}
	
	// Notifica a aplicação se callback foi registrado
	if (user_callbacks.disconnected) 
	{
		user_callbacks.disconnected(reason);
	}
}

/**
 * Callback chamado quando o objeto de conexão é reciclado pelo Zephyr
 * 
 * O Zephyr possui um pool limitado de objetos bt_conn. Quando uma conexão
 * é encerrada e o objeto é liberado para reutilização, este callback é chamado.
 * Aproveitamos para reiniciar o advertising automaticamente.
 */
static void on_recycled(void)
{
	LOG_DBG("Conexão reciclada - reiniciando advertising");
	// Reinicia advertising para aceitar novas conexões
	k_work_submit(&adv_work);
}

// Estrutura de callbacks de conexão registrada no Zephyr
static struct bt_conn_cb conn_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
	.recycled = on_recycled,
};

// === FUNÇÕES PRIVADAS - CONTROLE DE ADVERTISING ===

/**
 * Handler do work item para iniciar advertising
 * 
 * Esta função é executada de forma assíncrona pelo kernel do Zephyr quando
 * k_work_submit(&adv_work) é chamado. Usar work items permite iniciar advertising
 * de forma segura mesmo dentro de callbacks (contexto de interrupção).
 * 
 * Fluxo:
 * 1. Verifica se já está conectado (não pode fazer advertising conectado)
 * 2. Usa parâmetros customizados ou padrões (500ms)
 * 3. Chama API do Zephyr bt_le_adv_start()
 * 4. Notifica aplicação via callback adv_started
 */
static void adv_work_handler(struct k_work *work)
{
	// Não pode fazer advertising se já está conectado
	if (current_conn != NULL) 
	{
		LOG_WRN("Já conectado, não inicia advertising");
		return;
	}
	
	// Usa parâmetros customizados ou padrões
	const struct bt_le_adv_param *param = adv_param;
	if (!param) 
	{
		// Parâmetros padrão: conectável, 500ms, usa endereço fixo
		param = BT_LE_ADV_PARAM(
			(BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_USE_IDENTITY),
			MS_TO_BLE_UNITS(DEFAULT_ADV_INTERVAL_MIN_MS),
			MS_TO_BLE_UNITS(DEFAULT_ADV_INTERVAL_MAX_MS),
			NULL
		);
	}
	
	// Inicia advertising com os dados preparados em prepare_adv_data()
	// ad_data: dados principais (flags + nome)
	// sd_data: scan response (UUID do serviço)
	int err = bt_le_adv_start(param, ad_data, ad_data_count, sd_data, sd_data_count);
	if (err) 
	{
		LOG_ERR("Advertising falhou (err %d)", err);
		return;
	}
	
	LOG_INF("Advertising iniciado");
	
	// Notifica aplicação se callback foi registrado
	if (user_callbacks.adv_started) 
	{
		user_callbacks.adv_started();
	}
}

/**
 * Prepara os dados de advertising e scan response
 * 
 * O advertising BLE possui dois tipos de dados:
 * 
 * 1. ADVERTISING DATA (AD): Enviado em todo pacote de advertising
 *    - Flags: indica modo de descoberta e recursos suportados
 *    - Nome: nome completo do dispositivo (para identificação)
 * 
 * 2. SCAN RESPONSE DATA (SD): Enviado apenas quando scanner solicita
 *    - UUIDs: lista de serviços GATT oferecidos pelo dispositivo
 * 
 * Esta abordagem otimiza o uso de espaço (advertising data é limitado a 31 bytes)
 * e energia (scan response só é enviado quando necessário).
 */
static void prepare_adv_data(void)
{
	size_t name_len = strlen(device_name);
	
	// --- ADVERTISING DATA ---
	ad_data_count = 0; //Zerar o “contador de itens” do advertising
	
	// Flags: General Discoverable + BR/EDR Not Supported
	// General Discoverable: dispositivo sempre detectável
	// BR/EDR Not Supported: apenas BLE, sem Bluetooth Classic

	ad_data[ad_data_count].type = BT_DATA_FLAGS; //Definir o tipo do campo atual como FLAGS
	ad_data[ad_data_count].data_len = 1; //Definir que esse campo tem 1 byte de dados
	static const uint8_t flags = BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR; //Criar o byte de flags (duas flags): BT_LE_AD_GENERAL e BT_LE_AD_NO_BREDR
	ad_data[ad_data_count].data = &flags; //Apontar o campo de dados para o byte de flags
	ad_data_count++; //Avançar para o próximo campo do advertising
	//BT_LE_AD_GENERAL: “modo descobrível geral” → o dispositivo está anunciando que pode ser encontrado normalmente (não é “limitado por tempo”)
	//BT_LE_AD_NO_BREDR: “não suporta BR/EDR” → ou seja, não tem Bluetooth clássico, é somente BLE
	
	// Nome completo do dispositivo
	if (name_len > 0) 
	{
		ad_data[ad_data_count].type = BT_DATA_NAME_COMPLETE; //Define o tipo do campo como o nome completo do dispositivo
		ad_data[ad_data_count].data_len = name_len; //Define o tamanho do campo como o tamanho do nome
		ad_data[ad_data_count].data = (const uint8_t *)device_name; //Aponta o campo de dados para o nome do dispositivo
		ad_data_count++; //Avança para o próximo campo do advertising
	}
	
	// --- SCAN RESPONSE DATA ---
	sd_data_count = 0; //Zerar o “contador de itens” do scan response

	// UUID do serviço padrão Battery Service (0x180F)
	// Lista de UUID16 é little-endian: 0x180F -> { 0x0F, 0x18 }
	static const uint8_t bas_uuid16[] = { 0x0F, 0x18 }; //Colocar um serviço padrão (Battery Service 0x180F) na scan response

	sd_data[sd_data_count].type = BT_DATA_UUID16_ALL; //Diz que “o campo é uma lista de UUID16"
	sd_data[sd_data_count].data_len = sizeof(bas_uuid16); //Define o tamanho do campo como 2 bytes (tamanho do UUID16)
	sd_data[sd_data_count].data = bas_uuid16; //Aponta o campo de dados para o array do UUID16
	sd_data_count++; //Avança para o próximo campo do scan response
	
	// UUID do serviço customizado Buzzer Service
	// Declarado como static para garantir que o ponteiro permanece válido
	static const struct bt_uuid_128 buzzer_uuid = BT_UUID_INIT_128(BT_UUID_BUZZER_SERVICE_VAL); //Coloca um serviço customizado (UUID 128-bit do Buzzer Service)
	
	sd_data[sd_data_count].type = BT_DATA_UUID128_ALL; //Diz que “o campo é uma lista de UUID128”
	sd_data[sd_data_count].data_len = 16;  // UUID 128 bits = 16 bytes
	sd_data[sd_data_count].data = buzzer_uuid.val; //Apontar para os 16 bytes do UUID
	sd_data_count++; //Avança para o próximo campo do scan response
}

// === API PÚBLICA ===

/**
 * Inicializa o subsistema BLE
 * 
 * Esta é a primeira função que deve ser chamada antes de qualquer outra
 * operação BLE. Ela executa as seguintes etapas:
 * 
 * 1. Valida parâmetros de entrada
 * 2. Armazena nome do dispositivo e callbacks
 * 3. Habilita o stack Bluetooth do Zephyr (bt_enable)
 * 4. Registra callbacks de conexão no stack
 * 5. Prepara dados de advertising
 * 6. Inicializa work item para advertising assíncrono
 * 
 * IMPORTANTE: Chame esta função DEPOIS de inicializar os serviços GATT,
 * pois prepare_adv_data() referencia UUIDs dos serviços.
 */
int hal_ble_init(const char *device_name_param, const hal_ble_callbacks_t *callbacks)
{
	// Previne reinicialização
	if (initialized) 
	{
		LOG_WRN("HAL BLE já inicializado");
		return HAL_BLE_SUCCESS;
	}
	
	// Validação: nome do dispositivo é obrigatório
	if (!device_name_param) 
	{
		LOG_ERR("Nome do dispositivo não pode ser NULL");
		return HAL_BLE_ERROR_INVALID;
	}
	
	// Validação: nome não pode exceder limite do advertising data
	size_t name_len = strlen(device_name_param);
	if (name_len > MAX_DEVICE_NAME_LEN) 
	{
		LOG_ERR("Nome do dispositivo muito longo (max %d caracteres)", MAX_DEVICE_NAME_LEN);
		return HAL_BLE_ERROR_INVALID;
	}
	
	// Armazena nome do dispositivo localmente
	strncpy(device_name, device_name_param, MAX_DEVICE_NAME_LEN);
	device_name[MAX_DEVICE_NAME_LEN] = '\0';
	
	// Armazena callbacks se fornecidos
	if (callbacks) 
	{
		user_callbacks = *callbacks;
	}
	
	// Habilita o stack Bluetooth do Zephyr
	// NULL = inicialização síncrona (bloqueia até completar)
	int err = bt_enable(NULL);
	if (err) 
	{
		LOG_ERR("Falha ao habilitar Bluetooth (err %d)", err);
		return HAL_BLE_ERROR_INIT;
	}
	
	LOG_INF("Bluetooth habilitado");
	
	// Registra callbacks de conexão no stack Zephyr
	// Isso permite receber notificações de connected/disconnected/recycled
	bt_conn_cb_register(&conn_callbacks);
	
	// Prepara os dados que serão enviados no advertising
	// (flags, nome, UUIDs de serviços)
	prepare_adv_data();
	
	// Inicializa work item para advertising assíncrono
	// Permite iniciar advertising de forma segura de qualquer contexto
	k_work_init(&adv_work, adv_work_handler);
	
	// Marca módulo como inicializado
	initialized = true;
	
	LOG_INF("HAL BLE inicializado - Device: %s", device_name);
	return HAL_BLE_SUCCESS;
}

/**
 * Inicia o advertising BLE
 * 
 * Após chamar hal_ble_init(), use esta função para começar a anunciar o
 * dispositivo. Outros dispositivos BLE poderão descobrir e conectar.
 * 
 * O advertising pode ser customizado via adv_params:
 * - Intervalo: tempo entre pacotes de advertising (afeta consumo de energia)
 * - Connectable: se permite conexões ou é apenas beacon
 * - Use identity: usar MAC fixo (rastreavel) ou aleatório (privacidade)
 * 
 * Se adv_params = NULL, usa valores padrão (500ms, conectável, MAC fixo)
 * 
 * 1. Se não inicializou → erro.
 * 2. Se já conectado → erro.
 * 3. Se passou parâmetros → valida, traduz e salva.
 * 4. Agenda início do advertising.
 * 5. Retorna sucesso.
 * 
 * IMPORTANTE: O advertising para automaticamente quando uma conexão é
 * estabelecida. Use o callback adv_stopped para detectar isso.
 */
int hal_ble_start_advertising(const hal_ble_adv_params_t *adv_params)
{
	// Verifica se módulo foi inicializado
	if (!initialized) 
	{
		LOG_ERR("HAL BLE não inicializado");
		return HAL_BLE_ERROR_STATE;
	}
	
	// Não pode fazer advertising se já está conectado - current_conn guarda a conexão atual.
	if (current_conn != NULL) 
	{
		LOG_WRN("Já conectado, não pode iniciar advertising");
		return HAL_BLE_ERROR_STATE;
	}
	
	// Configura parâmetros customizados se fornecidos
	if (adv_params) //Se adv_params != NULL, o usuário quer customizar
	{
		// Validação: intervalos devem estar dentro dos limites da spec
		if (adv_params->interval_min_ms < ADV_INTERVAL_MIN_MS || //interval_min_ms não pode ser menor que o mínimo permitido (20ms).
		    adv_params->interval_min_ms > ADV_INTERVAL_MAX_MS || //interval_min_ms não pode ser maior que o máximo permitido (10240ms).
		    adv_params->interval_max_ms < ADV_INTERVAL_MIN_MS || //interval_max_ms não pode ser menor que o mínimo permitido (20ms).
		    adv_params->interval_max_ms > ADV_INTERVAL_MAX_MS || //interval_max_ms não pode ser maior que o máximo permitido (10240ms).
		    adv_params->interval_min_ms > adv_params->interval_max_ms) 
		{
			LOG_ERR("Parâmetros de advertising inválidos");
			return HAL_BLE_ERROR_INVALID;
		}
		
		// Monta opções de advertising
		uint32_t options = 0; //options é um conjunto de “flags” (bits) que dizem como o advertising deve ser

		if (adv_params->connectable) //connectable == true
		{
			options |= BT_LE_ADV_OPT_CONN;  // Aceita conexões
		}
		if (adv_params->use_identity) //use_identity == true
		{
			options |= BT_LE_ADV_OPT_USE_IDENTITY;  // Usa endereço MAC fixo 
		}
		
		// Preenche estrutura de parâmetros do Zephyr - Aqui acontece uma “tradução” como que o que o Zephyr usa 
		adv_param_storage.id = 0; //usa a identidade 0
		adv_param_storage.sid = 0; //set de advertising (para extended advertising), aqui fixo em 0
		adv_param_storage.secondary_max_skip = 0; //não pula eventos de advertising secundário
		adv_param_storage.options = options; //aplica as flags montadas
		adv_param_storage.interval_min = MS_TO_BLE_UNITS(adv_params->interval_min_ms); //converte ms → unidades BLE usando MS_TO_BLE_UNITS(ms) (ms * 8 / 5).
		adv_param_storage.interval_max = MS_TO_BLE_UNITS(adv_params->interval_max_ms); //converte ms → unidades BLE usando MS_TO_BLE_UNITS(ms) (ms * 8 / 5).
		adv_param_storage.peer = NULL; //peer = NULL: não é advertising direcionado a um peer específico
		
		adv_param = &adv_param_storage; //uarda um ponteiro para esses parâmetros, para serem usados mais tarde ao iniciar de fato o advertising.
		
		LOG_DBG("Parâmetros de advertising configurados: %u-%u ms",
		        adv_params->interval_min_ms, adv_params->interval_max_ms);
	} 
	else 
	{
		// Usa parâmetros padrão (serão aplicados em adv_work_handler)
		adv_param = NULL; //(500ms, conectável, MAC fixo)
	}
	
	// Submete work item para iniciar advertising de forma assíncrona
	// Isso permite chamar esta função de qualquer contexto (thread, callback, ISR)
	k_work_submit(&adv_work); //Em vez de iniciar advertising imediatamente, ele agenda um trabalho (adv_work) para rodar depois
	
	return HAL_BLE_SUCCESS;
}






