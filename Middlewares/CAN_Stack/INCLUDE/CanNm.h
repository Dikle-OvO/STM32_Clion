/******************************************************************************
 * @file   : CanNm.h
 * @brief  : Header of CAN Network Management AUTOSAR CP R24-11
 * @author : yangtl
 * @date   : 2025/04/23 12:04:07
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#ifndef CANNM_H
#define CANNM_H

/*! @addtogroup CanNm
 *  @{
 */

#include "CanNm_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CANNM_SW_MAJOR_VERSION            2
#define CANNM_SW_MINOR_VERSION            0
#define CANNM_SW_PATCH_VERSION            0

/* AUTOSAR Software specification version information */
#define CANNM_AR_RELEASE_MAJOR_VERSION    0x04u
#define CANNM_AR_RELEASE_MINOR_VERSION    0x04u
#define CANNM_AR_RELEASE_REVISION_VERSION 0x00u

#define CANNM_VENDOR_ID                   70u
#define CANNM_MODULE_ID                   31u

/**
 * @brief      Initialize the CanNm module.
 * @param[in]  cannmConfigPtr Pointer to a selected configuration structure
 * @retval     None
 *
 * Sync/Async: Synchronous
 */
void CanNm_Init(const CanNm_ConfigType *cannmConfigPtr);

/**
 * @fn     CanNm_DeInit()
 * @note   Caller of the CanNm_DeInit function has to ensure all CAN networks are in the Bus Sleep mode.
 * @brief  De-initializes the CanNm module.
 * @param  None
 * @retval None
 *
 * Sync/Async: Synchronous
 */
void CanNm_DeInit(void);

/**
 * @fn     CanNm_MainFunction()
 * @brief  Main function of the CanNm which processes the algorithm describes in that document.
 * @param  None
 * @retval None
 */
void CanNm_MainFunction(void);

/**
 * @brief      Passive startup of the AUTOSAR CAN NM. It triggers the transition from Bus-Sleep
 *             Mode or Prepare Bus Sleep Mode to the Network Mode in Repeat Message State.
 * @param[in]  nmChannelHandle Identification of the NM-channel
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Passive startup of network management has failed
 *
 * Sync/Async: Asynchronous
 */
Std_ReturnType CanNm_PassiveStartUp(NetworkHandleType nmChannelHandle);

#if (CANNM_PASSIVE_MODE_ENABLED == FALSE)
/**
 * @brief      Request the network, since ECU needs to communicate on the bus.
 * @param[in]  nmChannelHandle Identification of the NM-channel
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Requesting of network has failed
 *
 * Sync/Async: Asynchronous
 */
Std_ReturnType CanNm_NetworkRequest(NetworkHandleType nmChannelHandle);

/**
 * @brief      Release the network, since ECU doesn’t have to communicate on the bus.
 * @param[in]  nmChannelHandle Identification of the NM-channel
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Releasing of network has failed
 *
 * Sync/Async: Asynchronous
 */
Std_ReturnType CanNm_NetworkRelease(NetworkHandleType nmChannelHandle);
#endif

#if (CANNM_COM_CONTROL_ENABLED == TRUE)
/**
 * @brief      Disable the NM PDU transmission ability due to a ISO14229 Communication Control (28hex) service.
 * @param[in]  nmChannelHandle Identification of the NM-channel
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Disabling of NM PDU transmission ability has failed
 *
 * Sync/Async: Asynchronous
 */
Std_ReturnType CanNm_DisableCommunication(NetworkHandleType nmChannelHandle);

/**
 * @brief      Enable the NM PDU transmission ability due to a ISO14229 Communication Control (28hex) service.
 * @param[in]  nmChannelHandle Identification of the NM-channel
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Enabling of NM PDU transmission ability has failed
 *
 * Sync/Async: Asynchronous
 */
Std_ReturnType CanNm_EnableCommunication(NetworkHandleType nmChannelHandle);
#endif

#if (CANNM_USER_DATA_ENABLED == TRUE)
#  if (CANNM_PASSIVE_MODE_ENABLED == FALSE) && (CANNM_COM_USER_DATA_SUPPORT == FALSE)
/**
 * @brief      Set user data for NM PDUs transmitted next on the bus.
 * @param[in]  nmChannelHandle Identification of the NM-channel
 * @param[in]  nmUserDataPtr Pointer where the user data for the next transmitted NM PDU shall be copied from
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Setting of user data has failed
 *
 * Sync/Async: Synchronous
 */
