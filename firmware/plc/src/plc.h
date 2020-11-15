#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef WITH_CAN
#include "uavcan_common.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

int plc_init(void);

// ---------------------------------------------- MatIEC-compiled program API --

void config_init__(void);
void config_run__(unsigned long tick);

// "imported" from Matiec-compiled PLC program, [ns]
extern unsigned long long common_ticktime__;

// ---------------------------------------------- remote vars ------------------

typedef struct {
	uint8_t node_id;
	uint8_t index;
	uint8_t len;
	union {
		uint16_t *analog_vals;
		bool *digital_vals;
	};
} uavcan_vals_block_t;

typedef enum {
	PLC_STATE_RUNNING,
	PLC_STATE_PAUSED,
} plc_state_t;

void plc_set_state(plc_state_t state);

#ifdef WITH_CAN

extern uavcan_vals_block_t uavcan_dis_blocks[];
extern uavcan_vals_block_t uavcan_dos_blocks[];
extern uavcan_vals_block_t uavcan_ais_blocks[];
extern uavcan_vals_block_t uavcan_aos_blocks[];

extern const uint8_t uavcan_dis_blocks_len;
extern const uint8_t uavcan_dos_blocks_len;
extern const uint8_t uavcan_ais_blocks_len;
extern const uint8_t uavcan_aos_blocks_len;

#define EXT_BUFF_SIZE (IO_BUFFER_SIZE - REMOTE_VARS_INDEX)
extern bool ext_dos[EXT_BUFF_SIZE];
extern uint16_t ext_aos[EXT_BUFF_SIZE];
extern bool ext_dis[EXT_BUFF_SIZE];
extern uint16_t ext_ais[EXT_BUFF_SIZE];

#endif // ifdef WITH_CAN

// ---------------------------------------------- aux --------------------------

// Helper macros to access byte-divided bit arrays. Used in c plc programs only.
#define AIDX(i) ((i) / 8)
#define BIDX(i) ((i) % 8)

#ifdef __cplusplus
}
#endif
