#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "app_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint8_t node_id;
	uint8_t index;
} uavcan_io_block_t;

#ifdef __cplusplus
}
#endif