Std_ReturnType CanNm_SetUserData(NetworkHandleType nmChannelHandle, const uint8_t *nmUserDataPtr);
#  endif

/**
 * @brief      Get user data out of the most recently received NM PDU.
 * @param[in]  nmChannelHandle Identification of the NM-channel
 * @param[out] nmUserDataPtr Pointer where user data out of the most recently received NM PDU shall be copied to
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Getting of user data has failed
 *
 * Sync/Async: Synchronous
 */
Std_ReturnType CanNm_GetUserData(NetworkHandleType nmChannelHandle, uint8_t *nmUserDataPtr);
#endif

#if 0 /* TODO */
/**
 * @brief      Requests transmission of a PDU.
 * @param[in]  TxPduId Identifier of the PDU to be transmitted
 * @param[in]  PduInfoPtr Length of and pointer to the PDU data and pointer to MetaData.
 * @retval     E_OK: Transmit request has been accepted.
 * @retval     E_NOT_OK: Transmit request has not been accepted.
 *
 * Sync/Async: Synchronous
 */
Std_ReturnType CanNm_Transmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr);
#endif

/**
 * @brief      Get node identifier out of the most recently received NM PDU.
 * @param[in]  nmChannelHandle Identification of the NM-channel
 * @param[out] nmNodeIdPtr Pointer where node identifier out of the most recently received NM PDU shall be copied to
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Getting of the node identifier out of the most recently received
 *                       NM PDU has failed or is not configured for this network handle.
 *
 * Sync/Async: Synchronous
 */
Std_ReturnType CanNm_GetNodeIdentifier(NetworkHandleType nmChannelHandle, uint8_t *nmNodeIdPtr);

/**
 * @brief      Get node identifier configured for the local node.
 * @param[in]  nmChannelHandle Identification of the NM-channel
 * @param[out] nmNodeIdPtr Pointer where node identifier of the local node shall be copied to
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Getting of the node identifier of the local node has failed or
 *                       is not configured for this network handle.
 *
 * Sync/Async: Synchronous
 */
Std_ReturnType CanNm_GetLocalNodeIdentifier(NetworkHandleType nmChannelHandle, uint8_t *nmNodeIdPtr);

/**
 * @brief      Set Repeat Message Request Bit for NM PDUs transmitted next on the bus.
 * @param[in]  nmChannelHandle Identification of the NM-channel
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Setting of Repeat Message Request Bit has failed or
 *                       is not configured for this network handle.
 *
 * Sync/Async: Asynchronous
 */
Std_ReturnType CanNm_RepeatMessageRequest(NetworkHandleType nmChannelHandle);

/**
 * @brief      Get the whole PDU data out of the most recently received NM PDU.
 * @param[in]  nmChannelHandle Identification of the NM-channel
 * @param[out] nmPduDataPtr Pointer where NM PDU shall be copied to
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Getting of NM PDU Data has failed or is not
 *                       configured for this network handle.
 *
 * Sync/Async: Synchronous
 */
Std_ReturnType CanNm_GetPduData(NetworkHandleType nmChannelHandle, uint8_t *nmPduDataPtr);

/**
 * @brief      Returns the state and the mode of the network management.
 * @param[in]  nmChannelHandle Identification of the NM-channel
 * @param[out] nmStatePtr Pointer where state of the network management shall be copied to
 * @param[out] nmModePtr Pointer where the mode of the network management shall be copied to
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Getting of NM state has failed
 *
 * Sync/Async: Synchronous
 */
Std_ReturnType CanNm_GetState(NetworkHandleType nmChannelHandle, Nm_StateType *nmStatePtr, Nm_ModeType *nmModePtr);

#if (CANNM_VERSION_INFO_API == TRUE)
/**
 * @brief      This service returns the version information of this module.
 * @param[out] versioninfo Pointer to where to store the version information of this module
 * @retval     None
 * Sync/Async: Synchronous
 */
void CanNm_GetVersionInfo(Std_VersionInfoType *versioninfo);
#endif

#if (CANNM_BUS_SYNCHRONIZATION_ENABLED == TRUE)
/**
 * @brief      Request bus synchronization.
 * @param[in]  nmChannelHandle Identification of the NM-channel
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Requesting of bus synchronization has failed
 *
 * Sync/Async: Synchronous
 */
