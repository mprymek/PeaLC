#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "io.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	bool inverted;
	const uint8_t *const pins;
} gpio_io_block_t;

int gpio_init_input_block(const io_type_t io_type, const io_block_t *block);
int gpio_init_output_block(const io_type_t io_type, const io_block_t *block);
int gpio_update_input_block(const io_type_t io_type, io_block_t *block);
int gpio_update_output_block(const io_type_t io_type, io_block_t *block);

#ifdef __cplusplus
}
#endif
