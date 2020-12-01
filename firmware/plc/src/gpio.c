#include "app_config.h"
#include "gpio.h"
#include "hal.h"

int gpio_init_input_block(const io_type_t io_type, const io_block_t *block)
{
	gpio_io_block_t *data = block->driver_data;
	int result;

	switch (io_type) {
	case IO_TYPE_DIGITAL:
		for (size_t i = 0; i < block->length; i++) {
			log_debug("initializing DI pin %u", data->pins[i]);
			if ((result = set_pin_mode_di(data->pins[i]))) {
				return result;
			}
		}
		break;
	case IO_TYPE_ANALOG:
		for (size_t i = 0; i < block->length; i++) {
			log_debug("initializing AI pin %u", data->pins[i]);
			if ((result = set_pin_mode_ai(data->pins[i]))) {
				return result;
			}
		}
		break;
	}

	return 0;
}

int gpio_init_output_block(const io_type_t io_type, const io_block_t *block)
{
	gpio_io_block_t *data = block->driver_data;
	int result;

	switch (io_type) {
	case IO_TYPE_DIGITAL:
		for (size_t i = 0; i < block->length; i++) {
			log_debug("initializing DO pin %u", data->pins[i]);
			if ((result = set_pin_mode_do(data->pins[i]))) {
				return result;
			}
		}
		break;
	case IO_TYPE_ANALOG:
		for (size_t i = 0; i < block->length; i++) {
			log_debug("initializing AO pin %u", data->pins[i]);
			if ((result = set_pin_mode_ao(data->pins[i]))) {
				return result;
			}
		}
		break;
	}

	return 0;
}

int gpio_update_input_block(const io_type_t io_type, io_block_t *block)
{
	gpio_io_block_t *data = block->driver_data;

	uint8_t result;

	switch (io_type) {
	case IO_TYPE_DIGITAL:
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
		break;
	case IO_TYPE_ANALOG:
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
		break;
	}

	return IO_OK;
}

int gpio_update_output_block(const io_type_t io_type, io_block_t *block)
{
	gpio_io_block_t *data = block->driver_data;

	if (!block->dirty) {
		return IO_OK;
	}

	uint8_t result;

	switch (io_type) {
	case IO_TYPE_DIGITAL:
		for (size_t i = 0; i < block->length; i++) {
			if (true /* TODO: used */) {
				uint8_t pin = data->pins[i];
				bool *value = &((bool *)block->buff)[i];
				if ((result = set_do_pin_value(
					     pin, (data->inverted) ? !*value :
								     *value)) !=
				    IO_OK) {
					return result;
				}
			}
		}
		block->dirty = false;
		break;
	case IO_TYPE_ANALOG:
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
		break;
	}

	return IO_OK;
}
