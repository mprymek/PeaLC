#include <iec_std_lib.h>

#include "app_config.h"
#include "hal.h"
#include "io.h"
#include "locks.h"
#include "plc.h"
#include "uavcan_common.h" // node status constants
#include "ui.h"

// "exported" to Matiec-compiled PLC program
IEC_TIME __CURRENT_TIME;

static void connect_buffers(void);
static void plc_task(void *pvParameters);
static void update_time(void);
static void plc_run(void);
static void update_outputs(void);
static void update_inputs(void);

static TaskHandle_t plc_task_h = NULL;
static uint64_t tick = 0;

int plc_init()
{
	// initialize PLC program
	config_init__();
	connect_buffers();

	if (xTaskCreate(plc_task, "plc", STACK_SIZE_PLC, NULL,
			TASK_PRIORITY_PLC, &plc_task_h) != pdPASS) {
		log_error("Failed to create plc task.");
		die(DEATH_TASK_CREATION);
	}

	xEventGroupSetBits(global_event_group, PLC_INITIALIZED_BIT);

	return 0;
}

static void plc_task(void *pvParameters)
{
	if (!WAIT_BITS(PLC_RUNNING_BIT)) {
		die(DEATH_INITIALIZATION_TIMEOUT);
	}

	TickType_t last_wake = xTaskGetTickCount();

	for (;;) {
		update_time();

		if (IS_BIT_SET(PLC_RUNNING_BIT)) {
			ui_plc_tick();

#if LOGLEVEL >= LOGLEVEL_DEBUG
			float now_f =
				__CURRENT_TIME.tv_sec +
				__CURRENT_TIME.tv_nsec / (float)1000000000;
			PRINTF("\ntick=%llu    time=%.3fs    tick_time=%llums\n",
			       tick, now_f, common_ticktime__ / MILLION);

			// measure clock precision
			static uint32_t last_time = 0;
			uint32_t now = hal_uptime_usec();
			int32_t tdiff = now - last_time;
			PRINTF("time=%dus    precision: %d - %lld = %lldus\n",
			       now, tdiff, (common_ticktime__ / 1000),
			       tdiff - (common_ticktime__ / 1000));
			last_time = now;
#endif

			update_inputs();
			// execute plc program
			config_run__(tick);
			update_outputs();
		}

		vTaskDelayUntil(&last_wake,
				pdMS_TO_TICKS(common_ticktime__ / MILLION));
	}
}

void plc_set_state(plc_state_t state)
{
	switch (state) {
	case PLC_STATE_RUNNING:
		xEventGroupSetBits(global_event_group, PLC_RUNNING_BIT);
		ui_set_status("running");
#ifdef WITH_MQTT
		mqtt_publish5(MQTT_STATUS_TOPIC, MQTT_STATUS_RUNNING_MSG, 0, 1,
			      1);
#endif
		uavcan_node_status.mode = UAVCAN_NODE_MODE_1_0_OPERATIONAL;
		break;
	case PLC_STATE_PAUSED:
		xEventGroupClearBits(global_event_group, PLC_RUNNING_BIT);
		ui_set_status("paused");
#ifdef WITH_MQTT
		mqtt_publish5(MQTT_STATUS_TOPIC, MQTT_STATUS_PAUSED_MSG, 0, 1,
			      1);
#endif
		uavcan_node_status.mode = UAVCAN_NODE_MODE_1_0_MAINTENANCE;
		break;
	default:
		log_error("invalid plc state: %d", state);
	}
}

/*
NOTE: This is how __CURRENT_TIME is updated in OpenPLC. PLC clock can get
skewed if PLC task is not fired at the right moment. I don't know if
regular time increments are essential for OpenPLC runtime functions
so I think it's safer to stick with their implementation...
*/
static void update_time()
{
	__CURRENT_TIME.tv_nsec += common_ticktime__;
	while (__CURRENT_TIME.tv_nsec >= BILLION) {
		__CURRENT_TIME.tv_nsec -= BILLION;
		__CURRENT_TIME.tv_sec++;
	}

	tick++;
}

// ---------------------------------------------- program-specific vars --------

// generates something like:
//  BOOL __QX0_0;
#define __LOCATED_VAR(type, name, ...) type __##name;
#include "LOCATED_VARIABLES.h"
#undef __LOCATED_VAR

