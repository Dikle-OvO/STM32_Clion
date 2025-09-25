/******************************************************************************
 * @file   : Nm.h
 * @brief  : Header of Network Management Interface AUTOSAR CP R24-11
 * @author : yangtl
 * @date   : 2025/04/23 13:49:42
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#ifndef NM_H
#define NM_H

/*! @addtogroup Nm
 *  @{
 */

#include "Nm_Cbk.h"
#include "Nm_Cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(NM_PROTOCOL_AUTOSAR) && defined(NM_PROTOCOL_OSEK)
#error "Only one of NM_PROTOCOL_AUTOSAR and NM_PROTOCOL_OSEK can be defined, not both."
#elif !defined(NM_PROTOCOL_AUTOSAR) && !defined(NM_PROTOCOL_OSEK)
#   define NM_PROTOCOL_AUTOSAR
#   undef NM_MACRO_LAYER_ENABLED
#   define NM_MACRO_LAYER_ENABLED TRUE
#endif

#if defined(NM_PROTOCOL_AUTOSAR)
#   include "CanNm.h"
    typedef CanNm_ConfigType Nm_ConfigType;
#elif defined(NM_PROTOCOL_OSEK)
#   include "osek_nm.h"
    typedef OsekNmConfigType Nm_ConfigType;
#else
#   error "Please predefined symbols: NM_PROTOCOL_AUTOSAR or NM_PROTOCOL_OSEK"
#endif


#if (NM_MACRO_LAYER_ENABLED == TRUE)

#define NM_MAP_2_BUSNM(function)                              CanNm##function

#define Nm_Init(ConfigPtr)                                    NM_MAP_2_BUSNM(_Init)(ConfigPtr)
#define Nm_MainFunction(NetworkHandle)                        NM_MAP_2_BUSNM(_MainFunction)(NetworkHandle)
#define Nm_PassiveStartUp(NetworkHandle)                      NM_MAP_2_BUSNM(_PassiveStartUp)(NetworkHandle)
#define Nm_NetworkRequest(NetworkHandle)                      NM_MAP_2_BUSNM(_NetworkRequest)(NetworkHandle)
#define Nm_NetworkRelease(NetworkHandle)                      NM_MAP_2_BUSNM(_NetworkRelease)(NetworkHandle)
#define Nm_DisableCommunication(NetworkHandle)                NM_MAP_2_BUSNM(_DisableCommunication)(NetworkHandle)
#define Nm_EnableCommunication(NetworkHandle)                 NM_MAP_2_BUSNM(_EnableCommunication)(NetworkHandle)
#define Nm_UpdateIRA(NetworkHandle, PncBitVectorPtr)          NM_MAP_2_BUSNM(_UpdateIRA)(NetworkHandle, PncBitVectorPtr)
#define Nm_SetUserData(NetworkHandle, nmUserDataPtr)          NM_MAP_2_BUSNM(_SetUserData)(NetworkHandle, nmUserDataPtr)
#define Nm_GetUserData(NetworkHandle, nmUserDataPtr)          NM_MAP_2_BUSNM(_GetUserData)(NetworkHandle, nmUserDataPtr)
#define Nm_GetPduData(NetworkHandle, nmPduData)               NM_MAP_2_BUSNM(_GetPduData)(NetworkHandle, nmPduData)
#define Nm_RepeatMessageRequest(NetworkHandle)                NM_MAP_2_BUSNM(_RepeatMessageRequest)(NetworkHandle)
#define Nm_GetNodeIdentifier(NetworkHandle, nmNodeIdPtr)      NM_MAP_2_BUSNM(_GetNodeIdentifier)(NetworkHandle, nmNodeIdPtr)
#define Nm_GetLocalNodeIdentifier(NetworkHandle, nmNodeIdPtr) NM_MAP_2_BUSNM(_GetLocalNodeIdentifier)(NetworkHandle, nmNodeIdPtr)
#define Nm_CheckRemoteSleepIndication(nmNetworkHandle, nmRemoteSleepIndPtr) NM_MAP_2_BUSNM(_CheckRemoteSleepIndication)(nmNetworkHandle, nmRemoteSleepIndPtr)
#define Nm_GetState(nmNetworkHandle, nmStatePtr, nmModePtr)   NM_MAP_2_BUSNM(_GetState)(nmNetworkHandle, nmStatePtr, nmModePtr)
#define Nm_GetVersionInfo(nmVerInfoPtr)                       NM_MAP_2_BUSNM(_GetVersionInfo)(nmVerInfoPtr)

