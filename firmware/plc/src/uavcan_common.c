#include "app_config.h"
#ifdef WITH_CAN

#include <string.h>

#include <canard_dsdl.h>
#include <o1heap.h>

#include "app_config.h"
#include "hal.h"
#include "tools.h"
#include "uavcan_common.h"

//
// Globals
//

uavcan_node_info_t uavcan_node_info;
uavcan_heartbeat_t uavcan_node_status;
CanardInstance uavcan_canard;

//
// Locals
//

static O1HeapInstance *heap;
static uint8_t mem_pool[UAVCAN_MEM_POOL_SIZE]
	__attribute__((aligned(O1HEAP_ALIGNMENT)));

#ifdef UAVCAN_SUBSCRIBE_HEARTBEAT
static CanardRxSubscription heartbeat_subscription;
#endif
static CanardRxSubscription get_info_req_subscription;

static void *mem_allocate(CanardInstance *const ins, const size_t amount);
static void mem_free(CanardInstance *const ins, void *const pointer);
static void handle_transfer(const CanardTransfer *transfer);
#ifdef UAVCAN_SUBSCRIBE_HEARTBEAT
static void handle_heartbeat(const CanardTransfer *transfer);
#endif
static void handle_get_info_req(const CanardTransfer *transfer_req);

static void handle_set_dos_req(const CanardTransfer *transfer);
static void handle_set_dos_resp(const CanardTransfer *transfer);
static void handle_get_dis_req(const CanardTransfer *transfer);
static void handle_get_dis_resp(const CanardTransfer *transfer);
static void handle_set_aos_req(const CanardTransfer *transfer);
static void handle_set_aos_resp(const CanardTransfer *transfer);
static void handle_get_ais_req(const CanardTransfer *transfer);
static void handle_get_ais_resp(const CanardTransfer *transfer);

//
// Public functions
//

int uavcan_init()
{
	// init CAN HW
	int res;
	if ((res = can2_init())) {
		return res;
	}

	// init UAVCAN
	memset(&uavcan_node_info, 0, sizeof(uavcan_node_info));
	memset(&uavcan_node_status, 0, sizeof(uavcan_node_status));
	uavcan_node_status.health = UAVCAN_NODE_HEALTH_1_0_NOMINAL;
	uavcan_node_status.mode = UAVCAN_NODE_MODE_1_0_INITIALIZATION;

	heap = o1heapInit(mem_pool, UAVCAN_MEM_POOL_SIZE, NULL, NULL);
	if (heap == NULL) {
		die(DEATH_INIT_FAILED);
	}
	uavcan_canard = canardInit(mem_allocate, mem_free);
	uavcan_canard.mtu_bytes = CANARD_MTU_CAN_CLASSIC;
	uavcan_canard.node_id = UAVCAN_NODE_ID;

#ifdef UAVCAN_SUBSCRIBE_HEARTBEAT
	canardRxSubscribe(&uavcan_canard, CanardTransferKindMessage,
			  UAVCAN_HEARTBEAT_PORT_ID, 12,
			  CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
			  &heartbeat_subscription);
#endif

	canardRxSubscribe(&uavcan_canard, CanardTransferKindRequest,
			  UAVCAN_GET_INFO_PORT_ID, 0,
			  CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
			  &get_info_req_subscription);

	return uavcan_init2();
}

void uavcan_send_packets()
{
	for (const CanardFrame *frame = NULL;
	     (frame = canardTxPeek(&uavcan_canard)) != NULL;) {
		if ((frame->timestamp_usec == 0) ||
		    (frame->timestamp_usec > hal_uptime_usec())) {
			if (can2_send(frame->extended_can_id, frame->payload,
				      frame->payload_size)) {
				return;
			}

#if !defined(WITHOUT_COM_DEBUG) && (LOGLEVEL >= LOGLEVEL_DEBUG)
			uavcan_print_frame("<-", frame);
#endif
		}
		canardTxPop(&uavcan_canard);
		uavcan_canard.memory_free(&uavcan_canard, (CanardFrame *)frame);
	}
}

