// ---------------------------------------------- hw config --------------------

#if defined(STM32F1)
#define STM32
#endif

// ---------------------------------------------- communication ----------------

// how often to transmit node status message [ms]
#define UAVCAN_STATUS_PERIOD 1000

// ---------------------------------------------- MQTT -------------------------

#ifdef WITH_MQTT
#define WITH_WIFI
#endif

// maximal time to wait for mqtt semaphore [ms]
#define MQTT_MAX_PUBLISH_WAIT 10

#ifndef MQTT_USERNAME
// must be empty or end with a slash
#define MQTT_USERNAME ""
#endif

#ifndef MQTT_PASSWORD
// must be empty or end with a slash
#define MQTT_PASSWORD ""
#endif

#ifndef MQTT_STATUS_STARTING_MSG
#define MQTT_STATUS_STARTING_MSG "starting"
#endif

#ifndef MQTT_STATUS_RUNNING_MSG
#define MQTT_STATUS_RUNNING_MSG "running"
#endif

#ifndef MQTT_STATUS_PAUSED_MSG
#define MQTT_STATUS_PAUSED_MSG "paused"
#endif

#ifndef MQTT_STATUS_OFFLINE_MSG
#define MQTT_STATUS_OFFLINE_MSG "offline"
#endif

#ifndef MQTT_ROOT_TOPIC
// must be empty or end with a slash
#define MQTT_ROOT_TOPIC "plc/"
#endif

#define MQTT_SUBTOPIC(x) (MQTT_ROOT_TOPIC x)

#ifndef MQTT_STATUS_TOPIC
#define MQTT_STATUS_TOPIC MQTT_SUBTOPIC("status")
#endif

#ifndef MQTT_PAUSE_TOPIC
#define MQTT_PAUSE_TOPIC MQTT_SUBTOPIC("pause")
#endif

#ifndef MQTT_RESET_TOPIC
#define MQTT_RESET_TOPIC MQTT_SUBTOPIC("reset")
#endif

// ---------------------------------------------- ui ---------------------------

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

// ---------------------------------------------- io ---------------------------

#ifndef VIRT_AIS_NUM
#define VIRT_AIS_NUM 0
#endif

// ---------------------------------------------- tasks ------------------------

#define STACK_SIZE_PLC (configMINIMAL_STACK_SIZE + 3074)
#define STACK_SIZE_UAVCAN (configMINIMAL_STACK_SIZE + 3072)

#define TASK_PRIORITY_PLC (configMAX_PRIORITIES - 1)
#define TASK_PRIORITY_UAVCAN (tskIDLE_PRIORITY + 1)

// ---------------------------------------------- temperature sensors ----------

// temperature read interval [ms]
#ifndef TEMPS_READ_INTERVAL
#define TEMPS_READ_INTERVAL 60000
#endif
