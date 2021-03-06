/*
 * UAVCAN data structure definition for libcanard.
 *
 * Autogenerated, do not edit.
 *
 */
#include "automation/AnalogValues.h"
#include "canard.h"

#ifndef CANARD_INTERNAL_SATURATE
#define CANARD_INTERNAL_SATURATE(x, max) ( ((x) > max) ? max : ( (-(x) > max) ? (-max) : (x) ) );
#endif

#ifndef CANARD_INTERNAL_SATURATE_UNSIGNED
#define CANARD_INTERNAL_SATURATE_UNSIGNED(x, max) ( ((x) >= max) ? max : (x) );
#endif

#if defined(__GNUC__)
# define CANARD_MAYBE_UNUSED(x) x __attribute__((unused))
#else
# define CANARD_MAYBE_UNUSED(x) x
#endif

/**
  * @brief automation_AnalogValues_encode_internal
  * @param source : pointer to source data struct
  * @param msg_buf: pointer to msg storage
  * @param offset: bit offset to msg storage
  * @param root_item: for detecting if TAO should be used
  * @retval returns new offset
  */
uint32_t automation_AnalogValues_encode_internal(automation_AnalogValues* source,
  void* msg_buf,
  uint32_t offset,
  uint8_t CANARD_MAYBE_UNUSED(root_item))
{
    uint32_t c = 0;

    source->values_len = CANARD_INTERNAL_SATURATE_UNSIGNED(source->values_len, 3)
    canardEncodeScalar(msg_buf, offset, 2, (void*)&source->values_len); // 3
    offset += 2;

    // Static array (values)
    for (c = 0; c < 2; c++)
    {
        canardEncodeScalar(msg_buf, offset, 16, (void*)(source->values + c)); // 65535
        offset += 16;
    }

    return offset;
}

/**
  * @brief automation_AnalogValues_encode
  * @param source : Pointer to source data struct
  * @param msg_buf: Pointer to msg storage
  * @retval returns message length as bytes
  */
uint32_t automation_AnalogValues_encode(automation_AnalogValues* source, void* msg_buf)
{
    uint32_t offset = 0;

    offset = automation_AnalogValues_encode_internal(source, msg_buf, offset, 1);

    return (offset + 7 ) / 8;
}

/**
  * @brief automation_AnalogValues_decode_internal
  * @param transfer: Pointer to CanardRxTransfer transfer
  * @param payload_len: Payload message length
  * @param dest: Pointer to destination struct
  * @param dyn_arr_buf: NULL or Pointer to memory storage to be used for dynamic arrays
  *                     automation_AnalogValues dyn memory will point to dyn_arr_buf memory.
  *                     NULL will ignore dynamic arrays decoding.
  * @param offset: Call with 0, bit offset to msg storage
  * @retval new offset or ERROR value if < 0
  */
int32_t automation_AnalogValues_decode_internal(
  const CanardRxTransfer* transfer,
  uint16_t CANARD_MAYBE_UNUSED(payload_len),
  automation_AnalogValues* dest,
  uint8_t** CANARD_MAYBE_UNUSED(dyn_arr_buf),
  int32_t offset)
{
    int32_t ret = 0;
    uint32_t c = 0;

    ret = canardDecodeScalar(transfer, (uint32_t)offset, 2, false, (void*)&dest->values_len);
    if (ret != 2)
    {
        goto automation_AnalogValues_error_exit;
    }
    offset += 2;

    // Static array (values)
    for (c = 0; c < 2; c++)
    {
        ret = canardDecodeScalar(transfer, (uint32_t)offset, 16, false, (void*)(dest->values + c));
        if (ret != 16)
        {
            goto automation_AnalogValues_error_exit;
        }
        offset += 16;
    }
    return offset;

automation_AnalogValues_error_exit:
    if (ret < 0)
    {
        return ret;
    }
    else
    {
        return -CANARD_ERROR_INTERNAL;
    }
}

/**
  * @brief automation_AnalogValues_decode
  * @param transfer: Pointer to CanardRxTransfer transfer
  * @param payload_len: Payload message length
  * @param dest: Pointer to destination struct
  * @param dyn_arr_buf: NULL or Pointer to memory storage to be used for dynamic arrays
  *                     automation_AnalogValues dyn memory will point to dyn_arr_buf memory.
  *                     NULL will ignore dynamic arrays decoding.
  * @retval offset or ERROR value if < 0
  */
int32_t automation_AnalogValues_decode(const CanardRxTransfer* transfer,
  uint16_t payload_len,
  automation_AnalogValues* dest,
  uint8_t** dyn_arr_buf)
{
    const int32_t offset = 0;
    int32_t ret = 0;

    // Clear the destination struct
    for (uint32_t c = 0; c < sizeof(automation_AnalogValues); c++)
    {
        ((uint8_t*)dest)[c] = 0x00;
    }

    ret = automation_AnalogValues_decode_internal(transfer, payload_len, dest, dyn_arr_buf, offset);

    return ret;
}
