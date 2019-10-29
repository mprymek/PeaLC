#include "app_config.h"
#include "hal.h"
#include "tools.h"
#include "ui.h"


int ui_init()
{
    uint8_t pins[] = {
#ifdef PLC_TICK_PIN
        PLC_TICK_PIN,
#endif
#ifdef CAN_RX_OK_PIN
        CAN_RX_OK_PIN,
#endif
#ifdef CAN_TX_OK_PIN
        CAN_TX_OK_PIN,
#endif
    };

    for (int i = 0; i < sizeof(pins); i++)
    {
        if (set_do_pin_value(pins[i], STATUS_LED_VALUE(false))) {
            return -1;
        }
        if (set_pin_mode_do(pins[i])) {
            return -2;
        }
    }

    return 0;
}

void ui_set_status(const char *status) {
    PRINTF("status: %s\n", status);
}

void ui_plc_tick()
{
#ifdef PLC_TICK_PIN
    MAX_ONCE_PER(UI_MIN_PLC_TICK_DELAY, {
        static bool state = false;
        state = !state;
        set_do_pin_value(PLC_TICK_PIN, STATUS_LED_VALUE(state));
    });
#endif
}

void ui_can_rx()
{
#ifdef CAN_RX_OK_PIN
    MAX_ONCE_PER(UI_MIN_CAN_RX_OK_DELAY, {
        static bool state = false;
        state = !state;
        set_do_pin_value(CAN_RX_OK_PIN, STATUS_LED_VALUE(state));
    });
#endif
}

void ui_can_tx()
{
#ifdef CAN_TX_OK_PIN
    MAX_ONCE_PER(UI_MIN_CAN_TX_OK_DELAY, {
        static bool state = false;
        state = !state;
        set_do_pin_value(CAN_TX_OK_PIN, STATUS_LED_VALUE(state));
    });
#endif
}
