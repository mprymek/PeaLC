#ifdef ESP32

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// NOTE: FreeRTOS headers must be included before `driver/can.h`
#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <driver/can.h>

#include <uavcan_node.h>

#include "app_config.h"
#include "uavcan_impl.h"
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

void ui_can_rx()
{
#ifdef CAN_RX_OK_PIN
    MAX_ONCE_PER(UI_MIN_CAN_RX_OK_DELAY, {
        static bool state = false;
        state = !state;
        gpio_set_level(CAN_RX_OK_PIN, state);
    });
#endif
}

void ui_can_tx()
{
#ifdef CAN_TX_OK_PIN
    MAX_ONCE_PER(UI_MIN_CAN_TX_OK_DELAY, {
        static bool state = false;
        state = !state;
        gpio_set_level(CAN_TX_OK_PIN, state);
    });
#endif
}


// ---------------------------------------------- CAN --------------------------

static void can_watch_task(void *pvParameters);
TaskHandle_t can_watch_task_h = NULL;
volatile can_bus_state_t can_bus_state = CANBS_ERR_PASSIVE;

int can2_init()
{
    //Initialize configuration structures using macro initializers
    // tx, rx pins
    can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, CAN_MODE_NORMAL);
    can_timing_config_t t_config = CAN_TIMING_CONFIG_250KBITS();
    can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

    //Install CAN driver
    if (!can_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
    {
        log_error("Failed to install CAN driver");
        return -1;
    }

    // Reconfigure alerts to detect Error Passive and Bus-Off error states
    uint32_t alerts_to_enable = CAN_ALERT_ALL & (~(CAN_ALERT_TX_IDLE | CAN_ALERT_TX_SUCCESS));
    if (!can_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK)
    {
        log_error("Failed to reconfigure alerts");
        return -2;
    }

    // Start CAN driver
    if (can_start() != ESP_OK)
    {
        log_error("Failed to start driver");
        return -3;
    }

    if (xTaskCreate(can_watch_task, "can-w", configMINIMAL_STACK_SIZE + 2048, NULL, configMAX_PRIORITIES - 2, &can_watch_task_h) != pdPASS)
    {
        log_error("Failed to create can-w task");
        return -4;
    }

    return 0;
}

static void can_watch_task(void *pvParameters)
{
    uint32_t alerts;

    for (;;)
    {
        can_read_alerts(&alerts, 0);
        if (alerts != 0)
        {
            if (alerts & CAN_ALERT_BELOW_ERR_WARN)
            {
                log_info("CAN: below error warning limit");
            }
            if (alerts & CAN_ALERT_ERR_ACTIVE)
            {
                log_warning("CAN: entered error active state");
                can_bus_state = CANBS_ERR_ACTIVE;
            }
            if (alerts & CAN_ALERT_RECOVERY_IN_PROGRESS)
            {
                log_warning("CAN: bus recovery in progress");
            }
            if (alerts & CAN_ALERT_BUS_RECOVERED)
            {
                // NOTE: bus is "recovered" even when in fact there's still error on the bus
                log_info("CAN: bus recovered");
                // After recovery, the driver enters stopped state - we must start
                // it again.
                // See https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/can.html#driver-operation
                if (can_start() != ESP_OK)
                {
                    log_error("CAN: failed to start driver");
                    die(DEATH_CAN_DRIVER_ERROR);
                }
            }
            if (alerts & CAN_ALERT_ARB_LOST)
            {
                log_debug("CAN: arbitration lost");
            }
            if (alerts & CAN_ALERT_ABOVE_ERR_WARN)
            {
                log_warning("CAN: surpassed error warning limit");
            }
            if (alerts & CAN_ALERT_BUS_ERROR)
            {
                log_error("CAN: bus error");
            }
            if (alerts & CAN_ALERT_TX_FAILED)
            {
                log_error("CAN: TX failed");
            }
            if (alerts & CAN_ALERT_RX_QUEUE_FULL)
            {
                log_error("CAN: RX queue full");
            }
            if (alerts & CAN_ALERT_ERR_PASS)
            {
                log_error("CAN: entered error passive state");
                can_bus_state = CANBS_ERR_PASSIVE;
            }
            if (alerts & CAN_ALERT_BUS_OFF)
            {
                log_error("CAN: entered bus off state");
                can_bus_state = CANBS_BUS_OFF;
                for (int i = 3; i > 0; i--)
                {
                    log_warning("CAN: initiating bus recovery in %d", i);
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

void uavcan_get_unique_id(uint8_t out_uid[UAVCAN_PROTOCOL_HARDWAREVERSION_UNIQUE_ID_LENGTH])
{
    esp_efuse_mac_get_default(out_uid);
    for (int i = 6; i < UAVCAN_PROTOCOL_HARDWAREVERSION_UNIQUE_ID_LENGTH; i++)
    {
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

    if (can_receive(&esp_msg, 0) != ESP_OK)
    {
        return 0;
    }

    ui_can_rx();

    frame->id = esp_msg.identifier;
    if (esp_msg.flags & CAN_MSG_FLAG_EXTD)
    {
        frame->id |= CANARD_CAN_FRAME_EFF;
    }
    if (esp_msg.flags & CAN_MSG_FLAG_RTR)
    {
        frame->id |= CANARD_CAN_FRAME_RTR;
    }
    // TODO: error flag?!

    if (esp_msg.data_length_code > CANARD_CAN_FRAME_MAX_DATA_LEN)
    {
        log_error("ESP CAN frame too big (%u > %u)!", esp_msg.data_length_code, CANARD_CAN_FRAME_MAX_DATA_LEN);
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

    esp_msg.identifier = frame->id & (~(CANARD_CAN_FRAME_EFF | CANARD_CAN_FRAME_ERR | CANARD_CAN_FRAME_RTR));
    esp_msg.flags = 0;
    if (frame->id & CANARD_CAN_FRAME_EFF)
    {
        esp_msg.flags |= CAN_MSG_FLAG_EXTD;
    }
    if (frame->id & CANARD_CAN_FRAME_RTR)
    {
        esp_msg.flags |= CAN_MSG_FLAG_RTR;
    }
    // TODO: error flag?!

    if (frame->data_len > sizeof(esp_msg.data))
    {
        log_error("Canard frame too big (%u > %lu)!", frame->data_len, sizeof(esp_msg.data));
        return -1;
    }

    memcpy(esp_msg.data, frame->data, frame->data_len);
    esp_msg.data_length_code = frame->data_len;

#if LOGLEVEL >= LOGLEVEL_DEBUG
    print_frame("<-", frame);
#endif

    if (!can_transmit(&esp_msg, pdMS_TO_TICKS(10)) == ESP_OK)
    {
        log_error("CAN TX error");
        return -2;
    }

    ui_can_tx();

    return 0;
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
