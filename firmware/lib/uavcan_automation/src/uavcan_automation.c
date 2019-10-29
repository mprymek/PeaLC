#include <stdio.h>
#include <string.h>

#include "uavcan_node.h"

#include "automation/SetValues.h"
#include "automation/GetValues.h"
#include "automation/TellValues.h"

#include "uavcan_automation.h"


// more correct but usable inside function only
/*
#define max(a, b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
*/
#define max(a, b) (((a) > (b)) ? (a) : (b))

static void handle_SetValues(CanardInstance *ins, CanardRxTransfer *transfer);
static void handle_TellValues(CanardInstance *ins, CanardRxTransfer *transfer);
static void handle_GetValues_req(CanardInstance *ins, CanardRxTransfer *transfer);
static void handle_GetValues_resp(CanardInstance *ins, CanardRxTransfer *transfer);

bool uavcan_automation_should_accept_transfer(const CanardInstance *ins,
                                              uint64_t *out_data_type_signature,
                                              uint16_t data_type_id,
                                              CanardTransferType transfer_type,
                                              uint8_t source_node_id)
{
    //PRINTF("TT: %d DTI: %d\n", transfer_type, data_type_id);
    switch (transfer_type)
    {
    case CanardTransferTypeBroadcast:
        switch (data_type_id)
        {
        case AUTOMATION_SETVALUES_ID:
            *out_data_type_signature = AUTOMATION_SETVALUES_SIGNATURE;
            return true;
        case AUTOMATION_TELLVALUES_ID:
            *out_data_type_signature = AUTOMATION_TELLVALUES_SIGNATURE;
            return true;
        }
        break;
    case CanardTransferTypeRequest:
    case CanardTransferTypeResponse:
        switch (data_type_id)
        {
        case AUTOMATION_GETVALUES_ID:
            *out_data_type_signature = AUTOMATION_GETVALUES_SIGNATURE;
            return true;
        }
        break;
    }

    return false;
}

// compute max size for all handled messages
#define buff_size \
    max(AUTOMATION_SETVALUES_MAX_SIZE, \
        max(AUTOMATION_GETVALUES_REQUEST_MAX_SIZE, \
            AUTOMATION_GETVALUES_RESPONSE_MAX_SIZE))

uint8_t buff[buff_size];
uint8_t *buff_ptr = buff;

bool uavcan_automation_on_transfer_received(CanardInstance *ins, CanardRxTransfer *transfer)
{
    switch (transfer->transfer_type)
    {
    // BROADCASTS
    case CanardTransferTypeBroadcast:
        switch (transfer->data_type_id)
        {
        case AUTOMATION_SETVALUES_ID:
            handle_SetValues(ins, transfer);
            return true;
        case AUTOMATION_TELLVALUES_ID:
            handle_TellValues(ins, transfer);
            return true;
        }
        break;

    // REQUESTS
    case CanardTransferTypeRequest:
        switch (transfer->data_type_id)
        {
        case AUTOMATION_GETVALUES_ID:
            handle_GetValues_req(ins, transfer);
            return true;
        }
        break;

    // RESPONSES
    case CanardTransferTypeResponse:
        switch (transfer->data_type_id)
        {
        case AUTOMATION_GETVALUES_ID:
            handle_GetValues_resp(ins, transfer);
            return true;
        }
        break;
    }
    return false;
}

static void handle_TellValues(CanardInstance *ins, CanardRxTransfer *transfer)
{
    automation_TellValues tv;

    if (automation_TellValues_decode(transfer, (uint16_t)transfer->payload_len, &tv, &buff_ptr) < 0)
    {
        uavcan_error("a.TV decode failed");
        return;
    }
        if (tv.values.union_tag == AUTOMATION_VALUES_DIGITAL_VALUES)
        {
            if(tv.port_type.port_type == AUTOMATION_PORTTYPE_INPUT) {
                automation_on_tell_dis(transfer->source_node_id, tv.index, tv.values.digital_values.values, tv.values.digital_values.values_len);
            } else {
                // TODO: on_tell_dos
            }
        }
        else
        {
            if(tv.port_type.port_type == AUTOMATION_PORTTYPE_INPUT) {
                automation_on_tell_ais(transfer->source_node_id, tv.index, tv.values.analog_values.values, tv.values.analog_values.values_len);
            } else {
                // TODO: on_tell_aos
            }
        }
}

