// ---------------------------------------------- logging ----------------------

#define LOGLEVEL LOGLEVEL_DEBUG

// ---------------------------------------------- hw config --------------------

// remote variables start at this index
#define REMOTE_VARS_INDEX 8

// UI
#define CAN_RX_OK_PIN 14
#define CAN_TX_OK_PIN 12
#define WIFI_ERR_PIN 27
#define PLC_TICK_PIN 13
#define STATUS_LEDS_INVERTED
// IO pins mapping
#define DIS_NUM 4
#define DIS_PINS {16, 17, 5, 18}
#define DOS_NUM 1
#define DOS_PINS {2}
//#define AIS_NUM 2
//#define AIS_PINS {33, 32} // 26, 25 does not work?!
//#define AOS_NUM 2
//#define AOS_PINS {35, 34}
#define AIS_NUM 0
#define AIS_PINS {}
#define AOS_NUM 0
#define AOS_PINS {}

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

