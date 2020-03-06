#include <string.h>

#include <uavcan_node.h>
#include <uavcan_automation.h>

#include <automation/SetValues.h>
#include <automation/GetValues.h>

#include "app_config.h"
#include "hal.h"
#include "locks.h"
#include "plc.h"
#include "uavcan_impl.h"

void uavcan_task(void *pvParameters);

TaskHandle_t uavcan_task_h = NULL;

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

	uavcan_broadcast_status();

	if (xTaskCreate(uavcan_task, "uavcan", STACK_SIZE_UAVCAN, NULL,
			TASK_PRIORITY_UAVCAN, &uavcan_task_h) != pdPASS) {
		log_error("Failed to create uavcan status task");
		return -2;
	}

	return 0;
}

void uavcan_task(void *pvParameters)
{
	TickType_t last_wake = xTaskGetTickCount();
	uint64_t io_rxtx_delay = common_ticktime__ / 1000;

	// We must wait for PLC buffers initialization
	// TODO: We are not broadcasting node status and are not responding to NodeInfo
	//       during this period.
	if (!WAIT_BITS(PLC_INITIALIZED_BIT)) {
		die(DEATH_INITIALIZATION_TIMEOUT);
	}

	for (;;) {
		uavcan_vals_block_t *block;
		uint64_t now = hal_uptime_usec();

		static uint64_t last_io_rxtx = 0;
		if (now - last_io_rxtx >= io_rxtx_delay) {
			last_io_rxtx = now;

			// ask for digital inputs
			for (uint8_t i = 0; i < uavcan_dis_blocks_len; i++) {
				block = &uavcan_dis_blocks[i];
				log_com_debug("<- DI%d-%d@%d = ?", block->index,
					      block->index + block->len - 1,
					      block->node_id);
				if (automation_send_get_dis(block->node_id,
							    block->index,
							    block->len) < 0) {
					log_error("get DI TX failed");
				}
			}
			uavcan_update();

			// ask for analog inputs
			for (uint8_t i = 0; i < uavcan_ais_blocks_len; i++) {
				block = &uavcan_ais_blocks[i];
				log_com_debug("<- AI%d-%d@%d = ?", block->index,
					      block->index + block->len - 1,
					      block->node_id);
				if (automation_send_get_ais(block->node_id,
							    block->index,
							    block->len) < 0) {
					log_error("get AI TX failed");
				}
			}
			uavcan_update();

			// set digital outputs
			for (uint8_t i = 0; i < uavcan_dos_blocks_len; i++) {
				block = &uavcan_dos_blocks[i];
#if LOGLEVEL >= LOGLEVEL_DEBUG
				PRINTF("<- DO%d-%d@%d =", block->index,
				       block->index + block->len - 1,
				       block->node_id);
				for (int i = 0; i < block->len; i++) {
					PRINTF(" %d", block->digital_vals[i]);
				}
				PRINTF("\n");
#endif
				if (automation_send_set_dos(
					    block->node_id, block->index,
					    block->digital_vals, block->len,
					    CANARD_TRANSFER_PRIORITY_HIGH) <
				    0) {
					log_error("DO TX failed");
				}
			}
			uavcan_update();

			// set analog outputs
			for (uint8_t i = 0; i < uavcan_aos_blocks_len; i++) {
				block = &uavcan_aos_blocks[i];
#if LOGLEVEL >= LOGLEVEL_DEBUG
				PRINTF("<- AO%d-%d@%d =", block->index,
				       block->index + block->len - 1,
				       block->node_id);
				for (int i = 0; i < block->len; i++) {
					PRINTF(" %d", block->analog_vals[i]);
				}
				PRINTF("\n");
#endif
				if (automation_send_set_aos(
					    block->node_id, block->index,
					    block->analog_vals, block->len,
					    CANARD_TRANSFER_PRIORITY_HIGH) <
				    0) {
					log_error("AO TX failed");
				}
			}
			uavcan_update();
		}

		static uint64_t last_status = 0;
		if (now - last_status > UAVCAN_STATUS_PERIOD * 1000UL) {
			if (uavcan_broadcast_status() > 0) {
				last_status = now;
			}
		}

		uavcan_update();

		vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(UAVCAN_RXTX_PERIOD));
	}
}

void print_frame(const char *direction, const CanardCANFrame *frame)
{
	uint32_t id =
		frame->id & (~(CANARD_CAN_FRAME_EFF | CANARD_CAN_FRAME_ERR |
			       CANARD_CAN_FRAME_RTR));
	PRINTF("%s  %08x  ", direction, id);
	for (int i = 0; i < frame->data_len; i++) {
		PRINTF("%02x  ", frame->data[i]);
	}
	PRINTF("\n");
}

