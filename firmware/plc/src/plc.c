#include <iec_std_lib.h>

#include "app_config.h"
#include "gpio.h"
#include "hal.h"
#include "locks.h"
#include "plc.h"
#include "sparkplug.h"
#include "tools.h"
#include "uavcan_common.h" // node status constants
#include "uavcan_impl.h"
#include "ui.h"

// "exported" to Matiec-compiled PLC program
IEC_TIME __CURRENT_TIME;

static void init_buffers(void);
static void plc_task(void *pvParameters);
static void update_time(void);
static void plc_run(void);
static void update_outputs(void);
static void update_inputs(void);
static int update_input_block(const io_type_t io_type, io_block_t *block);
static int update_output_block(const io_type_t io_type, io_block_t *block);

static TaskHandle_t plc_task_h = NULL;
static uint64_t tick = 0;

int plc_init()
{
	// initialize PLC program
	config_init__();
	init_buffers();

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
#ifdef WITH_CAN
		uavcan_node_status.mode = UAVCAN_NODE_MODE_1_0_OPERATIONAL;
#endif
		break;
	case PLC_STATE_PAUSED:
		xEventGroupClearBits(global_event_group, PLC_RUNNING_BIT);
		ui_set_status("paused");
#ifdef WITH_MQTT
		mqtt_publish5(MQTT_STATUS_TOPIC, MQTT_STATUS_PAUSED_MSG, 0, 1,
			      1);
#endif
#ifdef WITH_CAN
		uavcan_node_status.mode = UAVCAN_NODE_MODE_1_0_MAINTENANCE;
#endif
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

// ---------------------------------------------- buffers ----------------------

//Booleans
IEC_BOOL *bool_input[DIV_UP(DIS_BUFFER_SIZE, 8)][8];
IEC_BOOL *bool_output[DIV_UP(DOS_BUFFER_SIZE, 8)][8];

//Bytes
//IEC_BYTE *byte_input[IO_BUFFER_SIZE];
//IEC_BYTE *byte_output[IO_BUFFER_SIZE];

//Analog I/O
IEC_UINT *int_input[AIS_BUFFER_SIZE];
IEC_UINT *int_output[AOS_BUFFER_SIZE];

//Memory
//IEC_UINT *int_memory[IO_BUFFER_SIZE];
//IEC_DINT *dint_memory[IO_BUFFER_SIZE];
//IEC_LINT *lint_memory[IO_BUFFER_SIZE];

//Special Functions
//IEC_LINT *special_functions[IO_BUFFER_SIZE];

void init_buffers()
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

	PRINTF("DIs ");
	size_t addr = 0;
	for (size_t bi = 0; bi < digital_inputs_blocks_len; bi++) {
		io_block_t *block = &digital_inputs_blocks[bi];
		// TODO: check bounds

		// check input is used in program
		bool enabled = false;
		for (uint8_t ai = 0; ai < block->length; ai++) {
			if ((enabled = (bool_input[AIDX(addr + ai)]
						  [BIDX(addr + ai)] != NULL))) {
				break;
			}
		}
		block->enabled = enabled;
		log_debug(" block %u: addr=%u len=%u enabled=%u", bi, addr,
			  block->length, enabled);
		if (enabled) {
			if (block->driver_type == IO_DRIVER_GPIO) {
				gpio_init_input_block(IO_TYPE_DIGITAL, block);
			}
		}
		addr += block->length;
	}

	PRINTF("AIs ");
	addr = 0;
	for (size_t bi = 0; bi < analog_inputs_blocks_len; bi++) {
		io_block_t *block = &analog_inputs_blocks[bi];
		// TODO: check bounds

		// check input is used in program
		bool enabled = false;
		for (uint8_t ai = 0; ai < block->length; ai++) {
			if ((enabled = (int_input[addr + ai] != NULL))) {
				break;
			}
		}
		block->enabled = enabled;
		log_debug(" block %u: addr=%u len=%u enabled=%u", bi, addr,
			  block->length, enabled);
		if (enabled) {
			if (block->driver_type == IO_DRIVER_GPIO) {
				gpio_init_input_block(IO_TYPE_ANALOG, block);
			}
		}
		addr += block->length;
	}

	PRINTF("DOs ");
	addr = 0;
	for (size_t bi = 0; bi < digital_outputs_blocks_len; bi++) {
		io_block_t *block = &digital_outputs_blocks[bi];
		// TODO: check bounds

		// check output is used in program
		bool enabled = false;
		for (uint8_t ai = 0; ai < block->length; ai++) {
			if ((enabled = (bool_output[AIDX(addr + ai)][BIDX(
						addr + ai)] != NULL))) {
				break;
			}
		}
		block->enabled = enabled;
		log_debug(" block %u: addr=%u len=%u enabled=%u", bi, addr,
			  block->length, enabled);
		if (enabled) {
			if (block->driver_type == IO_DRIVER_GPIO) {
				gpio_init_output_block(IO_TYPE_DIGITAL, block);
			}
		}
		addr += block->length;
	}

	PRINTF("AOs ");
	addr = 0;
	for (size_t bi = 0; bi < analog_outputs_blocks_len; bi++) {
		io_block_t *block = &analog_outputs_blocks[bi];
		// TODO: check bounds

		// check output is used in program
		bool enabled = false;
		for (uint8_t ai = 0; ai < block->length; ai++) {
			if ((enabled = (int_output[addr + ai] != NULL))) {
				break;
			}
		}
		block->enabled = enabled;
		log_debug(" block %u: addr=%u len=%u enabled=%u", bi, addr,
			  block->length, enabled);
		if (enabled) {
			if (block->driver_type == IO_DRIVER_GPIO) {
				gpio_init_output_block(IO_TYPE_ANALOG, block);
			}
		}
		addr += block->length;
	}
}