// hal callback - called by can2_receive()
void uavcan_receive_packet(uint32_t id, const void *payload,
			   size_t payload_size)
{
	CanardTransfer transfer;

	const CanardFrame frame = { .timestamp_usec = hal_uptime_usec(),
				    .extended_can_id = id,
				    .payload = payload,
				    .payload_size = payload_size };
#if !defined(WITHOUT_COM_DEBUG) && (LOGLEVEL >= LOGLEVEL_DEBUG)
	uavcan_print_frame("->", &frame);
#endif

	const int8_t result =
		canardRxAccept(&uavcan_canard, &frame, 0, &transfer);
	if (result < 0) {
		log_error("Canard RX error");
	} else if (result == 1) {
		handle_transfer(&transfer);
		uavcan_canard.memory_free(&uavcan_canard,
					  (void *)transfer.payload);
	} else {
		// Nothing to do.
		// The received frame is either invalid or it's a non-last frame of a multi-frame transfer.
		// Reception of an invalid frame is NOT reported as an error because it is not an error.
	}
}

void uavcan_print_frame(const char *direction, const CanardFrame *frame)
{
#ifdef PRINTF
	PRINTF("%s  %08x  ", direction, frame->extended_can_id);
	for (int i = 0; i < frame->payload_size; i++) {
		PRINTF("%02x  ", ((uint8_t *)frame->payload)[i]);
	}
	PRINTF("\n");
#endif
}

int uavcan_send_heartbeat()
{
	static uint8_t transfer_id = 0;

	// manually construct heartbeat packet...
	uint8_t buffer[7];
	canardDSDLSetUxx(&buffer[0], 0, hal_uptime_usec() / 1000000,
			 32); // uptime
	canardDSDLSetUxx(&buffer[0], 32, UAVCAN_NODE_HEALTH_1_0_NOMINAL,
			 2); // health
	canardDSDLSetUxx(&buffer[0], 40, UAVCAN_NODE_MODE_1_0_OPERATIONAL,
			 3); // mode
	canardDSDLSetUxx(&buffer[0], 48, 0, 8); // vssc

	const CanardTransfer transfer = { .timestamp_usec = 0,
					  .priority = CanardPriorityNominal,
					  .transfer_kind =
						  CanardTransferKindMessage,
					  .port_id = UAVCAN_HEARTBEAT_PORT_ID,
					  .remote_node_id =
						  CANARD_NODE_ID_UNSET,
					  .transfer_id = transfer_id,
					  .payload_size = sizeof(buffer),
					  .payload = &buffer[0] };
	transfer_id++;
	return (canardTxPush(&uavcan_canard, &transfer) > 0) ? 0 : -1;
}

int uavcan_send_set_dos(uint8_t node_id, uint8_t index, const bool *values,
			uint8_t values_len)
{
	static uint8_t transfer_id = 0;

	uint8_t buffer[34];
	canardDSDLSetUxx(&buffer[0], 0, index, 8);
	canardDSDLSetUxx(&buffer[0], 8, values_len, 8);
	for (int i = 0; i < values_len; i++) {
		canardDSDLSetBit(&buffer[0], 16 + i, values[i]);
	}

	const CanardTransfer transfer = {
		.timestamp_usec = 0 /* TODO: set transmission deadline */,
		.priority = CanardPriorityNominal,
		.transfer_kind = CanardTransferKindRequest,
		.port_id = UAVCAN_SET_DOS_PORT_ID,
		.remote_node_id = node_id,
		.transfer_id = transfer_id,
		.payload_size =
			2 + (values_len / 8) + ((values_len % 8) == 0 ? 0 : 1),
		.payload = &buffer[0]
	};
	transfer_id++;
	return (canardTxPush(&uavcan_canard, &transfer) > 0) ? 0 : -1;
}