// generates something like:
//  BOOL *QX0_0 = &__QX0_0;
#define __LOCATED_VAR(type, name, ...) type *name = &__##name;
#include "LOCATED_VARIABLES.h"
#undef __LOCATED_VAR

// ---------------------------------------------- remote vars ------------------

#ifdef WITH_CAN
uavcan_vals_block_t uavcan_dis_blocks[] = UAVCAN_DIS_BLOCKS;
uavcan_vals_block_t uavcan_dos_blocks[] = UAVCAN_DOS_BLOCKS;
uavcan_vals_block_t uavcan_ais_blocks[] = UAVCAN_AIS_BLOCKS;
uavcan_vals_block_t uavcan_aos_blocks[] = UAVCAN_AOS_BLOCKS;

const uint8_t uavcan_dis_blocks_len =
	sizeof(uavcan_dis_blocks) / sizeof(uavcan_dis_blocks[0]);
const uint8_t uavcan_dos_blocks_len =
	sizeof(uavcan_dos_blocks) / sizeof(uavcan_dos_blocks[0]);
const uint8_t uavcan_ais_blocks_len =
	sizeof(uavcan_ais_blocks) / sizeof(uavcan_ais_blocks[0]);
const uint8_t uavcan_aos_blocks_len =
	sizeof(uavcan_aos_blocks) / sizeof(uavcan_aos_blocks[0]);
#endif // ifdef WITH_CAN

#if 0
uint8_t mqtt_dis_blocks_len = 0;
mqtt_vals_block_t mqtt_dis_blocks[] = {};

uint8_t mqtt_ais_blocks_len = 0;
mqtt_vals_block_t mqtt_ais_blocks[] = {};
#endif

// ---------------------------------------------- buffers ----------------------

//Booleans
IEC_BOOL *bool_input[IO_BUFFER_SIZE][8];
IEC_BOOL *bool_output[IO_BUFFER_SIZE][8];

//Bytes
//IEC_BYTE *byte_input[IO_BUFFER_SIZE];
//IEC_BYTE *byte_output[IO_BUFFER_SIZE];

//Analog I/O
IEC_UINT *int_input[IO_BUFFER_SIZE];
IEC_UINT *int_output[IO_BUFFER_SIZE];

//Memory
//IEC_UINT *int_memory[IO_BUFFER_SIZE];
//IEC_DINT *dint_memory[IO_BUFFER_SIZE];
//IEC_LINT *lint_memory[IO_BUFFER_SIZE];

//Special Functions
//IEC_LINT *special_functions[IO_BUFFER_SIZE];

// external vars buffers
#ifdef WITH_CAN
bool ext_dos[EXT_BUFF_SIZE];
uint16_t ext_aos[EXT_BUFF_SIZE];
bool ext_dis[EXT_BUFF_SIZE];
uint16_t ext_ais[EXT_BUFF_SIZE];
#endif

#define AIDX(i) ((i) / 8)
#define BIDX(i) ((i) % 8)

void connect_buffers()
{
	// connect program vars to IO buffer
#define POOL_BOOL_I bool_input
#define POOL_BOOL_Q bool_output
#define POOL_UINT_I int_input
#define POOL_UINT_Q int_output
#define INDEX_BOOL(a, b) [a][b]
#define INDEX_UINT(a, b) [a]

// generates something like:
//      POOL_BOOL_Q[0][0] = __QX0_0;
// i.e.
//      bool_output[0][0] = __QX0_0;
#define __LOCATED_VAR(type, name, inout, type_sym, a, b)                       \
	POOL_##type##_##inout INDEX_##type(a, b) = name;
#include "LOCATED_VARIABLES.h"
#undef __LOCATED_VAR

	uint8_t dis_idx = 0, ais_idx = 0, dos_idx = 0, aos_idx = 0;

#ifdef WITH_CAN
	log_debug("\nexternal vars blocks...");

	// connect UAVCAN blocks
	for (int i = 0; i < uavcan_dis_blocks_len; i++) {
		uavcan_vals_block_t *block = &uavcan_dis_blocks[i];
		log_debug("uavcan DI block: node=%d index=%d len=%d -> %d",
			  block->node_id, block->index, block->len, dis_idx);
		block->digital_vals = &ext_dis[dis_idx];
		dis_idx += block->len;
		// TODO: check bounds
	}
	for (int i = 0; i < uavcan_ais_blocks_len; i++) {
		uavcan_vals_block_t *block = &uavcan_ais_blocks[i];
		log_debug("uavcan AI block: node=%d index=%d len=%d -> %d",
			  block->node_id, block->index, block->len, ais_idx);
		block->analog_vals = &ext_ais[ais_idx];
		ais_idx += block->len;
		// TODO: check bounds
	}
	for (int i = 0; i < uavcan_dos_blocks_len; i++) {
		uavcan_vals_block_t *block = &uavcan_dos_blocks[i];
		log_debug("uavcan DO block: node=%d index=%d len=%d -> %d",
			  block->node_id, block->index, block->len, dos_idx);
		block->digital_vals = &ext_dos[dos_idx];
		dos_idx += block->len;
		// TODO: check bounds
	}
	for (int i = 0; i < uavcan_aos_blocks_len; i++) {
		uavcan_vals_block_t *block = &uavcan_aos_blocks[i];
		log_debug("uavcan AO block: node=%d index=%d len=%d -> %d",
			  block->node_id, block->index, block->len, aos_idx);
		block->analog_vals = &ext_aos[aos_idx];
		aos_idx += block->len;
		// TODO: check bounds
	}
#endif // ifdef WITH_CAN
}

