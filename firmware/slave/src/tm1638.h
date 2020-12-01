#pragma once

#include <stdint.h>

#include "io.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint8_t offset;
} tm1638_io_block_t;

int tm1638_init();

int tm1638_update_input_block(const io_type_t io_type, io_block_t *block);
int tm1638_update_output_block(const io_type_t io_type, io_block_t *block);

#ifdef __cplusplus
}
#endif
