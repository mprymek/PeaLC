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

// ---------------------------------------------- IO ---------------------------

int set_pin_mode_di(int pin) {
    gpio_pad_select_gpio(pin);
    if (gpio_set_direction(pin, GPIO_MODE_INPUT) != ESP_OK)
    {
        return -1;
    }
    if (gpio_pullup_en(pin) != ESP_OK)
    {
        return -1;
    }
    return 0;
}

int set_pin_mode_do(int pin) {
    gpio_pad_select_gpio(pin);
#ifdef DOS_PINS_INVERTED
    gpio_set_level(pin, 1);
#else
    gpio_set_level(pin, 0);
#endif
    if (gpio_set_direction(pin, GPIO_MODE_OUTPUT) != ESP_OK)
    {
        return -1;
    }
    return 0;
}

int set_pin_mode_ai(int pin) {
    log_error("TODO: analog pins not implemented for ESP32");
    return -1;
}

int set_pin_mode_ao(int pin) {
    log_error("TODO: analog pins not implemented for ESP32");
    return -1;
}

int set_do_pin_value(int pin, bool value) {
    if (gpio_set_level(pin, value) != ESP_OK)
    {
        return -1;
    }
    return 0;
}

int get_di_pin_value(int pin, bool *value) {
    *value = gpio_get_level(pin);
    return 0;
}

int set_ao_pin_value(int pin, uint16_t value) {
    log_error("TODO: analog pins not implemented for ESP32");
    return -1;
}

int get_ai_pin_value(int pin, uint16_t *value) {
    log_error("TODO: analog pins not implemented for ESP32");
    return -1;
}

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
