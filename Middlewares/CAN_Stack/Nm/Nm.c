/******************************************************************************
 * @file   : Nm.c
 * @brief  :
 * @author : yangtl
 * @date   : 2025/05/19 21:36:04
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#include "Nm.h"

#if (NM_MACRO_LAYER_ENABLED != TRUE)

void Nm_Init(const Nm_ConfigType* ConfigPtr)
{
#if defined(NM_PROTOCOL_AUTOSAR)
    CanNm_Init(ConfigPtr);
#elif defined(NM_PROTOCOL_OSEK)
    OsekNm_Init(ConfigPtr);
#endif
}

void Nm_MainFunction(void)
{
#if defined(NM_PROTOCOL_AUTOSAR)
    CanNm_MainFunction();
#elif defined(NM_PROTOCOL_OSEK)
    OsekNm_MainFunction();
#endif
}

Std_ReturnType Nm_PassiveStartUp(NetworkHandleType NetworkHandle)
{
#if defined(NM_PROTOCOL_AUTOSAR)
    return CanNm_PassiveStartUp(NetworkHandle);
#elif defined(NM_PROTOCOL_OSEK)
#endif
}

Std_ReturnType Nm_NetworkRequest(NetworkHandleType NetworkHandle)
{
#if defined(NM_PROTOCOL_AUTOSAR)
    return CanNm_NetworkRequest(NetworkHandle);
#elif defined(NM_PROTOCOL_OSEK)
#endif
}

Std_ReturnType Nm_NetworkRelease(NetworkHandleType NetworkHandle)
{
#if defined(NM_PROTOCOL_AUTOSAR)
    return CanNm_NetworkRelease(NetworkHandle);
#elif defined(NM_PROTOCOL_OSEK)
#endif
}

Std_ReturnType Nm_DisableCommunication(NetworkHandleType NetworkHandle)
{
#if defined(NM_PROTOCOL_AUTOSAR)
#if (CANNM_COM_CONTROL_ENABLED == TRUE)
    return CanNm_DisableCommunication(NetworkHandle);
#else
    return E_NOT_OK;
#endif
#elif defined(NM_PROTOCOL_OSEK)
#endif
}

Std_ReturnType Nm_EnableCommunication(NetworkHandleType NetworkHandle)
{
#if defined(NM_PROTOCOL_AUTOSAR)
#if (CANNM_COM_CONTROL_ENABLED == TRUE)
    return CanNm_EnableCommunication(NetworkHandle);
#else
    return E_NOT_OK;
#endif
#elif defined(NM_PROTOCOL_OSEK)
#endif
}

void Nm_UpdateIRA(NetworkHandleType NetworkHandle, const uint8* PncBitVectorPtr)
{
#if defined(NM_PROTOCOL_AUTOSAR)
    // return CanNm_UpdateIRA(NetworkHandle, PncBitVectorPtr);
#elif defined(NM_PROTOCOL_OSEK)
#endif
}

Std_ReturnType Nm_SetUserData(NetworkHandleType NetworkHandle, const uint8* nmUserDataPtr)
{
#if defined(NM_PROTOCOL_AUTOSAR)
    return CanNm_SetUserData(NetworkHandle, nmUserDataPtr);
#elif defined(NM_PROTOCOL_OSEK)
#endif
}

Std_ReturnType Nm_GetUserData(NetworkHandleType NetworkHandle, uint8* nmUserDataPtr)
{
#if defined(NM_PROTOCOL_AUTOSAR)
    return CanNm_GetUserData(NetworkHandle, nmUserDataPtr);
#elif defined(NM_PROTOCOL_OSEK)
#endif
}

Std_ReturnType Nm_GetPduData(NetworkHandleType NetworkHandle, uint8* nmPduData)
{
#if defined(NM_PROTOCOL_AUTOSAR)
    return CanNm_GetPduData(NetworkHandle, nmPduData);
#elif defined(NM_PROTOCOL_OSEK)
#endif
}

Std_ReturnType Nm_RepeatMessageRequest(NetworkHandleType NetworkHandle)
{
#if defined(NM_PROTOCOL_AUTOSAR)
    return CanNm_RepeatMessageRequest(NetworkHandle);
#elif defined(NM_PROTOCOL_OSEK)
#endif
}

Std_ReturnType Nm_GetNodeIdentifier(NetworkHandleType NetworkHandle, uint8* nmNodeIdPtr)
{
#if defined(NM_PROTOCOL_AUTOSAR)
    return CanNm_GetNodeIdentifier(NetworkHandle, nmNodeIdPtr);
#elif defined(NM_PROTOCOL_OSEK)
#endif
}

Std_ReturnType Nm_GetLocalNodeIdentifier(NetworkHandleType NetworkHandle, uint8* nmNodeIdPtr)
{
#if defined(NM_PROTOCOL_AUTOSAR)
    return CanNm_GetLocalNodeIdentifier(NetworkHandle, nmNodeIdPtr);
#elif defined(NM_PROTOCOL_OSEK)
#endif
}

Std_ReturnType Nm_CheckRemoteSleepIndication(NetworkHandleType nmNetworkHandle, boolean* nmRemoteSleepIndPtr)
{
#if defined(NM_PROTOCOL_AUTOSAR)
#if CANNM_REMOTE_SLEEP_IND_ENABLED == TRUE
    return CanNm_CheckRemoteSleepIndication(nmNetworkHandle, nmRemoteSleepIndPtr);
#else
    return E_NOT_OK;
#endif
#elif defined(NM_PROTOCOL_OSEK)
#endif
}

Std_ReturnType Nm_GetState(NetworkHandleType nmNetworkHandle, Nm_StateType* nmStatePtr, Nm_ModeType* nmModePtr)
{
#if defined(NM_PROTOCOL_AUTOSAR)
    return CanNm_GetState(nmNetworkHandle, nmStatePtr, nmModePtr);
#elif defined(NM_PROTOCOL_OSEK)
#endif
}

void Nm_GetVersionInfo(Std_VersionInfoType* nmVerInfoPtr)
{
#if defined(NM_PROTOCOL_AUTOSAR)
#if (CANNM_VERSION_INFO_API == TRUE)
    return CanNm_GetVersionInfo(nmVerInfoPtr);
#else
    return E_NOT_OK;
#endif
#elif defined(NM_PROTOCOL_OSEK)
#endif
}

#endif /* NM_MACRO_LAYER_ENABLED != TRUE */

/*********************************END OF FILE*********************************/
