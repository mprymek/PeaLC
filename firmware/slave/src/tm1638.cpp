#include "app_config.h"

#ifdef WITH_TM1638

#include <Arduino.h>

#include <TM1638plus.h>

#include "tm1638.h"

static void display_number(uint32_t value);

static TM1638plus tm(TM_STROBE_PIN, TM_CLOCK_PIN, TM_DIO_PIN, true);

int tm1638_init()
{
	tm.displayBegin();
	return 0;
}

int tm1638_update_input_block(const io_type_t io_type, io_block_t *block)
{
	tm1638_io_block_t *data = (tm1638_io_block_t *)block->driver_data;

	if ((io_type != IO_TYPE_DIGITAL) ||
	    (block->length + data->offset > 8)) {
		return IO_DOES_NOT_EXIST;
	}

	uint8_t buttons = tm.readButtons();

	for (size_t i = 0; i < block->length; i++) {
		bool *buff_value = &((bool *)block->buff)[i];
		bool value = buttons & (1UL << (i + data->offset));
		if (value != *buff_value) {
			*buff_value = value;
			block->dirty = true;
		}
	}

	return IO_OK;
}

int tm1638_update_output_block(const io_type_t io_type, io_block_t *block)
{
	tm1638_io_block_t *data = (tm1638_io_block_t *)block->driver_data;

	switch (io_type) {
	case IO_TYPE_DIGITAL:
		if (block->length + data->offset > 8) {
			return IO_DOES_NOT_EXIST;
		}
		for (size_t i = 0; i < block->length; i++) {
			bool *value = &((bool *)block->buff)[i];
			tm.setLED(i + data->offset, *value);
		}
		block->dirty = false;
		return IO_OK;
	case IO_TYPE_ANALOG:
		if (block->length > 1) {
			return IO_DOES_NOT_EXIST;
		}
		uint16_t *value = &((uint16_t *)block->buff)[0];
		display_number(*value);
		block->dirty = false;
		return IO_OK;
	}
	return IO_DOES_NOT_EXIST;
}

static void display_number(uint32_t value)
{
	bool leading_zero = true;
	uint32_t divider = 10000000;
	for (int pos = 0; pos < 7; pos++) {
		uint8_t digit = value / divider;
		value -= digit * divider;
		divider /= 10;
		if (digit) {
			leading_zero = false;
		}
		if (leading_zero) {
			tm.displayASCII(pos, ' ');
		} else {
			tm.displayHex(pos, digit);
		}
	}
	tm.displayHex(7, value);
}

#endif
