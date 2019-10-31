#ifdef ESP32

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// NOTE: FreeRTOS headers must be included before `driver/can.h`
#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <driver/can.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <esp_event_loop.h>

#include <uavcan_node.h>

#include "app_config.h"
#include "uavcan_impl.h"
#include "hal.h"
#include "locks.h"
#include "ui.h"
#include "tools.h"

// IDF-functions return code check
#define RET_CHECK(x, msg)                                                      \
	if ((x) != ESP_OK) {                                                   \
		log_error(msg " failed");                                      \
		return -1;                                                     \
	}

// ---------------------------------------------- logging ----------------------

int log_init(void)
{
	// Disable buffering on stdin
	// We need this to immediatelly get logs even when there's no NL
	// (like in init "x ... y ... z OK" messages).
	setvbuf(stdout, NULL, _IONBF, 0);
	return 0;
}

#define TO_PRINTF                                                              \
	va_list args;                                                          \
	va_start(args, fmt);                                                   \
	vprintf(fmt, args);                                                    \
	va_end(args);                                                          \
	printf("\n");

void log_error2(const char *fmt, ...)
{
	TO_PRINTF
}
void log_warning2(const char *fmt, ...)
{
	TO_PRINTF
}
void log_info2(const char *fmt, ...)
{
	TO_PRINTF
}
void log_debug2(const char *fmt, ...)
{
	TO_PRINTF
}

// ---------------------------------------------- IO ---------------------------

int set_pin_mode_di(int pin)
{
	gpio_pad_select_gpio(pin);
	if (gpio_set_direction(pin, GPIO_MODE_INPUT) != ESP_OK) {
		return -1;
	}
	if (gpio_pullup_en(pin) != ESP_OK) {
		return -1;
	}
	return 0;
}

int set_pin_mode_do(int pin)
{
	gpio_pad_select_gpio(pin);
#ifdef DOS_PINS_INVERTED
	gpio_set_level(pin, 1);
#else
	gpio_set_level(pin, 0);
#endif
	if (gpio_set_direction(pin, GPIO_MODE_OUTPUT) != ESP_OK) {
		return -1;
	}
	return 0;
}

int set_pin_mode_ai(int pin)
{
	log_error("TODO: analog pins not implemented for ESP32");
	return -1;
}

int set_pin_mode_ao(int pin)
{
	log_error("TODO: analog pins not implemented for ESP32");
	return -1;
}

int set_do_pin_value(int pin, bool value)
{
	if (gpio_set_level(pin, value) != ESP_OK) {
		return -1;
	}
	return 0;
}

int get_di_pin_value(int pin, bool *value)
{
	*value = gpio_get_level(pin);
	return 0;
}

int set_ao_pin_value(int pin, uint16_t value)
{
	log_error("TODO: analog pins not implemented for ESP32");
	return -1;
}

int get_ai_pin_value(int pin, uint16_t *value)
{
	log_error("TODO: analog pins not implemented for ESP32");
	return -1;
}

// ---------------------------------------------- CAN --------------------------

static void can_watch_task(void *pvParameters);
TaskHandle_t can_watch_task_h = NULL;
volatile can_bus_state_t can_bus_state = CANBS_ERR_PASSIVE;

int can2_init()
{
	//Initialize configuration structures using macro initializers
	// tx, rx pins
	can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(
		CAN_TX_PIN, CAN_RX_PIN, CAN_MODE_NORMAL);
	can_timing_config_t t_config = CAN_TIMING_CONFIG_250KBITS();
	can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

	//Install CAN driver
	if (!can_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
		log_error("Failed to install CAN driver");
		return -1;
	}

	// Reconfigure alerts to detect Error Passive and Bus-Off error states
	uint32_t alerts_to_enable =
		CAN_ALERT_ALL & (~(CAN_ALERT_TX_IDLE | CAN_ALERT_TX_SUCCESS));
	if (!can_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK) {
		log_error("Failed to reconfigure alerts");
		return -2;
	}

	// Start CAN driver
	if (can_start() != ESP_OK) {
		log_error("Failed to start driver");
		return -3;
	}

	if (xTaskCreate(can_watch_task, "can-w",
			configMINIMAL_STACK_SIZE + 2048, NULL,
			configMAX_PRIORITIES - 2,
			&can_watch_task_h) != pdPASS) {
		log_error("Failed to create can-w task");
		return -4;
	}

	return 0;
}

