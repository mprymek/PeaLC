#ifdef ESP32
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern EventGroupHandle_t global_event_group;

#define PLC_INITIALIZED_BIT BIT0
#define PLC_RUNNING_BIT BIT1
#define WIFI_READY_BIT BIT2
#define MQTT_READY_BIT BIT3

int locks_init(void);

// convenience functions
#define MAX_BIT_WAIT_TIME pdMS_TO_TICKS(20000)
#define WAIT_BITS(bits)                                                        \
	((xEventGroupWaitBits(global_event_group, bits, pdFALSE, pdTRUE,       \
			      MAX_BIT_WAIT_TIME) &                             \
	  (bits)) == (bits))
#define IS_BIT_SET(bit) (xEventGroupGetBits(global_event_group) & (bit))

#ifdef __cplusplus
}
#endif
