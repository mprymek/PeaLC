//
// Reads an analog input value from a remote node and writes it to a remote node
// analog output.
//

// config
#define INPUT_ADDR 0
#define OUTPUT_ADDR 0
#define READ_DELAY_MS 10
#define DEBUG_PROG

#include "app_config.h"

#ifdef PROG_ANALOG_REMOTE

#include <stdbool.h>

#include <iec_std_lib.h>

#include "hal.h"

extern IEC_UINT *int_input[];
extern IEC_UINT *int_output[];

// PLC tick time [ns]
unsigned long long common_ticktime__;

// ---------------------------------------------- program ----------------------

static IEC_UINT in_val, out_val;

void config_init__(void)
{
	common_ticktime__ = READ_DELAY_MS * MILLISECOND_NS;

	// Map input variables to PLC IO memory.
	int_input[INPUT_ADDR] = &in_val;

	// Map output variables to PLC IO memory.
	int_output[OUTPUT_ADDR] = &out_val;
}

void config_run__(unsigned long tick)
{
	uint32_t now_ms =
		__CURRENT_TIME.tv_sec * 1000 + __CURRENT_TIME.tv_nsec / 1000000;

	static uint32_t last_change = 0;
	if (now_ms - last_change >= READ_DELAY_MS) {
		last_change = now_ms;
#ifdef DEBUG_PROG
		PRINTF("value=%u\n", in_val);
#endif
		out_val = in_val;
	}
}

#endif // ifdef PROG_ANALOG_REMOTE