int uavcan_send_set_aos(uint8_t node_id, uint8_t index, const uint16_t *values,
			uint8_t values_len)
{
	static uint8_t transfer_id = 0;

	uint8_t buffer[34];
	canardDSDLSetUxx(&buffer[0], 0, index, 8);
	canardDSDLSetUxx(&buffer[0], 8, values_len, 8);
	for (int i = 0; i < values_len; i++) {
		canardDSDLSetUxx(&buffer[0], 16 + 16 * i, values[i], 16);
	}

	const CanardTransfer transfer = {
		.timestamp_usec = 0 /* TODO: set transmission deadline */,
		.priority = CanardPriorityNominal,
		.transfer_kind = CanardTransferKindRequest,
		.port_id = UAVCAN_SET_AOS_PORT_ID,
		.remote_node_id = node_id,
		.transfer_id = transfer_id,
		.payload_size = 2 + values_len * 2,
		.payload = &buffer[0]
	};
	transfer_id++;
	return (canardTxPush(&uavcan_canard, &transfer) > 0) ? 0 : -1;
}

static int send_get_xis(CanardPortID port, uint8_t node_id, uint8_t index,
			uint8_t length)
{
	static uint8_t transfer_id = 0;

	uint8_t buffer[2];
	canardDSDLSetUxx(&buffer[0], 0, index, 8);
	canardDSDLSetUxx(&buffer[0], 8, length, 8);

	const CanardTransfer transfer = {
		.timestamp_usec = 0 /* TODO: set transmission deadline */,
		.priority = CanardPriorityNominal,
		.transfer_kind = CanardTransferKindRequest,
		.port_id = port,
		.remote_node_id = node_id,
		.transfer_id = transfer_id,
		.payload_size = 2,
		.payload = &buffer[0]
	};
	transfer_id++;
	return (canardTxPush(&uavcan_canard, &transfer) > 0) ? 0 : -1;
}

int uavcan_send_get_dis(uint8_t node_id, uint8_t index, uint8_t length)
{
	return send_get_xis(UAVCAN_GET_DIS_PORT_ID, node_id, index, length);
}

int uavcan_send_get_ais(uint8_t node_id, uint8_t index, uint8_t length)
{
	return send_get_xis(UAVCAN_GET_AIS_PORT_ID, node_id, index, length);
}

//
// Private functions
//

inline static void *mem_allocate(CanardInstance *const ins, const size_t amount)
{
	return o1heapAllocate(heap, amount);
}

inline static void mem_free(CanardInstance *const ins, void *const pointer)
{
	o1heapFree(heap, pointer);
}

static void handle_transfer(const CanardTransfer *transfer)
{
	switch (transfer->transfer_kind) {
	case CanardTransferKindMessage:
		switch (transfer->port_id) {
#ifdef UAVCAN_SUBSCRIBE_HEARTBEAT
		case UAVCAN_HEARTBEAT_PORT_ID:
			handle_heartbeat(transfer);
			return;
#endif
		}
		break;

	case CanardTransferKindRequest:
		switch (transfer->port_id) {
		case UAVCAN_GET_INFO_PORT_ID:
			handle_get_info_req(transfer);
			return;
		case UAVCAN_SET_DOS_PORT_ID:
			handle_set_dos_req(transfer);
			return;
		case UAVCAN_SET_AOS_PORT_ID:
			handle_set_aos_req(transfer);
			return;
		case UAVCAN_GET_DIS_PORT_ID:
			handle_get_dis_req(transfer);
			return;
		case UAVCAN_GET_AIS_PORT_ID:
			handle_get_ais_req(transfer);
			return;
		}
		break;

	case CanardTransferKindResponse:
		switch (transfer->port_id) {
		case UAVCAN_SET_DOS_PORT_ID:
			handle_set_dos_resp(transfer);
			return;
		case UAVCAN_SET_AOS_PORT_ID:
			handle_set_aos_resp(transfer);
			return;
		case UAVCAN_GET_DIS_PORT_ID:
			handle_get_dis_resp(transfer);
			return;
		case UAVCAN_GET_AIS_PORT_ID:
			handle_get_ais_resp(transfer);
			return;
		}
		break;
	}
	log_warning("Unhandled UAVCAN transfer. port=%u", transfer->port_id);
}

