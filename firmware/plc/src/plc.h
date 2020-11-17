#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "io.h"

#ifdef __cplusplus
extern "C" {
#endif

int plc_init(void);

// ---------------------------------------------- MatIEC-compiled program API --

void config_init__(void);
void config_run__(unsigned long tick);

// "imported" from Matiec-compiled PLC program, [ns]
extern unsigned long long common_ticktime__;

// ---------------------------------------------- plc --------------------------

typedef enum {
	PLC_STATE_RUNNING,
	PLC_STATE_PAUSED,
} plc_state_t;

void plc_set_state(plc_state_t state);

// ---------------------------------------------- aux --------------------------

// Helper macros to access byte-divided bit arrays. Used in c plc programs only.
#define AIDX(i) ((i) / 8)
#define BIDX(i) ((i) % 8)

#ifdef __cplusplus
}
#endif
