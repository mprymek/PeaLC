#include <Arduino.h>

#include "uavcan_node.h"
#include "uavcan_automation.h"

#include "automation/SetValues.h"
#include "automation/GetValues.h"

#include "app_config.h"
#include "hal.h"
#include "io.h"
#include "uavcan_impl.h"

int uavcan2_init()
{
	// init CAN HW
	int res;
	if ((res = can2_init())) {
		return res;
	}

	// init UAVCAN
	uavcan_init();

	uavcan_node_status.health = UAVCAN_PROTOCOL_NODESTATUS_HEALTH_OK;
	uavcan_node_status.mode =
		UAVCAN_PROTOCOL_NODESTATUS_MODE_INITIALIZATION;

	//uavcan_node_info.hardware_version.major = HW_VERSION_MAJOR;
	//uavcan_node_info.hardware_version.minor = HW_VERSION_MINOR;
	uavcan_get_unique_id(uavcan_node_info.hardware_version.unique_id);
	//uavcan_node_info.hardware_version.certificate_of_authenticity.data = ;
	//uavcan_node_info.hardware_version.certificate_of_authenticity.len = 0;
	uavcan_node_info.software_version.major = APP_VERSION_MAJOR;
	uavcan_node_info.software_version.minor = APP_VERSION_MINOR;
	//uavcan_node_info.software_version.image_crc = ;
	//uavcan_node_info.software_version.optional_field_flags = ;
	//uavcan_node_info.software_version.vcs_commit = GIT_HASH;
	uavcan_node_info.name.data = (uint8_t *)APP_NAME;
	uavcan_node_info.name.len = strlen(APP_NAME);

	return 0;
}

bool uavcan_user_should_accept_transfer(const CanardInstance *ins,
					uint64_t *out_data_type_signature,
					uint16_t data_type_id,
					CanardTransferType transfer_type,
					uint8_t source_node_id)
{
	if (uavcan_automation_should_accept_transfer(
		    ins, out_data_type_signature, data_type_id, transfer_type,
		    source_node_id)) {
		return true;
	}

	return false;
}

void uavcan_user_on_transfer_received(CanardInstance *ins,
				      CanardRxTransfer *transfer)
{
	if (uavcan_automation_on_transfer_received(ins, transfer)) {
		return;
	}

	PRINTS("Unexpected transfer\n");
}

void print_frame(const char *direction, const CanardCANFrame *frame)
{
	uint32_t id =
		frame->id & (~(CANARD_CAN_FRAME_EFF | CANARD_CAN_FRAME_ERR |
			       CANARD_CAN_FRAME_RTR));
	PRINTS(direction);
	PRINTS("  ");
	PRINTX(id);
	for (int i = 0; i < frame->data_len; i++) {
		PRINTS(" ");
		PRINTX(frame->data[i]);
	}
	PRINTS("\n");
}

uint8_t automation_set_dos(uint8_t source_node_id, uint8_t start_index,
			   const bool *values, uint8_t len)
{
	for (int i = 0; i < len; i++) {
		if (io_set_do(start_index + i, values[i]) != IO_OK) {
			return 1;
		}
	}
#if LOGLEVEL >= LOGLEVEL_DEBUG
	PRINTS("DO");
	PRINTU(start_index);
	PRINTS("-");
	PRINTU(start_index + len);
	PRINTS(" :=");
	for (int i = 0; i < len; i++) {
		PRINTS(" ");
		PRINTU(values[i]);
	}
	PRINTS("\n");
#endif

	return 0;
}

uint8_t automation_set_aos(uint8_t source_node_id, uint8_t start_index,
			   const uint16_t *values, uint8_t len)
{
	for (int i = 0; i < len; i++) {
		if (io_set_ao(start_index + i, values[i]) != IO_OK) {
			return 1;
		}
	}

#if LOGLEVEL >= LOGLEVEL_DEBUG
	PRINTS("AO");
	PRINTU(start_index);
	PRINTS("-");
	PRINTU(start_index + len);
	PRINTS(" :=");
	for (int i = 0; i < len; i++) {
		PRINTS(" ");
		PRINTU(values[i]);
	}
	PRINTS("\n");
#endif

	return 0;
}

uint8_t automation_get_dis(uint8_t source_node_id, uint8_t start_index,
			   bool *values, uint8_t len)
{
	for (int i = 0; i < len; i++) {
		switch (io_get_di(start_index + i, &values[i])) {
		case IO_OK:
			break;
		case IO_DOES_NOT_EXIST:
			return AUTOMATION_GETVALUES_RESPONSE_BAD_ARGUMENT;
		default:
			return AUTOMATION_GETVALUES_RESPONSE_HW_ERROR;
		}
	}

	// TODO: debug print

	return AUTOMATION_GETVALUES_RESPONSE_OK;
}

uint8_t automation_get_ais(uint8_t source_node_id, uint8_t start_index,
			   uint16_t *values, uint8_t len)
{
	for (int i = 0; i < len; i++) {
		switch (io_get_ai(start_index + i, &values[i])) {
		case IO_OK:
			break;
		case IO_DOES_NOT_EXIST:
			return AUTOMATION_GETVALUES_RESPONSE_BAD_ARGUMENT;
		default:
			return AUTOMATION_GETVALUES_RESPONSE_HW_ERROR;
		}
	}

	// TODO: debug print

	return AUTOMATION_GETVALUES_RESPONSE_OK;
}

// not used
void uavcan_on_node_status(uint8_t source_node_id,
			   uavcan_protocol_NodeStatus *node_status)
{
}
void automation_on_get_dis_response(uint8_t source_node_id, uint8_t start_index,
				    bool *values, uint8_t len)
{
}
void automation_on_get_ais_response(uint8_t source_node_id, uint8_t start_index,
				    uint16_t *values, uint8_t len)
{
}
void automation_on_tell_dis(uint8_t source_node_id, uint8_t index, bool *values,
			    uint8_t len)
{
}
void automation_on_tell_ais(uint8_t source_node_id, uint8_t index,
			    uint16_t *values, uint8_t len)
{
}

// ---------------------------------------------- UAVCAN callbacks -------------

uint64_t uavcan_uptime_usec()
{
	return hal_uptime_usec();
}

uint32_t uavcan_uptime_sec()
{
	return hal_uptime_msec() / 1000;
}
