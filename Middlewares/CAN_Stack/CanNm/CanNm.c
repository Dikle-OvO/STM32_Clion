/******************************************************************************
 * @file   : CanNm.c
 * @brief  :
 * @author : yangtl
 * @date   : 2025/04/23 17:40:57
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#include <string.h>
#include "CanNm.h"
#include "CanNm_Priv.h"
#include "util_platform.h"

// 获取用户数据指针（偏移量）
// 使用了GNU C的语法糖，即语句表达式({})，允许在宏中包含多条语句，并返回最后一条语句的结果，获取用户数据指针
//#define GET_USER_DATA_PTR(pcfg, name, dataLen)  ({ \
//        uint8 offset = GetUserDataOffset(pcfg, &(dataLen)); \
//        &(pcfg)->CanNm##name->SduDataPtr[offset]; \     // 使用了宏连接符 `##` ，基地址+偏移量
//    })


static inline uint8 GetUserDataOffset(const CanNm_ChannelConfig *pcfg, PduLengthType *dataLen);

static inline uint8* GetUserDataPtr(const CanNm_ChannelConfig *pcfg, const char* name, PduLengthType *dataLen) {
    uint8 offset = GetUserDataOffset(pcfg, dataLen);
    // 处理CanNm##name的拼接（需根据实际类型调整，这里假设是TxPduRef/RxPduRef）
    if (strcmp(name, "TxPduRef") == 0) {
        return &(pcfg)->CanNmTxPduRef->SduDataPtr[offset];
    } else if (strcmp(name, "RxPduRef") == 0) {
        return &(pcfg)->CanNmRxPduRef->SduDataPtr[offset];
    }
    return NULL;  // 错误处理
}

typedef enum {
    CANNM_UNINIT,
    CANNM_INIT,
} CanNm_InitStatusType;

typedef struct {
    CanNm_ChannelContextType *pChannelContexts;
    uint8                     NumberOfChannels;
    CanNm_InitStatusType      InitStatus;
} CanNm_HandleType;

static CanNm_HandleType canNmHandle;

#if (CANNM_VERSION_INFO_API == TRUE)
void CanNm_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (versioninfo) {
        versioninfo->vendorID = CANNM_VENDOR_ID;
        versioninfo->moduleID = CANNM_MODULE_ID;
        versioninfo->sw_major_version = CANNM_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = CANNM_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = CANNM_SW_PATCH_VERSION;
    }
}
#endif

static inline uint8 GetUserDataOffset(const CanNm_ChannelConfig *pcfg, PduLengthType *dataLen)
{
    uint8 offset = 0;

    if (pcfg->CanNmPduNidPosition != CANNM_PDU_OFF) {
        offset += pcfg->CanNmPduNidPosition; // 跳过NID网络标识字段
    }

    if (pcfg->CanNmPduCbvPosition != CANNM_PDU_OFF) {
        offset += pcfg->CanNmPduCbvPosition; // 跳过CBV控制位字段
    }

    offset += 1; // 跳过帧中固定存在的 1 字节管理字段（如版本标识或状态位），具体含义由配置决定

#if (CANNM_GLOBAL_PN_SUPPORT == TRUE) // 部分联网
    if (pcfg->CanNmPnEnabled) {
        if (pcfg->NmPncVectorOffset == offset) {
            offset = pcfg->NmPncVectorOffset + pcfg->NmPncVectorLength;
            *dataLen = pcfg->CanNmTxPduRef->SduLength - offset;
        } else {
            *dataLen = pcfg->CanNmTxPduRef->SduLength - pcfg->NmPncVectorOffset - offset;
        }
    } else
#endif
    {
        *dataLen = pcfg->CanNmTxPduRef->SduLength - offset; // 最后datalen（用户数据最大长度）= NM帧总长度 - 总偏移量
    }

    return offset;
}

static inline void InitTxRxPDU(const CanNm_ChannelConfig *pcfg)
{
    PduLengthType dataLen = 0;
//    uint8 *dataPtr = GET_USER_DATA_PTR(pcfg, TxPduRef, dataLen);
		uint8 *dataPtr = GetUserDataPtr(pcfg, "TxPduRef", &dataLen);

    if ((pcfg->CanNmNodeIdEnabled) && (pcfg->CanNmPduNidPosition != CANNM_PDU_OFF)) {
        pcfg->CanNmTxPduRef->SduDataPtr[pcfg->CanNmPduNidPosition] = pcfg->CanNmNodeId;
    }

    if (pcfg->CanNmPduCbvPosition != CANNM_PDU_OFF) {
        pcfg->CanNmTxPduRef->SduDataPtr[pcfg->CanNmPduCbvPosition] = 0;
    }

#if (CANNM_GLOBAL_PN_SUPPORT == TRUE)
    if (pcfg->CanNmPnEnabled) {
        memset(&pcfg->CanNmTxPduRef->SduDataPtr[pcfg->NmPncVectorOffset], 0, pcfg->NmPncVectorLength);
    }
#endif

    memset(pcfg->CanNmRxPduRef->SduDataPtr, 0, pcfg->CanNmRxPduRef->SduLength);
    memset(dataPtr, 0xFFu, dataLen);
}

void CanNm_Init(const CanNm_ConfigType *cannmConfigPtr)
{
    if ((NULL_PTR != cannmConfigPtr) && (cannmConfigPtr->NumberOfChannels > 0) // 配置参数有效性检查，四个参数非空
        && (NULL_PTR != cannmConfigPtr->pChannelContexts) && (NULL_PTR != cannmConfigPtr->pChannelConfigs)) {
        if (canNmHandle.InitStatus != CANNM_INIT) {
            for (uint8 ch = 0; ch < cannmConfigPtr->NumberOfChannels; ++ch) {
                const CanNm_ChannelConfig *pcfg = &cannmConfigPtr->pChannelConfigs[ch]; // 获取通道配置
                CanNm_ChannelContextType *pctx = &cannmConfigPtr->pChannelContexts[ch]; // 获取通道上下文
                InitTxRxPDU(pcfg); // 初始化发送和接收PDU
                memset(pctx, 0, sizeof(CanNm_ChannelContextType)); // 清零通道上下文，默认配置就没用了
                pctx->userData = (void *)pcfg; // 设置用户数据指针
                pctx->TransmissionEnabled = 1; // 启用传输
                pctx->State = NM_STATE_BUS_SLEEP; // 设置初始状态为总线睡眠
            }
            // 初始化全局句柄
            memset(&canNmHandle, 0, sizeof(canNmHandle));
            canNmHandle.pChannelContexts = cannmConfigPtr->pChannelContexts;
            canNmHandle.NumberOfChannels = cannmConfigPtr->NumberOfChannels;
            canNmHandle.InitStatus = CANNM_INIT;
        }
    }
}

void CanNm_DeInit(void)
{
    if (canNmHandle.InitStatus != CANNM_UNINIT) {
        boolean deinit = TRUE;
        for (uint8 ch = 0; ch < canNmHandle.NumberOfChannels; ++ch) {
            CanNm_ChannelContextType *pctx = &canNmHandle.pChannelContexts[ch];
            if (pctx->State != NM_STATE_BUS_SLEEP) {
                deinit = FALSE;
                break;
            }
        }

        if (deinit) {
            memset(&canNmHandle, 0, sizeof(canNmHandle));
            canNmHandle.InitStatus = CANNM_UNINIT;
        }
    }
}

// 主函数
void CanNm_MainFunction(void)
{
    for (uint8 ch = 0; ch < canNmHandle.NumberOfChannels; ++ch) {
        Hsm_DispatchEvent(&canNmHandle.pChannelContexts[ch]);
    }
}

// 被动启动
Std_ReturnType CanNm_PassiveStartUp(NetworkHandleType nmChannelHandle)
{
    Std_ReturnType ret = E_NOT_OK;

    if (nmChannelHandle < canNmHandle.NumberOfChannels) {
        CanNm_ChannelContextType *pctx = &canNmHandle.pChannelContexts[nmChannelHandle];
        if ((NM_STATE_BUS_SLEEP == pctx->State) || (NM_STATE_PREPARE_BUS_SLEEP == pctx->State)) {
            U_SAFE_AREA0 {
                pctx->PassiveStartUp = 1;
            }
            ret = E_OK;
        }
    }

    return ret;
}

// 网络请求
#if (CANNM_PASSIVE_MODE_ENABLED == FALSE)
Std_ReturnType CanNm_NetworkRequest(NetworkHandleType nmChannelHandle)
{
    Std_ReturnType ret = E_NOT_OK;

    if (nmChannelHandle < canNmHandle.NumberOfChannels) {
        CanNm_ChannelContextType *pctx = &canNmHandle.pChannelContexts[nmChannelHandle];
        U_SAFE_AREA0 {
            pctx->Requested = 1;
            SET_EVT_FLAGS(pctx, EVT_NETWORK_CTRL);
        }
        ret = E_OK;
    }

    return ret;
}

// 网络释放
Std_ReturnType CanNm_NetworkRelease(NetworkHandleType nmChannelHandle)
{
    Std_ReturnType ret = E_NOT_OK;

    if (nmChannelHandle < canNmHandle.NumberOfChannels) {
        CanNm_ChannelContextType *pctx = &canNmHandle.pChannelContexts[nmChannelHandle];
        U_SAFE_AREA0 {
            pctx->Requested = 0;
            SET_EVT_FLAGS(pctx, EVT_NETWORK_CTRL);
        }
        ret = E_OK;
    }

    return ret;
}
#endif

// 使能/禁用通信
#if (CANNM_COM_CONTROL_ENABLED == TRUE)
static inline void SetTransmissionAbility(CanNm_ChannelContextType *pctx, uint8 enable)
{
    U_SAFE_AREA0 {
        pctx->TransmissionEnabled = enable;
        SET_EVT_FLAGS(pctx, EVT_COMMUNICATION_CTRL);
    }
}

Std_ReturnType CanNm_DisableCommunication(NetworkHandleType nmChannelHandle)
{
    Std_ReturnType ret = E_NOT_OK;

    if (nmChannelHandle < canNmHandle.NumberOfChannels) {
        CanNm_ChannelContextType *pctx = &canNmHandle.pChannelContexts[nmChannelHandle];
        if ((pctx->State != NM_STATE_BUS_SLEEP) && (pctx->State != NM_STATE_PREPARE_BUS_SLEEP)) {
            SetTransmissionAbility(pctx, 0);
            ret = E_OK;
        }
    }

    return ret;
}

Std_ReturnType CanNm_EnableCommunication(NetworkHandleType nmChannelHandle)
{
    Std_ReturnType ret = E_NOT_OK;

    if (nmChannelHandle < canNmHandle.NumberOfChannels) {
        CanNm_ChannelContextType *pctx = &canNmHandle.pChannelContexts[nmChannelHandle];
        if ((pctx->State != NM_STATE_BUS_SLEEP) && (pctx->State != NM_STATE_PREPARE_BUS_SLEEP)) {
            SetTransmissionAbility(pctx, 1);
            ret = E_OK;
        }
    }

    return ret;
}
#endif

#if (CANNM_USER_DATA_ENABLED == TRUE)

#if (CANNM_PASSIVE_MODE_ENABLED == FALSE) && (CANNM_COM_USER_DATA_SUPPORT == FALSE)
Std_ReturnType CanNm_SetUserData(NetworkHandleType nmChannelHandle, const uint8_t *nmUserDataPtr)
{
    Std_ReturnType ret = E_NOT_OK;

    if ((nmChannelHandle < canNmHandle.NumberOfChannels) && (NULL_PTR != nmUserDataPtr)) { // 检查通道句柄和用户数据指针有效性
        PduLengthType dataLen = 0;
        const CanNm_ChannelConfig *pcfg = (const CanNm_ChannelConfig *)canNmHandle.pChannelContexts[nmChannelHandle].userData; // 获取通道配置指针
//        uint8 *dataPtr = GET_USER_DATA_PTR(pcfg, TxPduRef, dataLen); // 获取发送PDU数据指针
				uint8 *dataPtr = GetUserDataPtr(pcfg, "TxPduRef", &dataLen);

        U_SAFE_AREA0 {
            memcpy(dataPtr, nmUserDataPtr, dataLen);
        }
        ret = E_OK;
    }

    return ret;
}
#endif

Std_ReturnType CanNm_GetUserData(NetworkHandleType nmChannelHandle, uint8_t *nmUserDataPtr)
{
    Std_ReturnType ret = E_NOT_OK;

    if ((nmChannelHandle < canNmHandle.NumberOfChannels) && (NULL_PTR != nmUserDataPtr)) {
        PduLengthType dataLen = 0;
        const CanNm_ChannelConfig *pcfg = (const CanNm_ChannelConfig *)canNmHandle.pChannelContexts[nmChannelHandle].userData;
//        uint8 *dataPtr = GET_USER_DATA_PTR(pcfg, RxPduRef, dataLen);
				uint8 *dataPtr = GetUserDataPtr(pcfg, "TxPduRef", &dataLen);

        U_SAFE_AREA0 {
            memcpy(nmUserDataPtr, dataPtr, dataLen);
        }
        ret = E_OK;
    }

    return ret;
}
#endif


// 重复消息请求
Std_ReturnType CanNm_RepeatMessageRequest(NetworkHandleType nmChannelHandle)
{
    Std_ReturnType ret = E_NOT_OK;

    if (nmChannelHandle < canNmHandle.NumberOfChannels) {
        CanNm_ChannelContextType *pctx = &canNmHandle.pChannelContexts[nmChannelHandle];
        if ((pctx->State == NM_STATE_NORMAL_OPERATION) || (pctx->State == NM_STATE_READY_SLEEP)) {
            U_SAFE_AREA0 {
                SET_EVT_FLAGS(pctx, EVT_REPEAT_MSG_REQUEST);
            }
            ret = E_OK;
        }
    }

    return ret;
}

#if 0 /* TODO */
Std_ReturnType CanNm_Transmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr)
{

}
#endif

