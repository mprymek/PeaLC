#include <string.h>

#include "canard.h"

#include "uavcan/protocol/NodeStatus.h"
#include "uavcan/protocol/GetNodeInfo.h"
#include "uavcan/protocol/RestartNode.h"
#include "uavcan/protocol/param/GetSet.h"
#include "uavcan/protocol/debug/LogMessage.h"

#include "uavcan_node.h"

// how often to send NodeStatus [ms]
#ifndef NODE_STATUS_PERIOD
#define NODE_STATUS_PERIOD 1000
#endif

#ifndef UAVCAN_MEM_POOL_SIZE
#define UAVCAN_MEM_POOL_SIZE 1024
#endif

// globals
uavcan_protocol_NodeStatus uavcan_node_status;
uavcan_protocol_GetNodeInfoResponse uavcan_node_info;

CanardInstance g_canard;                                   //The library instance
static uint8_t g_canard_memory_pool[UAVCAN_MEM_POOL_SIZE]; //Arena for memory allocation, used by the library
static bool restart_pending = false;

void uavcan_on_transfer_received(CanardInstance *ins, CanardRxTransfer *transfer);
bool uavcan_should_accept_transfer(const CanardInstance *ins,
                                   uint64_t *out_data_type_signature,
                                   uint16_t data_type_id,
                                   CanardTransferType transfer_type,
                                   uint8_t source_node_id);

static void handle_NodeStatus(CanardInstance *ins, CanardRxTransfer *transfer);
static void handle_GetNodeInfo(CanardInstance *ins, CanardRxTransfer *transfer);
static void handle_RestartNode(CanardInstance *ins, CanardRxTransfer *transfer);
static void handle_param_GetSet(CanardInstance *ins, CanardRxTransfer *transfer);

void uavcan_init()
{
    memset(&uavcan_node_info, 0, sizeof(uavcan_node_info));
    memset(&uavcan_node_status, 0, sizeof(uavcan_node_status));
    uavcan_node_status.mode = UAVCAN_PROTOCOL_NODESTATUS_MODE_INITIALIZATION;

    canardInit(&g_canard,                     // Uninitialized library instance
               g_canard_memory_pool,          // Raw memory chunk used for dynamic allocation
               sizeof(g_canard_memory_pool),  // Size of the above, in bytes
               uavcan_on_transfer_received,   // Callback, see CanardOnTransferReception
               uavcan_should_accept_transfer, // Callback, see CanardShouldAcceptTransfer
               NULL);

    canardSetLocalNodeID(&g_canard, UAVCAN_NODE_ID);
}

void uavcan_update()
{
    CanardCANFrame frame;

    // RX
    if (uavcan_can_rx(&frame))
    {
        int16_t res = canardHandleRxFrame(&g_canard,
                                          &frame,
                                          uavcan_uptime_usec());
        if (res != CANARD_OK && (res != -CANARD_ERROR_RX_NOT_WANTED) && (res != -CANARD_ERROR_RX_WRONG_ADDRESS))
        {
            uavcan_error("Canard error: %d", -res);
        }
    }

    uint64_t now_us = uavcan_uptime_usec();

    // should be called about once per second
    uint64_t last_cleanup = 0;
    if (now_us - last_cleanup > CANARD_RECOMMENDED_STALE_TRANSFER_CLEANUP_INTERVAL_USEC)
    {
        canardCleanupStaleTransfers(&g_canard, uavcan_uptime_usec());
        last_cleanup = now_us;
    }

    // TX
    const CanardCANFrame *txf = canardPeekTxQueue(&g_canard);
    while (txf)
    {
        const int tx_res = uavcan_can_tx(txf);
        if (tx_res == 0)
        {
            canardPopTxQueue(&g_canard);
        }
        txf = canardPeekTxQueue(&g_canard);
    }

    if (restart_pending)
    {
        uavcan_restart();
    }
}

