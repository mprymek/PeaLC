#include "app_config.h"

#ifdef WITH_CAN

#include <canard.h>
#include <canard_dsdl.h>

#include "gpio.h"
#include "hal.h"
#include "io.h"
#include "slave.h"
#include "tools.h"
#include "uavcan_common.h"

static CanardRxSubscription set_dos_req_subscription;
static CanardRxSubscription get_dis_req_subscription;
static CanardRxSubscription set_aos_req_subscription;
static CanardRxSubscription get_ais_req_subscription;

int uavcan_init2()
{
	canardRxSubscribe(&uavcan_canard, CanardTransferKindRequest,
			  UAVCAN_SET_DOS_PORT_ID, UAVCAN_SET_DOS_REQ_EXT,
			  CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
			  &set_dos_req_subscription);

	canardRxSubscribe(&uavcan_canard, CanardTransferKindRequest,
			  UAVCAN_GET_DIS_PORT_ID, UAVCAN_GET_DIS_REQ_EXT,
			  CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
			  &get_dis_req_subscription);

	canardRxSubscribe(&uavcan_canard, CanardTransferKindRequest,
			  UAVCAN_SET_AOS_PORT_ID, UAVCAN_SET_AOS_REQ_EXT,
			  CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
			  &set_aos_req_subscription);

	canardRxSubscribe(&uavcan_canard, CanardTransferKindRequest,
			  UAVCAN_GET_AIS_PORT_ID, UAVCAN_GET_AIS_REQ_EXT,
			  CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
			  &get_ais_req_subscription);

	return 0;
}

void uavcan_update()
{
	static uint32_t last_status = 0;

	uint32_t now = hal_uptime_msec();
	if (now - last_status > UAVCAN_STATUS_PERIOD) {
		if (!uavcan_send_heartbeat()) {
			last_status = now;
		}
	}

	uavcan_send_packets();
	can2_receive();
}

uint8_t uavcan_on_set_dos_req(const CanardNodeID remote_node_id,
			      const uint8_t index, const uint8_t *const data,
			      const uint8_t length)
{
	uint8_t result;
	const size_t data_len = DIV_UP(length, 8);

#if LOGLEVEL >= LOGLEVEL_DEBUG
	PRINTS("-> DO");
	PRINTU(index);
	PRINTS("-");
	PRINTU(index + length - 1);
	PRINTS(" :=");
#endif

	size_t block_start = 0;
	bool last_block = false;
	for (size_t bi = 0; bi < digital_outputs_blocks_len; bi++) {
		io_block_t *block = &digital_outputs_blocks[bi];

		int offset = index - block_start;

		if (offset >= (int)block->length) {
			goto NEXT_BLOCK;
		}
		if (length + offset <= (int)block->length) {
			last_block = true;
		}

#if LOGLEVEL >= LOGLEVEL_DEBUG
		PRINTS(" [bl ");
		PRINTU(bi);
		PRINTS("]");
#endif

		int start = MAX(0, offset);
		int end = MIN((int)block->length, ((int)length) + offset);
		for (size_t addr = start; addr < end; addr++) {
			// AKA: block->buff[addr] = msg[addr-offset];
			bool value =
				canardDSDLGetBit(data, data_len, addr - offset);
			((bool *)block->buff)[addr] = value;
#if LOGLEVEL >= LOGLEVEL_DEBUG
			PRINTS(" ");
			PRINTU(value);
#endif
		}
		block->dirty = true;
		if ((result = update_output_block(IO_TYPE_DIGITAL, block)) != IO_OK) {
			break;
		}

		if (last_block) {
			break;
		}

	NEXT_BLOCK:
		block_start += block->length;
	}

	if (!last_block) {
		log_warning("no more IO blocks!");
		result = IO_DOES_NOT_EXIST;
	}

#if LOGLEVEL >= LOGLEVEL_DEBUG
	PRINTS("\n");
#endif

	switch (result) {
	case IO_OK:
		return UAVCAN_GET_SET_RESULT_OK;
		break;
	case IO_HW_ERROR:
		return UAVCAN_GET_SET_RESULT_HW_ERROR;
		break;
	}
	return UAVCAN_GET_SET_RESULT_BAD_ARGUMENT;
}

