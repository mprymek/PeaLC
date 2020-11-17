#include "app_config.h"
#include "gpio.h"
#include "hal.h"

int gpio_init_input_block(const io_block_t *block)
{
	gpio_io_block_t *data = block->driver_data;
	int result;

	if (block->values_type == IO_VALUES_BOOL) {
		for (size_t i = 0; i < block->length; i++) {
			log_debug("initializing DI pin %u", data->pins[i]);
			if ((result = set_pin_mode_di(data->pins[i]))) {
				return result;
			}
		}
	} else if (block->values_type == IO_VALUES_UINT) {
		for (size_t i = 0; i < block->length; i++) {
			log_debug("initializing AI pin %u", data->pins[i]);
			if ((result = set_pin_mode_ai(data->pins[i]))) {
				return result;
			}
		}
	} else {
		// TODO: ERROR
	}

	return 0;
}

int gpio_init_output_block(const io_block_t *block)
{
	gpio_io_block_t *data = block->driver_data;
	int result;

	if (block->values_type == IO_VALUES_BOOL) {
		for (size_t i = 0; i < block->length; i++) {
			log_debug("initializing DO pin %u", data->pins[i]);
			if ((result = set_pin_mode_do(data->pins[i]))) {
				return result;
			}
		}
	} else if (block->values_type == IO_VALUES_UINT) {
		for (size_t i = 0; i < block->length; i++) {
			log_debug("initializing AO pin %u", data->pins[i]);
			if ((result = set_pin_mode_ao(data->pins[i]))) {
				return result;
			}
		}
	} else {
		// TODO: ERROR
	}

	return 0;
}

int gpio_update_input_block(io_block_t *block)
{
	gpio_io_block_t *data = block->driver_data;

	uint8_t result;

	if (block->values_type == IO_VALUES_BOOL) {
		for (size_t i = 0; i < block->length; i++) {
			if (true /* TODO: enabled? */) {
				uint8_t pin = data->pins[i];
				bool value;
				if ((result = get_di_pin_value(pin, &value)) !=
				    IO_OK) {
					return result;
				}
				if (data->inverted) {
					value = !value;
				}
				bool *buff_value = &((bool *)block->buff)[i];
				if (value != *buff_value) {
					*buff_value = value;
					block->dirty = true;
				}
			}
		}
	} else if (block->values_type == IO_VALUES_UINT) {
		for (size_t i = 0; i < block->length; i++) {
			if (true /* TODO: enabled? */) {
				uint8_t pin = data->pins[i];
				uint16_t value;
				if ((result = get_ai_pin_value(pin, &value)) !=
				    IO_OK) {
					return result;
				}
				uint16_t *buff_value =
					&((uint16_t *)block->buff)[i];
				if (value != *buff_value) {
					*buff_value = value;
					block->dirty = true;
				}
			}
		}
	} else {
		log_error("invalid input block type!");
		return IO_DOES_NOT_EXIST;
	}

	return IO_OK;
}

int gpio_update_output_block(io_block_t *block)
{
	gpio_io_block_t *data = block->driver_data;

	if (!block->dirty) {
		return IO_OK;
	}

	uint8_t result;

	if (block->values_type == IO_VALUES_BOOL) {
		for (size_t i = 0; i < block->length; i++) {
			if (true /* TODO: used */) {
				uint8_t pin = data->pins[i];
				bool *value = &((bool *)block->buff)[i];
				if ((result = set_do_pin_value(pin, *value)) !=
				    IO_OK) {
					return result;
				}
			}
		}
		block->dirty = false;
	} else if (block->values_type == IO_VALUES_UINT) {
		for (size_t i = 0; i < block->length; i++) {
			if (true /* TODO: used */) {
				uint8_t pin = data->pins[i];
				uint16_t *value = &((uint16_t *)block->buff)[i];
				if ((result = set_ao_pin_value(pin, *value)) !=
				    IO_OK) {
					return result;
				}
			}
		}
		block->dirty = false;
	} else {
		log_error("invalid output block type!");
		return IO_DOES_NOT_EXIST;
	}

	return IO_OK;
}
