/******************************************************************************
 * @file   : Com.c
 * @brief  : Communication module implements the function of sending/receiving
 *           signals according to Specification of AUTOSAR CP.
 * @author : yangtl
 * @date   : 2022/03/17 14:31:01
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#include <string.h>
#include "Com.h"

/*---------------------------------
 * MACROS & CONSTANTS
 */
#define COM_TX_ENABLE(p_tx_rte, p_cfg, enable)                         \
    do {                                                               \
        utick_reset(&(p_tx_rte)->cycleTimeout, (p_cfg)->startOffsetP); \
        (p_tx_rte)->enabled = ((enable) ? 1 : 0);                      \
    } while (0)

#define COM_RX_MONITOR(p_rx_rte, p_cfg, enable)                     \
        do {                                                        \
            utick_reset(&(p_rx_rte)->timeout, (p_cfg)->timeWindow); \
            (p_rx_rte)->missing = 0;                                \
            (p_rx_rte)->present = 0;                                \
            (p_rx_rte)->monitor = ((enable) ? 1 : 0);               \
        } while (0)


#define COM_IPDU_GROUP_IS_START(groupId)            (comHandle.groupActiveStatus & (1u << (groupId)))
#define COM_IPDU_GROUP_RX_DM_IS_ENABLE(groupId)     (comHandle.groupRxDMStatus & (1u << (groupId)))
#define COM_IPDU_GROUP_IS_TXENABLE(groupId)         (comHandle.groupTxEnable & (1u << (groupId)))
#define COM_IPDU_GROUP_IS_RXENABLE(groupId)         (comHandle.groupRxEnable & (1u << (groupId)))

/*---------------------------------
 * TYPEDEFS
 */
typedef struct {
    const Com_ConfigType *configPtr;

    /* A bit mean a I-PDU group, Up to 32 groups */
    uint32_t groupActiveStatus; /* Starts a preconfigured I-PDU group, bit value: 0=deactived, 1=actived */
    uint32_t groupRxDMStatus;   /* reception deadline monitoring for the I-PDUs within the given I-PDU group. */
    uint32_t groupTxEnable;
    uint32_t groupRxEnable;
} ComHandleType;

/*---------------------------------
 * GLOBAL VARIABLES
 */

/*---------------------------------
 * LOCAL VARIABLES
 */
static ComHandleType comHandle;

/*---------------------------------
 * EXTERNAL VARIABLES
 */

/*---------------------------------
 * EXTERNAL FUNCTIONS DECLARATION
 */

/*---------------------------------
 * LOCAL FUNCTIONS DECLARATION
 */


/*---------------------------------
 * PUBLIC FUNCTIONS
 */
void Com_Init(const Com_ConfigType *config)
{
    memset(&comHandle, 0, sizeof(comHandle)); // 清零
    if (config) {
        comHandle.configPtr = config;
        for (uint8_t i = 0; i < config->numOfPduGroup; ++i) {
            Com_CommunicationControl(i, TRUE, TRUE);
        }
    }
}

void Com_DeInit(void)
{
    memset(&comHandle, 0, sizeof(comHandle));
}

 // 启动I-PDU组
void Com_IpduGroupStart(Com_IpduGroupIdType IpduGroupId, boolean initialize)
{
    const Com_ConfigType *configPtr = comHandle.configPtr; // 获取配置指针
    const ComIPduConfigType *pIPdu;
    uint32_t i;

    if (configPtr && IpduGroupId < configPtr->numOfPduGroup) {
        pIPdu = &configPtr->pPduGroup[IpduGroupId];
        if (pIPdu->pTxPdus) {
            for (i = 0; i < pIPdu->numOfTxPdus; ++i) {
                if (initialize) {
                    /** TODO: initialize Tx pduInfo; Now just Initialize to 0. */
                    memset(pIPdu->pTxPdus[i].pduInfo.SduDataPtr, 0, pIPdu->pTxPdus[i].pduInfo.SduLength);
                }

                COM_TX_ENABLE(&pIPdu->pTxRtes[i], &pIPdu->pTxPdus[i].msgInfo, TRUE);
            }
        }

        if (pIPdu->pRxPdus) {
            for (i = 0; i < pIPdu->numOfRxPdus; ++i) {
                if (initialize) {
                    /** TODO: initialize Tx pduInfo; Now just Initialize to 0. */
                    memset(pIPdu->pRxPdus[i].pduInfo.SduDataPtr, 0, pIPdu->pRxPdus[i].pduInfo.SduLength);
                }
            }
        }

        comHandle.groupActiveStatus |= (1u << IpduGroupId); //标记使能
    }
}

