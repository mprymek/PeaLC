#pragma once

#include <stddef.h>

#include "app_config.h"

typedef enum {
	IO_OK = 0,
	IO_HW_ERROR,
	IO_DOES_NOT_EXIST,
} io_result_t;

typedef enum {
	IO_DRIVER_GPIO,
	IO_DRIVER_UAVCAN,
#ifdef WITH_TM1638
	IO_DRIVER_TM1638,
#endif
#ifdef WITH_SPARKPLUG
	IO_DRIVER_SPARKPLUG,
#endif
} io_driver_type_t;

typedef enum {
	IO_TYPE_DIGITAL,
	IO_TYPE_ANALOG,
} io_type_t;

typedef struct {
	const io_driver_type_t driver_type;
	bool enabled;
	bool dirty;
	const size_t length;
	void *const buff;
	void *const driver_data;
} io_block_t;

extern io_block_t digital_inputs_blocks[];
extern const size_t digital_inputs_blocks_len;
extern io_block_t analog_inputs_blocks[];
extern const size_t analog_inputs_blocks_len;
extern io_block_t digital_outputs_blocks[];
extern const size_t digital_outputs_blocks_len;
extern io_block_t analog_outputs_blocks[];
extern const size_t analog_outputs_blocks_len;