// ---------------------------------------------- IO ---------------------------

#define GPIO_DIGITAL2(_inverted, ...)                                          \
	{                                                                      \
		.driver_type = IO_DRIVER_GPIO,                                 \
		.length = ARGS_NUM(__VA_ARGS__),                               \
		.buff = (bool[ARGS_NUM(__VA_ARGS__)]){},                       \
		.driver_data = &(gpio_io_block_t){                             \
			.inverted = _inverted,                                 \
			.pins = (uint8_t[]){ __VA_ARGS__ },                    \
		},                                                             \
	}

#define GPIO_DIGITAL(...) GPIO_DIGITAL2(FALSE, __VA_ARGS__)
#define GPIO_DIGITAL_INVERTED(...) GPIO_DIGITAL2(TRUE, __VA_ARGS__)

#define GPIO_ANALOG(...)                                                       \
	{                                                                      \
		.driver_type = IO_DRIVER_GPIO,                                 \
		.length = ARGS_NUM(__VA_ARGS__),                               \
		.buff = (uint16_t[ARGS_NUM(__VA_ARGS__)]){},                   \
		.driver_data = &(gpio_io_block_t){                             \
			.pins = (uint8_t[]){ __VA_ARGS__ },                    \
		},                                                             \
	}

#define UAVCAN_DIGITAL(_node_id, _index, _length)                              \
	{                                                                      \
		.driver_type = IO_DRIVER_UAVCAN, .length = _length,            \
		.buff = (bool[_length]){},                                     \
		.driver_data = &(uavcan_io_block_t){                           \
			.node_id = _node_id,                                   \
			.index = _index,                                       \
		},                                                             \
	}

#define UAVCAN_ANALOG(_node_id, _index, _length)                               \
	{                                                                      \
		.driver_type = IO_DRIVER_UAVCAN, .length = _length,            \
		.buff = (uint16_t[_length]){},                                 \
		.driver_data = &(uavcan_io_block_t){                           \
			.node_id = _node_id,                                   \
			.index = _index,                                       \
		},                                                             \
	}

#define SPARKPLUG_DIGITAL(_metric)                                             \
	{                                                                      \
		.driver_type = IO_DRIVER_SPARKPLUG, .length = 1,               \
		.buff = (bool[1]){},                                           \
		.driver_data = &(sp_io_block_t){                               \
			.metric = _metric,                                     \
		},                                                             \
	}

#define SPARKPLUG_ANALOG(_metric)                                              \
	{                                                                      \
		.driver_type = IO_DRIVER_SPARKPLUG, .length = 1,               \
		.buff = (uint16_t[1]){},                                       \
		.driver_data = &(sp_io_block_t){                               \
			.metric = _metric,                                     \
		},                                                             \
	}

#define GPIO GPIO_DIGITAL
#define GPIO_INVERTED GPIO_DIGITAL_INVERTED
#define UAVCAN UAVCAN_DIGITAL
#define SPARKPLUG SPARKPLUG_DIGITAL
io_block_t digital_inputs_blocks[] = DIGITAL_INPUTS;
io_block_t digital_outputs_blocks[] = DIGITAL_OUTPUTS;
#undef GPIO
#undef GPIO_INVERTED
#undef UAVCAN
#undef SPARKPLUG

const size_t digital_inputs_blocks_len =
	sizeof(digital_inputs_blocks) / sizeof(io_block_t);
const size_t digital_outputs_blocks_len =
	sizeof(digital_outputs_blocks) / sizeof(io_block_t);