static void handle_SetValues(CanardInstance *ins, CanardRxTransfer *transfer)
{
    automation_SetValues sv;

    if (automation_SetValues_decode(transfer, (uint16_t)transfer->payload_len, &sv, &buff_ptr) < 0)
    {
        uavcan_error("a.SV decode failed");
        return;
    }
    if (sv.node_id == UAVCAN_NODE_ID)
    {
        if (sv.values.union_tag == AUTOMATION_VALUES_DIGITAL_VALUES)
        {
            automation_set_dos(transfer->source_node_id, sv.index, sv.values.digital_values.values, sv.values.digital_values.values_len);
        }
        else
        {
            //automation_set_aos(transfer->source_node_id, sv.index, sv.values.analog_values.values.data, sv.values.analog_values.values.len);
            automation_set_aos(transfer->source_node_id, sv.index, sv.values.analog_values.values, sv.values.analog_values.values_len);
        }
    }
}

static void handle_GetValues_req(CanardInstance *ins, CanardRxTransfer *transfer)
{
    automation_GetValuesRequest req;
    automation_GetValuesResponse resp;

    if (automation_GetValuesRequest_decode(transfer, (uint16_t)transfer->payload_len, &req, &buff_ptr) < 0)
    {
        uavcan_error("a.GV req decode failed");
        return;
    }

    // TODO
    if (req.port_type.port_type != AUTOMATION_PORTTYPE_INPUT) {
        uavcan_error("a.GV-output not implemented");
        return;
    }

    switch (req.vals_type.value_type)
    {
    case AUTOMATION_VALUETYPE_DIGITAL:
        if (req.length > AUTOMATION_DIGITALVALUES_VALUES_LENGTH)
        {
            uavcan_error("a.GV: len too big");
            resp.result = AUTOMATION_GETVALUES_RESPONSE_BAD_ARGUMENT;
        }
        else
        {
            resp.result = automation_get_dis(transfer->source_node_id, req.index, resp.values.digital_values.values, req.length);
            resp.port_type.port_type = AUTOMATION_PORTTYPE_INPUT;
            resp.index = req.index;
            resp.values.union_tag = AUTOMATION_VALUES_DIGITAL_VALUES;
            resp.values.digital_values.values_len = req.length;
        }
        break;
    case AUTOMATION_VALUETYPE_ANALOG:
        /*
        if (req.length > AUTOMATION_ANALOGVALUES_VALUES_MAX_LENGTH)
        {
            uavcan_error("a.GV: len too big");
            resp.result = AUTOMATION_GETVALUES_RESPONSE_BAD_ARGUMENT;
        }
        else
        {
            uint16_t values[AUTOMATION_ANALOGVALUES_VALUES_MAX_LENGTH];
            resp.result = automation_get_ais(transfer->source_node_id, req.index, values, req.length);
            resp.values.analog_values.values.data = values;
            resp.values.union_tag = AUTOMATION_VALUES_ANALOG_VALUES;
            resp.values.analog_values.values.len = req.length;
        }
        */
        if (req.length > AUTOMATION_ANALOGVALUES_VALUES_LENGTH)
        {
            uavcan_error("a.GV: len too big");
            resp.result = AUTOMATION_GETVALUES_RESPONSE_BAD_ARGUMENT;
        }
        else
        {
            resp.result = automation_get_ais(transfer->source_node_id, req.index, resp.values.analog_values.values, req.length);
            resp.port_type.port_type = AUTOMATION_PORTTYPE_INPUT;
            resp.index = req.index;
            resp.values.union_tag = AUTOMATION_VALUES_ANALOG_VALUES;
            resp.values.analog_values.values_len = req.length;
        }
        break;
    default:
        resp.result = AUTOMATION_GETVALUES_RESPONSE_BAD_ARGUMENT;
        uavcan_error("Unexpected value type");
        return;
    }
    resp.index = req.index;

    uint32_t len = automation_GetValuesResponse_encode(&resp, buff);
    if (uavcan_send_response(
            transfer,
            AUTOMATION_GETVALUES_SIGNATURE,
            AUTOMATION_GETVALUES_ID,
            buff,
            len))
    {
        uavcan_error("a.GV resp TX failed");
    }
}

