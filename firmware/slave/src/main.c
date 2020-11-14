#include <Arduino.h>

#include "app_config.h"
#include "dallas.h"
#include "hal.h"
#include "io.h"
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
	PRINTS(" (UAVCAN v.0)\n\n");

	START("ui", ui_init());
#ifdef WITH_DALLAS
	START("dallas", dallas_init());
#endif
	START("io", io_init());
	START("UAVCAN", uavcan_init());

	PRINTS("-----------------------------------------------------\n");
}

void loop()
{
	for (;;) {
#ifdef WITH_DALLAS
		dallas_update();
#endif
		uavcan_update();
	}
}