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
// CAN
#define CAN_RX_PIN 23
#define CAN_TX_PIN 22

//#define WITH_OLED

// ---------------------------------------------- communication config ---------

#define APP_NAME "PeaPLC-CPU"
#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 1

// how often to transmit node status message [ms]
#define UAVCAN_STATUS_PERIOD 1000

// how often to run uavcan RX/TX [ms]
#define UAVCAN_RXTX_PERIOD 200

// ---------------------------------------------- remote blocks ----------------

#define UAVCAN_DIS_BLOCKS { \
        {.node_id = 51, .index = 0, .len = 1}, \
    }
#define UAVCAN_DOS_BLOCKS { \
        {.node_id = 51, .index = 0, .len = 1}, \
    }
#define UAVCAN_AIS_BLOCKS {}
#define UAVCAN_AOS_BLOCKS {}

// ---------------------------------------------- defaults & internal ----------
#define STACK_SIZE_PLC (configMINIMAL_STACK_SIZE + 3074)
#define STACK_SIZE_UAVCAN (configMINIMAL_STACK_SIZE + 3072)

#define TASK_PRIORITY_PLC (configMAX_PRIORITIES - 1)
#define TASK_PRIORITY_UAVCAN (tskIDLE_PRIORITY + 1)

#ifdef STATUS_LEDS_INVERTED
#define STATUS_LED_VALUE(x) (!(x))
#else
#define STATUS_LED_VALUE(x) (x)
#endif

#ifndef UI_MIN_PLC_TICK_DELAY
// blink at most once per this period [ms]
#define UI_MIN_PLC_TICK_DELAY 1000
#endif

#ifndef UI_MIN_CAN_RX_OK_DELAY
// blink at most once per this period [ms]
#define UI_MIN_CAN_RX_OK_DELAY 100
#endif

#ifndef UI_MIN_CAN_TX_OK_DELAY
// blink at most once per this period [ms]
#define UI_MIN_CAN_TX_OK_DELAY 100
#endif

