// ---------------------------------------------- logging ----------------------

#define LOGLEVEL LOGLEVEL_INFO

// communication debug is very chatty, maybe we don't want it...
//#define WITHOUT_COM_DEBUG

// ---------------------------------------------- hw config --------------------
// UI
#define CAN_RX_OK_PIN 14
#define CAN_TX_OK_PIN 12
//#define WIFI_OK_PIN 27
#define MQTT_OK_PIN 27
#define PLC_TICK_PIN 13

#define STATUS_LEDS_INVERTED

#define UI_MIN_PLC_TICK_DELAY 500
#define UI_MIN_CAN_RX_OK_DELAY 500
#define UI_MIN_CAN_TX_OK_DELAY 500

// IO
#define DIGITAL_INPUTS                                                         \
	{                                                                      \
		GPIO_INVERTED(0, 16, 17, 5, 18), UAVCAN(51, 0, 4)              \
	}

#define ANALOG_INPUTS                                                          \
	{                                                                      \
		/*GPIO(33, 32),*/ UAVCAN(51, 0, 2),                            \
	}

#define DIGITAL_OUTPUTS                                                        \
	{                                                                      \
		GPIO(14, 13, 12, 27), UAVCAN(51, 0, 4),                        \
	}

#define ANALOG_OUTPUTS                                                         \
	{                                                                      \
		/*GPIO(35, 34),*/ UAVCAN(51, 0, 2)                             \
	}

// CAN
#define CAN_RX_PIN 23
#define CAN_TX_PIN 22

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

// ---------------------------------------------- SparkPlug --------------------

#define WITH_SPARKPLUG

#ifndef SP_NODE
#define SP_NODE APP_NAME
#endif

#ifndef SP_GROUP
#define SP_GROUP "PeaLC"
#endif

#define SP_TOPIC_NODE(msg_type) ("spBv1.0/" SP_GROUP "/" msg_type "/" SP_NODE)
#define SP_TOPIC_DEVICE(device, msg_type)                                      \
	("spBv1.0/" SP_GROUP "/" msg_type "/" SP_NODE "/" device)

// ---------------------------------------------- demo programs ----------------

//#define PROG_BLINK
//#define PROG_BLINK_REMOTE
//#define PROG_DICE
//#define PROG_DICE_REMOTE
//#define PROG_DICE_REMOTE_ANALOG
//#define PROG_ANALOG_REMOTE

// programs using status LEDs as outputs
#if defined(PROG_BLINK) || defined(PROG_DICE)
#undef CAN_RX_OK_PIN
#undef CAN_TX_OK_PIN
#undef MQTT_OK_PIN
#undef PLC_TICK_PIN
#undef DIGITAL_OUTPUTS
#define DIGITAL_OUTPUTS                                                        \
	{                                                                      \
		GPIO(14, 13, 12, 27)                                           \
	}
#endif // PROG_BLINK, PROG_DICE

// ---------------------------------------------- defaults & internal ----------

#include "app_config_defaults.h"
