#include "app_config.h"
#ifdef WITH_CAN

#include <canard_dsdl.h>

#include "hal.h"
#include "locks.h"
#include "plc.h"
#include "tools.h"
#include "uavcan_common.h"

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
				if (uavcan_send_get_dis(block->node_id,
							block->index,
							block->len) < 0) {
					log_error("get DI TX failed");
				}
			}

#if 0
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

#endif
			// set digital outputs
			for (uint8_t i = 0; i < uavcan_dos_blocks_len; i++) {
				block = &uavcan_dos_blocks[i];
#if !defined(WITHOUT_COM_DEBUG) && (LOGLEVEL >= LOGLEVEL_DEBUG)
				PRINTF("<- DO%d-%d@%d =", block->index,
				       block->index + block->len - 1,
				       block->node_id);
				for (int i = 0; i < block->len; i++) {
					PRINTF(" %d", block->digital_vals[i]);
				}
				PRINTF("\n");
#endif

				if (uavcan_send_set_dos(block->node_id,
							block->index,
							block->digital_vals,
							block->len) != 0) {
					log_error("DO TX failed");
				}
			}

#if 0
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
#endif
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
			    uint8_t index, const uint8_t *const data,
			    uint8_t length)
{
	if (result != UAVCAN_GET_SET_RESULT_OK) {
		log_error("can't get DIs from %u@%u-%u: result=%u",
			  remote_node_id, index, index + length, result);
		return;
	}

#if LOGLEVEL >= LOGLEVEL_DEBUG
	PRINTF("-> DI%u-%u@%u =", index, index + length - 1,
	       transfer->remote_node_id);
#endif

	// buffer length in bytes
	const size_t data_len = DIV_UP(length, 8);

	for (uint8_t i = 0; i < uavcan_dis_blocks_len; i++) {
		uavcan_vals_block_t *block = &uavcan_dis_blocks[i];
		if (!(block->node_id == remote_node_id &&
		      block->index == index && block->len == length)) {
			continue;
		}
		for (uint8_t j = 0; j < length; j++) {
			bool value = canardDSDLGetBit(data, data_len, i);
#if LOGLEVEL >= LOGLEVEL_DEBUG
			PRINTF(" %d", value);
#endif
			block->digital_vals[j] = value;
		}
#if LOGLEVEL >= LOGLEVEL_DEBUG
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
