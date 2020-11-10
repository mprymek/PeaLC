#if defined(ARDUINO)
// because of pin names like PC13 etc.
#include <Arduino.h>
#endif

#include "app_config.h"
#include "hal.h"
#include "tools.h"
#include "ui.h"

#ifdef STATUS_LEDS_INVERTED
#define STATUS_LED_VALUE(x) (!(x))
#else
#define STATUS_LED_VALUE(x) (x)
#endif

#define INIT_PIN(pin, value)                                                   \
	if (set_do_pin_value(pin, STATUS_LED_VALUE(value))) {                  \
		return -1;                                                     \
	}                                                                      \
	if (set_pin_mode_do(pin)) {                                            \
		return -2;                                                     \
	}

int ui_init()
{
#ifdef PLC_TICK_PIN
	INIT_PIN(PLC_TICK_PIN, false);
#endif
#ifdef CAN_RX_OK_PIN
	INIT_PIN(CAN_RX_OK_PIN, false);
#endif
#ifdef CAN_TX_OK_PIN
	INIT_PIN(CAN_TX_OK_PIN, false);
#endif
#ifdef WIFI_OK_PIN
	INIT_PIN(WIFI_OK_PIN, false);
#endif
	// not OK until connected
	ui_wifi_ok(false);
#ifdef MQTT_OK_PIN
	INIT_PIN(MQTT_OK_PIN, false);
#endif
	// not OK until connected
	ui_mqtt_ok(false);

	return 0;
}

void ui_set_status(const char *status)
{
	log_debug("status: %s", status);
}

void ui_plc_tick()
{
#ifdef PLC_TICK_PIN
	MAX_ONCE_PER(UI_MIN_PLC_TICK_DELAY, {
		static bool state = false;
		state = !state;
		set_do_pin_value(PLC_TICK_PIN, STATUS_LED_VALUE(state));
	});
#endif
}

void ui_can_rx()
{
#ifdef CAN_RX_OK_PIN
	MAX_ONCE_PER(UI_MIN_CAN_RX_OK_DELAY, {
		static bool state = false;
		state = !state;
		set_do_pin_value(CAN_RX_OK_PIN, STATUS_LED_VALUE(state));
	});
#endif
}

void ui_can_tx()
{
#ifdef CAN_TX_OK_PIN
	MAX_ONCE_PER(UI_MIN_CAN_TX_OK_DELAY, {
		static bool state = false;
		state = !state;
		set_do_pin_value(CAN_TX_OK_PIN, STATUS_LED_VALUE(state));
	});
#endif
}

void ui_wifi_ok(bool value)
{
#ifdef WIFI_OK_PIN
	set_do_pin_value(WIFI_OK_PIN, STATUS_LED_VALUE(value));
#endif
}

void ui_mqtt_ok(bool value)
{
#ifdef MQTT_OK_PIN
	set_do_pin_value(MQTT_OK_PIN, STATUS_LED_VALUE(value));
#endif
}
