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
#include <mqtt_client.h>

#include "app_config.h"
#include "hal.h"
#include "locks.h"
#include "plc.h"
#include "tools.h"
#include "ui.h"

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

#ifdef WITH_CAN

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
				// UAVCAN is not fully functional until PLC is initialized
				// => supress this message until then.
				if (IS_BIT_SET(PLC_INITIALIZED_BIT)) {
					log_error("CAN: RX queue full");
				}
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

int can2_send(uint32_t id, const void *payload, size_t payload_size)
{
	can_message_t esp_msg;

	if (payload_size > sizeof(esp_msg.data)) {
		return -1;
	}

	esp_msg.identifier = id;
	memcpy(esp_msg.data, payload, payload_size);
	esp_msg.data_length_code = payload_size;
	esp_msg.flags = CAN_MSG_FLAG_EXTD;

	if (can_transmit(&esp_msg, 0) != ESP_OK) {
		return -2;
	}

	ui_can_tx();

	return 0;
}

void can2_receive()
{
	can_message_t esp_msg;

	while (can_receive(&esp_msg, 0) == ESP_OK) {
		uint32_t id = esp_msg.identifier;

		// ignore not-extended-format messages
		if (!(esp_msg.flags & CAN_MSG_FLAG_EXTD)) {
			return;
		}

		if (esp_msg.data_length_code > 8) {
			log_error("ESP CAN frame too big (%u > 8)!",
				  esp_msg.data_length_code);
			return;
		}

		ui_can_rx();
		can2_on_receive(esp_msg.identifier, esp_msg.data,
				esp_msg.data_length_code);
	}
}
#endif // ifdef WITH_CAN

// ---------------------------------------------- wifi -------------------------

#ifdef WITH_WIFI

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event);

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

	if (!WAIT_BITS(WIFI_READY_BIT)) {
		die(DEATH_INITIALIZATION_TIMEOUT);
	}

	return 0;
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
	switch (event->event_id) {
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		ui_wifi_ok(true);
		xEventGroupSetBits(global_event_group, WIFI_READY_BIT);
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		ui_wifi_ok(false);
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

// ---------------------------------------------- MQTT -------------------------

#ifdef WITH_MQTT

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);
static void mqtt_handle_msg(esp_mqtt_event_handle_t event);

static esp_mqtt_client_handle_t mqtt_client;
static SemaphoreHandle_t publish_mutex;

int mqtt_init()
{
	esp_mqtt_client_config_t mqtt_cfg = {
		.uri = MQTT_BROKER_URL,
		.event_handle = mqtt_event_handler,
		.lwt_topic = MQTT_STATUS_TOPIC,
		.lwt_msg = MQTT_STATUS_OFFLINE_MSG,
		.lwt_qos = 1,
		.lwt_retain = 1,
		.username = MQTT_USERNAME,
		.password = MQTT_PASSWORD,
	};

	if ((publish_mutex = xSemaphoreCreateMutex()) == NULL) {
		return -1;
	}

	if ((mqtt_client = esp_mqtt_client_init(&mqtt_cfg)) == NULL) {
		return -2;
	}

	// wait for wifi
	if (!WAIT_BITS(WIFI_READY_BIT)) {
		die(DEATH_INITIALIZATION_TIMEOUT);
	}

	RET_CHECK(esp_mqtt_client_start(mqtt_client), "mqtt client start");

	if (!WAIT_BITS(MQTT_READY_BIT)) {
		die(DEATH_INITIALIZATION_TIMEOUT);
	}

	return 0;
}

