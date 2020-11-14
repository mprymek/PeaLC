#include "app_config.h"

#include <canard.h>
#include <canard_dsdl.h>

#include "hal.h"
#include "io.h"
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
#if LOGLEVEL >= LOGLEVEL_DEBUG
	PRINTS("-> DO");
	PRINTU(index);
	PRINTS("-");
	PRINTU(index + length);
	PRINTS(" :=");
#endif

	uint8_t result;
	bool value;
	for (int i = 0; i < length; i++) {
		value = canardDSDLGetBit(data, DIV_UP(length, 8), i);
#if LOGLEVEL >= LOGLEVEL_DEBUG
		PRINTS(" ");
		PRINTU(value);
#endif
		if ((result = io_set_do(index + i, value)) != IO_OK) {
			break;
		}
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
#if LOGLEVEL >= LOGLEVEL_DEBUG
	PRINTS("-> AO");
	PRINTU(index);
	PRINTS("-");
	PRINTU(index + length);
	PRINTS(" :=");
#endif

	uint8_t result;
	uint16_t value;
	for (int i = 0; i < length; i++) {
		value = canardDSDLGetU16(data, length * 2, i * 16, 16);
#if LOGLEVEL >= LOGLEVEL_DEBUG
		PRINTS(" ");
		PRINTU(value);
#endif
		if ((result = io_set_ao(index + i, value)) != IO_OK) {
			break;
		}
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
	PRINTU(index + length);
	PRINTS(" = ?\n");
#endif

	uint8_t result;
	bool value;
	for (int i = 0; i < length; i++) {
		if ((result = io_get_di(index + i, &value)) != IO_OK) {
			break;
		}
		canardDSDLSetUxx(&data[0], i, value, 1);
	}
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
	PRINTU(index + length);
	PRINTS(" = ?\n");
#endif

	uint8_t result;
	uint16_t value;
	for (int i = 0; i < length; i++) {
		if ((result = io_get_ai(index + i, &value)) != IO_OK) {
			break;
		}
		canardDSDLSetUxx(&data[0], i * 16, value, 16);
	}
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
