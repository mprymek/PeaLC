// ---------------------------------------------- logging ----------------------

#define LOGLEVEL LOGLEVEL_DEBUG

// ---------------------------------------------- hw config --------------------

// ui
#define CAN_RX_OK_PIN 14
#define CAN_TX_OK_PIN 12
#define WIFI_ERR_PIN 27
#define PLC_TICK_PIN 13
#define STATUS_LEDS_INVERTED

//#define WITH_OLED

// ---------------------------------------------- defaults & internal ----------
#define STACK_SIZE_PLC (configMINIMAL_STACK_SIZE + 3074)

#define APP_NAME UAVCAN_APP_NAME
#define APP_VER_MAJOR UAVCAN_APP_VER_MAJOR
#define APP_VER_MINOR UAVCAN_APP_VER_MINOR

#ifdef STATUS_LEDS_INVERTED
#define STATUS_LED_VALUE(x) (!(x))
#else
#define STATUS_LED_VALUE(x) (x)
#endif

#ifndef UI_MIN_PLC_TICK_DELAY
// blink at most once per this period [ms]
#define UI_MIN_PLC_TICK_DELAY 1000
#endif