// We provide topic_len because it can be determined statically for constant strings
// so we do not need to call strlen again and again...
static bool is_topic(esp_mqtt_event_handle_t event, const char *topic,
		     size_t topic_len)
{
	// topic is zero-terminated string, event->topic is not, therefore we are using topic_len-1
	return strncmp((const char *)event->topic, topic,
		       MIN(event->topic_len, topic_len - 1)) == 0;
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
	esp_mqtt_client_handle_t client = event->client;
	switch (event->event_id) {
	case MQTT_EVENT_CONNECTED:
		mqtt_publish5(MQTT_STATUS_TOPIC, MQTT_STATUS_STARTING_MSG, 0, 1,
			      1);
		esp_mqtt_client_subscribe(client, MQTT_SUBTOPIC("#"), 1);
#ifdef MQTT_WALL_CLOCK_TOPIC
		esp_mqtt_client_subscribe(client, MQTT_WALL_CLOCK_TOPIC, 0);
#endif
		break;
	case MQTT_EVENT_SUBSCRIBED:
		xEventGroupSetBits(global_event_group, MQTT_READY_BIT);
		log_debug("mqtt subscribed");
		ui_mqtt_ok(true);
		break;
	case MQTT_EVENT_ERROR:
		log_error("mqtt error");
		ui_mqtt_ok(false);
		break;
	case MQTT_EVENT_DISCONNECTED:
		xEventGroupClearBits(global_event_group, MQTT_READY_BIT);
		ui_mqtt_ok(false);
		break;
	case MQTT_EVENT_PUBLISHED:
		ui_mqtt_ok(true);
		break;
	case MQTT_EVENT_DATA:
		log_debug("MQTT-> %.*s: %.*s", event->topic_len, event->topic,
			  event->data_len, event->data);
		mqtt_handle_msg(event);
		break;
	case MQTT_EVENT_UNSUBSCRIBED:
	case MQTT_EVENT_BEFORE_CONNECT:
		break;
	}
	return ESP_OK;
}

static void mqtt_handle_msg(esp_mqtt_event_handle_t event)
{
#ifdef MQTT_WALL_CLOCK_TOPIC
	if (is_topic(event, MQTT_WALL_CLOCK_TOPIC,
		     sizeof(MQTT_WALL_CLOCK_TOPIC))) {
		xEventGroupSetBits(global_event_group, GOT_CLOCK_BIT);
		if (hal_parse_time(event->data, event->data_len,
				   (uint8_t *)&wc_hrs, (uint8_t *)&wc_mins,
				   (uint8_t *)&wc_secs)) {
			log_error("Invalid wall clock time format");
		}
		return;
	}
#endif
	// PLC pause
	if (is_topic(event, MQTT_PAUSE_TOPIC, sizeof(MQTT_PAUSE_TOPIC))) {
		if (event->data_len > 0) {
			if (event->data[0] != '0') {
				plc_set_state(PLC_STATE_PAUSED);
			} else {
				plc_set_state(PLC_STATE_RUNNING);
			}
		}
		return;
	}

	// reset
	if (is_topic(event, MQTT_RESET_TOPIC, sizeof(MQTT_RESET_TOPIC))) {
		if (event->data_len > 0) {
			hal_restart();
		}
		return;
	}

	log_error("unexpected mqtt msg: topic=%.*s payload=%.*s",
		  event->topic_len, event->topic, event->data_len, event->data);
}

/*
NOTE from ESP-IDF manual:
This API could be executed from a user task or from a mqtt event callback i.e. internal mqtt task
(API is protected by internal mutex, so it might block if a longer data
receive operation is in progress.

But this is obviously not the case with our ESP-IDF version. We will use our own
lock and hope it will mitigate te problem.
See https://github.com/espressif/esp-idf/issues/2975 - locking added to ESP-IDF v. 4.1-dev
*/
int mqtt_publish5(const char *topic, const char *data, int data_len, int qos,
		  int retain)
{
	if (!publish_mutex) {
		log_warning("mqtt publish before mqtt init");
		return -3;
	}
	if (xSemaphoreTake(publish_mutex,
			   pdMS_TO_TICKS(MQTT_MAX_PUBLISH_WAIT))) {
		int res = esp_mqtt_client_publish(mqtt_client, topic, data,
						  data_len, qos, retain);
		xSemaphoreGive(publish_mutex);
		return (res == -1) ? -1 : 0;
	}
	log_error("Failed to acquire mqtt lock");

	return -2;
}

int mqtt_publish(const char *topic, const char *data, int data_len)
{
	return mqtt_publish5(topic, data, data_len, 0, 0);
}

#endif // ifdef WITH_MQTT

// ---------------------------------------------- misc -------------------------

void hal_restart(void)
{
	esp_restart();
}

void die(uint8_t reason)
{
	PRINTF("\n\nDYING BECAUSE %d\n\n", reason);
	hal_restart();
}

uint64_t hal_uptime_usec()
{
	// NOTE: esp_timer_get_time returns int64_t only but it's still
	//       OK - overflows in ~292471 years :)
	return esp_timer_get_time();
}

uint32_t hal_uptime_msec()
{
	return esp_timer_get_time() / 1000;
}

#endif // #ifdef ESP32