#ifdef UAVCAN_SUBSCRIBE_HEARTBEAT
static void handle_heartbeat(const CanardTransfer *transfer)
{
	uint32_t uptime = canardDSDLGetU32(transfer->payload,
					   transfer->payload_size, 0, 32);
	uint8_t health = canardDSDLGetU8(transfer->payload,
					 transfer->payload_size, 32, 2);
	uint8_t mode = canardDSDLGetU8(transfer->payload,
				       transfer->payload_size, 40, 3);
	uint8_t vssc = canardDSDLGetU32(transfer->payload,
					transfer->payload_size, 48, 8);

	uavcan_on_heartbeat(transfer->remote_node_id, uptime, health, mode,
			    vssc);
}
#endif

static void handle_get_info_req(const CanardTransfer *transfer_req)
{
	static uint8_t transfer_id = 0;

	// Max extent is 448 but we need only (248 + 50*8 + 16)/8 = 83B
	uint8_t buffer[83];

	memset(buffer, 0, sizeof(buffer));

	// UAVCAN protocol version major, minor
	canardDSDLSetUxx(buffer, 0, 1, 8);
	canardDSDLSetUxx(buffer, 8, 0, 8);
	// UAVCAN hw version major, minor
	canardDSDLSetUxx(buffer, 16, 2, 8);
	canardDSDLSetUxx(buffer, 24, 0, 8);
	// UAVCAN sw version major, minor
	canardDSDLSetUxx(buffer, 32, 2, 8);
	canardDSDLSetUxx(buffer, 40, 0, 8);
	// sw VCS revision
	//canardDSDLSetUxx(buffer, 48, 0, 64);
	// uuid
	//canardDSDLSetUxx(buffer, 112, 0, 128);
	// name
	uint8_t name_len = MIN(strlen(APP_NAME), 50);
	canardDSDLSetUxx(buffer, 240, name_len, 8);
	canardDSDLCopyBits(name_len * 8, 0, 248, APP_NAME, buffer);
	size_t i = 248 + name_len * 8;
	// sw image crc len
	//canardDSDLSetUxx(buffer, i, 0, 8);
	i += 8;
	// certificate of authenticity len
	//canardDSDLSetUxx(buffer, i, 0, 8);
	i += 8;

	const CanardTransfer transfer = {
		.timestamp_usec = 0,
		.priority = CanardPriorityNominal,
		.transfer_kind = CanardTransferKindResponse,
		.port_id = UAVCAN_GET_INFO_PORT_ID,
		.remote_node_id = transfer_req->remote_node_id,
		.transfer_id = transfer_req->transfer_id,
		.payload_size = DIV_UP(i, 8),
		.payload = buffer
	};
	transfer_id++;
	canardTxPush(&uavcan_canard, &transfer);
}

void handle_set_dos_req(const CanardTransfer *transfer_req)
{
	uint8_t index = canardDSDLGetU8(transfer_req->payload,
					transfer_req->payload_size, 0, 8);
	uint8_t length = canardDSDLGetU8(transfer_req->payload,
					 transfer_req->payload_size, 8, 8);

	if (length > 64) {
		log_warning("Invalid length in SetDOs request");
		return;
	}
	// TODO: check payload length!

	const uint8_t *data = &(((uint8_t *)transfer_req->payload)[2]);
	uint8_t result = uavcan_on_set_dos_req(transfer_req->remote_node_id,
					       index, data, length);

	uint8_t buffer[2];
	// result
	canardDSDLSetUxx(buffer, 0, result, 2);
	// index
	canardDSDLSetUxx(buffer, 2, index, 8);

	const CanardTransfer transfer = {
		.timestamp_usec = 0,
		.priority = CanardPriorityNominal,
		.transfer_kind = CanardTransferKindResponse,
		.port_id = transfer_req->port_id,
		.remote_node_id = transfer_req->remote_node_id,
		.transfer_id = transfer_req->transfer_id,
		.payload_size = 2,
		.payload = buffer
	};
	canardTxPush(&uavcan_canard, &transfer);
}