#define GPIO GPIO_ANALOG
#define UAVCAN UAVCAN_ANALOG
#define SPARKPLUG SPARKPLUG_ANALOG
io_block_t analog_inputs_blocks[] = ANALOG_INPUTS;
io_block_t analog_outputs_blocks[] = ANALOG_OUTPUTS;
#undef GPIO
#undef UAVCAN
#undef SPARKPLUG

const size_t analog_inputs_blocks_len =
	sizeof(analog_inputs_blocks) / sizeof(io_block_t);
const size_t analog_outputs_blocks_len =
	sizeof(analog_outputs_blocks) / sizeof(io_block_t);

void update_inputs()
{
	log_debug("updating digital inputs");
	size_t addr = 0;
	for (size_t bi = 0; bi < digital_inputs_blocks_len; bi++) {
		io_block_t *block = &digital_inputs_blocks[bi];
		if (block->enabled) {
			update_input_block(IO_TYPE_DIGITAL, block);
			if (block->dirty) {
				for (size_t ai = 0; ai < block->length; ai++) {
					IEC_BOOL *value =
						bool_input[AIDX(addr + ai)]
							  [BIDX(addr + ai)];
					if (value) {
						*value = ((bool *)block
								  ->buff)[ai];
						log_debug("IX%u.%u = %u\n",
							  AIDX(addr + ai),
							  BIDX(addr + ai),
							  *value);
					}
				}
				block->dirty = false;
			}
		}
		addr += block->length;
	}

	log_debug("updating analog inputs");
	addr = 0;
	for (size_t bi = 0; bi < analog_inputs_blocks_len; bi++) {
		io_block_t *block = &analog_inputs_blocks[bi];
		if (block->enabled) {
			update_input_block(IO_TYPE_ANALOG, block);
			if (block->dirty) {
				for (size_t ai = 0; ai < block->length; ai++) {
					IEC_UINT *value = int_input[addr + ai];
					if (value) {
						*value = ((uint16_t *)block
								  ->buff)[ai];
						log_debug("IW%u = %u\n",
							  addr + ai, *value);
					}
				}
				block->dirty = false;
			}
		}
		addr += block->length;
	}
}

void update_outputs()
{
	log_debug("updating digital outputs");
	size_t addr = 0;
	for (size_t bi = 0; bi < digital_outputs_blocks_len; bi++) {
		io_block_t *block = &digital_outputs_blocks[bi];
		if (block->enabled) {
			// copy values to IO buffer
			for (size_t ai = 0; ai < block->length; ai++) {
				IEC_BOOL *value = bool_output[AIDX(addr + ai)]
							     [BIDX(addr + ai)];
				if (value) {
					bool *buff_value =
						&((bool *)block->buff)[ai];
					if (*value != *buff_value) {
						log_debug("QX%u.%u = %u\n",
							  AIDX(addr + ai),
							  BIDX(addr + ai),
							  *value);
						*buff_value = *value;
						block->dirty = true;
					}
				}
			}
			update_output_block(IO_TYPE_DIGITAL, block);
		}
		addr += block->length;
	}

	log_debug("updating analog outputs");
	addr = 0;
	for (size_t bi = 0; bi < analog_outputs_blocks_len; bi++) {
		io_block_t *block = &analog_outputs_blocks[bi];
		if (block->enabled) {
			// copy values to IO buffer
			for (size_t ai = 0; ai < block->length; ai++) {
				IEC_UINT *value = int_output[addr + ai];
				if (value) {
					uint16_t *buff_value =
						&((uint16_t *)block->buff)[ai];
					if (*value != *buff_value) {
						log_debug("QW%u = %u\n",
							  addr + ai, *value);
						*buff_value = *value;
						block->dirty = true;
					}
				}
			}
			update_output_block(IO_TYPE_ANALOG, block);
		}
		addr += block->length;
	}
}

// Most of the drivers update buffers asynchronously - no action needed here.
static int update_input_block(const io_type_t io_type, io_block_t *block)
{
	switch (block->driver_type) {
	case IO_DRIVER_GPIO:
		return gpio_update_input_block(io_type, block);
	case IO_DRIVER_SPARKPLUG:
	case IO_DRIVER_UAVCAN:
		/* noop */;
	}

	return IO_OK;
}

static int update_output_block(const io_type_t io_type, io_block_t *block)
{
	switch (block->driver_type) {
	case IO_DRIVER_GPIO:
		return gpio_update_output_block(io_type, block);
	case IO_DRIVER_SPARKPLUG:
		return sp_update_output_block(io_type, block);
	case IO_DRIVER_UAVCAN:
		/* noop */;
	}

	return IO_OK;
}
