// ---------------------------------------------- logging ----------------------

#define LOGLEVEL LOGLEVEL_DEBUG

// ---------------------------------------------- hw config --------------------

#ifdef __AVR__

// UI
// do NOT use LED_BUILTIN, it's SPI CLK!
#define CAN_RX_OK_PIN PIN_A4
// IO pins mapping
/*
#define DIS_PINS {8}
#define AIS_PINS {PIN_A0, PIN_A1}
#define DOS_PINS {3, 4, 5, 7}
#define AOS_PINS {6, 9}
*/
#define DIS_PINS {}
#define AIS_PINS {PIN_A0, PIN_A1, PIN_A2, PIN_A3}
#define DOS_PINS {4, 5, 6, 7}
#define AOS_PINS {}
#define DOS_PINS_INVERTED
// CAN
#define CAN_CS_PIN 10
// OneWire
#define ONEWIRE_PIN 8
// Dallas
#define WITH_DALLAS
#define TEMPS_NUM 1
// VIRTUAL AIS
#define VIRT_AIS_NUM 1
#define TEMPS_IDX_START 0

#endif // ifdef __AVR__


#ifdef STM32F1

// IO pins mapping
#define DIS_PINS {}
#define AIS_PINS {}
#define DOS_PINS {PB12, PB13, PB14, PB15}
#define AOS_PINS {}
#define DOS_PINS_INVERTED

#endif // #ifdef STM32F1

// ---------------------------------------------- communication ----------------

#define APP_NAME "PeaPLC-slave"
#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 1

// ---------------------------------------------- defaults & internal ----------

#include "app_config_defaults.h"