// ---------------------------------------------- UAVCAN callbacks -------------

#if LOGLEVEL >= LOGLEVEL_ERROR
void uavcan_error(const char *fmt, ...)
{
	// TODO: correctly redirect uavcan_error to vsnprintf or such....
	PRINTF(fmt);
	PRINTF("\n");
}
#else
void uavcan_error(const char *fmt, ...);
#endif

uint64_t uavcan_uptime_usec()
{
	return hal_uptime_usec();
}

uint32_t uavcan_uptime_sec()
{
	return hal_uptime_msec() / 1000;
}

void uavcan_restart(void)
{
	hal_restart();
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

	log_error("Unexpected transfer, id=%d", transfer->data_type_id);
}

void uavcan_on_node_status(uint8_t source_node_id,
			   uavcan_protocol_NodeStatus *node_status)
{
	log_com_debug("Node %d uptime = %d", source_node_id,
		      node_status->uptime_sec);
}

void automation_on_get_dis_response(uint8_t source_node_id, uint8_t index,
				    bool *values, uint8_t len)
{
#if LOGLEVEL >= LOGLEVEL_DEBUG
	PRINTF("-> DI%d-%d@%d =", index, index + len - 1, source_node_id);
	for (int i = 0; i < len; i++) {
		PRINTF(" %d", values[i]);
	}
	PRINTF("\n");
#endif
	for (uint8_t i = 0; i < uavcan_dis_blocks_len; i++) {
		uavcan_vals_block_t *block = &uavcan_dis_blocks[i];
		if (!(block->node_id == source_node_id &&
		      block->index == index && block->len == len)) {
			continue;
		}
		for (uint8_t j = 0; j < len; j++) {
			block->digital_vals[j] = values[j];
		}
		return;
	}
	log_warning("Unexpected DI received");
}

void automation_on_get_ais_response(uint8_t source_node_id, uint8_t index,
				    uint16_t *values, uint8_t len)
{
#if LOGLEVEL >= LOGLEVEL_DEBUG
	PRINTF("-> AI%d-%d@%d =", index, index + len - 1, source_node_id);
	for (int i = 0; i < len; i++) {
		PRINTF(" %d", values[i]);
	}
	PRINTF("\n");
#endif
	for (uint8_t i = 0; i < uavcan_ais_blocks_len; i++) {
		uavcan_vals_block_t *block = &uavcan_ais_blocks[i];
		if (!(block->node_id == source_node_id &&
		      block->index == index && block->len == len)) {
			continue;
		}
		for (uint8_t j = 0; j < len; j++) {
			block->analog_vals[j] = values[j];
		}
		return;
	}
	log_warning("Unexpected AI received");
}

// digital inputs broadcast - handle in the same manner as request responses
void automation_on_tell_dis(uint8_t source_node_id, uint8_t index, bool *values,
			    uint8_t len)
{
	automation_on_get_dis_response(source_node_id, index, values, len);
}

// analog inputs broadcast - handle in the same manner as request responses
void automation_on_tell_ais(uint8_t source_node_id, uint8_t index,
			    uint16_t *values, uint8_t len)
{
	automation_on_get_ais_response(source_node_id, index, values, len);
}

// ------------------------------------ unsupported requests -------------------

uint8_t automation_set_dos(uint8_t source_node_id, uint8_t index,
			   const bool *values, uint8_t len)
{
	log_error("Node %d is trying to set my DOs!", source_node_id);
	return 1;
}

uint8_t automation_set_aos(uint8_t source_node_id, uint8_t output_id,
			   const uint16_t *values, uint8_t len)
{
	log_error("Node %d is trying to set my AOs!", source_node_id);
	return 1;
}

uint8_t automation_get_dis(uint8_t source_node_id, uint8_t index, bool *values,
			   uint8_t len)
{
	log_error("Node %d is trying to get my DIs!", source_node_id);
	return AUTOMATION_GETVALUES_RESPONSE_BAD_ARGUMENT;
}

uint8_t automation_get_ais(uint8_t source_node_id, uint8_t index,
			   uint16_t *values, uint8_t len)
{
	log_error("Node %d is trying to get my AIs!", source_node_id);
	return AUTOMATION_GETVALUES_RESPONSE_BAD_ARGUMENT;
}