void handle_set_aos_req(const CanardTransfer *transfer_req)
{
	uint8_t index = canardDSDLGetU8(transfer_req->payload,
					transfer_req->payload_size, 0, 8);
	uint8_t length = canardDSDLGetU8(transfer_req->payload,
					 transfer_req->payload_size, 8, 8);

	if (length > 4) {
		log_warning("Invalid length in SetAOs request");
		return;
	}
	// TODO: check payload length!

	const uint8_t *data = &(((uint8_t *)transfer_req->payload)[2]);
	uint8_t result = uavcan_on_set_aos_req(transfer_req->remote_node_id,
					       index, data, length);

	uint8_t buffer[2];
	// result
	canardDSDLSetUxx(buffer, 0, result, 2);
	// index
	canardDSDLSetUxx(buffer, 2, index, 8);

	const CanardTransfer transfer = {
		.timestamp_usec = 0,
		.priority = CanardPriorityNominal,
		.transfer_kind = CanardTransferKindResponse,
		.port_id = transfer_req->port_id,
		.remote_node_id = transfer_req->remote_node_id,
		.transfer_id = transfer_req->transfer_id,
		.payload_size = 2,
		.payload = buffer
	};
	canardTxPush(&uavcan_canard, &transfer);
}

static void handle_get_dis_req(const CanardTransfer *transfer_req)
{
	// deserialize request
	uint8_t index = canardDSDLGetU8(transfer_req->payload,
					transfer_req->payload_size, 0, 8);
	uint8_t length = canardDSDLGetU8(transfer_req->payload,
					 transfer_req->payload_size, 8, 8);

	// serialize response
	uint8_t buffer[UAVCAN_GET_DIS_RESP_EXT];
	// bit 0,1 = result - will be set later
	// index
	canardDSDLSetUxx(buffer, 2, index, 8);
	// length (aligned to 1B)
	canardDSDLSetUxx(buffer, 16, length, 8);

	if (length > UAVCAN_DIGITAL_VALUES_MAX_LENGTH) {
		log_warning("Invalid length in GetDIs request");
		return;
	}

	uint8_t *data = &buffer[3];
	uint8_t result = uavcan_on_get_dis_req(transfer_req->remote_node_id,
					       index, data, length);

	// result
	canardDSDLSetUxx(buffer, 0, result, 2);

	const CanardTransfer transfer = {
		.timestamp_usec = 0,
		.priority = CanardPriorityNominal,
		.transfer_kind = CanardTransferKindResponse,
		.port_id = transfer_req->port_id,
		.remote_node_id = transfer_req->remote_node_id,
		.transfer_id = transfer_req->transfer_id,
		.payload_size = DIV_UP(24 + length, 8),
		.payload = buffer
	};
	canardTxPush(&uavcan_canard, &transfer);
}

static void handle_get_ais_req(const CanardTransfer *transfer_req)
{
	// deserialize request
	uint8_t index = canardDSDLGetU8(transfer_req->payload,
					transfer_req->payload_size, 0, 8);
	uint8_t length = canardDSDLGetU8(transfer_req->payload,
					 transfer_req->payload_size, 8, 8);

	// serialize respond
	uint8_t buffer[UAVCAN_GET_AIS_RESP_EXT];
	// bit 0,1 = result - will be set later
	// index
	canardDSDLSetUxx(buffer, 2, index, 8);
	// length (aligned to 1B)
	canardDSDLSetUxx(buffer, 16, length, 8);

	if (length > UAVCAN_ANALOG_VALUES_MAX_LENGTH) {
		log_warning("Invalid length in GetAIs request");
		return;
	}

	uint8_t *data = &buffer[3];
	uint8_t result = uavcan_on_get_ais_req(transfer_req->remote_node_id,
					       index, data, length);

	// result
	canardDSDLSetUxx(buffer, 0, result, 2);

	const CanardTransfer transfer = {
		.timestamp_usec = 0,
		.priority = CanardPriorityNominal,
		.transfer_kind = CanardTransferKindResponse,
		.port_id = transfer_req->port_id,
		.remote_node_id = transfer_req->remote_node_id,
		.transfer_id = transfer_req->transfer_id,
		.payload_size = DIV_UP(24 + length * 16, 8),
		.payload = buffer
	};
	canardTxPush(&uavcan_canard, &transfer);
}

