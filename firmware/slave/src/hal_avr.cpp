#ifdef __AVR__

#include <Arduino.h>

#include <SPI.h>
#include <mcp_can.h>

#include <uavcan_node.h>

#include "app_config.h"
#include "hal.h"
#include "ui.h"
#include "uavcan_impl.h" // print_frame


// ---------------------------------------------- CAN --------------------------

MCP_CAN CAN0(CAN_CS_PIN);

int can2_init()
{
    if (CAN0.begin(CAN_500KBPS) != CAN_OK)
    {
        return -1;
    }

    return 0;
}


// ---------------------------------------------- UAVCAN -----------------------

// TODO: read unique id from HW
void uavcan_get_unique_id(uint8_t out_uid[UAVCAN_PROTOCOL_HARDWAREVERSION_UNIQUE_ID_LENGTH])
{
    for (int i = 0; i < UAVCAN_PROTOCOL_HARDWAREVERSION_UNIQUE_ID_LENGTH; i++)
    {
        out_uid[i] = 0xFF;
    }
}

void (*reset)(void) = 0;
void uavcan_restart(void)
{
    reset();
}

int uavcan_can_rx(CanardCANFrame *frame)
{
    if (CAN0.checkReceive() != CAN_MSGAVAIL)
    {
        return 0;
    }

    ui_can_rx();

    // we MUST call readMsgBuff first and getCanId after
    CAN0.readMsgBuf(&frame->data_len, frame->data);
    frame->id = CAN0.getCanId();
    if (1 /*CAN0.isExtendedFrame()*/)
    {
        frame->id |= CANARD_CAN_FRAME_EFF;
    }
    if (0 /*CAN0.isRemoteRequest()*/)
    {
        frame->id |= CANARD_CAN_FRAME_RTR;
    }

#if LOGLEVEL >= LOGLEVEL_DEBUG
    print_frame("->", frame);
#endif

    return 1;
}

int uavcan_can_tx(const CanardCANFrame *frame)
{
#if LOGLEVEL >= LOGLEVEL_DEBUG
    print_frame("<-", frame);
#endif

    unsigned long id2 = frame->id & (~(CANARD_CAN_FRAME_EFF | CANARD_CAN_FRAME_ERR | CANARD_CAN_FRAME_RTR));
    byte ext = (frame->id & CANARD_CAN_FRAME_EFF) ? 1 : 0;
    CAN0.sendMsgBuf(id2, ext, frame->data_len, (byte *)frame->data);

    ui_can_tx();

    return 0;
}

#endif // ifdef __AVR__
