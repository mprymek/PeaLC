//
// Sets a remote analog output to numbers VALUE_MIN to VALUE_MAX.
// Pauses if a local or remote digital input is set.
//

// config
#define OUTPUT_ADDR 0
#define VALUE_MIN 1
#define VALUE_MAX 5
#define PAUSE_INPUT_ADDR 0
#define REMOTE_PAUSE_INPUT_ADDR 5
#define CHANGE_DELAY_MS 50
#define PROG_DEBUG

#include "app_config.h"

#ifdef PROG_DICE_REMOTE_ANALOG

#include <stdbool.h>

#include <iec_std_lib.h>

#include "hal.h"
#include "plc.h"

extern IEC_BOOL *bool_input[IO_BUFFER_SIZE][8];
extern IEC_UINT *int_output[IO_BUFFER_SIZE];

// PLC tick time [ns]
unsigned long long common_ticktime__;

// ---------------------------------------------- program ----------------------

static IEC_UINT value = VALUE_MIN;
static IEC_BOOL pause_local = FALSE, pause_remote = FALSE;

void config_init__(void)
{
	common_ticktime__ = CHANGE_DELAY_MS * MILLISECOND_NS;

	// Map output variables to PLC IO memory.
	int_output[OUTPUT_ADDR] = &value;

	// Map input variables to PLC IO memory.
	bool_input[AIDX(PAUSE_INPUT_ADDR)][BIDX(PAUSE_INPUT_ADDR)] =
		&pause_local;
	bool_input[AIDX(REMOTE_PAUSE_INPUT_ADDR)]
		  [BIDX(REMOTE_PAUSE_INPUT_ADDR)] = &pause_remote;
}

void config_run__(unsigned long tick)
{
	uint32_t now_ms =
		__CURRENT_TIME.tv_sec * 1000 + __CURRENT_TIME.tv_nsec / 1000000;

	static uint32_t last_change = 0;
	static uint8_t i = 0;
	if (now_ms - last_change >= CHANGE_DELAY_MS) {
		if (!pause_local && !pause_remote) {
			last_change = now_ms;

			value++;
			if (value > VALUE_MAX) {
				value = VALUE_MIN;
			}
		}

#ifdef PROG_DEBUG
		PRINTF("switches: %u %u\tvalue: %u\n", pause_local,
		       pause_remote, value);
#endif
	}
}

#endif // ifdef PROG_DICE_REMOTE_ANALOG