// ---------------------------------------------- IO ---------------------------

void update_inputs()
{
	log_debug("updating inputs");

	// local
	for (uint8_t i = 0; i < REMOTE_VARS_INDEX; i++) {
		// digital
		uint8_t a = AIDX(i);
		uint8_t b = BIDX(i);
		IEC_BOOL *bval = bool_input[a][b];
		if (bval != NULL) {
			bool val2;
			io_get_di(i, &val2);
			log_debug("IX%u.%u = %u", a, b, val2);
			*bval = val2;
		}

		// analog
		IEC_UINT *uval = int_input[i];
		if (uval != NULL) {
			io_get_ai(i, uval);
			log_debug("IW%u.%u = %u", a, b, *uval);
		}
	}

#ifdef WITH_CAN
	// remote
	for (uint8_t i = REMOTE_VARS_INDEX; i < IO_BUFFER_SIZE; i++) {
		// digital
		uint8_t a = AIDX(i);
		uint8_t b = BIDX(i);
		IEC_BOOL *bval = bool_input[a][b];
		if (bval != NULL) {
			*bval = ext_dis[i - REMOTE_VARS_INDEX];
			log_debug("IX%u.%u (R) = %u", a, b, *bval);
		}

		// analog
		IEC_UINT *uval = int_input[i];
		if (uval != NULL) {
			*uval = ext_ais[i - REMOTE_VARS_INDEX];
			log_debug("IW%u.%u (R) = %u\n", a, b, *uval);
		}
	}
#endif // ifdef WITH_CAN
}

void update_outputs()
{
	log_debug("updating outputs");

	// local
	for (uint8_t i = 0; i < REMOTE_VARS_INDEX; i++) {
		// digital
		uint8_t a = AIDX(i);
		uint8_t b = BIDX(i);
		IEC_BOOL *bval = bool_output[a][b];
		if (bval != NULL) {
			log_debug("QX%u.%u = %u", a, b, *bval);
			io_set_do(i, *bval);
		}

		// analog
		IEC_UINT *uval = int_output[i];
		if (uval != NULL) {
			log_debug("QW%u.%u = %u", a, b, *uval);
			io_set_ao(i, *uval);
		}
	}

#ifdef WITH_CAN
	// remote
	for (uint8_t i = REMOTE_VARS_INDEX; i < IO_BUFFER_SIZE; i++) {
		// digital
		uint8_t a = AIDX(i);
		uint8_t b = BIDX(i);
		IEC_BOOL *bval = bool_output[a][b];
		if (bval != NULL) {
			log_debug("QX%u.%u (R) = %u\n", a, b, *bval);
			ext_dos[i - REMOTE_VARS_INDEX] = *bval;
		}

		// analog
		IEC_UINT *uval = int_output[i];
		if (uval != NULL) {
			log_debug("QW%u.%u (R) = %u\n", a, b, *uval);
			ext_aos[i - REMOTE_VARS_INDEX] = *uval;
		}
	}
#endif // ifdef WITH_CAN
}