static void handle_set_dos_resp(const CanardTransfer *transfer)
{
	uint8_t result = canardDSDLGetU32(transfer->payload,
					  transfer->payload_size, 0, 2);
	uint8_t index = canardDSDLGetU32(transfer->payload,
					 transfer->payload_size, 2, 8);
	uavcan_on_set_dos_resp(transfer->remote_node_id, result, index);
}

static void handle_set_aos_resp(const CanardTransfer *transfer)
{
	uint8_t result = canardDSDLGetU32(transfer->payload,
					  transfer->payload_size, 0, 2);
	uint8_t index = canardDSDLGetU32(transfer->payload,
					 transfer->payload_size, 2, 8);
	uavcan_on_set_aos_resp(transfer->remote_node_id, result, index);
}

static void handle_get_dis_resp(const CanardTransfer *transfer)
{
	uint8_t result = canardDSDLGetU32(transfer->payload,
					  transfer->payload_size, 0, 2);
	uint8_t index = canardDSDLGetU32(transfer->payload,
					 transfer->payload_size, 2, 8);
	// length aligned to 1B
	uint8_t length = canardDSDLGetU8(transfer->payload,
					 transfer->payload_size, 16, 8);

	// TODO: check payload length!
	if (length > 64) {
		log_warning("Invalid length in GetDIs response");
		return;
	}

	// Calling this with a pointer into payload is strictly not correct,
	// we should use something like:
	//	for (uint8_t j = 0; j < length; j++) {
	//		bool value = canardDSDLGetBit(transfer->payload,
	//					      transfer->payload_size,
	//					      24 + i);
	//	}
	// but it's not necessary because there really is a bit array in the payload.
	const uint8_t *data = &(((uint8_t *)transfer->payload)[3]);
	uavcan_on_get_dis_resp(transfer->remote_node_id, result, index, data,
			       length);
}

static void handle_get_ais_resp(const CanardTransfer *transfer)
{
	uint8_t result = canardDSDLGetU32(transfer->payload,
					  transfer->payload_size, 0, 2);
	uint8_t index = canardDSDLGetU32(transfer->payload,
					 transfer->payload_size, 2, 8);
	// length aligned to 1B
	uint8_t length = canardDSDLGetU8(transfer->payload,
					 transfer->payload_size, 16, 8);

	// TODO: check payload length!
	if (length > 64) {
		log_warning("Invalid length in GetAIs response");
		return;
	}

	// Calling this with a pointer into payload is strictly not correct,
	// we should use something like:
	//	for (uint8_t j = 0; j < length; j++) {
	//		bool value = canardDSDLGetBit(transfer->payload,
	//					      transfer->payload_size,
	//					      24 + i);
	//	}
	// but it's not necessary because there really is a bit array in the payload.
	const uint8_t *data = &(((uint8_t *)transfer->payload)[3]);
	uavcan_on_get_ais_resp(transfer->remote_node_id, result, index, data,
			       length);
}

//
// Default callbacks implementations
//

// hal callback - called by can2_receive()
void can2_on_receive(uint32_t id, const void *payload, size_t payload_size)
	__attribute__((weak));
void can2_on_receive(uint32_t id, const void *payload, size_t payload_size)
{
	uavcan_receive_packet(id, payload, payload_size);
}

#endif // ifdef WITH_CAN