/************** Non-Standard API **************/
#define Nm_RxIndication(RxPduId, PduInfoPtr)                  NM_MAP_2_BUSNM(_RxIndication)(RxPduId, PduInfoPtr)
#define Nm_IsNetworkModeRequested(NetworkHandle)              NM_MAP_2_BUSNM(_IsNetworkModeRequested)(NetworkHandle)
#define Nm_IsNetworkModeConfirmed(NetworkHandle)              NM_MAP_2_BUSNM(_IsNetworkModeConfirmed)(NetworkHandle)

#else

/**
 * @brief      Initialize the NM Interface.
 * @param[in]  ConfigPtr Pointer to the selected configuration set.
 * @retval     None
 *
 * Sync/Async: Synchronous
 */
void Nm_Init(const Nm_ConfigType* ConfigPtr);

/**
 * @brief      This function implements the processes of the NM Interface, which need a fix cyclic scheduling.
 * @param[in]  None
 * @retval     None
 */
void Nm_MainFunction(void);

/**
 * @brief      Passive startup of the NM.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Passive startup of network management has failed
 *
 * Sync/Async: Asynchronous
 */
Std_ReturnType Nm_PassiveStartUp(NetworkHandleType NetworkHandle);

/**
 * @brief      Request the network, since ECU needs to communicate on the bus.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Requesting of network has failed
 *
 * Sync/Async: Asynchronous
 */
Std_ReturnType Nm_NetworkRequest(NetworkHandleType NetworkHandle);

/**
 * @brief      Release the network, since ECU doesn’t have to communicate on the bus.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Releasing of network has failed
 *
 * Sync/Async: Asynchronous
 */
Std_ReturnType Nm_NetworkRelease(NetworkHandleType NetworkHandle);

/**
 * @brief      Disable the NM PDU transmission ability.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Disabling of NM PDU transmission ability has failed
 *
 * Sync/Async: Asynchronous
 */
Std_ReturnType Nm_DisableCommunication(NetworkHandleType NetworkHandle);

/**
 * @brief      Enable the NM PDU transmission ability.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Enabling of NM PDU transmission ability has failed
 *
 * Sync/Async: Asynchronous
 */
Std_ReturnType Nm_EnableCommunication(NetworkHandleType NetworkHandle);

/**
 * @brief      Indication by ComM of internal PNC requests. This is used to aggregate the internal PNC requests.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @param[in]  PncBitVectorPtr Pointer to the bit vector with all PNC bits set to "1" of internal requested PNCs (IRA)
 * @retval     None
 *
 * Sync/Async: Asynchronous
 */
void           Nm_UpdateIRA(NetworkHandleType NetworkHandle, const uint8* PncBitVectorPtr);

/**
 * @brief      Set user data for NM PDUs transmitted next on the bus.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @param[in]  nmUserDataPtr Pointer where the user data for the next transmitted NM PDU shall be copied from
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Setting of user data has failed
 *
 * Sync/Async: Synchronous
 */
Std_ReturnType Nm_SetUserData(NetworkHandleType NetworkHandle, const uint8* nmUserDataPtr);

/**
 * @brief      Get user data out of the most recently received NM PDU.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @param[out] nmUserDataPtr Pointer where user data out of the most recently received NM PDU shall be copied to
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Getting of user data has failed
 *
 * Sync/Async: Synchronous
 */
Std_ReturnType Nm_GetUserData(NetworkHandleType NetworkHandle, uint8* nmUserDataPtr);

