/******************************************************************************
 * @file   : Com.h
 * @brief  : Communication module implements the function of sending/receiving
 *           signals according to Specification of AUTOSAR CP.
 * @author : yangtl
 * @date   : 2022/03/17 14:31:01
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#ifndef __COM_H__
#define __COM_H__

/*---------------------------------
 * INCLUDES
 */
#include "Com_Types.h"
#include "Com_Custom.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------
 * MACROS & CONSTANTS
 */
#define COM_SERVICE_NOT_AVAILABLE   ((Std_ReturnType)0x80)
#define COM_BUSY                    ((Std_ReturnType)0x81)

/*---------------------------------
 * TYPEDEFS
 */

typedef enum {
  COM_INIT,
  COM_UNINIT
} Com_StatusType;

/*---------------------------------
 * FUNCTIONS
 */

void Com_Init(const Com_ConfigType *config);
void Com_DeInit(void);
void Com_IpduGroupStart(Com_IpduGroupIdType IpduGroupId, boolean initialize);
void Com_IpduGroupStop(Com_IpduGroupIdType IpduGroupId);
void Com_EnableReceptionDM(Com_IpduGroupIdType IpduGroupId);
void Com_DisableReceptionDM(Com_IpduGroupIdType IpduGroupId);
Com_StatusType Com_GetStatus(void);

void Com_MainFunctionRx(void);
void Com_MainFunctionTx(void);

/**
 * @brief      By a call to Com_TriggerIPDUSend the I-PDU with the given ID is triggered for transmission.
 * @param[in]  TxPduId The I-PDU-ID of the I-PDU that shall be triggered for sending
 * @param[in]  active Only for the COM_MSG_IF_ACTIVE and COM_MSG_PA message type
 * @retval     E_OK: I-PDU was triggered for transmission
 * @retval     E_NOT_OK: I-PDU is stopped, the transmission could not be triggered
 */
Std_ReturnType Com_TriggerIPDUSend(PduIdType TxPduId, boolean active);

/**
 * @brief      updates the signal object by SignalId with the signal referenced by the SignalDataPtr parameter.
 * @param[in]  SignalId Id of signal to be sent.
 * @param[in]  SignalDataPtr Reference to the signal data to be transmitted.
 * @retval     E_OK: service has been accepted
 * @retval     COM_SERVICE_NOT_AVAILABLE: corresponding I-PDU group was stopped
 *
 * Sync/Async: Asynchronous
 */
uint8 Com_SendSignal(Com_SignalIdType SignalId, const void *SignalDataPtr);

/**
 * @brief      copies the data of the signal identified by SignalId to the location specified by SignalDataPtr.
 * @param[in]  SignalId Id of signal to be received.
 * @param[out] SignalDataPtr Reference to the location where the received signal data shall be stored.
 * @retval     E_OK: service has been accepted
 * @retval     COM_SERVICE_NOT_AVAILABLE: corresponding I-PDU group was stopped
 *
 * Sync/Async: Synchronous
 */
uint8 Com_ReceiveSignal(Com_SignalIdType SignalId, void *SignalDataPtr);


/* Callback Functions and Notifications */
void Com_RxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr);
Std_ReturnType Com_TriggerTransmit(PduIdType TxPduId, PduInfoType *PduInfoPtr);
void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/* Other(Non-standard) */
void Com_CommunicationControl(Com_IpduGroupIdType IpduGroupId, boolean TxEnable, boolean RxEnable);
boolean Com_IsSendEnable(Com_IpduGroupIdType IpduGroupId);
boolean Com_IsRecvEnable(Com_IpduGroupIdType IpduGroupId);
const PduInfoType* Com_GetRxPduInfo(PduIdType RxPduId);
const PduInfoType* Com_GetTxPduInfo(PduIdType TxPduId);
ComRxStatusType Com_RecvStatus(PduIdType RxPduId);


/************** Expected interfaces **************/

extern Std_ReturnType PduR_ComTransmit(PduIdType PduId, const PduInfoType *PduInfoPtr);

#ifdef __cplusplus
}
#endif

#endif /* __COM_H__ */
/*********************************END OF FILE*********************************/
