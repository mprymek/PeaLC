#include "app_config.h"

#ifdef WITH_DALLAS

#include <Arduino.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include "dallas.h"
#include "hal.h"
#include "io.h"


// how long to wait for temperature conversion [ms]
#define WAIT_INTERVAL 1000

typedef enum {
	STATE_START,
	STATE_READY,
	STATE_WAITING,
} state_t;

state_t state = STATE_START;

OneWire one_wire(ONEWIRE_PIN);
DallasTemperature dallas(&one_wire);

static void prepare(void);
static void read_temps(void);


int dallas_init(void)
{
	dallas.begin();
	// set async
	dallas.setWaitForConversion(false);

	return 0;
}

void dallas_update(void)
{
	unsigned long now = millis();
	static unsigned long last_state_change = 0;

	if (state == STATE_READY && now - last_state_change > TEMPS_READ_INTERVAL - WAIT_INTERVAL)
	{
		prepare();
		state = STATE_WAITING;
		last_state_change = now;
		return;
	}
	if (state == STATE_WAITING && now - last_state_change > WAIT_INTERVAL)
	{
		read_temps();
		state = STATE_READY;
		last_state_change = now;
	}
	// initialize last_state_change
	if (state == STATE_START) {
		last_state_change = now;
		state = STATE_READY;
		return;
	}
}

static void prepare(void)
{
	dallas.requestTemperatures();
}

static void read_temps(void)
{
	float val_f;
	for (int i = 0; i < TEMPS_NUM; i++)
	{
		// TODO: getTempCByIndex is slow. Use faster functions (get by addr ones)
		//       Also, index will change when sensor is (dis)connected.
		val_f = dallas.getTempCByIndex(i);
		if (val_f == DEVICE_DISCONNECTED_C)
		{
#if LOGLEVEL >= LOGLEVEL_DEBUG
			PRINTS("TEMP");
			PRINTU(i);
			PRINTS(" ERR!\n");
#endif
			virt_ais[TEMPS_IDX_START + i] = UINT16_MAX;
		}
		else
		{
			virt_ais[TEMPS_IDX_START + i] = (val_f * 100) + 5000;
#if LOGLEVEL >= LOGLEVEL_DEBUG
			PRINTS("TEMP");
			PRINTU(i);
			PRINTS(" = ");
			PRINTU(virt_ais[TEMPS_IDX_START + i]);
			PRINTS("\n");
#endif
		}
	}
}

#endif // WITH_DALLAS
