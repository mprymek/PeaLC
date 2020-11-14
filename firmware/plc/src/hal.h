#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // needed for size_t on STM32

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------- FreeRTOS ---------------------

#ifdef ESP32
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

// ---------------------------------------------- logging ----------------------
int log_init(void);

void log_error2(const char *format, ...);
void log_warning2(const char *format, ...);
void log_info2(const char *format, ...);
void log_debug2(const char *format, ...);

#ifdef ESP32
#define PRINTF(format, ...) printf(format, ##__VA_ARGS__)
#endif

#if defined(__AVR__)
#define DBG_SERIAL Serial
#endif

#if defined(STM32F1)
#define DBG_SERIAL Serial3
#endif

#if defined(STM32F1) || defined(__AVR__)
#define PRINTS(x) prints(x)
#define PRINTU(x) printu(x)
#define PRINTX(x) printx(x)

void prints(const char *s);
void printu(uint16_t u);
void printx(uint16_t x);
#endif

#define LOGLEVEL_NOTHING 0
#define LOGLEVEL_ERROR 1
#define LOGLEVEL_WARNING 2
#define LOGLEVEL_INFO 3
#define LOGLEVEL_DEBUG 4

#ifndef LOGLEVEL
#define LOGLEVEL LOGLEVEL_WARNING
#endif

#if LOGLEVEL >= LOGLEVEL_ERROR
#define log_error log_error2
#else
#define log_error(...) ;
#endif

#if LOGLEVEL >= LOGLEVEL_WARNING
#define log_warning log_warning2
#else
#define log_warning(...) ;
#endif

#if LOGLEVEL >= LOGLEVEL_INFO
#define log_info log_info2
#else
#define log_info(...) ;
#endif

#if LOGLEVEL >= LOGLEVEL_DEBUG
#define log_debug log_debug2
#else
#define log_debug(...) ;
#endif

#ifdef WITHOUT_COM_DEBUG
#define log_com_debug(...)                                                     \
	{                                                                      \
	}
#else
#define log_com_debug log_debug
#endif

// ---------------------------------------------- IO ---------------------------

int set_pin_mode_di(int pin);
int set_pin_mode_do(int pin);
int set_pin_mode_ai(int pin);
int set_pin_mode_ao(int pin);

int set_do_pin_value(int pin, bool value);
int get_di_pin_value(int pin, bool *value);
int set_ao_pin_value(int pin, uint16_t value);
int get_ai_pin_value(int pin, uint16_t *value);

// ---------------------------------------------- CAN --------------------------

typedef enum {
	CANBS_ERR_ACTIVE = 0,
	CANBS_ERR_PASSIVE,
	CANBS_BUS_OFF,
} can_bus_state_t;

int can2_init(void);
int can2_send(uint32_t id, const void *payload, size_t payload_len);
void can2_receive();

// callbacks
void can2_on_receive(uint32_t id, const void *payload, size_t payload_size);

extern volatile can_bus_state_t can_bus_state;

// ---------------------------------------------- wifi -------------------------

int wifi_init(void);

// ---------------------------------------------- MQTT -------------------------

int mqtt_init(void);
int mqtt_publish(const char *topic, const char *data, int data_len);
int mqtt_publish5(const char *topic, const char *data, int data_len, int qos,
		  int retain);

// ---------------------------------------------- misc -------------------------

typedef enum {
	DEATH_UNKNOWN = 0,
	DEATH_INIT_FAILED,
	DEATH_UNREACHABLE_REACHED,
	DEATH_TASK_CREATION,
	DEATH_CAN_DRIVER_ERROR,
	DEATH_INITIALIZATION_TIMEOUT,
} death_reasons_t;

void die(uint8_t reason);

void hal_restart(void);
// overflows in ~49.7 days
uint32_t hal_uptime_msec();
// overflows in ~584942 years
uint64_t hal_uptime_usec();

#ifdef __cplusplus
}
#endif
