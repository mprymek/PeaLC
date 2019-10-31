#include "app_config.h"

#ifdef STM32
#include <Arduino.h>

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <core_cm3.h>

#ifdef STM32F1
#include <stm32f1xx_hal.h>
#include <stm32f1xx_hal_can.h>
#else
#error unsupported STM32 family
#endif

#include <uavcan_node.h>

#include "uavcan_impl.h"
#include "hal.h"
#include "tools.h"
#include "ui.h"

// ---------------------------------------------- CAN --------------------------

static CAN_HandleTypeDef hcan;

int can2_init()
{
	hcan.Instance = CAN1;
	hcan.Init.Mode = CAN_MODE_NORMAL;
	hcan.Init.TimeTriggeredMode = DISABLE;
	hcan.Init.AutoBusOff = DISABLE;
	hcan.Init.AutoWakeUp = DISABLE;
	hcan.Init.AutoRetransmission = ENABLE;
	hcan.Init.ReceiveFifoLocked = DISABLE;
	hcan.Init.TransmitFifoPriority = DISABLE;

	// timing calculator:
	// http://www.bittiming.can-wiki.info/
	// set: Clock rate = 36, SJW = 1

	// or: https://github.com/GBert/misc/blob/bc7502695b2d0b45b50148531df3d047d846eda3/stm32-slcan/stm32-slcan.c#L124

	// CAN_SPEED == 250000
	hcan.Init.Prescaler = 9;
	hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
	hcan.Init.TimeSeg1 = CAN_BS1_13TQ;
	hcan.Init.TimeSeg2 = CAN_BS2_2TQ;

	if (HAL_CAN_Init(&hcan) != HAL_OK) {
		log_error("CAN init failed");
		return -1;
	}

	CAN_FilterTypeDef filter;
	filter.FilterBank = 0;
	filter.FilterMode = CAN_FILTERMODE_IDMASK;
	filter.FilterScale = CAN_FILTERSCALE_32BIT;
	filter.FilterIdHigh = 0;
	filter.FilterIdLow = 0;
	filter.FilterMaskIdHigh = 0;
	filter.FilterMaskIdLow = 0;
	filter.FilterFIFOAssignment = CAN_RX_FIFO0;
	filter.FilterActivation = ENABLE;
	filter.SlaveStartFilterBank = 14;
	if (HAL_CAN_ConfigFilter(&hcan, &filter) != HAL_OK) {
		log_error("CAN filter config failed");
		return -1;
	}

	//HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 1, 0);
	//HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
	//HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);

	if (HAL_CAN_Start(&hcan) != HAL_OK) {
		log_error("CAN start failed");
		return -1;
	}

	return 0;
}

void HAL_CAN_MspInit(CAN_HandleTypeDef *canHandle)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	if (canHandle->Instance == CAN1) {
		// CAN1 clock enable
		__HAL_RCC_CAN1_CLK_ENABLE();

#ifdef USE_ALT_CAN
		// RX = PB8, TX = PB9
#error alternative CAN config does not work atm
		__HAL_RCC_GPIOB_CLK_ENABLE();

		GPIO_InitStruct.Pin = GPIO_PIN_8;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_9;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
		__HAL_AFIO_REMAP_CAN1_2();
#else
		// RX = PA11, TX = PA12
		__HAL_RCC_GPIOA_CLK_ENABLE();

		GPIO_InitStruct.Pin = GPIO_PIN_11;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_12;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif // #ifdef USE_ALT_CAN
	}
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef *canHandle)
{
	if (canHandle->Instance == CAN1) {
		// Peripheral clock disable
		__HAL_RCC_CAN1_CLK_DISABLE();

#ifdef USE_ALT_CAN
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8 | GPIO_PIN_9);
#else
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);
#endif
	}
}

// ---------------------------------------------- UAVCAN -----------------------

void uavcan_get_unique_id(
	uint8_t out_uid[UAVCAN_PROTOCOL_HARDWAREVERSION_UNIQUE_ID_LENGTH])
{
	// For other STM32 families, see e.g. http://blog.gorski.pm/stm32-unique-id
#ifdef STM32F1
	unsigned long *id = (unsigned long *)0x1FFFF7E8;
#else
#error uavcan_get_unique_id(...) not implemented for this STM32 family
#endif
	assert(UAVCAN_PROTOCOL_HARDWAREVERSION_UNIQUE_ID_LENGTH >= 12);
	for (int i = 0; i < 12; i++) {
		out_uid[i] = id[i];
	}
	for (int i = 12; i < UAVCAN_PROTOCOL_HARDWAREVERSION_UNIQUE_ID_LENGTH;
	     i++) {
		out_uid[i] = 0;
	}
}

void uavcan_restart(void)
{
	NVIC_SystemReset();
}

int uavcan_can_rx(CanardCANFrame *frame)
{
	CAN_RxHeaderTypeDef header;

	if (HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) == 0)
		return 0;

	if (HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &header, frame->data) !=
	    HAL_OK)
		return 0;

	// Set UAVCAN frame flags
	if (header.IDE == CAN_ID_EXT) {
		frame->id = header.ExtId | CANARD_CAN_FRAME_EFF;
	} else {
		frame->id = header.StdId;
	}
	if (header.RTR == CAN_RTR_REMOTE) {
		frame->id |= CANARD_CAN_FRAME_RTR;
	}
	// TODO: error flag?!

	frame->data_len = header.DLC;

#if LOGLEVEL >= LOGLEVEL_DEBUG
	print_frame("->", frame);
#endif

	ui_can_rx();

	return 1;
}

int uavcan_can_tx(const CanardCANFrame *frame)
{
	uint32_t mailBox;
	HAL_StatusTypeDef res;
	CAN_TxHeaderTypeDef can1message;

	can1message.ExtId = 0;
	can1message.StdId = 0;
	can1message.TransmitGlobalTime = DISABLE;
	can1message.RTR = (frame->id & CANARD_CAN_FRAME_RTR) ? CAN_RTR_REMOTE :
							       CAN_RTR_DATA;
	can1message.DLC = frame->data_len;

	if (frame->id & CANARD_CAN_FRAME_EFF) {
		can1message.IDE = CAN_ID_EXT;
		can1message.ExtId = frame->id & (~(CANARD_CAN_FRAME_EFF |
						   CANARD_CAN_FRAME_ERR |
						   CANARD_CAN_FRAME_RTR));
	} else {
		can1message.IDE = CAN_ID_STD;
		can1message.StdId = frame->id & (~(CANARD_CAN_FRAME_EFF |
						   CANARD_CAN_FRAME_ERR |
						   CANARD_CAN_FRAME_RTR));
	}

#if LOGLEVEL >= LOGLEVEL_DEBUG
	print_frame("<-", frame);
#endif
	// TODO: discarding "const" qualifier!
	if ((res = HAL_CAN_AddTxMessage(&hcan, &can1message,
					(uint8_t *)frame->data, &mailBox)) !=
	    HAL_OK) {
		uavcan_error("CAN TX error");
		return -2;
	}

	ui_can_tx();

	return 0;
}

#endif // #ifdef STM32
