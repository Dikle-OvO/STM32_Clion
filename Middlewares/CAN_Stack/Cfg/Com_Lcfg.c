/******************************************************************************
 * @file   : Com_Lcfg.c
 * @brief  : Link time configuration for Communication (Com) module
 * @author : yangtl
 * @date   : 2025/05/29 11:33:51
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#include <stdio.h>
#include "Com_Lcfg.h"
#include "util_macro.h"
#include "util_platform.h"

#include "PduR.h"
#include "Pdur_Lcfg.h"

Std_ReturnType PduR_ComTransmit(PduIdType PduId, const PduInfoType *PduInfoPtr)
{
    const ComPduMetaDataType *pMetaData = (ComPduMetaDataType *)PduInfoPtr->MetaDataPtr;

    // util_printf("Com[%d] Tx: canid=%x, data(%lubytes):", pMetaData->channel, pMetaData->canId, PduInfoPtr->SduLength);
    // for (PduLengthType i = 0; i < PduInfoPtr->SduLength; ++i) {
    //     printf(" %02X", PduInfoPtr->SduDataPtr[i]);
    // }
    // printf("\n");
	// Com_TxConfirmation(PduId, E_OK);//未配置底层，直接在这通知
		
	if (PDUR_TX_RET_OK != PduR_Send(&gPduRLink, PDUR_MID_COM, pMetaData->canId, 0, PduInfoPtr->SduDataPtr, 8, 0)) {
        return E_NOT_OK;
    }
		
    return E_OK;
}

/************** CAN1 Com configuration **************/

static ComTxRteType can1ComTxRte[10];// 数组项对应每个报文
static ComRxRteType can1ComRxRte[10];

static void Com_CbkCan1TxAck(PduIdType PduId)
{
	// util_printf("%d: TX Ack\r\n",PduId);
}

static void Com_CbkCan1TxErr(PduIdType PduId)
{
	util_printf("%d: TX Err\r\n",PduId);
}

static void Com_CbkCan1RxAck(PduIdType PduId, PduInfoType const *PduInfoPtr)
{
	util_printf("%d: RX Ack\r\n",PduId);
	
	uint8_t gearValid = 0, gearStatus = 0, speedValid = 0;
	uint16_t speed = 0;
	Com_ReceiveSignal(SIG_RX_IDX_VCU_VehicleGearValid_0x200, &gearValid);
	Com_ReceiveSignal(SIG_RX_IDX_VCU_VehicleGearStatus_0x200, &gearStatus);
	Com_ReceiveSignal(SIG_RX_IDX_VCU_VehicleSpeedValid_0x200, &speedValid);
	Com_ReceiveSignal(SIG_RX_IDX_VCU_VehicleSpeed_0x200, &speed); // 接收车辆状态信号
	util_printf(">>Com_ReceiveSignal: gearValid=%d, gearStatus=%d; speedValid=%d, speed=%d\r\n",
							gearValid, gearStatus, speedValid, speed);
}

static void Com_CbkCan1RxTout(PduIdType PduId, PduInfoType const *PduInfoPtr)
{

}

/************** CAN1 Com configuration **************/

static ComTxRteType can2ComTxRte[10];
static ComRxRteType can2ComRxRte[10];

static void Com_CbkCan2TxAck(PduIdType PduId)
{

}

static void Com_CbkCan2TxErr(PduIdType PduId)
{

}

static void Com_CbkCan2RxAck(PduIdType PduId, PduInfoType const *PduInfoPtr)
{

}

static void Com_CbkCan2RxTout(PduIdType PduId, PduInfoType const *PduInfoPtr)
{

}

/************** Configuration of the Com module **************/

static const ComIPduConfigType comIPduCfgGroups[] = {
    {
        .cbkTxAck = Com_CbkCan1TxAck,
        .cbkTxErr = Com_CbkCan1TxErr,
        .cbkRxAck = Com_CbkCan1RxAck,
        .cbkRxTout = Com_CbkCan1RxTout,

        .pTxRtes = can1ComTxRte,
        .pRxRtes = can1ComRxRte,
        .pTxPdus = gTxMsgsCfg_BLE,
        .pRxPdus = gRxMsgsCfg_BLE,
        .pTxSignal = gTxSignalsCfg_BLE,
        .pRxSignal = gRxSignalsCfg_BLE,
        .numOfTxPdus = U_COUNTOF(gTxMsgsCfg_BLE), //TX PDU数量
        .numOfRxPdus = U_COUNTOF(gRxMsgsCfg_BLE),
        .numOfTxSignal = U_COUNTOF(gTxSignalsCfg_BLE),
        .numOfRxSignal = U_COUNTOF(gRxSignalsCfg_BLE),
        .inverseOrder = FALSE, /* Intel: FALSE; Motorola: TRUE */
    },
#if 0
    {
        .cbkTxAck = Com_CbkCan2TxAck,
        .cbkTxErr = Com_CbkCan2TxErr,
        .cbkRxAck = Com_CbkCan2RxAck,
        .cbkRxTout = Com_CbkCan2RxTout,

        .pTxRtes = can2ComTxRte,
        .pRxRtes = can2ComRxRte,
        .pTxPdus = NULL,
        .pRxPdus = NULL,
        .pTxSignal = NULL,
        .pRxSignal = NULL,
        .numOfTxPdus = 0,
        .numOfRxPdus = 0,
        .numOfTxSignal = 0,
        .numOfRxSignal = 0,
        .inverseOrder = FALSE, /* Intel: FALSE; Motorola: TRUE */
    },
#endif
};

const Com_ConfigType gComConfigExample = {
    .pPduGroup = comIPduCfgGroups,
    .numOfPduGroup = U_COUNTOF(comIPduCfgGroups),
    .cyclePeriod = 10, /* 10ms */
};

/*********************************END OF FILE*********************************/