int16_t uavcan_broadcast_status()
{
    static uint8_t transfer_id = 0;
    uint8_t buff[UAVCAN_PROTOCOL_NODESTATUS_MAX_SIZE];

    uavcan_node_status.uptime_sec = uavcan_uptime_sec();

    uint32_t len = uavcan_protocol_NodeStatus_encode(&uavcan_node_status, buff);

    return canardBroadcast(&g_canard,
                           UAVCAN_PROTOCOL_NODESTATUS_SIGNATURE,
                           UAVCAN_PROTOCOL_NODESTATUS_ID,
                           &transfer_id,
                           CANARD_TRANSFER_PRIORITY_LOW,
                           buff,
                           len);
}

/**
 * The application must implement this function and supply a pointer to it to the library during initialization.
 * The library calls this function to determine whether the transfer should be received.
 *
 * If the application returns true, the value pointed to by 'out_data_type_signature' must be initialized with the
 * correct data type signature, otherwise transfer reception will fail with CRC mismatch error. Please refer to the
 * specification for more details about data type signatures. Signature for any data type can be obtained in many
 * ways; for example, using the command line tool distributed with Libcanard (see the repository).
 */
bool uavcan_should_accept_transfer(const CanardInstance *ins,
                                   uint64_t *out_data_type_signature,
                                   uint16_t data_type_id,
                                   CanardTransferType transfer_type,
                                   uint8_t source_node_id)
{

    //Serial.printf("ttype = %d  dtype = %d\n", transfer_type, data_type_id);
    switch (transfer_type)
    {
    case CanardTransferTypeBroadcast:
        switch (data_type_id)
        {
        case UAVCAN_PROTOCOL_NODESTATUS_ID:
            *out_data_type_signature = UAVCAN_PROTOCOL_NODESTATUS_SIGNATURE;
            return true;
        }
        break;
    case CanardTransferTypeRequest:
        switch (data_type_id)
        {
        case UAVCAN_PROTOCOL_GETNODEINFO_ID:
            *out_data_type_signature = UAVCAN_PROTOCOL_GETNODEINFO_SIGNATURE;
            return true;
        case UAVCAN_PROTOCOL_RESTARTNODE_ID:
            *out_data_type_signature = UAVCAN_PROTOCOL_RESTARTNODE_SIGNATURE;
            return true;
        case UAVCAN_PROTOCOL_PARAM_GETSET_ID:
            *out_data_type_signature = UAVCAN_PROTOCOL_PARAM_GETSET_SIGNATURE;
            return true;
        }
        break;
    case CanardTransferTypeResponse:
        break;
    }
    return uavcan_user_should_accept_transfer(ins,
                                              out_data_type_signature,
                                              data_type_id,
                                              transfer_type,
                                              source_node_id);
}

int16_t uavcan_broadcast(
    uint64_t data_type_signature,
    uint16_t data_type_id,
    uint8_t *inout_transfer_id,
    uint8_t priority,
    const void *payload,
    uint16_t payload_len)
{
    int16_t res = canardBroadcast(&g_canard,
                                  data_type_signature,
                                  data_type_id,
                                  inout_transfer_id,
                                  priority,
                                  payload,
                                  payload_len);
    return (res < 0) ? res : 0;
}

int16_t uavcan_send_response(
    CanardRxTransfer *transfer,
    uint64_t data_type_signature,
    uint16_t data_type_id,
    const void *payload,
    uint16_t payload_len)
{
    // TODO: we do not have transport available here :(
    // canardReleaseRxTransferPayload(&g_canard, transfer);
    int16_t res = canardRequestOrRespond(&g_canard,
                                         transfer->source_node_id,
                                         data_type_signature,
                                         data_type_id,
                                         &transfer->transfer_id,
                                         transfer->priority,
                                         CanardResponse,
                                         payload,
                                         payload_len);
    return (res < 0) ? res : 0;
}

int16_t uavcan_send_request(
    uint8_t destination_node_id,
    uint64_t data_type_signature,
    uint16_t data_type_id,
    uint8_t *inout_transfer_id,
    uint8_t priority,
    const void *payload,
    uint16_t payload_len)
{
    int16_t res = canardRequestOrRespond(&g_canard,
                                         destination_node_id,
                                         data_type_signature,
                                         data_type_id,
                                         inout_transfer_id,
                                         priority,
                                         CanardRequest,
                                         payload,
                                         payload_len);
    return (res < 0) ? res : 0;
}