void Com_IpduGroupStop(Com_IpduGroupIdType IpduGroupId)
{
    const Com_ConfigType *configPtr = comHandle.configPtr;

    if (configPtr && IpduGroupId < configPtr->numOfPduGroup) {
        comHandle.groupActiveStatus &= ~(1u << IpduGroupId);
    }
}

// 使能超时监控
void Com_EnableReceptionDM(Com_IpduGroupIdType IpduGroupId)
{
    const Com_ConfigType *configPtr = comHandle.configPtr;
    const ComIPduConfigType *pIPdu;
    uint32_t i;

    if (configPtr && IpduGroupId < configPtr->numOfPduGroup) {
        pIPdu = &configPtr->pPduGroup[IpduGroupId];
        if (pIPdu->pRxPdus) {
            for (i = 0; i < pIPdu->numOfRxPdus; ++i) {
                COM_RX_MONITOR(&pIPdu->pRxRtes[i], &pIPdu->pRxPdus[i].msgInfo, TRUE);// 重置接收定时器、初始化接收状态、使能监控
            }
        }

        comHandle.groupRxDMStatus |= (1u << IpduGroupId);
    }
}

void Com_DisableReceptionDM(Com_IpduGroupIdType IpduGroupId)
{
    const Com_ConfigType *configPtr = comHandle.configPtr;
    const ComIPduConfigType *pIPdu;
    uint32_t i;

    if (configPtr && IpduGroupId < configPtr->numOfPduGroup) {
        pIPdu = &configPtr->pPduGroup[IpduGroupId];
        if (pIPdu->pRxPdus) {
            for (i = 0; i < pIPdu->numOfRxPdus; ++i) {
                COM_RX_MONITOR(&pIPdu->pRxRtes[i], &pIPdu->pRxPdus[i].msgInfo, FALSE);
            }
        }

        comHandle.groupRxDMStatus &= ~(1u << IpduGroupId);
    }
}

Com_StatusType Com_GetStatus(void)
{
    return comHandle.configPtr ? COM_INIT : COM_UNINIT;
}

void Com_MainFunctionRx(void)
{
    const Com_ConfigType *configPtr = comHandle.configPtr;
    const ComIPduConfigType *pIPdu;
    ComRxRteType *pRte;
    uint32_t i, j;

    for (i = 0; i < configPtr->numOfPduGroup; ++i) {
        if (!COM_IPDU_GROUP_IS_RXENABLE(i) || !COM_IPDU_GROUP_IS_START(i) || !COM_IPDU_GROUP_RX_DM_IS_ENABLE(i)) {
            continue;
        }

        pIPdu = &configPtr->pPduGroup[i];
        for (j = 0; j < pIPdu->numOfRxPdus; ++j) {
            pRte = &pIPdu->pRxRtes[j];
            if (utick_is_expired(&pRte->timeout)) { /* recv timeout 该IPDU超过设定时间未接收*/
                if (!pRte->missing) {
                    pRte->missing = 1;
                    pRte->present = 0;
                    if (pIPdu->cbkRxTout) {
                        pIPdu->cbkRxTout(j, &pIPdu->pRxPdus[j].pduInfo); // 若配置了超时回调，则调用该函数上报超时事件
                    }
                }
            }
        }
    }
}

