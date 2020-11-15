#include "app_config.h"

#if defined(PROG_DICE_REMOTE)

#include <stdbool.h>

#include <iec_std_lib.h>

#include "hal.h"
#include "plc.h"

extern IEC_BOOL *bool_input[IO_BUFFER_SIZE][8];
extern IEC_BOOL *bool_output[IO_BUFFER_SIZE][8];

// PLC tick time [ns]
unsigned long long common_ticktime__;

// ---------------------------------------------- program ----------------------

#define CHANGE_DELAY_MS 50
//#define DEBUG

IEC_BOOL led0, led1, led2, led3, switch_local, switch_remote;

void config_init__(void)
{
	common_ticktime__ = CHANGE_DELAY_MS * MILLISECOND_NS;

	// Connect remote output variables to IO buffer.
	uint8_t i = REMOTE_VARS_INDEX;
	bool_output[AIDX(i)][BIDX(i)] = &led0;
	i++;
	bool_output[AIDX(i)][BIDX(i)] = &led1;
	i++;
	bool_output[AIDX(i)][BIDX(i)] = &led2;
	i++;
	bool_output[AIDX(i)][BIDX(i)] = &led3;

	// Connect local input variable to IO buffer.
	i = 0;
	bool_input[AIDX(i)][BIDX(i)] = &switch_local;

	// Connect remote input variables to IO buffer.
	i = REMOTE_VARS_INDEX;
	bool_input[AIDX(i)][BIDX(i)] = &switch_remote;
}

void config_run__(unsigned long tick)
{
	uint32_t now_ms =
		__CURRENT_TIME.tv_sec * 1000 + __CURRENT_TIME.tv_nsec / 1000000;

	static uint32_t last_change = 0;
	static uint8_t i = 0;
	if (now_ms - last_change >= CHANGE_DELAY_MS) {
		if (!switch_local && !switch_remote) {
			last_change = now_ms;

			i = (i + 1) % 4;

			switch (i) {
			case 0:
				led0 = 1;
				led1 = 0;
				led2 = 0;
				led3 = 0;
				break;
			case 1:
				led0 = 0;
				led1 = 1;
				led2 = 0;
				led3 = 0;
				break;
			case 2:
				led0 = 0;
				led1 = 0;
				led2 = 1;
				led3 = 0;
				break;
			case 3:
				led0 = 0;
				led1 = 0;
				led2 = 0;
				led3 = 1;
				break;
			}
		}

#ifdef DEBUG
		PRINTF("switches: local=%u remote=%u\tleds: %u %u %u %u\n",
		       switch_local, switch_remote, led0, led1, led2, led3);
#endif
	}
}

#endif // defined(PROG_DICE_REMOTE)