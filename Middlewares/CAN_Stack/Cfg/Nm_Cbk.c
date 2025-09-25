/******************************************************************************
 * @file   : Nm_Cbk.c
 * @brief  : Network Management (Nm) module callback functions
 * @author : yangtl
 * @date   : 2025/05/15 19:35:07
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#include <stdio.h>
#include "Nm_Cbk.h"
#include "CanNm.h"

#include "PduR.h"
#include "Pdur_Lcfg.h"
#include "Nm_Lcfg.h"
#include "Com_Lcfg.h"

/************** Expected interfaces **************/

Std_ReturnType CanNmIf_Transmit(PduIdType CanTxPduId, const PduInfoType *PduInfoPtr)
{
    util_printf("CanNmIf Tx: CanTxPduId=%d, data(%lubytes):", CanTxPduId, PduInfoPtr->SduLength);
    for (PduLengthType i = 0; i < PduInfoPtr->SduLength; ++i) {
        printf(" %02X", PduInfoPtr->SduDataPtr[i]);
    }
    printf("\n");
		
		if (PDUR_TX_RET_OK != PduR_Send(&gPduRLink, PDUR_MID_COM, CanTxPduId, 0, PduInfoPtr->SduDataPtr, 8, 0)) {
        return E_NOT_OK;
    }

    return E_OK;
}


/**************  Standard Call-back notifications **************/

void Nm_NetworkStartIndication(NetworkHandleType nmNetworkHandle)
{
    util_printf("Nm%d NetworkStartInd\n", nmNetworkHandle);
		Nm_PassiveStartUp(nmNetworkHandle);
}

void Nm_NetworkMode(NetworkHandleType nmNetworkHandle)
{
    util_printf("Nm%d NetworkMode\n", nmNetworkHandle);
		
		Nm_NetworkRequest(nmNetworkHandle);
	
		Com_IpduGroupStart(nmNetworkHandle, FALSE);
    Com_EnableReceptionDM(nmNetworkHandle);
}

void Nm_BusSleepMode(NetworkHandleType nmNetworkHandle)
{
    util_printf("Nm%d BusSleepMode\n", nmNetworkHandle);
}

void Nm_PrepareBusSleepMode(NetworkHandleType nmNetworkHandle)
{
    util_printf("Nm%d PrepareBusSleepMode\n", nmNetworkHandle);
		Com_IpduGroupStop(nmNetworkHandle);
    Com_DisableReceptionDM(nmNetworkHandle);
    PduR_TxBufferFlush(&gPduRLink);
}


void Nm_NetworkModeConfirmation(NetworkHandleType nmNetworkHandle)
{
    util_printf("Nm%d NetworkModeConfirmation\n", nmNetworkHandle);
}

/************** Extra Call-back notifications **************/

void Nm_StateChangeNotification(NetworkHandleType nmNetworkHandle, Nm_StateType nmPreviousState, Nm_StateType nmCurrentState)
{
    static const char *NM_STATE_STR[] = {
        "NM_STATE_UNINIT",
        "NM_STATE_BUS_SLEEP",
        "NM_STATE_PREPARE_BUS_SLEEP",
        "NM_STATE_READY_SLEEP",
        "NM_STATE_NORMAL_OPERATION",
        "NM_STATE_REPEAT_MESSAGE",
        "NM_STATE_SYNCHRONIZE",
        "NM_STATE_OFFLINE",
    };

    util_printf("Nm%d %s -> %s\n", nmNetworkHandle, NM_STATE_STR[nmPreviousState], NM_STATE_STR[nmCurrentState]);
}

void Nm_RepeatMessageIndication(NetworkHandleType nmNetworkHandle, boolean pnLearningBitSet)
{

}

/*********************************END OF FILE*********************************/
