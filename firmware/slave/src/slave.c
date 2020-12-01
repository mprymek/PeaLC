#if defined(ARDUINO)
// because of pin names like PC13 etc.
#include <Arduino.h>
#endif

#include <stdbool.h>

#include "app_config.h"
#include "gpio.h"
#include "hal.h"
#include "tm1638.h"
#include "tools.h"

static void init_io();

int slave_init()
{
	init_io();
	return 0;
}

// ---------------------------------------------- IO ---------------------------

static void init_io()
{
	size_t addr = 0;

	PRINTS("DIs ");
	addr = 0;
	for (size_t bi = 0; bi < digital_inputs_blocks_len; bi++) {
		io_block_t *block = &digital_inputs_blocks[bi];
		log_debug("DI block %u: addr=%u len=%u", bi, addr,
			  block->length);
		block->enabled = true;
		if (block->driver_type == IO_DRIVER_GPIO) {
			gpio_init_input_block(IO_TYPE_DIGITAL, block);
		}
		addr += block->length;
	}

	PRINTS("DOs ");
	addr = 0;
	for (size_t bi = 0; bi < digital_outputs_blocks_len; bi++) {
		io_block_t *block = &digital_outputs_blocks[bi];
		log_debug("DO block %u: addr=%u len=%u", bi, addr,
			  block->length);
		block->enabled = true;
		if (block->driver_type == IO_DRIVER_GPIO) {
			gpio_init_output_block(IO_TYPE_DIGITAL, block);
		}
		addr += block->length;
	}

	PRINTS("AIs ");
	addr = 0;
	for (size_t bi = 0; bi < analog_inputs_blocks_len; bi++) {
		io_block_t *block = &analog_inputs_blocks[bi];
		log_debug("AI block %u: addr=%u len=%u", bi, addr,
			  block->length);
		block->enabled = true;
		if (block->driver_type == IO_DRIVER_GPIO) {
			gpio_init_input_block(IO_TYPE_ANALOG, block);
		}
		addr += block->length;
	}

	PRINTS("AOs ");
	addr = 0;
	for (size_t bi = 0; bi < analog_outputs_blocks_len; bi++) {
		io_block_t *block = &analog_outputs_blocks[bi];
		log_debug("AO block %u: addr=%u len=%u", bi, addr,
			  block->length);
		block->enabled = true;
		if (block->driver_type == IO_DRIVER_GPIO) {
			gpio_init_output_block(IO_TYPE_ANALOG, block);
		}
		addr += block->length;
	}
}

int update_input_block(const io_type_t io_type, io_block_t *block)
{
	switch (block->driver_type) {
	case IO_DRIVER_GPIO:
		return gpio_update_input_block(io_type, block);
#ifdef WITH_TM1638
	case IO_DRIVER_TM1638:
		return tm1638_update_input_block(io_type, block);
#endif
	case IO_DRIVER_UAVCAN:
		/* not used in slave */;
	}

	return IO_OK;
}

int update_output_block(const io_type_t io_type, io_block_t *block)
{
	switch (block->driver_type) {
	case IO_DRIVER_GPIO:
		return gpio_update_output_block(io_type, block);
#ifdef WITH_TM1638
	case IO_DRIVER_TM1638:
		return tm1638_update_output_block(io_type, block);
#endif
	case IO_DRIVER_UAVCAN:
		/* not used in slave */;
	}

	return IO_OK;
}

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

#define GPIO_DIGITAL(...) GPIO_DIGITAL2(false, __VA_ARGS__)
#define GPIO_DIGITAL_INVERTED(...) GPIO_DIGITAL2(true, __VA_ARGS__)

#define GPIO_ANALOG(...)                                                       \
	{                                                                      \
		.driver_type = IO_DRIVER_GPIO,                                 \
		.length = ARGS_NUM(__VA_ARGS__),                               \
		.buff = (uint16_t[ARGS_NUM(__VA_ARGS__)]){},                   \
		.driver_data = &(gpio_io_block_t){                             \
			.pins = (uint8_t[]){ __VA_ARGS__ },                    \
		},                                                             \
	}

#ifdef WITH_TM1638
#define TM1638_ANALOG                                                          \
	{                                                                      \
		.driver_type = IO_DRIVER_TM1638, .length = 1,                  \
		.buff = (uint16_t[1])                                          \
		{                                                              \
		}                                                              \
	}

#define TM1638_DIGITAL(_offset, _length)                                       \
	{                                                                      \
		.driver_type = IO_DRIVER_TM1638, .length = _length,            \
		.buff = (uint16_t[_length]){},                                 \
		.driver_data = &(tm1638_io_block_t){ .offset = _offset },      \
	}
#endif // ifdef WITH_TM1638

#define GPIO GPIO_DIGITAL
#define GPIO_INVERTED GPIO_DIGITAL_INVERTED
#ifdef WITH_TM1638
#define TM1638 TM1638_DIGITAL
#endif
io_block_t digital_inputs_blocks[] = DIGITAL_INPUTS;
io_block_t digital_outputs_blocks[] = DIGITAL_OUTPUTS;
#undef GPIO
#undef GPIO_INVERTED
#ifdef WITH_TM1638
#undef TM1638
#endif

const size_t digital_inputs_blocks_len =
	sizeof(digital_inputs_blocks) / sizeof(io_block_t);
const size_t digital_outputs_blocks_len =
	sizeof(digital_outputs_blocks) / sizeof(io_block_t);

#define GPIO GPIO_ANALOG
#ifdef WITH_TM1638
#define TM1638 TM1638_ANALOG
#endif
io_block_t analog_inputs_blocks[] = ANALOG_INPUTS;
io_block_t analog_outputs_blocks[] = ANALOG_OUTPUTS;
#undef GPIO
#ifdef WITH_TM1638
#undef TM1638
#endif

const size_t analog_inputs_blocks_len =
	sizeof(analog_inputs_blocks) / sizeof(io_block_t);
const size_t analog_outputs_blocks_len =
	sizeof(analog_outputs_blocks) / sizeof(io_block_t);