uint8_t uavcan_on_set_aos_req(const CanardNodeID remote_node_id,
			      const uint8_t index, const uint8_t *const data,
			      const uint8_t length)
{
	uint8_t result;
	const size_t data_len = length * 2;

#if LOGLEVEL >= LOGLEVEL_DEBUG
	PRINTS("-> AO");
	PRINTU(index);
	PRINTS("-");
	PRINTU(index + length - 1);
	PRINTS(" :=");
#endif

	size_t block_start = 0;
	bool last_block = false;
	for (size_t bi = 0; bi < analog_outputs_blocks_len; bi++) {
		io_block_t *block = &analog_outputs_blocks[bi];

		int offset = index - block_start;

		if (offset >= (int)block->length) {
			goto NEXT_BLOCK;
		}
		if (length + offset <= (int)block->length) {
			last_block = true;
		}

#if LOGLEVEL >= LOGLEVEL_DEBUG
		PRINTS(" [bl ");
		PRINTU(bi);
		PRINTS("]");
#endif

		int start = MAX(0, offset);
		int end = MIN((int)block->length, ((int)length) + offset);
		for (size_t addr = start; addr < end; addr++) {
			// AKA: block->buff[addr] = msg[addr-offset];
			uint16_t value = canardDSDLGetU16(
				data, data_len, (addr - offset) * 16, 16);
			((uint16_t *)block->buff)[addr] = value;
#if LOGLEVEL >= LOGLEVEL_DEBUG
			PRINTS(" ");
			PRINTU(value);
#endif
		}
		block->dirty = true;
		if ((result = update_output_block(IO_TYPE_ANALOG, block)) != IO_OK) {
			break;
		}

		if (last_block) {
			break;
		}

	NEXT_BLOCK:
		block_start += block->length;
	}

	if (!last_block) {
		log_warning("no more IO blocks!");
		result = IO_DOES_NOT_EXIST;
	}

#if LOGLEVEL >= LOGLEVEL_DEBUG
	PRINTS("\n");
#endif

	switch (result) {
	case IO_OK:
		return UAVCAN_GET_SET_RESULT_OK;
		break;
	case IO_HW_ERROR:
		return UAVCAN_GET_SET_RESULT_HW_ERROR;
		break;
	}
	return UAVCAN_GET_SET_RESULT_BAD_ARGUMENT;
}

uint8_t uavcan_on_get_dis_req(const CanardNodeID remote_node_id,
			      const uint8_t index, uint8_t *const data,
			      const uint8_t length)
{
#if LOGLEVEL >= LOGLEVEL_DEBUG
	PRINTS("-> DI");
	PRINTU(index);
	PRINTS("-");
	PRINTU(index + length - 1);
	PRINTS(" = ?\n");
	PRINTS("<- DI");
	PRINTU(index);
	PRINTS("-");
	PRINTU(index + length - 1);
	PRINTS(" =");
#endif

	uint8_t result;
	size_t block_start = 0;
	bool last_block = false;
	for (size_t bi = 0; bi < digital_inputs_blocks_len; bi++) {
		io_block_t *block = &digital_inputs_blocks[bi];

		int offset = index - block_start;

		if (offset >= (int)block->length) {
			goto NEXT_BLOCK;
		}
		if (length + offset <= (int)block->length) {
			last_block = true;
		}

#if LOGLEVEL >= LOGLEVEL_DEBUG
		PRINTS(" [bl ");
		PRINTU(bi);
		PRINTS("]");
#endif

		log_error(" DIB:%u(%p) ", block->driver_type, block);
		if ((result = update_input_block(IO_TYPE_DIGITAL, block)) != IO_OK) {
			break;
		}

		int start = MAX(0, offset);
		int end = MIN((int)block->length, ((int)length) + offset);
		for (size_t addr = start; addr < end; addr++) {
			// AKA: msg[addr-offset] = block->buff[addr];
			bool *value = &((bool *)block->buff)[addr];
			canardDSDLSetUxx(&data[0], addr - offset, *value, 1);
#if LOGLEVEL >= LOGLEVEL_DEBUG
			PRINTS(" ");
			PRINTU(*value);
#endif
		}

		if (last_block) {
			break;
		}

	NEXT_BLOCK:
		block_start += block->length;
	}

	if (!last_block) {
		log_warning("no more IO blocks!");
		result = IO_DOES_NOT_EXIST;
	}

#if LOGLEVEL >= LOGLEVEL_DEBUG
	PRINTS("\n");
#endif

	switch (result) {
	case IO_OK:
		return UAVCAN_GET_SET_RESULT_OK;
		break;
	case IO_HW_ERROR:
		return UAVCAN_GET_SET_RESULT_HW_ERROR;
		break;
	}
	return UAVCAN_GET_SET_RESULT_BAD_ARGUMENT;
}

