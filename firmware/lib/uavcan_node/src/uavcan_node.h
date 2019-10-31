#include "canard.h"
#include "uavcan/protocol/NodeStatus.h"
#include "uavcan/protocol/GetNodeInfo.h"

#ifdef __cplusplus
extern "C"
{
#endif


// API
void uavcan_init(void);
void uavcan_update(void);

int16_t uavcan_broadcast_status(void);
int uavcan_log(uint8_t level, const char *text);

int16_t uavcan_broadcast(
    uint64_t data_type_signature,
    uint16_t data_type_id,
    uint8_t *inout_transfer_id,
    uint8_t priority,
    const void *payload,
    uint16_t payload_len);

int16_t uavcan_send_response(
    CanardRxTransfer *transfer,
    uint64_t data_type_signature,
    uint16_t data_type_id,
    const void *payload,
    uint16_t payload_len);

int16_t uavcan_send_request(
    uint8_t destination_node_id,
    uint64_t data_type_signature,
    uint16_t data_type_id,
    uint8_t *inout_transfer_id,
    uint8_t priority,
    const void *payload,
    uint16_t payload_len);


// globals
extern volatile uavcan_protocol_NodeStatus uavcan_node_status;
extern uavcan_protocol_GetNodeInfoResponse uavcan_node_info;


// messages handlers callbacks
void uavcan_on_node_status(uint8_t source_node_id, uavcan_protocol_NodeStatus *node_status);

// logging callbacks
void uavcan_error(const char * fmt, ...);
void uavcan_warning(const char * fmt, ...);
void uavcan_info(const char * fmt, ...);
void uavcan_debug(const char * fmt, ...);


// HAL callbacks
int uavcan_can_tx(const CanardCANFrame *frame);
int uavcan_can_rx(CanardCANFrame *frame);
void uavcan_get_unique_id(uint8_t out_uid[UAVCAN_PROTOCOL_HARDWAREVERSION_UNIQUE_ID_LENGTH]);
uint32_t uavcan_uptime_sec(void);
uint64_t uavcan_uptime_usec(void);
void uavcan_restart(void);


// UAVCAN transfer callbacks
bool uavcan_user_should_accept_transfer(const CanardInstance *ins,
                                        uint64_t *out_data_type_signature,
                                        uint16_t data_type_id,
                                        CanardTransferType transfer_type,
                                        uint8_t source_node_id);

void uavcan_user_on_transfer_received(CanardInstance *ins,
                                      CanardRxTransfer *transfer);


#ifdef __cplusplus
}
#endif