void Com_MainFunctionTx(void)
{
    const Com_ConfigType *configPtr = comHandle.configPtr;
    const ComIPduConfigType *pIPdu;
    const ComTxMsgInfoType *pMsgInfo;
    ComTxRteType *pRte;
    uint32_t i, j;
    boolean isSend;

    for (i = 0; i < configPtr->numOfPduGroup; ++i) {
        if (!COM_IPDU_GROUP_IS_TXENABLE(i) || !COM_IPDU_GROUP_IS_START(i)) { // 过滤无效IPDU组
            continue;
        }

        pIPdu = &configPtr->pPduGroup[i];
        for (j = 0; j < pIPdu->numOfTxPdus; ++j) {
            isSend = FALSE;
            pRte = &pIPdu->pTxRtes[j];
            pMsgInfo = &pIPdu->pTxPdus[j].msgInfo;

            /* tramsit timeout handle 处理超时发送*/
            if (COM_TX_STA_GOIING == pRte->txStatus) {
                if (utick_is_expired(&pRte->txTimeout)) {
                    pRte->txStatus = COM_TX_STA_FAILED;
                    if (pIPdu->cbkTxErr) {
                        pIPdu->cbkTxErr(j);
											util_printf("CAN Msg Time out：%d\r\n", pRte->txTimeout);
                    }
                }
            }

            switch (pMsgInfo->msgType) { // 按消息类型判定是否需要发送
            case COM_MSG_PERIODIC: // 周期性发送
                /* Whether to send a msg on time? */
                if (utick_is_expired(&pRte->cycleTimeout)) {
                    utick_reset(&pRte->cycleTimeout, pMsgInfo->cycleTime); /* reset timer */
                    isSend = TRUE;
                }
                break;

            case COM_MSG_ON_EVENT: // 事件触发发送
                if (pRte->repetitionCnt > 0) {
                    if (utick_is_expired(&pRte->repeatTimeout) && utick_is_expired(&pRte->minDelayTimeout)) {
                        pRte->repetitionCnt -= 1;
                        utick_reset(&pRte->repeatTimeout, pMsgInfo->repeatTime);
                        utick_reset(&pRte->minDelayTimeout, pMsgInfo->minDelayTime);
                        isSend = TRUE;
                    }
                }
                break;

            case COM_MSG_IF_ACTIVE: // 激活/非激活发送
                if (pRte->active) { /** Active value: fast transmit periodically */
                    if (utick_is_expired(&pRte->cycleTimeout) && utick_is_expired(&pRte->minDelayTimeout)) {
                        utick_reset(&pRte->cycleTimeout, pMsgInfo->fastCycleTime);
                        utick_reset(&pRte->minDelayTimeout, pMsgInfo->minDelayTime);
                        isSend = TRUE;
                    }
                } else {            /** Inactive value: transmit N times */
                    if (pRte->repetitionCnt > 0) {
                        if (utick_is_expired(&pRte->repeatTimeout) && utick_is_expired(&pRte->minDelayTimeout)) {
                            pRte->repetitionCnt -= 1;
                            utick_reset(&pRte->repeatTimeout, pMsgInfo->repeatTime);
                            utick_reset(&pRte->minDelayTimeout, pMsgInfo->minDelayTime);
                            isSend = TRUE;
                        }
                    }
                }
                break;

            case COM_MSG_POE: // 周期性+事件触发混合发送
                /** @note If a periodic and an event triggered message condition overlaps,
                 *  the message shall be sent only by NRepetitionPOE times.
                 */

                if (0 == pRte->repetitionCnt) { /** no event: transmit periodically */
                    /** @note This sent out is delayed until the minimum delay time is elapsed,
                     *  However, after that the next period of the periodic part is shortened.
                     */
                    if (utick_is_expired(&pRte->cycleTimeout)) {
                        utick_reset(&pRte->cycleTimeout, pMsgInfo->cycleTime);
                        if (utick_is_expired(&pRte->minDelayTimeout)) {
                            isSend = TRUE;
                        } else {
                            pRte->delayed = 1;
                        }
                    } else {
                        if ((pRte->delayed) && utick_is_expired(&pRte->minDelayTimeout)) {
                            pRte->delayed = 0;
                            isSend = TRUE;
                        }
                    }
                } else {    /** an event occurs: transmit N times */
                    if (utick_is_expired(&pRte->repeatTimeout) && utick_is_expired(&pRte->minDelayTimeout)) {
                        pRte->repetitionCnt -= 1;
                        utick_reset(&pRte->repeatTimeout, pMsgInfo->repeatTime);
                        utick_reset(&pRte->minDelayTimeout, pMsgInfo->minDelayTime);
                        isSend = TRUE;
                    }
                }
                break;

            case COM_MSG_PA: // 激活+周期混合发送
                if (pRte->active) { /** Active value: fast transmit periodically */
                    if (utick_is_expired(&pRte->cycleTimeout) && utick_is_expired(&pRte->minDelayTimeout)) {
                        utick_reset(&pRte->cycleTimeout, pMsgInfo->fastCycleTime);
                        utick_reset(&pRte->minDelayTimeout, pMsgInfo->minDelayTime);
                        isSend = TRUE;
                    }
                } else {            /** Inactive value: transmit N times */
                    if (pRte->repetitionCnt <= 0) { /** Has been transmitted N times, so transmit periodically thereafter. */
                        if (utick_is_expired(&pRte->cycleTimeout) && utick_is_expired(&pRte->minDelayTimeout)) {
                            utick_reset(&pRte->cycleTimeout, pMsgInfo->cycleTime);
                            utick_reset(&pRte->minDelayTimeout, pMsgInfo->minDelayTime);
                            isSend = TRUE;
                        }
                    } else {
                        if (utick_is_expired(&pRte->repeatTimeout) && utick_is_expired(&pRte->minDelayTimeout)) {
                            if (pRte->repetitionCnt == pMsgInfo->repetition) { /** Is is the first message transmitted? */
                                utick_reset(&pRte->cycleTimeout, pMsgInfo->cycleTime);
                            }

                            pRte->repetitionCnt -= 1;
                            utick_reset(&pRte->repeatTimeout, pMsgInfo->repeatTime);
                            utick_reset(&pRte->minDelayTimeout, pMsgInfo->minDelayTime);
                            isSend = TRUE;
                        }
                    }
                }
                break;

            default: break;
            }

            if (isSend) { // 为真即执行实际发送逻辑
                Std_ReturnType result;
                if (pIPdu->inverseOrder) { //逆序发送标志位：Intel: false; Motorola: true，分别对应小端和大端，在Lcfg中PiPdu配置
                    PduLengthType n;
                    PduInfoType pduInfo;
                    uint8 buffer[COM_PDU_DATA_MAX_SIZE] = {0};

                    pduInfo.MetaDataPtr = pIPdu->pTxPdus[j].pduInfo.MetaDataPtr;
                    pduInfo.SduLength = pIPdu->pTxPdus[j].pduInfo.SduLength;
                    pduInfo.SduDataPtr = buffer;
                    for (n = 0; n < pduInfo.SduLength; ++n) {
                        buffer[pduInfo.SduLength - 1 - n] = pIPdu->pTxPdus[j].pduInfo.SduDataPtr[n];
                    }

                    result = PduR_ComTransmit(j, &pduInfo);
                } else {
                    result = PduR_ComTransmit(j, &pIPdu->pTxPdus[j].pduInfo);
                }

                if (E_OK == result) {
									utick_reset(&pIPdu->pTxRtes[j].txTimeout, 5); // 在这里决定了报文的超时时间，由原来的1改为5ms
//									util_printf("%d\r\n",pIPdu->pTxRtes[j].txTimeout);
                    pIPdu->pTxRtes[j].txStatus = COM_TX_STA_GOIING;
										
									/* 受限于轮询，其实在上述调用的时候就直接成功了，上面又改成了going，打个补丁 */
										pIPdu->pTxRtes[j].txStatus = COM_TX_STA_SUCCESS; 
									
                } else {
                    pIPdu->pTxRtes[j].txStatus = COM_TX_STA_FAILED;
                    if (pIPdu->cbkTxErr) {
                        pIPdu->cbkTxErr(j);
                    }
                }
            }
        }
    }
}


