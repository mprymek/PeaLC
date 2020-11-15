// ---------------------------------------------- logging ----------------------

#define LOGLEVEL LOGLEVEL_INFO

// communication debug is very chatty, maybe we don't want it...
//#define WITHOUT_COM_DEBUG

// ---------------------------------------------- hw config --------------------

// remote variables start at this index
#define REMOTE_VARS_INDEX 8

// UI
#define CAN_RX_OK_PIN 14
#define CAN_TX_OK_PIN 12
//#define WIFI_OK_PIN 27
#define MQTT_OK_PIN 27
#define PLC_TICK_PIN 13

#define UI_MIN_PLC_TICK_DELAY 500
#define UI_MIN_CAN_RX_OK_DELAY 500
#define UI_MIN_CAN_TX_OK_DELAY 500

// IO
#define DIS_PINS                                                               \
	{                                                                      \
		16, 17, 5, 18                                                  \
	}
#define DIS_PINS_INVERTED
#define DOS_PINS                                                               \
	{                                                                      \
		2                                                              \
	}
#if 0
#define AIS_PINS                                                               \
	{                                                                      \
		33, 32                                                         \
	}
#define AOS_PINS                                                               \
	{                                                                      \
		35, 34                                                         \
	}
#endif
// CAN
#define CAN_RX_PIN 23
#define CAN_TX_PIN 22

// remote variables start at this index
#define REMOTE_VARS_INDEX 8

// ---------------------------------------------- ui ---------------------------

#define STATUS_LEDS_INVERTED

// ---------------------------------------------- remote blocks ----------------

#define UAVCAN_DIS_BLOCKS                                                      \
	{                                                                      \
		{ .node_id = 51, .index = 0, .len = 4 },                       \
	}
#define UAVCAN_DOS_BLOCKS                                                      \
	{                                                                      \
		{ .node_id = 51, .index = 0, .len = 4 },                       \
	}

// ---------------------------------------------- communication config ---------

#define UAVCAN_NODE_ID 50

#define APP_NAME "PeaLC-CPU"
#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 1

#define WITH_CAN

// ---------------------------------------------- WiFi -------------------------

#define WIFI_SSID "MY_WIFI"
#define WIFI_PASSWD "supersecret"

// ---------------------------------------------- MQTT -------------------------

#define WITH_MQTT
#define MQTT_BROKER_URL "mqtt://1.2.3.4"
//#define MQTT_USERNAME ""
//#define MQTT_PASSWORD ""

// ---------------------------------------------- demo programs ----------------

//#define PROG_BLINK
//#define PROG_BLINK_REMOTE
//#define PROG_DICE
//#define PROG_DICE_REMOTE

// programs using status LEDs as outputs
#if defined(PROG_BLINK) || defined(PROG_DICE)

#undef CAN_RX_OK_PIN
#undef CAN_TX_OK_PIN
#undef MQTT_OK_PIN
#undef PLC_TICK_PIN

#undef DOS_PINS
#define DOS_PINS                                                               \
	{                                                                      \
		14, 13, 12, 27                                                 \
	}

#endif

// programs using ESP32 BOOT switch as input
#if defined(PROG_DICE) || defined(PROG_DICE_REMOTE)

#undef DIS_PINS
#define DIS_PINS                                                               \
	{                                                                      \
		0                                                              \
	}
#define DIS_PINS_INVERTED

#endif

// programs using remote outputs
#if defined(PROG_BLINK_REMOTE) || defined(PROG_DICE_REMOTE)

#undef UAVCAN_DOS_BLOCKS
#define UAVCAN_DOS_BLOCKS                                                      \
	{                                                                      \
		{ .node_id = 51, .index = 0, .len = 4 },                       \
	}

#endif

// programs using remote inputs
#if defined(PROG_DICE_REMOTE)

#undef UAVCAN_DIS_BLOCKS
#define UAVCAN_DIS_BLOCKS                                                      \
	{                                                                      \
		{ .node_id = 51, .index = 0, .len = 1 },                       \
	}

#endif

// ---------------------------------------------- defaults & internal ----------

#include "app_config_defaults.h"