Std_ReturnType CanNm_RequestBusSynchronization(NetworkHandleType nmChannelHandle);
#endif

#if (CANNM_REMOTE_SLEEP_IND_ENABLED == TRUE)
/**
 * @brief      Check if remote sleep indication takes place or not.
 * @param[in]  nmChannelHandle Identification of the NM-channel
 * @param[out] nmRemoteSleepIndPtr Pointer where check result of remote sleep indication shall be copied to
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Checking of remote sleep indication bits has failed
 *
 * Sync/Async: Synchronous
 */
Std_ReturnType CanNm_CheckRemoteSleepIndication(NetworkHandleType nmChannelHandle, boolean *nmRemoteSleepIndPtr);

/**
 * @brief      Set the NM Coordinator Sleep Ready bit in the Control Bit Vector
 * @param[in]  nmChannelHandle Identification of the NM-channel
 * @param[in]  nmSleepReadyBit Value written to ReadySleep Bit in CBV
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Writing of remote sleep indication bit has failed
 *
 * Sync/Async: Synchronous
 */
Std_ReturnType CanNm_SetSleepReadyBit(NetworkHandleType nmChannelHandle, boolean nmSleepReadyBit);
#endif

/************** Non-Standard API **************/

/**
 * @brief      Check whether the NM is Network Mode requested or not.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @retval     TRUE: Network Mode is requested.
 * @retval     FALSE: Network Mode is not requested.
 */
boolean CanNm_IsNetworkModeRequested(NetworkHandleType nmChannelHandle);

/**
 * @brief      Check whether at least one NM message has been successfully sent.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @retval     TRUE: At least one NM message has been successfully sent.
 * @retval     FALSE: No NM message has been successfully sent.
 */
boolean CanNm_IsNetworkModeConfirmed(NetworkHandleType nmChannelHandle);

#if (CANNM_GLOBAL_PN_SUPPORT == TRUE)
/**
 * @brief      Indication by CanNm of internal PNC requests. This is used to aggregate the internal PNC requests.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @param[in]  PncBitVectorPtr Pointer to the bit vector with all PNC bits set to "1" of internal requested PNCs (IRA)
 * @retval     None
 *
 * Sync/Async: Asynchronous
 */
Std_ReturnType CanNm_UpdateIRA(NetworkHandleType nmChannelHandle, const uint8* PncBitVectorPtr);
#endif


/************** Callback notifications **************/

#if ((CANNM_PASSIVE_MODE_ENABLED == FALSE) && (CANNM_IMMEDIATE_TX_CONF_ENABLED == FALSE))
/**
 * @brief      The lower layer communication interface module confirms the transmission of a PDU,
 *             or the failure to transmit a PDU.
 * @param[in]  TxPduId ID of the PDU that has been transmitted
 * @param[in]  result  E_OK: The PDU was transmitted.
 *                     E_NOT_OK: Transmission of the PDU failed.
 * @retval     None
 *
 * Sync/Async: Synchronous
 */
void CanNm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);
#endif

/**
 * @brief      Indication of a received PDU from a lower layer communication interface module.
 * @param[in]  RxPduId ID of the received PDU
 * @param[in]  PduInfoPtr Contains the length (SduLength) of the received PDU, a pointer
 *                        to a buffer (SduDataPtr) containing the PDU, and the MetaData related to this PDU.
 * @retval     None
 *
 * Sync/Async: Synchronous
 */
void CanNm_RxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr);

#if 0 /* TODO */
/**
 * @brief      Enables the PN filter functionality on the indicated NM channel. Availability: The API is only
 *             available if CanNmGlobalPnSupport is TRUE.
 * @param[in]  nmChannelHandle Identification of the NM-channel
 * @retval     None
 *
 * Sync/Async: Synchronous
 */
void CanNm_ConfirmPnAvailability(NetworkHandleType nmChannelHandle);

Std_ReturnType CanNm_TriggerTransmit(PduIdType TxPduId, PduInfoType *PduInfoPtr);
#endif

/************** Expected interfaces **************/

extern Std_ReturnType CanNmIf_Transmit(PduIdType CanTxPduId, const PduInfoType *PduInfoPtr);

#ifdef __cplusplus
}
#endif

/*! @}*/

#endif /* CANNM_H */
/*********************************END OF FILE*********************************/