uint8_t uavcan_on_get_ais_req(const CanardNodeID remote_node_id,
			      const uint8_t index, uint8_t *const data,
			      const uint8_t length)
{
#if LOGLEVEL >= LOGLEVEL_DEBUG
	PRINTS("-> AI");
	PRINTU(index);
	PRINTS("-");
	PRINTU(index + length - 1);
	PRINTS(" = ?\n");
	PRINTS("<- AI");
	PRINTU(index);
	PRINTS("-");
	PRINTU(index + length - 1);
	PRINTS(" =");
#endif

	uint8_t result;
	size_t block_start = 0;
	bool last_block = false;
	for (size_t bi = 0; bi < analog_inputs_blocks_len; bi++) {
		io_block_t *block = &analog_inputs_blocks[bi];

		int offset = index - block_start;

		if (offset >= (int)block->length) {
			goto NEXT_BLOCK;
		}
		if (length + offset <= (int)block->length) {
			last_block = true;
		}

#if LOGLEVEL >= LOGLEVEL_DEBUG
		PRINTS(" [bl ");
		PRINTU(bi);
		PRINTS("]");
#endif

		if ((result = update_input_block(IO_TYPE_ANALOG, block)) != IO_OK) {
			break;
		}

		int start = MAX(0, offset);
		int end = MIN((int)block->length, ((int)length) + offset);
		for (size_t addr = start; addr < end; addr++) {
			// AKA: msg[addr-offset] = block->buff[addr];
			uint16_t *value = &((uint16_t *)block->buff)[addr];
			canardDSDLSetUxx(&data[0], (addr - offset) * 16, *value,
					 16);
#if LOGLEVEL >= LOGLEVEL_DEBUG
			PRINTS(" ");
			PRINTU(*value);
#endif
		}

		if (last_block) {
			break;
		}

	NEXT_BLOCK:
		block_start += block->length;
	}

	if (!last_block) {
		log_warning("no more IO blocks!");
		result = IO_DOES_NOT_EXIST;
	}

#if LOGLEVEL >= LOGLEVEL_DEBUG
	PRINTS("\n");
#endif

	switch (result) {
	case IO_OK:
		return UAVCAN_GET_SET_RESULT_OK;
		break;
	case IO_HW_ERROR:
		return UAVCAN_GET_SET_RESULT_HW_ERROR;
		break;
	}
	return UAVCAN_GET_SET_RESULT_BAD_ARGUMENT;
}

// ---------------------------------------------- unused callbacks -------------

void uavcan_on_set_dos_resp(const CanardNodeID remote_node_id,
			    const uint8_t result, const uint8_t index)
{
}

void uavcan_on_set_aos_resp(const CanardNodeID remote_node_id,
			    const uint8_t result, const uint8_t index)
{
}

void uavcan_on_get_dis_resp(const CanardNodeID remote_node_id,
			    const uint8_t result, const uint8_t index,
			    const uint8_t *const data, const uint8_t length)
{
}

void uavcan_on_get_ais_resp(const CanardNodeID remote_node_id,
			    const uint8_t result, const uint8_t index,
			    const uint8_t *const data, const uint8_t length)
{
}

#endif // #ifdef WITH_CAN
