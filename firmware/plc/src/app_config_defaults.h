// ---------------------------------------------- hw config --------------------

#if defined(STM32F1)
#define STM32
#endif

// ---------------------------------------------- communication ----------------

// how often to transmit node status message [ms]
#define UAVCAN_STATUS_PERIOD 1000

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