static void handle_GetValues_resp(CanardInstance *ins, CanardRxTransfer *transfer)
{
    automation_GetValuesResponse resp;

    if (automation_GetValuesResponse_decode(transfer, transfer->payload_len, &resp, &buff_ptr) < 0)
    {
        uavcan_error("a.GV resp decode failed");
        return;
    }

    if (resp.result != AUTOMATION_GETVALUES_RESPONSE_OK) {
        // TODO: error response callback
        uavcan_error("Error response: GetValues(%d,%d,%d) = %d", transfer->source_node_id, resp.values.union_tag, resp.index, resp.result);
        return;
    }

    switch (resp.values.union_tag)
    {
    case AUTOMATION_VALUES_DIGITAL_VALUES:
        if(resp.port_type.port_type == AUTOMATION_PORTTYPE_INPUT) {
            automation_on_get_dis_response(transfer->source_node_id, resp.index, resp.values.digital_values.values, resp.values.digital_values.values_len);
        } else {
            // TODO: get dos
        }
        return;
    case AUTOMATION_VALUES_ANALOG_VALUES:
        if(resp.port_type.port_type == AUTOMATION_PORTTYPE_INPUT) {
            automation_on_get_ais_response(transfer->source_node_id, resp.index, resp.values.analog_values.values, resp.values.analog_values.values_len);
        } else {
            // TODO: get aos
        }
        return;
    }
}

static uint8_t tell_transfer_id = 0;

static int16_t automation_send_tell_d(uint8_t port_type, uint8_t index, const bool *values, uint8_t len, uint8_t priority) {
    uint8_t buff[AUTOMATION_TELLVALUES_MAX_SIZE];
    automation_TellValues msg;

    if(len > AUTOMATION_DIGITALVALUES_VALUES_LENGTH) {
        return -1;
    }

    msg.port_type.port_type = port_type;
    msg.index = index;
    msg.values.union_tag = AUTOMATION_VALUES_DIGITAL_VALUES;
    msg.values.digital_values.values_len = len;
    for(int i=0; i<len; i++) {
        msg.values.digital_values.values[i] = values[i];
    }
    uint32_t msg_len = automation_TellValues_encode(&msg, buff);

    return uavcan_broadcast(
        AUTOMATION_TELLVALUES_SIGNATURE,
        AUTOMATION_TELLVALUES_ID,
        &tell_transfer_id,
        priority,
        buff,
        msg_len);
}

static int16_t automation_send_tell_a(uint8_t port_type, uint8_t index, const uint16_t *values, uint8_t len, uint8_t priority) {
    uint8_t buff[AUTOMATION_TELLVALUES_MAX_SIZE];
    automation_TellValues msg;

    if(len > AUTOMATION_ANALOGVALUES_VALUES_LENGTH) {
        return -1;
    }

    msg.port_type.port_type = port_type;
    msg.index = index;
    msg.values.union_tag = AUTOMATION_VALUES_ANALOG_VALUES;
    msg.values.analog_values.values_len = len;
    for(int i=0; i<len; i++) {
        msg.values.analog_values.values[i] = values[i];
    }
    uint32_t msg_len = automation_TellValues_encode(&msg, buff);

    return uavcan_broadcast(
        AUTOMATION_TELLVALUES_SIGNATURE,
        AUTOMATION_TELLVALUES_ID,
        &tell_transfer_id,
        priority,
        buff,
        msg_len);
}

int16_t automation_send_tell_dis(uint8_t index, const bool *values, uint8_t len) {
    return automation_send_tell_d(AUTOMATION_PORTTYPE_INPUT, index, values, len, CANARD_TRANSFER_PRIORITY_MEDIUM);
}
int16_t automation_send_tell_ais(uint8_t index, const uint16_t *values, uint8_t len) {
    return automation_send_tell_a(AUTOMATION_PORTTYPE_INPUT, index, values, len, CANARD_TRANSFER_PRIORITY_MEDIUM);
}