static inline Std_ReturnType ComStartSendIPDU(const ComIPduConfigType *pIPdu, PduIdType PduId, boolean active)
{
    Std_ReturnType ret = E_OK;
    ComTxRteType *pRte = &pIPdu->pTxRtes[PduId];
    const ComTxMsgInfoType *pMsgInfo = &pIPdu->pTxPdus[PduId].msgInfo;

    switch (pMsgInfo->msgType) {
    case COM_MSG_ON_EVENT:
    case COM_MSG_POE:
        /** The value is set to 0 means the transmission of the first msg shall be triggered as soon as possible */
        utick_reset(&pRte->repeatTimeout, 0);
        pRte->repetitionCnt = pMsgInfo->repetition;
        break;

    case COM_MSG_IF_ACTIVE:
    case COM_MSG_PA:
        if (active) {
            pRte->active = 1;      /** Active value: fast transmit periodically */
            utick_reset(&pRte->cycleTimeout, 0);
        } else {
            pRte->active = 0;      /** Inactive value: transmit N times */
            pRte->repetitionCnt = pMsgInfo->repetition;
            utick_reset(&pRte->repeatTimeout, 0);
        }
        break;

    default:    /** Other types of msg do not support triggering sending */
        ret = E_NOT_OK;
        break;
    }

    return ret;
}