// 获取节点ID
Std_ReturnType CanNm_GetNodeIdentifier(NetworkHandleType nmChannelHandle, uint8_t *nmNodeIdPtr)
{
    Std_ReturnType ret = E_NOT_OK;

    if ((nmChannelHandle < canNmHandle.NumberOfChannels) && (NULL_PTR != nmNodeIdPtr)) {
        const CanNm_ChannelConfig *pcfg = (const CanNm_ChannelConfig *)&canNmHandle.pChannelContexts[nmChannelHandle].userData;
        if ((pcfg->CanNmNodeIdEnabled) && (pcfg->CanNmPduNidPosition != CANNM_PDU_OFF)) {
            *nmNodeIdPtr = pcfg->CanNmRxPduRef->SduDataPtr[pcfg->CanNmPduNidPosition];
            ret = E_OK;
        }
    }

    return ret;
}

// 获取本地节点ID
Std_ReturnType CanNm_GetLocalNodeIdentifier(NetworkHandleType nmChannelHandle, uint8_t *nmNodeIdPtr)
{
    Std_ReturnType ret = E_NOT_OK;

    if ((nmChannelHandle < canNmHandle.NumberOfChannels) && (NULL_PTR != nmNodeIdPtr)) {
        const CanNm_ChannelConfig *pcfg = (const CanNm_ChannelConfig *)&canNmHandle.pChannelContexts[nmChannelHandle].userData;
        if (pcfg->CanNmNodeIdEnabled) {
            *nmNodeIdPtr = pcfg->CanNmNodeId;
            ret = E_OK;
        }
    }

    return ret;
}