int16_t automation_send_set_dos(uint8_t destination_node_id, uint8_t index, const bool *values, uint8_t values_len, uint8_t priority)
{
    uint8_t buff[AUTOMATION_SETVALUES_MAX_SIZE];
    automation_SetValues sv;
    static uint8_t transfer_id = 0;

    sv.node_id = destination_node_id;
    sv.values.union_tag = AUTOMATION_VALUES_DIGITAL_VALUES;
    sv.index = index;
    for (uint8_t i = 0; i < values_len; i++)
    {
        sv.values.digital_values.values[i] = values[i];
    }
    sv.values.digital_values.values_len = values_len;

    uint32_t len = automation_SetValues_encode(&sv, buff);

    return uavcan_broadcast(
        AUTOMATION_SETVALUES_SIGNATURE,
        AUTOMATION_SETVALUES_ID,
        &transfer_id,
        priority,
        buff,
        len);
}

int16_t automation_send_set_aos(uint8_t destination_node_id, uint8_t index, const uint16_t *values, uint8_t values_len, uint8_t priority)
{
    uint8_t buff[AUTOMATION_SETVALUES_MAX_SIZE];
    automation_SetValues sv;
    static uint8_t transfer_id = 0;

    sv.node_id = destination_node_id;
    sv.values.union_tag = AUTOMATION_VALUES_ANALOG_VALUES;
    sv.index = index;
    for (uint8_t i = 0; i < values_len; i++)
    {
        //sv.values.analog_values.values.data[i] = values[i];
        sv.values.analog_values.values[i] = values[i];
    }
    sv.values.analog_values.values_len = values_len;

    uint32_t len = automation_SetValues_encode(&sv, buff);

    return uavcan_broadcast(
        AUTOMATION_SETVALUES_SIGNATURE,
        AUTOMATION_SETVALUES_ID,
        &transfer_id,
        priority,
        buff,
        len);
}

static int16_t send_get_values(uint8_t destination_node_id, uint8_t port_type, uint8_t vals_type, uint8_t index, uint8_t len)
{
    static uint8_t transfer_id = 0;
    uint8_t buff[AUTOMATION_GETVALUES_REQUEST_MAX_SIZE];
    automation_GetValuesRequest req;

    req.index = index;
    req.length = len;
    req.vals_type.value_type = vals_type;
    uint32_t msg_len = automation_GetValuesRequest_encode(&req, buff);

    return uavcan_send_request(
        destination_node_id,
        AUTOMATION_GETVALUES_SIGNATURE,
        AUTOMATION_GETVALUES_ID,
        &transfer_id,
        CANARD_TRANSFER_PRIORITY_HIGH,
        buff,
        msg_len);
}

int16_t automation_send_get_dis(uint8_t destination_node_id, uint8_t index, uint8_t len)
{
    return send_get_values(destination_node_id, AUTOMATION_PORTTYPE_INPUT, AUTOMATION_VALUETYPE_DIGITAL, index, len);
}

int16_t automation_send_get_ais(uint8_t destination_node_id, uint8_t index, uint8_t len)
{
    return send_get_values(destination_node_id, AUTOMATION_PORTTYPE_INPUT, AUTOMATION_VALUETYPE_ANALOG, index, len);
}

/*
int16_t automation_send_set_do_sync(uint8_t destination_node_id, uint8_t output_id, bool value)
{
    static uint8_t transfer_id = 0;
    uint8_t buff[AUTOMATION_SETDIGITALOUTPUTSYNC_REQUEST_MAX_SIZE];
    automation_SetDigitalOutputSyncRequest sdo_req;
    sdo_req.output_id = output_id;
    sdo_req.value = value;
    uint32_t len = automation_SetDigitalOutputSyncRequest_encode(&sdo_req, buff);

    return uavcan_send_request(
        destination_node_id,
        AUTOMATION_SETDIGITALOUTPUTSYNC_SIGNATURE,
        AUTOMATION_SETDIGITALOUTPUTSYNC_ID,
        &transfer_id,
        CANARD_TRANSFER_PRIORITY_MEDIUM,
        buff,
        len);
}
*/