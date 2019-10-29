#include "app_config.h"
#include "hal.h"
#include "io.h"
#include "plc.h"
#include "tools.h"
#include "ui.h"
#include "uavcan_impl.h"

static void main_init();
static void main_task(void *pvParameters);

#define START(lbl, init_fun)        \
    {                               \
        ui_set_status(lbl);         \
        PRINTF(lbl " ... ");        \
        if (init_fun)               \
        {                           \
            die(DEATH_INIT_FAILED); \
        }                           \
        else                        \
        {                           \
            PRINTF("OK\n");         \
        }                           \
    }

static void main_init()
{
    if (log_init())
    {
        die(DEATH_INIT_FAILED);
    }
    PRINTF("\n");
    PRINTF("-----------------------------------------------------\n");
    PRINTF("%s v. %d.%d (UAVCAN v.0)\n\n", APP_NAME, APP_VERSION_MAJOR, APP_VERSION_MINOR);

    START("ui", ui_init());
    START("io", io_init());
    START("UAVCAN", uavcan2_init());
    START("plc", plc_init());

    PRINTF("-----------------------------------------------------\n");
    ui_set_status("running");
}

static void main_task(void *pvParameters)
{
    TickType_t delay = portMAX_DELAY;

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
}

#ifdef ESP32
void app_main()
{
    main_init();
    main_task(NULL);
    die(DEATH_UNREACHABLE_REACHED);
}
#endif // ifdef ESP32
