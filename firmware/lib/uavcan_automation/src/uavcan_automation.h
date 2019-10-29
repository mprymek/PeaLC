#include <stdint.h>

#include "canard.h"

#ifdef __cplusplus
extern "C"
{
#endif

// API

bool uavcan_automation_should_accept_transfer(const CanardInstance *ins,
                                              uint64_t *out_data_type_signature,
                                              uint16_t data_type_id,
                                              CanardTransferType transfer_type,
                                              uint8_t source_node_id);

bool uavcan_automation_on_transfer_received(CanardInstance *ins, CanardRxTransfer *transfer);

int16_t automation_send_get_dis(uint8_t destination_node_id, uint8_t index, uint8_t len);
int16_t automation_send_get_ais(uint8_t destination_node_id, uint8_t index, uint8_t len);
int16_t automation_send_tell_dis(uint8_t index, const bool *values, uint8_t len);
int16_t automation_send_tell_ais(uint8_t index, const uint16_t *values, uint8_t len);

int16_t automation_send_set_dos(uint8_t destination_node_id,
                               uint8_t start_output_id,
                               const bool *values,
                               uint8_t values_len,
                               uint8_t priority);

int16_t automation_send_set_aos(uint8_t destination_node_id,
                               uint8_t start_output_id,
                               const uint16_t *values,
                               uint8_t values_len,
                               uint8_t priority);

// callbacks

uint8_t automation_set_dos(uint8_t source_node_id, uint8_t output_id, const bool *values, uint8_t len);
uint8_t automation_set_aos(uint8_t source_node_id, uint8_t output_id, const uint16_t *values, uint8_t len);
uint8_t automation_get_dis(uint8_t source_node_id, uint8_t index, bool *values, uint8_t len);
uint8_t automation_get_ais(uint8_t source_node_id, uint8_t index, uint16_t *values, uint8_t len);
void automation_on_get_dis_response(uint8_t source_node_id, uint8_t index, bool *values, uint8_t len);
void automation_on_get_ais_response(uint8_t source_node_id, uint8_t index, uint16_t *values, uint8_t len);
void automation_on_tell_dis(uint8_t source_node_id, uint8_t index, bool *values, uint8_t len);
void automation_on_tell_ais(uint8_t source_node_id, uint8_t index, uint16_t *values, uint8_t len);

#ifdef __cplusplus
}
#endif
