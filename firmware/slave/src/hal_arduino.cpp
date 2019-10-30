// common Arduino HAL

#if defined(ARDUINO)

#include <Arduino.h>

#include <uavcan_node.h>

#include "app_config.h"
#include "hal.h"


// ---------------------------------------------- logging ----------------------

int log_init(void)
{
	DBG_SERIAL.begin(115200);

    return 0;
}

void prints(const char *s)
{
    DBG_SERIAL.print(s);
}

void printu(uint16_t u)
{
    DBG_SERIAL.print(u);
}

void printx(uint16_t x)
{
    DBG_SERIAL.print(x, HEX);
}

// NOTE: We don't have printf -> print at least fmt. Something is better than nothing...

void log_error2(const char *fmt...)
{
    PRINTS(fmt); PRINTS("\n");
}

void log_warning2(const char *fmt...)
{
    PRINTS(fmt); PRINTS("\n");
}

void log_info2(const char *fmt...)
{
    PRINTS(fmt); PRINTS("\n");
}

void log_debug2(const char *fmt...)
{
    PRINTS(fmt); PRINTS("\n");
}

// ---------------------------------------------- IO ---------------------------

int set_pin_mode_di(int pin)
{
    pinMode(pin, INPUT_PULLUP);
    return 0;
}

int set_pin_mode_do(int pin)
{
#ifdef DOS_OPEN_DRAIN
    pinMode(pin, OUTPUT_OPEN_DRAIN);
#else
    pinMode(pin, OUTPUT);
#endif
    return 0;
}

int set_pin_mode_ai(int pin)
{
#ifdef INPUT_ANALOG
    pinMode(pin, INPUT_ANALOG);
#else
    pinMode(pin, INPUT);
#endif
    return 0;
}

int set_pin_mode_ao(int pin)
{
    pinMode(pin, OUTPUT);
    return 0;
}

int set_do_pin_value(int pin, bool value)
{
    digitalWrite(pin, value);
    return 0;
}

int get_di_pin_value(int pin, bool *value)
{
    *value = digitalRead(pin);
    return 0;
}

int set_ao_pin_value(int pin, uint16_t value)
{
    analogWrite(pin, value);
    return 0;
}

int get_ai_pin_value(int pin, uint16_t *value)
{
    *value = analogRead(pin);
    return 0;
}

// ---------------------------------------------- misc -------------------------

void die(uint8_t reason)
{
    PRINTS("\n\nDYING BECAUSE "); PRINTU(reason); PRINTS("\n\n");
    for (;;)
        ;
}

uint64_t uptime_usec()
{
    return micros();
}

uint32_t uptime_msec()
{
    return millis();
}


// ---------------------------------------------- UAVCAN -----------------------

#if LOGLEVEL >= LOGLEVEL_ERROR
void uavcan_error(const char *fmt, ...)
{
    // TODO: printf args...
    PRINTS(fmt);
}
#else
void uavcan_error(const char *fmt, ...) {}
#endif


#endif // #if defined(ARDUINO)