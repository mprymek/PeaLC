#include "app_config.h"
#ifdef WITH_CAN

#include <stdbool.h>

#include <canard_dsdl.h>

#include "hal.h"
#include "locks.h"
#include "plc.h"
#include "tools.h"
#include "uavcan_common.h"
#include "uavcan_impl.h"

static TaskHandle_t uavcan_task_h = NULL;

void uavcan_task(void *pvParameters);

static CanardRxSubscription get_dis_resp_subscription;

int uavcan_init2()
{
	canardRxSubscribe(&uavcan_canard, CanardTransferKindResponse,
			  UAVCAN_GET_DIS_PORT_ID, 24,
			  CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
			  &get_dis_resp_subscription);

	//uavcan_node_info.hw_ver.major = HW_VERSION_MAJOR;
	//uavcan_node_info.hw_ver.minor = HW_VERSION_MINOR;
	uavcan_node_info.sw_ver.major = APP_VERSION_MAJOR;
	uavcan_node_info.sw_ver.minor = APP_VERSION_MINOR;
	//uavcan_node_info.vcs_revision = GIT_HASH;
	uavcan_node_info.name = APP_NAME;

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

	// We must wait for PLC buffers initialization
	// TODO: We are not broadcasting node status and are not responding to NodeInfo
	//       during this period.
	if (!WAIT_BITS(PLC_INITIALIZED_BIT)) {
		die(DEATH_INITIALIZATION_TIMEOUT);
	}

	uint64_t io_rxtx_delay = common_ticktime__ / 1000;

	for (;;) {
		uint64_t now = hal_uptime_usec();
		static uint64_t last_io_rxtx = 0;
		if (now - last_io_rxtx >= io_rxtx_delay) {
			last_io_rxtx = now;

			// ask for digital inputs
			for (size_t i = 0; i < digital_inputs_blocks_len; i++) {
				io_block_t *block = &digital_inputs_blocks[i];
				uavcan_io_block_t *data = block->driver_data;
				if (!block->enabled ||
				    block->driver_type != IO_DRIVER_UAVCAN) {
					continue;
				}
				log_com_debug("%u: <- DI%u-%u@%u = ?",
					      hal_uptime_msec(), data->index,
					      data->index + block->length - 1,
					      data->node_id);
				if (uavcan_send_get_dis(data->node_id,
							data->index,
							block->length) < 0) {
					log_error("get DI TX failed");
				}
			}

			// set digital outputs
			for (size_t i = 0; i < digital_outputs_blocks_len;
			     i++) {
				io_block_t *block = &digital_outputs_blocks[i];
				uavcan_io_block_t *data = block->driver_data;
				if (!block->enabled ||
				    block->driver_type != IO_DRIVER_UAVCAN) {
					continue;
				}
				if (!block->dirty) {
					continue;
				}
				#if !defined(WITHOUT_COM_DEBUG) && (LOGLEVEL >= LOGLEVEL_DEBUG)
				PRINTF("<- DO%u-%u@%u =", data->index,
				       data->index + block->length - 1,
				       data->node_id);
				for (int i = 0; i < block->length; i++) {
					PRINTF(" %u", ((bool *)block->buff)[i]);
				}
				PRINTF("\n");
				#endif
				if (uavcan_send_set_dos(
					    data->node_id, data->index,
					    block->buff, block->length) < 0) {
					log_error("DO TX failed");
					continue;
				}
				block->dirty = false;
			}
		}

		static uint64_t last_status = 0;
		if (now - last_status > UAVCAN_STATUS_PERIOD * 1000UL) {
			if (!uavcan_send_heartbeat()) {
				last_status = now;
			}
		}

		uavcan_send_packets();
		can2_receive();

		vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(UAVCAN_RXTX_PERIOD));
	}
}

void uavcan_on_set_dos_resp(CanardNodeID remote_node_id, uint8_t result,
			    uint8_t index)
{
	if (result != UAVCAN_GET_SET_RESULT_OK) {
		log_error("can't set DOs from %u@%u: result=%u", remote_node_id,
			  index, result);
	}
}

void uavcan_on_get_dis_resp(CanardNodeID remote_node_id, uint8_t result,
			    uint8_t index, const uint8_t *const payload,
			    uint8_t length)
{
	if (result != UAVCAN_GET_SET_RESULT_OK) {
		log_error("can't get DIs from %u@%u-%u: result=%u",
			  remote_node_id, index, index + length, result);
		return;
	}

	for (size_t bi = 0; bi < digital_inputs_blocks_len; bi++) {
		io_block_t *block = &digital_inputs_blocks[bi];
		uavcan_io_block_t *data = block->driver_data;

		if ((block->driver_type != IO_DRIVER_UAVCAN) ||
		    (data->node_id != remote_node_id) ||
		    (data->index != index) || (block->length != length)) {
			continue;
		}

		const size_t payload_len = DIV_UP(length, 8);
		for (uint8_t ai = 0; ai < length; ai++) {
			bool value = canardDSDLGetBit(payload, payload_len, ai);
			bool *buff_value = &((bool *)block->buff)[ai];
			if (value != *buff_value) {
				*buff_value = value;
				block->dirty = true;
			}
		}

		#if LOGLEVEL >= LOGLEVEL_DEBUG
		PRINTF("%u: -> DI%u-%u@%u =", hal_uptime_msec(), index,
		       index + length - 1, remote_node_id);
		PRINTF(" payload=%u", *payload);
		for (uint8_t ai = 0; ai < length; ai++) {
			PRINTF(" %u", ((bool *)block->buff)[ai]);
		}
		PRINTF("\n");
		#endif

		return;
	}

	log_warning("Unexpected DI received");
}

// ---------------------------------------------- unused callbacks -------------

uint8_t uavcan_on_set_dos_req(CanardNodeID remote_node_id, uint8_t index,
			      const uint8_t *const data, uint8_t length)
{
	return UAVCAN_GET_SET_RESULT_BAD_ARGUMENT;
}

uint8_t uavcan_on_set_aos_req(CanardNodeID remote_node_id, uint8_t index,
			      const uint8_t *const data, uint8_t length)
{
	return UAVCAN_GET_SET_RESULT_BAD_ARGUMENT;
}

uint8_t uavcan_on_get_dis_req(CanardNodeID remote_node_id, uint8_t index,
			      uint8_t *data, uint8_t length)
{
	return UAVCAN_GET_SET_RESULT_BAD_ARGUMENT;
}

uint8_t uavcan_on_get_ais_req(CanardNodeID remote_node_id, uint8_t index,
			      uint8_t *data, uint8_t length)
{
	return UAVCAN_GET_SET_RESULT_BAD_ARGUMENT;
}

#endif // ifdef WITH_CAN
