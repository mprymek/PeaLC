//
// Turns on multiple digital outputs one by one. Pauses if a local or remote
// digital input is set.
//

// config
#define OUTPUTS_START_ADDR 4
#define OUTPUTS_NUM 4
#define PAUSE_INPUT_ADDR 0
#define REMOTE_PAUSE_INPUT_ADDR 5
#define CHANGE_DELAY_MS 50
#define PROG_DEBUG

#include "app_config.h"

#ifdef PROG_DICE_REMOTE

#include <stdbool.h>

#include <iec_std_lib.h>

#include "hal.h"
#include "plc.h"

extern IEC_BOOL *bool_input[IO_BUFFER_SIZE][8];
extern IEC_BOOL *bool_output[IO_BUFFER_SIZE][8];

// PLC tick time [ns]
unsigned long long common_ticktime__;

// ---------------------------------------------- program ----------------------

static IEC_BOOL leds[OUTPUTS_NUM];
static IEC_BOOL pause_local, pause_remote;

void config_init__(void)
{
	common_ticktime__ = CHANGE_DELAY_MS * MILLISECOND_NS;

	// Map output variables to PLC IO memory.
	for (uint8_t addr = 0; addr < OUTPUTS_NUM; addr++) {
		bool_output[AIDX(addr + OUTPUTS_START_ADDR)]
			   [BIDX(addr + OUTPUTS_START_ADDR)] = &leds[addr];
	}

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

			// clear previous
			leds[i] = 0;
			i = (i + 1) % OUTPUTS_NUM;
			// set next
			leds[i] = 1;
		}

#ifdef PROG_DEBUG
		PRINTF("switches: %u %u\tleds:", pause_local, pause_remote);
		for (uint8_t j = 0; j < OUTPUTS_NUM; j++) {
			PRINTF(" %u", leds[j]);
		}
		PRINTF("\n");
#endif
	}
}

#endif // ifdef PROG_DICE_REMOTE