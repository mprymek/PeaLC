#include "app_config.h"
#include "hal.h"
#include "io.h"

static const uint8_t DI_PIN[DIS_NUM] = DIS_PINS;
static const uint8_t DO_PIN[DOS_NUM] = DOS_PINS;
static const uint8_t AI_PIN[AIS_NUM] = AIS_PINS;
static const uint8_t AO_PIN[AOS_NUM] = AOS_PINS;

#ifdef DOS_PINS_INVERTED
#define DO_PIN_VALUE(x) (!(x))
#else
#define DO_PIN_VALUE(x) (x)
#endif

#ifdef DIS_PINS_INVERTED
#define DI_PIN_VALUE(x) (!(x))
#else
#define DI_PIN_VALUE(x) (x)
#endif

#define AO_PIN_VALUE(x) (x)
#define AI_PIN_VALUE(x) (x)

int io_init()
{
    int res;

    for (uint8_t i = 0; i < DIS_NUM; i++)
    {
        if ((res = set_pin_mode_di(DI_PIN[i])))
        {
            return res;
        }
    }
    for (uint8_t i = 0; i < DOS_NUM; i++)
    {
        if ((res = set_pin_mode_do(DO_PIN[i])))
        {
            return res;
        }
    }
    for (uint8_t i = 0; i < AIS_NUM; i++)
    {
        if ((res = set_pin_mode_ai(AI_PIN[i])))
        {
            return res;
        }
    }
    for (uint8_t i = 0; i < AOS_NUM; i++)
    {
        if ((res = set_pin_mode_ao(AO_PIN[i])))
        {
            return res;
        }
    }

    return 0;
}

uint8_t io_set_do(uint8_t index, bool value)
{
    log_debug("DO%d (pin %d) := %d", index, DO_PIN[index], value);
    if (index >= DOS_NUM)
    {
        return IO_DOES_NOT_EXIST;
    }
    if (set_do_pin_value(DO_PIN[index], DO_PIN_VALUE(value)))
    {
        return IO_HW_ERROR;
    }
    return IO_OK;
}

uint8_t io_set_ao(uint8_t index, uint16_t value)
{
    log_debug("AO%d (pin %d) = %u", index, AO_PIN[index], value);
    if (index >= AOS_NUM)
    {
        return IO_DOES_NOT_EXIST;
    }
    if (set_ao_pin_value(AO_PIN[index], AO_PIN_VALUE(value)))
    {
        return IO_HW_ERROR;
    }
    return IO_OK;
}

uint8_t io_get_di(uint8_t index, bool *value)
{
    bool value2;

    if (index >= DIS_NUM)
    {
        return IO_DOES_NOT_EXIST;
    }
    if (get_di_pin_value(DI_PIN[index], &value2))
    {
        return IO_HW_ERROR;
    }
    *value = DI_PIN_VALUE(value2);
    log_debug("DI%d (pin %d) = %d", index, DI_PIN[index], value2);
    return IO_OK;
}

uint8_t io_get_ai(uint8_t index, uint16_t *value)
{
    uint16_t value2;

    if (index >= AIS_NUM)
    {
        return IO_DOES_NOT_EXIST;
    }
    if (get_ai_pin_value(AI_PIN[index], &value2))
    {
        return IO_HW_ERROR;
    }
    *value = AI_PIN_VALUE(value2);
    log_debug("AI%d (pin %d) = %u", index, AI_PIN[index], value2);
    return IO_OK;
}
