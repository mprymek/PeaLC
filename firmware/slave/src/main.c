#include <Arduino.h>

#include "app_config.h"
#include "dallas.h"
#include "hal.h"
#include "slave.h"
#include "tm1638.h"
#include "ui.h"
#include "uavcan_common.h"
#include "uavcan_impl.h"

#define START(lbl, init_fun)                                                   \
	{                                                                      \
		PRINTS(lbl " ... ");                                           \
		if (init_fun) {                                                \
			die(DEATH_INIT_FAILED);                                \
		} else {                                                       \
			PRINTS("OK\n");                                        \
		}                                                              \
	}

void setup()
{
	if (log_init()) {
		die(DEATH_INIT_FAILED);
	}
	PRINTS("\n");
	PRINTS("-----------------------------------------------------\n");
	PRINTS(APP_NAME);
	PRINTS(" v. ");
	PRINTU(APP_VERSION_MAJOR);
	PRINTS(".");
	PRINTU(APP_VERSION_MINOR);
#ifdef WITH_CAN
	PRINTS(" (UAVCAN v.1)\n\n");
#else
	PRINTS(" (no CAN)\n\n");
#endif

	START("ui", ui_init());
#ifdef WITH_TM1638
	START("TM1638", tm1638_init());
#endif
#ifdef WITH_DALLAS
	START("dallas", dallas_init());
#endif
	START("slave", slave_init());
#ifdef WITH_CAN
	START("UAVCAN", uavcan_init());
#endif

	PRINTS("-----------------------------------------------------\n");
}

void loop()
{
	for (;;) {
#ifdef WITH_DALLAS
		dallas_update();
#endif
#ifdef WITH_CAN
		uavcan_update();
#endif
	}
}