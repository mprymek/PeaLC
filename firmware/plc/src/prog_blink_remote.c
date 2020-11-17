//
// Blinks with a remote digital output.
//

// config
#define OUTPUT_ADDR 4
#define BLINK_DELAY_MS 1000
#define PROG_DEBUG

#include "app_config.h"

#ifdef PROG_BLINK_REMOTE

#include <stdbool.h>

#include <iec_std_lib.h>

#include "hal.h"
#include "plc.h"

extern IEC_BOOL *bool_output[IO_BUFFER_SIZE][8];

// PLC tick time [ns]
unsigned long long common_ticktime__;

// ---------------------------------------------- program ----------------------

static IEC_BOOL led;

void config_init__(void)
{
	common_ticktime__ = BLINK_DELAY_MS * MILLISECOND_NS;

	// Map output variables to PLC IO memory.
	bool_output[AIDX(OUTPUT_ADDR)][BIDX(OUTPUT_ADDR)] = &led;
}

void config_run__(unsigned long tick)
{
	uint32_t now_ms =
		__CURRENT_TIME.tv_sec * 1000 + __CURRENT_TIME.tv_nsec / 1000000;

	static uint32_t last_change = 0;
	if (now_ms - last_change >= BLINK_DELAY_MS) {
		last_change = now_ms;
		led = !led;
#ifdef PROG_DEBUG
		PRINTF("led=%u\n", led);
#endif
	}
}

#endif // ifdef PROG_BLINK_REMOTE