/* TODO: Std_ReturnType Com_TriggerIPDUSend(PduIdType PduId) */
Std_ReturnType Com_TriggerIPDUSend(PduIdType PduId, boolean active)
{
    Std_ReturnType ret = E_NOT_OK;
    const Com_ConfigType *configPtr = comHandle.configPtr;
    const ComIPduConfigType *pIPdu;

    Com_IpduGroupIdType IpduGroupId = COM_GET_GROUPID(PduId);
    PduId = COM_GET_PDUID(PduId);

    if (IpduGroupId < configPtr->numOfPduGroup) {
        if (COM_IPDU_GROUP_IS_START(IpduGroupId)) {
            pIPdu = &configPtr->pPduGroup[IpduGroupId];
            if (PduId < pIPdu->numOfTxPdus) {
                ret = ComStartSendIPDU(pIPdu, PduId, active);
            }
        }
    }

    return ret;
}

// 发送信号
uint8 Com_SendSignal(Com_SignalIdType SignalId, const void *SignalDataPtr)
{
    const Com_ConfigType *configPtr = comHandle.configPtr;
    const ComIPduConfigType *pIPdu;

    // 提取ID
    Com_IpduGroupIdType IpduGroupId = COM_GET_GROUPID(SignalId);
    SignalId = COM_GET_PDUID(SignalId);

    if (IpduGroupId < configPtr->numOfPduGroup) {
#if (COM_OPT_CHECK_AVAILABLE == TRUE)
        if (!COM_IPDU_GROUP_IS_START(IpduGroupId)) {
            return COM_SERVICE_NOT_AVAILABLE;
        }
#endif
        // 找到信号配置，调用“信号写入函数”（从 DBC 生成的 SetSignal_XXX）
        pIPdu = &configPtr->pPduGroup[IpduGroupId];
        if (SignalId < pIPdu->numOfTxSignal) {
            pIPdu->pTxSignal[SignalId].pfnSetSignal(SignalDataPtr);// 调用函数
            ComStartSendIPDU(pIPdu, pIPdu->pTxSignal[SignalId].PduId, FALSE);
            return E_OK;
        }
    }

    return E_NOT_OK;
}

// 接收信号,与发送信号类似
uint8 Com_ReceiveSignal(Com_SignalIdType SignalId, void *SignalDataPtr)
{
    const Com_ConfigType *configPtr = comHandle.configPtr;
    const ComIPduConfigType *pIPdu;

    Com_IpduGroupIdType IpduGroupId = COM_GET_GROUPID(SignalId);
    SignalId = COM_GET_PDUID(SignalId);

    if (IpduGroupId < configPtr->numOfPduGroup) {
#if (COM_OPT_CHECK_AVAILABLE == TRUE)
        if (!COM_IPDU_GROUP_IS_START(IpduGroupId)) {
            return COM_SERVICE_NOT_AVAILABLE;
        }
#endif

        pIPdu = &configPtr->pPduGroup[IpduGroupId];
        if (SignalId < pIPdu->numOfRxSignal) {
            pIPdu->pRxSignal[SignalId].pfnGetSignal(SignalDataPtr);// 同样是调用函数
            return E_OK;
        }
    }

    return E_NOT_OK;
}