// 获取PDU数据
Std_ReturnType CanNm_GetPduData(NetworkHandleType nmChannelHandle, uint8_t *nmPduDataPtr)
{
    Std_ReturnType ret = E_NOT_OK;

    if ((nmChannelHandle < canNmHandle.NumberOfChannels) && (NULL_PTR != nmPduDataPtr)) {
        const CanNm_ChannelConfig *pcfg = (const CanNm_ChannelConfig *)&canNmHandle.pChannelContexts[nmChannelHandle].userData;
        memcpy(nmPduDataPtr, pcfg->CanNmRxPduRef->SduDataPtr, pcfg->CanNmRxPduRef->SduLength);
        ret = E_OK;
    }

    return ret;
}

// 网络句柄号，状态，模式
Std_ReturnType CanNm_GetState(NetworkHandleType nmChannelHandle, Nm_StateType *nmStatePtr, Nm_ModeType *nmModePtr)
{
    Std_ReturnType ret = E_NOT_OK;

    if (nmChannelHandle < canNmHandle.NumberOfChannels) {
        CanNm_ChannelContextType *pctx = &canNmHandle.pChannelContexts[nmChannelHandle];

        if (nmStatePtr) {
            *nmStatePtr = pctx->State;
            ret = E_OK;
        }

        if (nmModePtr) {
            if (NM_STATE_BUS_SLEEP == pctx->State) {
                *nmModePtr = NM_MODE_BUS_SLEEP;
            } else if (NM_STATE_PREPARE_BUS_SLEEP == pctx->State) {
                *nmModePtr = NM_MODE_PREPARE_BUS_SLEEP;
            } else if (NM_STATE_REPEAT_MESSAGE >= pctx->State) {
                *nmModePtr = NM_MODE_NETWORK;
            } else {
                *nmModePtr = NM_MODE_SYNCHRONIZE;
            }

            ret = E_OK;
        }
    }

    return ret;
}