// default implementation: do not accept
/*
__attribute__((weak)) bool uavcan_user_should_accept_transfer(const CanardInstance *ins,
                                                              uint64_t *out_data_type_signature,
                                                              uint16_t data_type_id,
                                                              CanardTransferType transfer_type,
                                                              uint8_t source_node_id)
{
    return false;
}
*/

/**
 * This function will be invoked by the library every time a transfer is successfully received.
 * If the application needs to send another transfer from this callback, it is highly recommended
 * to call canardReleaseRxTransferPayload() first, so that the memory that was used for the block
 * buffer can be released and re-used by the TX queue.
 */
void uavcan_on_transfer_received(CanardInstance *ins, CanardRxTransfer *transfer)
{
    switch (transfer->transfer_type)
    {
    case CanardTransferTypeBroadcast:
        switch (transfer->data_type_id)
        {
        case UAVCAN_PROTOCOL_NODESTATUS_ID:
            handle_NodeStatus(ins, transfer);
            return;
        }
        break;
    case CanardTransferTypeRequest:
        switch (transfer->data_type_id)
        {
        case UAVCAN_PROTOCOL_GETNODEINFO_ID:
            handle_GetNodeInfo(ins, transfer);
            return;
        case UAVCAN_PROTOCOL_RESTARTNODE_ID:
            handle_RestartNode(ins, transfer);
            return;
        case UAVCAN_PROTOCOL_PARAM_GETSET_ID:
            handle_param_GetSet(ins, transfer);
            return;
        }
        break;
    }
    uavcan_user_on_transfer_received(ins, transfer);
}

// default implementation: NOOP
/*
__attribute__((weak)) void uavcan_user_on_transfer_received(CanardInstance *ins, CanardRxTransfer *transfer)
{
}
*/

static void handle_NodeStatus(CanardInstance *ins, CanardRxTransfer *transfer)
{
    uint8_t buff[UAVCAN_PROTOCOL_NODESTATUS_MAX_SIZE];
    uavcan_protocol_NodeStatus msg;

    int32_t res = uavcan_protocol_NodeStatus_decode(transfer,
                                                    transfer->payload_len,
                                                    &msg,
                                                    NULL);
    if (res < 0)
    {
        uavcan_error("uavcan.protocol.NodeStatus decode failed");
        return;
    }
    uavcan_on_node_status(transfer->source_node_id, &msg);
}

static void handle_GetNodeInfo(CanardInstance *ins, CanardRxTransfer *transfer)
{
    uint8_t buff[UAVCAN_PROTOCOL_GETNODEINFO_RESPONSE_MAX_SIZE];

    uint32_t len = uavcan_protocol_GetNodeInfoResponse_encode(&uavcan_node_info, buff);

    canardReleaseRxTransferPayload(ins, transfer);
    canardRequestOrRespond(ins,
                           transfer->source_node_id,
                           UAVCAN_PROTOCOL_GETNODEINFO_SIGNATURE,
                           UAVCAN_PROTOCOL_GETNODEINFO_ID,
                           &transfer->transfer_id,
                           transfer->priority,
                           CanardResponse,
                           buff,
                           len);
}

static void handle_RestartNode(CanardInstance *ins, CanardRxTransfer *transfer)
{
    uavcan_protocol_RestartNodeResponse resp;
    uint8_t buff[UAVCAN_PROTOCOL_RESTARTNODE_RESPONSE_MAX_SIZE];

    resp.ok = true;

    uint32_t len = uavcan_protocol_RestartNodeResponse_encode(&resp, buff);

    canardReleaseRxTransferPayload(ins, transfer);
    canardRequestOrRespond(ins,
                           transfer->source_node_id,
                           UAVCAN_PROTOCOL_RESTARTNODE_SIGNATURE,
                           UAVCAN_PROTOCOL_RESTARTNODE_ID,
                           &transfer->transfer_id,
                           transfer->priority,
                           CanardResponse,
                           buff,
                           len);
    restart_pending = true;
}

