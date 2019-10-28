#include <iec_std_lib.h>

#include "app_config.h"
#include "hal.h"
#include "io.h"
#include "plc.h"

static void connect_buffers(void);
static void plc_task(void *pvParameters);
static void update_time(void);
static void plc_run(void);
static void update_outputs(void);
static void update_inputs(void);

static TaskHandle_t plc_task_h = NULL;
static bool running = true;
static uint64_t tick = 0;

#define BILLION 1000000000
#define MILLION 1000000

int plc_init()
{
    // initialize PLC program
    config_init__();
    connect_buffers();

    if (xTaskCreate(plc_task, "plc", STACK_SIZE_PLC, NULL, configMAX_PRIORITIES - 1, &plc_task_h) != pdPASS)
    {
        log_error("Failed to create plc task.");
        die(DEATH_TASK_CREATION);
    }

    return 0;
}

static void plc_task(void *pvParameters)
{
    TickType_t last_wake = xTaskGetTickCount();

    for (;;)
    {
        update_time();

        if (running)
        {
            ui_plc_tick();
#if LOGLEVEL >= LOGLEVEL_INFO
            float now_f = __CURRENT_TIME.tv_sec + __CURRENT_TIME.tv_nsec / (float)1000000000;
            PRINTF("\ntick=%llu    time=%.3fs    tick_time=%llums\n", tick, now_f, common_ticktime__ / MILLION);
#endif

#if LOGLEVEL >= LOGLEVEL_DEBUG
            // measure clock precission
            static uint32_t last_time = 0;
            uint32_t now = uptime_usec();
            int32_t tdiff = now - last_time;
            PRINTF("time=%dus    precision: %d - %lld = %lldus\n", now, tdiff, (common_ticktime__ / 1000), tdiff - (common_ticktime__ / 1000));
            last_time = now;
#endif

            update_inputs();
            // execute plc program
            config_run__(tick);
            update_outputs();
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(common_ticktime__ / MILLION));
    }
}

/*
NOTE: This is how __CURRENT_TIME is updated in OpenPLC. PLC clock can get
skewed if PLC task is not fired at the right moment. I don't know if
regular time increments are essential for OpenPLC runtime functions
so I think it's safer to stick with their implementation...
*/
static void update_time()
{
    __CURRENT_TIME.tv_nsec += common_ticktime__;
    while (__CURRENT_TIME.tv_nsec >= BILLION)
    {
        __CURRENT_TIME.tv_nsec -= BILLION;
        __CURRENT_TIME.tv_sec++;
    }

    tick++;
}

// ---------------------------------------------- program-specific vars --------

// generates something like:
//  BOOL __QX0_0;
#define __LOCATED_VAR(type, name, ...) type __##name;
#include "LOCATED_VARIABLES.h"
#undef __LOCATED_VAR

// generates something like:
//  BOOL *QX0_0 = &__QX0_0;
#define __LOCATED_VAR(type, name, ...) type *name = &__##name;
#include "LOCATED_VARIABLES.h"
#undef __LOCATED_VAR

// ---------------------------------------------- buffers ----------------------

//Booleans
IEC_BOOL *bool_input[IO_BUFFER_SIZE][8];
IEC_BOOL *bool_output[IO_BUFFER_SIZE][8];

//Bytes
//IEC_BYTE *byte_input[IO_BUFFER_SIZE];
//IEC_BYTE *byte_output[IO_BUFFER_SIZE];

//Analog I/O
IEC_UINT *int_input[IO_BUFFER_SIZE];
IEC_UINT *int_output[IO_BUFFER_SIZE];

//Memory
//IEC_UINT *int_memory[IO_BUFFER_SIZE];
//IEC_DINT *dint_memory[IO_BUFFER_SIZE];
//IEC_LINT *lint_memory[IO_BUFFER_SIZE];

//Special Functions
//IEC_LINT *special_functions[IO_BUFFER_SIZE];

#define AIDX(i) ((i) / 8)
#define BIDX(i) ((i) % 8)

void connect_buffers()
{
    // connect program vars to IO buffer
#define POOL_BOOL_I bool_input
#define POOL_BOOL_Q bool_output
#define POOL_UINT_I int_input
#define POOL_UINT_Q int_output
#define INDEX_BOOL(a, b) [a][b]
#define INDEX_UINT(a, b) [a]

// generates something like:
//      POOL_BOOL_Q[0][0] = __QX0_0;
// i.e.
//      bool_output[0][0] = __QX0_0;
#define __LOCATED_VAR(type, name, inout, type_sym, a, b) POOL_##type##_##inout INDEX_##type(a, b) = name;
#include "LOCATED_VARIABLES.h"
#undef __LOCATED_VAR

}


// ---------------------------------------------- IO ---------------------------

void update_inputs()
{
    log_debug("updating inputs");

    // local
    for (uint8_t i = 0; i < REMOTE_VARS_INDEX; i++)
    {
        uint8_t a = AIDX(i);
        uint8_t b = BIDX(i);
        IEC_BOOL *bval = bool_input[a][b];
        if (bval != NULL)
        {
            bool val2;
            io_get_di(i, &val2);
            log_debug("IX%u.%u = %u", a, b, val2);
            *bval = val2;
        }

        IEC_UINT *uval = int_input[i];
        if (uval != NULL)
        {
            io_get_ai(i, uval);
            log_debug("IW%u.%u = %u", a, b, *uval);
        }
    }

}

void update_outputs()
{
    log_debug("updating outputs");

    // local
    for (uint8_t i = 0; i < REMOTE_VARS_INDEX; i++)
    {
        uint8_t a = AIDX(i);
        uint8_t b = BIDX(i);
        IEC_BOOL *bval = bool_output[a][b];
        if (bval != NULL)
        {
            log_debug("QX%u.%u = %u", a, b, *bval);
            io_set_do(i, *bval);
        }

        IEC_UINT *uval = int_output[i];
        if (uval != NULL)
        {
            log_debug("QW%u.%u = %u", a, b, *uval);
            io_set_ao(i, *uval);
        }
    }

}