#if (CANNM_BUS_SYNCHRONIZATION_ENABLED == TRUE)
Std_ReturnType CanNm_RequestBusSynchronization(NetworkHandleType nmChannelHandle)
{
}
#endif

#if (CANNM_REMOTE_SLEEP_IND_ENABLED == TRUE)
Std_ReturnType CanNm_CheckRemoteSleepIndication(NetworkHandleType nmChannelHandle, boolean *nmRemoteSleepIndPtr)
{
}

Std_ReturnType CanNm_SetSleepReadyBit(NetworkHandleType nmChannelHandle, boolean nmSleepReadyBit)
{
}
#endif

/************** Callback notifications **************/

#if ((CANNM_PASSIVE_MODE_ENABLED == FALSE) && (CANNM_IMMEDIATE_TX_CONF_ENABLED == FALSE))
void CanNm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    NetworkHandleType nmChannelHandle = (NetworkHandleType)TxPduId;

    if (nmChannelHandle < canNmHandle.NumberOfChannels) {
        if (result == E_OK) {
            CanNm_ChannelContextType *pctx = &canNmHandle.pChannelContexts[nmChannelHandle];
            U_SAFE_AREA0 {
                SET_EVT_FLAGS(pctx, EVT_TX_CONFIRMATION);
            }
        }
    }
}
#endif