static void handle_param_GetSet(CanardInstance *ins, CanardRxTransfer *transfer)
{
    uavcan_protocol_param_GetSetRequest req;
#define GETSETREQ_NAME_MAX_SIZE 96 // max size needed for the dynamic arrays
    // Reserve some memory for the dynamic arrays from the stack
    uint8_t req_buff[GETSETREQ_NAME_MAX_SIZE];
    uint8_t *req_buff_ptr = req_buff;

    uavcan_protocol_param_GetSetResponse resp;
    uint8_t resp_buff[UAVCAN_PROTOCOL_PARAM_GETSET_RESPONSE_MAX_SIZE];

    int32_t res = uavcan_protocol_param_GetSetRequest_decode(transfer,
                                                             transfer->payload_len,
                                                             &req,
                                                             &req_buff_ptr);

    if (res < 0)
    {
        uavcan_error("uavcan.protocol.param.GetSet decode failed");
        return;
    }

    if (req.value.union_tag == UAVCAN_PROTOCOL_PARAM_VALUE_EMPTY)
    {
        if (req.index == 0)
        {
            resp.min_value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
            resp.min_value.integer_value = 1;
            resp.default_value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
            resp.default_value.integer_value = 2;
            resp.max_value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
            resp.max_value.integer_value = 3;
            resp.value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
            resp.value.integer_value = 2;
            resp.name.data = (uint8_t *)"first";
            resp.name.len = 5;
        }
        else if (req.index == 1)
        {
            resp.min_value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
            resp.min_value.integer_value = 4;
            resp.default_value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
            resp.default_value.integer_value = 5;
            resp.max_value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
            resp.max_value.integer_value = 6;
            resp.value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
            resp.value.integer_value = 5;
            resp.name.data = (uint8_t *)"second";
            resp.name.len = 6;
        }
        else
        {
            resp.min_value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_EMPTY;
            resp.default_value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_EMPTY;
            resp.max_value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_EMPTY;
            resp.value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_EMPTY;
            resp.name.data = NULL;
            resp.name.len = 0;
        }
    }
    else
    {
        if (req.value.union_tag != UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE)
        {
            // TODO: error response?
            return;
        }
        uavcan_error("%d := %d", req.index, req.value.integer_value);
        resp.min_value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
        resp.min_value.integer_value = 1;
        resp.default_value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
        resp.default_value.integer_value = 2;
        resp.max_value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
        resp.max_value.integer_value = 3;
        resp.value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
        resp.value.integer_value = req.value.integer_value;
        resp.name.data = (uint8_t *)"first";
        resp.name.len = 5;
    }

    uint32_t len = uavcan_protocol_param_GetSetResponse_encode(&resp, resp_buff);

    canardReleaseRxTransferPayload(ins, transfer);
    canardRequestOrRespond(ins,
                           transfer->source_node_id,
                           UAVCAN_PROTOCOL_PARAM_GETSET_SIGNATURE,
                           UAVCAN_PROTOCOL_PARAM_GETSET_ID,
                           &transfer->transfer_id,
                           transfer->priority,
                           CanardResponse,
                           resp_buff,
                           len);
}

int uavcan_log(uint8_t level, const char *text)
{
    uint8_t buff[UAVCAN_PROTOCOL_DEBUG_LOGMESSAGE_MAX_SIZE];
    uavcan_protocol_debug_LogMessage msg;
    static uint8_t transfer_id = 0;

    msg.level.value = level;
    msg.source.len = 0;
    msg.source.data = 0;
    msg.text.len = strlen(text);
    msg.text.data = (uint8_t *)text;

    uint32_t len = uavcan_protocol_debug_LogMessage_encode(&msg, buff);

    return canardBroadcast(&g_canard,
                           UAVCAN_PROTOCOL_DEBUG_LOGMESSAGE_SIGNATURE,
                           UAVCAN_PROTOCOL_DEBUG_LOGMESSAGE_ID,
                           &transfer_id,
                           CANARD_TRANSFER_PRIORITY_LOW,
                           buff,
                           len);
}

void uavcan_release_rx_transfer_payload(CanardRxTransfer *transfer)
{
    canardReleaseRxTransferPayload(&g_canard, transfer);
}