/**
 * @brief      Get the whole PDU data out of the most recently received NM PDU.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @param[out] nmPduDataPtr Pointer where NM PDU shall be copied to
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Getting of NM PDU Data has failed or is not
 *                       configured for this network handle.
 *
 * Sync/Async: Synchronous
 */
Std_ReturnType Nm_GetPduData(NetworkHandleType NetworkHandle, uint8* nmPduData);

/**
 * @brief      Set Repeat Message Request Bit for NM PDUs transmitted next on the bus.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Setting of Repeat Message Request Bit has failed or
 *                       is not configured for this network handle.
 *
 * Sync/Async: Asynchronous
 */
Std_ReturnType Nm_RepeatMessageRequest(NetworkHandleType NetworkHandle);

 /**
 * @brief      Get node identifier out of the most recently received NM PDU.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @param[out] nmNodeIdPtr Pointer where node identifier out of the most recently received NM PDU shall be copied to
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Getting of the node identifier out of the most recently received
 *                       NM PDU has failed or is not configured for this network handle.
 *
 * Sync/Async: Synchronous
 */
Std_ReturnType Nm_GetNodeIdentifier(NetworkHandleType NetworkHandle, uint8* nmNodeIdPtr);

/**
 * @brief      Get node identifier configured for the local node.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @param[out] nmNodeIdPtr Pointer where node identifier of the local node shall be copied to
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Getting of the node identifier of the local node has failed or
 *                       is not configured for this network handle.
 *
 * Sync/Async: Synchronous
 */
Std_ReturnType Nm_GetLocalNodeIdentifier(NetworkHandleType NetworkHandle, uint8* nmNodeIdPtr);

/**
 * @brief      Check if remote sleep indication takes place or not.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @param[out] nmRemoteSleepIndPtr Pointer where check result of remote sleep indication shall be copied to
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Checking of remote sleep indication bits has failed
 *
 * Sync/Async: Synchronous
 */
Std_ReturnType Nm_CheckRemoteSleepIndication(NetworkHandleType nmNetworkHandle, boolean* nmRemoteSleepIndPtr);

/**
 * @brief      Returns the state and the mode of the network management.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @param[out] nmStatePtr Pointer where state of the network management shall be copied to
 * @param[out] nmModePtr Pointer where the mode of the network management shall be copied to
 * @retval     E_OK: No error
 * @retval     E_NOT_OK: Getting of NM state has failed
 *
 * Sync/Async: Synchronous
 */
Std_ReturnType Nm_GetState(NetworkHandleType nmNetworkHandle, Nm_StateType* nmStatePtr, Nm_ModeType* nmModePtr);

/**
 * @brief      This service returns the version information of this module.
 * @param[out] versioninfo Pointer to where to store the version information of this module
 * @retval     None
 * Sync/Async: Synchronous
 */
void           Nm_GetVersionInfo(Std_VersionInfoType* nmVerInfoPtr);


/************** Non-Standard API **************/

/**
 * @brief      Indication of a received PDU from a lower layer communication interface module.
 * @param[in]  RxPduId ID of the received PDU
 * @param[in]  PduInfoPtr Contains the length (SduLength) of the received PDU, a pointer
 *                        to a buffer (SduDataPtr) containing the PDU, and the MetaData related to this PDU.
 * @retval     None
 *
 * Sync/Async: Synchronous
 */
void Nm_RxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr);

/**
 * @brief      Check whether the NM is Network Mode requested or not.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @retval     TRUE: Network Mode is requested.
 * @retval     FALSE: Network Mode is not requested.
 */
boolean Nm_IsNetworkModeRequested(NetworkHandleType nmChannelHandle);

/**
 * @brief      Check whether at least one NM message has been successfully sent.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @retval     TRUE: At least one NM message has been successfully sent.
 * @retval     FALSE: No NM message has been successfully sent.
 */
boolean Nm_IsNetworkModeConfirmed(NetworkHandleType nmChannelHandle);

#endif /* NM_MACRO_LAYER_ENABLED */

#ifdef __cplusplus
}
#endif

/*! @}*/

#endif /* NM_H */
/*********************************END OF FILE*********************************/
