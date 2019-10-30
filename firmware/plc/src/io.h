#include <stdint.h>
#include <stdbool.h>

#define IO_OK 0
#define IO_HW_ERROR 1
#define IO_DOES_NOT_EXIST 2

#ifdef __cplusplus
extern "C"
{
#endif

int io_init();

uint8_t io_set_do(uint8_t index, bool value);
uint8_t io_set_ao(uint8_t index, uint16_t value);
uint8_t io_get_di(uint8_t index, bool *value);
uint8_t io_get_ai(uint8_t index, uint16_t *value);

extern uint16_t virt_ais[];

#ifdef __cplusplus
}
#endif
