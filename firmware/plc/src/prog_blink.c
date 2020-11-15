#include "app_config.h"

#if defined(PROG_BLINK)

#include <stdbool.h>

#include <iec_std_lib.h>

#include "hal.h"
#include "plc.h"

extern IEC_BOOL *bool_output[IO_BUFFER_SIZE][8];

// PLC tick time [ns]
unsigned long long common_ticktime__;

// ---------------------------------------------- program ----------------------

#define BLINK_DELAY_MS 1000
//#define DEBUG

IEC_BOOL led;

void config_init__(void)
{
	common_ticktime__ = BLINK_DELAY_MS * MILLISECOND_NS;

	// Connect IO buffer to our variables.
	uint8_t i = 0;
	bool_output[AIDX(i)][BIDX(i)] = &led;
}

void config_run__(unsigned long tick)
{
	uint32_t now_ms =
		__CURRENT_TIME.tv_sec * 1000 + __CURRENT_TIME.tv_nsec / 1000000;

	static uint32_t last_change = 0;
	if (now_ms - last_change >= BLINK_DELAY_MS) {
		last_change = now_ms;
		led = !led;
#ifdef DEBUG
		PRINTF("led=%u\n", led);
#endif
	}
}

#endif // defined(PROG_BLINK)