static void can_watch_task(void *pvParameters)
{
	uint32_t alerts;

	for (;;) {
		can_read_alerts(&alerts, 0);
		if (alerts != 0) {
			if (alerts & CAN_ALERT_BELOW_ERR_WARN) {
				log_info("CAN: below error warning limit");
			}
			if (alerts & CAN_ALERT_ERR_ACTIVE) {
				log_warning("CAN: entered error active state");
				can_bus_state = CANBS_ERR_ACTIVE;
			}
			if (alerts & CAN_ALERT_RECOVERY_IN_PROGRESS) {
				log_warning("CAN: bus recovery in progress");
			}
			if (alerts & CAN_ALERT_BUS_RECOVERED) {
				// NOTE: bus is "recovered" even when in fact there's still error on the bus
				log_info("CAN: bus recovered");
				// After recovery, the driver enters stopped state - we must start
				// it again.
				// See https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/can.html#driver-operation
				if (can_start() != ESP_OK) {
					log_error(
						"CAN: failed to start driver");
					die(DEATH_CAN_DRIVER_ERROR);
				}
			}
			if (alerts & CAN_ALERT_ARB_LOST) {
				log_debug("CAN: arbitration lost");
			}
			if (alerts & CAN_ALERT_ABOVE_ERR_WARN) {
				log_warning(
					"CAN: surpassed error warning limit");
			}
			if (alerts & CAN_ALERT_BUS_ERROR) {
				log_error("CAN: bus error");
			}
			if (alerts & CAN_ALERT_TX_FAILED) {
				log_error("CAN: TX failed");
			}
			if (alerts & CAN_ALERT_RX_QUEUE_FULL) {
				log_error("CAN: RX queue full");
			}
			if (alerts & CAN_ALERT_ERR_PASS) {
				log_error("CAN: entered error passive state");
				can_bus_state = CANBS_ERR_PASSIVE;
			}
			if (alerts & CAN_ALERT_BUS_OFF) {
				log_error("CAN: entered bus off state");
				can_bus_state = CANBS_BUS_OFF;
				for (int i = 3; i > 0; i--) {
					log_warning(
						"CAN: initiating bus recovery in %d",
						i);
					vTaskDelay(pdMS_TO_TICKS(1000));
				}
				can_initiate_recovery(); //Needs 128 occurrences of bus free signal
				log_info("CAN: bus recovery initiated");
			}
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

// ---------------------------------------------- UAVCAN -----------------------

void uavcan_get_unique_id(
	uint8_t out_uid[UAVCAN_PROTOCOL_HARDWAREVERSION_UNIQUE_ID_LENGTH])
{
	esp_efuse_mac_get_default(out_uid);
	for (int i = 6; i < UAVCAN_PROTOCOL_HARDWAREVERSION_UNIQUE_ID_LENGTH;
	     i++) {
		out_uid[i] = 0xFF;
	}
}

void uavcan_restart(void)
{
	esp_restart();
}

int uavcan_can_rx(CanardCANFrame *frame)
{
	can_message_t esp_msg;

	if (can_receive(&esp_msg, 0) != ESP_OK) {
		return 0;
	}

	ui_can_rx();

	frame->id = esp_msg.identifier;
	if (esp_msg.flags & CAN_MSG_FLAG_EXTD) {
		frame->id |= CANARD_CAN_FRAME_EFF;
	}
	if (esp_msg.flags & CAN_MSG_FLAG_RTR) {
		frame->id |= CANARD_CAN_FRAME_RTR;
	}
	// TODO: error flag?!

	if (esp_msg.data_length_code > CANARD_CAN_FRAME_MAX_DATA_LEN) {
		log_error("ESP CAN frame too big (%u > %u)!",
			  esp_msg.data_length_code,
			  CANARD_CAN_FRAME_MAX_DATA_LEN);
		return 0;
	}

	memcpy(frame->data, esp_msg.data, esp_msg.data_length_code);
	frame->data_len = esp_msg.data_length_code;

#if LOGLEVEL >= LOGLEVEL_DEBUG
	print_frame("->", frame);
#endif

	return 1;
}

int uavcan_can_tx(const CanardCANFrame *frame)
{
	can_message_t esp_msg;

	esp_msg.identifier =
		frame->id & (~(CANARD_CAN_FRAME_EFF | CANARD_CAN_FRAME_ERR |
			       CANARD_CAN_FRAME_RTR));
	esp_msg.flags = 0;
	if (frame->id & CANARD_CAN_FRAME_EFF) {
		esp_msg.flags |= CAN_MSG_FLAG_EXTD;
	}
	if (frame->id & CANARD_CAN_FRAME_RTR) {
		esp_msg.flags |= CAN_MSG_FLAG_RTR;
	}
	// TODO: error flag?!

	if (frame->data_len > sizeof(esp_msg.data)) {
		log_error("Canard frame too big (%u > %lu)!", frame->data_len,
			  sizeof(esp_msg.data));
		return -1;
	}

	memcpy(esp_msg.data, frame->data, frame->data_len);
	esp_msg.data_length_code = frame->data_len;

#if LOGLEVEL >= LOGLEVEL_DEBUG
	print_frame("<-", frame);
#endif

	if (!can_transmit(&esp_msg, pdMS_TO_TICKS(10)) == ESP_OK) {
		log_error("CAN TX error");
		return -2;
	}

	ui_can_tx();

	return 0;
}

// ---------------------------------------------- wifi -------------------------

#ifdef WITH_WIFI

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event);

const static int WIFI_CONNECTED_BIT = BIT0;

int wifi_init()
{
	RET_CHECK(nvs_flash_init(), "NVS init");
	tcpip_adapter_init();
	RET_CHECK(esp_event_loop_init(wifi_event_handler, NULL),
		  "Wifi event loop creation");
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	RET_CHECK(esp_wifi_init(&cfg), "Wifi init");
	RET_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM),
		  "esp_wifi_set_storage");
	wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWD,
        },
    };
	RET_CHECK(esp_wifi_set_mode(WIFI_MODE_STA), "esp_wifi_set_mode");
	RET_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config),
		  "esp_wifi_set_config");
	RET_CHECK(esp_wifi_start(), "esp_wifi_start");

	PRINTF("connecting ");
	if (!WAIT_BIT(WIFI_READY_BIT)) {
		return -1;
	}

	ui_wifi_ok(true);

	return 0;
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
	switch (event->event_id) {
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		xEventGroupSetBits(global_event_group, WIFI_READY_BIT);
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		log_warning("wifi disconnected");
		// TODO: check error
		esp_wifi_connect();
		xEventGroupClearBits(global_event_group, WIFI_READY_BIT);
		break;
	default:
		break;
	}
	return ESP_OK;
}

#endif // ifdef WITH_WIFI

// ---------------------------------------------- misc -------------------------

void die(uint8_t reason)
{
	PRINTF("\n\nDYING BECAUSE %d\n\n", reason);
	for (;;)
		;
}

uint64_t uptime_usec()
{
	// TODO: esp_timer_get_time returns int64_t only
	return esp_timer_get_time();
}

uint32_t uptime_msec()
{
	return esp_timer_get_time() / 1000;
}

#endif // #ifdef ESP32
