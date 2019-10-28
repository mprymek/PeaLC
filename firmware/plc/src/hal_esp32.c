#ifdef ESP32

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <driver/gpio.h>

#include "app_config.h"
#include "hal.h"
#include "tools.h"

// ---------------------------------------------- logging ----------------------

int log_init(void)
{
    // Disable buffering on stdin
    // We need this to immediatelly get logs even when there's no NL
    // (like in init "x ... y ... z OK" messages).
    setvbuf(stdout, NULL, _IONBF, 0);
    return 0;
}

#define TO_PRINTF        \
    va_list args;        \
    va_start(args, fmt); \
    vprintf(fmt, args);  \
    va_end(args);        \
    printf("\n");

void log_error2(const char *fmt, ...)
{
    TO_PRINTF
}
void log_warning2(const char *fmt, ...) { TO_PRINTF }
void log_info2(const char *fmt, ...) { TO_PRINTF }
void log_debug2(const char *fmt, ...) { TO_PRINTF }


// ---------------------------------------------- UI ---------------------------

int ui_init()
{
    uint8_t pins[] = {
#ifdef CAN_RX_OK_PIN
        CAN_RX_OK_PIN,
#endif
#ifdef CAN_TX_OK_PIN
        CAN_TX_OK_PIN,
#endif
#ifdef PLC_TICK_PIN
        PLC_TICK_PIN,
#endif
#ifdef WIFI_ERR_PIN
        WIFI_ERR_PIN,
#endif
    };

    for (int i = 0; i < sizeof(pins); i++)
    {
        gpio_pad_select_gpio(pins[i]);
        if (gpio_set_direction(pins[i], GPIO_MODE_OUTPUT) != ESP_OK)
        {
            return -1;
        }
        if (gpio_set_level(pins[i], STATUS_LED_VALUE(true)) != ESP_OK)
        {
            return -2;
        }
    }

    return 0;
}

void ui_set_status(const char *status) {
#ifdef WITH_OLED
    oled_plc_tick();
#else
    PRINTF("status: %s\n", status);
#endif
}

void ui_plc_tick()
{
#ifdef WITH_OLED
    oled_plc_tick();
#endif
#ifdef PLC_TICK_PIN
    MAX_ONCE_PER(UI_MIN_PLC_TICK_DELAY, {
        static bool state = false;
        state = !state;
        gpio_set_level(PLC_TICK_PIN, state);
    });
#endif
}

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
