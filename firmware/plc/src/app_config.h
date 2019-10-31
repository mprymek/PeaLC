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
#define DIS_PINS                                                               \
	{                                                                      \
		16, 17, 5, 18                                                  \
	}
#define DOS_PINS                                                               \
	{                                                                      \
		2                                                              \
	}
//#define AIS_PINS {33, 32} // 26, 25 does not work?!
//#define AOS_PINS {35, 34}
#define AIS_PINS                                                               \
	{                                                                      \
	}
#define AOS_PINS                                                               \
	{                                                                      \
	}
// CAN
#define CAN_RX_PIN 23
#define CAN_TX_PIN 22

// ---------------------------------------------- remote blocks ----------------

#define UAVCAN_DIS_BLOCKS                                                      \
	{                                                                      \
		{ .node_id = 51, .index = 0, .len = 1 },                       \
	}
#define UAVCAN_DOS_BLOCKS                                                      \
	{                                                                      \
		{ .node_id = 51, .index = 0, .len = 1 },                       \
	}
#define UAVCAN_AIS_BLOCKS                                                      \
	{                                                                      \
	}
#define UAVCAN_AOS_BLOCKS                                                      \
	{                                                                      \
	}

// ---------------------------------------------- communication config ---------

#define APP_NAME "PeaPLC-CPU"
#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 1

// how often to run uavcan RX/TX [ms]
#define UAVCAN_RXTX_PERIOD 200

// ---------------------------------------------- WiFi -------------------------

#define WIFI_SSID "MY_WIFI"
#define WIFI_PASSWD "supersecret"

// ---------------------------------------------- defaults & internal ----------

#include "app_config_defaults.h"