/* Callback Functions and Notifications */
void Com_RxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr)
{
    const ComPduMetaDataType *metaDataPtr = (ComPduMetaDataType *)PduInfoPtr->MetaDataPtr;
    const ComPduMetaDataType *pMeta;

    const Com_ConfigType *configPtr = comHandle.configPtr;
    const ComIPduConfigType *pIPdu;
    ComRxRteType *pRte;
    uint32_t j, len;

    Com_IpduGroupIdType groupId = metaDataPtr->channel;

    if ((groupId < configPtr->numOfPduGroup) && COM_IPDU_GROUP_IS_START(groupId) && COM_IPDU_GROUP_IS_RXENABLE(groupId)) {
        pIPdu = &configPtr->pPduGroup[groupId];
        for (j = 0; j < pIPdu->numOfRxPdus; ++j) {
            pRte = &pIPdu->pRxRtes[j];
            pMeta = (ComPduMetaDataType *)pIPdu->pRxPdus[j].pduInfo.MetaDataPtr;

            if (pMeta->canId == metaDataPtr->canId) {
                {   //if (COM_IPDU_GROUP_RX_DM_IS_ENABLE(groupId)) {
                    utick_reset(&pRte->timeout, pIPdu->pRxPdus[j].msgInfo.timeWindow);
                    if (!pRte->present) {
                        pRte->present = 1;
                    }

                    if (pRte->missing) {
                        pRte->missing = 0;
                    }
                }

                /* copy data into IPDU */
                if (PduInfoPtr->SduLength > pIPdu->pRxPdus[j].pduInfo.SduLength) {
                    len = pIPdu->pRxPdus[j].pduInfo.SduLength;
                } else {
                    len = PduInfoPtr->SduLength;
                }
                if (pIPdu->inverseOrder) {
                    for (uint32_t n = 0; n < len; ++n) {
                        pIPdu->pRxPdus[j].pduInfo.SduDataPtr[len - 1 - n] = PduInfoPtr->SduDataPtr[n];
                    }
                } else {
                    memcpy(pIPdu->pRxPdus[j].pduInfo.SduDataPtr, PduInfoPtr->SduDataPtr, len);
                }

                if (pIPdu->cbkRxAck) {
                    pIPdu->cbkRxAck(j, &pIPdu->pRxPdus[j].pduInfo);
                }
            }
        }
    }

    (void)RxPduId;
}

Std_ReturnType Com_TriggerTransmit(PduIdType TxPduId, PduInfoType *PduInfoPtr)
{
    /* TODO */
    return COM_SERVICE_NOT_AVAILABLE;
}

// 发送的回调函数，应该在物理层发送完成后被调用
void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    const Com_ConfigType *configPtr = comHandle.configPtr;
    const ComIPduConfigType *pIPdu;

    Com_IpduGroupIdType IpduGroupId = COM_GET_GROUPID(TxPduId);
    TxPduId = COM_GET_PDUID(TxPduId);

    if (IpduGroupId < configPtr->numOfPduGroup) {
        if (COM_IPDU_GROUP_IS_START(IpduGroupId)) {
            pIPdu = &configPtr->pPduGroup[IpduGroupId];
            if (TxPduId < pIPdu->numOfTxPdus) {
                if (E_OK == result) {
                    pIPdu->pTxRtes[TxPduId].txStatus = COM_TX_STA_SUCCESS;
                    if (pIPdu->cbkTxAck) {
                        pIPdu->cbkTxAck(TxPduId);
//												util_printf("%d is ok\r\n",TxPduId);
                    }
                } else {
                    pIPdu->pTxRtes[TxPduId].txStatus = COM_TX_STA_FAILED;
                    if (pIPdu->cbkTxErr) {
                        pIPdu->cbkTxErr(TxPduId);
//												util_printf("%d is err\r\n",TxPduId);
                    }
                }
            }
        }
    }
}


