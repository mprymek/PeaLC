#ifdef __cplusplus
extern "C"
{
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
#define PRINTF(format, ...) \
    printf(format, ##__VA_ARGS__)
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


// ---------------------------------------------- UI ---------------------------

int ui_init(void);
void ui_plc_tick(void);
void ui_set_status(const char *status);


// ---------------------------------------------- misc -------------------------

typedef enum
{
    DEATH_UNKNOWN = 0,
    DEATH_INIT_FAILED,
    DEATH_UNREACHABLE_REACHED,
    DEATH_TASK_CREATION,
} death_reasons_t;

void die(uint8_t reason);

uint32_t uptime_msec();
uint64_t uptime_usec();

#ifdef __cplusplus
}
#endif