void CanNm_RxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr)
{
    NetworkHandleType nmChannelHandle = (NetworkHandleType)RxPduId; // 通过 PDU ID 定位 NM 通道

    if (nmChannelHandle < canNmHandle.NumberOfChannels) {
        CanNm_ChannelContextType  *pctx = &canNmHandle.pChannelContexts[nmChannelHandle]; // 获取通道上下文与配置
        const CanNm_ChannelConfig *pcfg = (const CanNm_ChannelConfig *)pctx->userData;

        if (pcfg->CanNmRxPduRef->SduLength == PduInfoPtr->SduLength) {
            Std_ReturnType ret = E_OK;
#if (CANNM_GLOBAL_PN_SUPPORT == TRUE)
            if (pcfg->CanNmPnEnabled && (!pcfg->CanNmAllNmMessagesKeepAwake)) {
                ret = CanNm_PnFilterAlgorithm(pcfg, PduInfoPtr);
            }
#endif

            if (ret == E_OK) {
							U_SAFE_AREA0 { // 临界区保护宏
								memcpy(pcfg->CanNmRxPduRef->SduDataPtr, PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength); // 将接收帧的原始数据拷贝到通道的接收缓冲区供后续解析
                    SET_EVT_FLAGS(pctx, EVT_RX_INDICATION);
                }
            }
        }
    }
}

#if 0 /* TODO */
void CanNm_ConfirmPnAvailability(NetworkHandleType nmChannelHandle)
{

}

Std_ReturnType CanNm_TriggerTransmit(PduIdType TxPduId, PduInfoType *PduInfoPtr)
{
    return E_NOT_OK;
}
#endif

/************** Non-Standard API **************/

boolean CanNm_IsNetworkModeRequested(NetworkHandleType nmChannelHandle)
{
    boolean requested = FALSE;

    if (nmChannelHandle < canNmHandle.NumberOfChannels) {
        CanNm_ChannelContextType *pctx = &canNmHandle.pChannelContexts[nmChannelHandle];
        requested = (pctx->Requested != 0);
    }

    return requested;
}

boolean CanNm_IsNetworkModeConfirmed(NetworkHandleType nmChannelHandle)
{
    boolean confirm = FALSE;

    if (nmChannelHandle < canNmHandle.NumberOfChannels) {
        CanNm_ChannelContextType *pctx = &canNmHandle.pChannelContexts[nmChannelHandle];
        confirm = (pctx->NetworkModeConfirmed != 0);
    }

    return confirm;
}

#if (CANNM_GLOBAL_PN_SUPPORT == TRUE)
/**
 * @brief      Indication by ComM of internal PNC requests. This is used to aggregate the internal PNC requests.
 * @param[in]  NetworkHandle Identification of the NM-channel
 * @param[in]  PncBitVectorPtr Pointer to the bit vector with all PNC bits set to "1" of internal requested PNCs (IRA)
 * @retval     None
 *
 * Sync/Async: Asynchronous
 */
Std_ReturnType CanNm_UpdateIRA(NetworkHandleType nmChannelHandle, const uint8* PncBitVectorPtr)
{
    Std_ReturnType ret = E_NOT_OK;

    if ((nmChannelHandle < canNmHandle.NumberOfChannels) && (NULL_PTR != PncBitVectorPtr)) {
        const CanNm_ChannelConfig *pcfg = (const CanNm_ChannelConfig *)canNmHandle.pChannelContexts[nmChannelHandle].userData;

        if ((pcfg->CanNmPnEnabled) && (pcfg->NmPncVectorOffset < pcfg->CanNmTxPduRef->SduLength)) {
            uint8 *dataPtr = &pcfg->CanNmTxPduRef->SduDataPtr[pcfg->NmPncVectorOffset];
            U_SAFE_AREA0 {
                memcpy(dataPtr, PncBitVectorPtr, pcfg->NmPncVectorLength);
            }
            ret = E_OK;
        }
    }

    return ret;
}
#endif

/*********************************END OF FILE*********************************/