/* Other(Non-standard) */
void Com_CommunicationControl(Com_IpduGroupIdType IpduGroupId, boolean TxEnable, boolean RxEnable)
{
    const Com_ConfigType *configPtr = comHandle.configPtr;

    if (configPtr && IpduGroupId < configPtr->numOfPduGroup) {
        uint32_t gidMask = 1u << IpduGroupId;

        if (TxEnable) {
            comHandle.groupTxEnable |= gidMask;
        } else {
            comHandle.groupTxEnable &= ~gidMask;
        }

        if (RxEnable) {
            comHandle.groupRxEnable |= gidMask;
        } else {
            comHandle.groupRxEnable &= ~gidMask;
        }
    }
}

boolean Com_IsSendEnable(Com_IpduGroupIdType IpduGroupId)
{
    const Com_ConfigType *configPtr = comHandle.configPtr;
    boolean enable = FALSE;

    if (configPtr && IpduGroupId < configPtr->numOfPduGroup) {
        if (comHandle.groupTxEnable & (1u << IpduGroupId)) {
            enable = TRUE;
        }
    }

    return enable;
}

boolean Com_IsRecvEnable(Com_IpduGroupIdType IpduGroupId)
{
    const Com_ConfigType *configPtr = comHandle.configPtr;
    boolean enable = FALSE;

    if (configPtr && IpduGroupId < configPtr->numOfPduGroup) {
        if (comHandle.groupRxEnable & (1u << IpduGroupId)) {
            enable = TRUE;
        }
    }

    return enable;
}

const PduInfoType* Com_GetRxPduInfo(PduIdType RxPduId)
{
    const Com_ConfigType *configPtr = comHandle.configPtr;
    const ComIPduConfigType *pIPdu;
    const PduInfoType *pPduInfo = NULL;

    Com_IpduGroupIdType IpduGroupId = COM_GET_GROUPID(RxPduId);
    RxPduId = COM_GET_PDUID(RxPduId);

    if (IpduGroupId < configPtr->numOfPduGroup) {
        pIPdu = &configPtr->pPduGroup[IpduGroupId];
        if (RxPduId < pIPdu->numOfRxPdus) {
            pPduInfo = &pIPdu->pRxPdus[RxPduId].pduInfo;
        }
    }

    return pPduInfo;
}

const PduInfoType* Com_GetTxPduInfo(PduIdType TxPduId)
{
    const Com_ConfigType *configPtr = comHandle.configPtr;
    const ComIPduConfigType *pIPdu;
    const PduInfoType *pPduInfo = NULL;

    Com_IpduGroupIdType IpduGroupId = COM_GET_GROUPID(TxPduId);
    TxPduId = COM_GET_PDUID(TxPduId);

    if (IpduGroupId < configPtr->numOfPduGroup) {
        pIPdu = &configPtr->pPduGroup[IpduGroupId];
        if (TxPduId < pIPdu->numOfTxPdus) {
            pPduInfo = &pIPdu->pTxPdus[TxPduId].pduInfo;
        }
    }

    return pPduInfo;
}

ComRxStatusType Com_RecvStatus(PduIdType RxPduId)
{
    const Com_ConfigType *configPtr = comHandle.configPtr;
    const ComIPduConfigType *pIPdu;
    ComRxRteType *pRte = NULL;
    ComRxStatusType status = COM_RX_STA_INIT;

    Com_IpduGroupIdType IpduGroupId = COM_GET_GROUPID(RxPduId);
    RxPduId = COM_GET_PDUID(RxPduId);

    if (IpduGroupId < configPtr->numOfPduGroup) {
        pIPdu = &configPtr->pPduGroup[IpduGroupId];
        if (RxPduId < pIPdu->numOfRxPdus) {
            pRte = &pIPdu->pRxRtes[RxPduId];
        }
    }

    if (pRte) {
        if (pRte->monitor) {
            if (pRte->present) {
                status = COM_RX_STA_PRESENT;
            } else if (pRte->missing) {
                status = COM_RX_STA_MISSING;
            } else {
                status = COM_RX_STA_UNKNOWN;
            }
        } else {
            status = COM_RX_STA_INIT;
        }
    }

    return status;
}

/*---------------------------------
 * LOCAL FUNCTIONS
 */

/*********************************END OF FILE*********************************/
