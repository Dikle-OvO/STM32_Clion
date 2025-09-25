/******************************************************************************
 * @file   : CanNm_Priv.c
 * @brief  :
 * @author : yangtl
 * @date   : 2025/05/12 11:46:53
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#include "CanNm_Priv.h"
#include "util_tick.h"
#include "util_macro.h"
#include "util_platform.h"

#define HSM_DEEP_MAX    2

/** Control Bit Vector */
#define CBV_BIT_REPEAT_MESSAGE_REQUEST                  0U
#define CBV_BIT_PN_SHUTDOWN_REQUEST                     1U
#define CBV_BIT_NM_COORDINATOR_SLEEP_READY              3U
#define CBV_BIT_ACTIVE_WAKEUP                           4U
#define CBV_BIT_PARTIAL_NETWORK_LEARNING                5U
#define CBV_BIT_PARTIAL_NETWORK_INFORMATION             6U

static void BusSleep_onTransition(CanNm_ChannelContextType *pctx, StateActionType action);
static const HsmState *BusSleep_HandleEvent(CanNm_ChannelContextType *pctx);
static void PrepareBusSleep_onTransition(CanNm_ChannelContextType *pctx, StateActionType action);
static const HsmState *PrepareBusSleep_HandleEvent(CanNm_ChannelContextType *pctx);
static void NetworkMode_onTransition(CanNm_ChannelContextType *pctx, StateActionType action);
static const HsmState *NetworkMode_HandleEvent(CanNm_ChannelContextType *pctx);
static void RepeatMessage_onTransition(CanNm_ChannelContextType *pctx, StateActionType action);
static const HsmState *RepeatMessage_HandleEvent(CanNm_ChannelContextType *pctx);
static void NormalOperation_onTransition(CanNm_ChannelContextType *pctx, StateActionType action);
static const HsmState *NormalOperation_HandleEvent(CanNm_ChannelContextType *pctx);
static void ReadySleep_onTransition(CanNm_ChannelContextType *pctx, StateActionType action);
static const HsmState *ReadySleep_HandleEvent(CanNm_ChannelContextType *pctx);

static const HsmState canNmFsmStates[] = { // 状态机状态数组
    [0] = { /* NetworkMode */
        .onTransition = NetworkMode_onTransition, // 状态切换的回调函数
        .handleEvent = NetworkMode_HandleEvent, // 状态下的事件处理函数
        .parent = NULL_PTR,  // 父状态
    },
    [NM_STATE_BUS_SLEEP] = {
        .onTransition = BusSleep_onTransition,
        .handleEvent = BusSleep_HandleEvent,
        .parent = NULL_PTR,
    },
    [NM_STATE_PREPARE_BUS_SLEEP] = {
        .onTransition = PrepareBusSleep_onTransition,
        .handleEvent = PrepareBusSleep_HandleEvent,
        .parent = NULL_PTR,
    },
    [NM_STATE_READY_SLEEP] = {
        .onTransition = ReadySleep_onTransition,
        .handleEvent = ReadySleep_HandleEvent,
        .parent = &canNmFsmStates[0],
    },
    [NM_STATE_NORMAL_OPERATION] = {
        .onTransition = NormalOperation_onTransition,
        .handleEvent = NormalOperation_HandleEvent,
        .parent = &canNmFsmStates[0],
    },
    [NM_STATE_REPEAT_MESSAGE] = {
        .onTransition = RepeatMessage_onTransition,
        .handleEvent = RepeatMessage_HandleEvent,
        .parent = &canNmFsmStates[0],
    },
};


static inline const HsmState* GetCanNmFsmState(Nm_StateType nmstate)
{
    return &canNmFsmStates[nmstate];
}

static inline const HsmState* Hsm_GetFsmState(CanNm_ChannelContextType *pctx)
{
    const HsmState *s = NULL_PTR;
    if (pctx->State < U_COUNTOF(canNmFsmStates)) { // 确保状态在有效范围内
        s = GetCanNmFsmState(pctx->State);
    }

    return s;
}

static inline void Hsm_EnterState(CanNm_ChannelContextType *pctx, const HsmState *s)
{
    if (s->onTransition) {
        s->onTransition(pctx, STA_ACTION_ENTER);
    }
}

static inline void Hsm_ExitState(CanNm_ChannelContextType *pctx, const HsmState *s)
{
    if (s->onTransition) {
        s->onTransition(pctx, STA_ACTION_EXIT);
    }
}

static const HsmState* Hsm_FindLowestCommonAncestor(const HsmState *a, const HsmState *b)
{
    const HsmState *path[HSM_DEEP_MAX] = {NULL_PTR};
    int depth = 0;
    while ((a != NULL_PTR) && (depth < HSM_DEEP_MAX)) {
        path[depth++] = a;
        a = a->parent;
    }

    while (b != NULL_PTR) {
        for (int i = 0; i < depth; ++i) {
            if (b == path[i]) {
                return b;
            }
        }
        b = b->parent;
    }
    return NULL_PTR;
}

static void Hsm_TransitionTo(CanNm_ChannelContextType *pctx, const HsmState *target)
{
    if (pctx->State < U_COUNTOF(canNmFsmStates)) {
        const HsmState *current = GetCanNmFsmState(pctx->State);
        const HsmState *lca = Hsm_FindLowestCommonAncestor(current, target); // 查找当前状态和目标状态的最低公共祖先

        /* exit path: current -> lca (exclude lca) 直到退出当前状态到父状态*/
        while (current != lca) {
            Hsm_ExitState(pctx, current);
            current = current->parent;
        }

        /* enter path: target.parent -> target 进入目标状态的父状态*/
        if ((NULL_PTR == lca) && target->parent) {
            Hsm_EnterState(pctx, target->parent);   
        }

        Hsm_EnterState(pctx, target); // 进入目标状态
    }
}
//HSM（Hierarchical State Machine，层次状态机）
void Hsm_DispatchEvent(CanNm_ChannelContextType *pctx)
{
    const HsmState *current = Hsm_GetFsmState(pctx); //获取当前通道的状态机状态
    while (current != NULL_PTR) {
        if (current->handleEvent) {
            const HsmState *target = current->handleEvent(pctx); // 执行当前状态的回调
            if (target) {
                Hsm_TransitionTo(pctx, target); //如果处理事件返回了目标状态，则进行状态转换
                break;
            }
        }

        current = current->parent;
    }
}

// 设定NM帧的CBV位
static inline void SetTxPduCbvBit(const CanNm_ChannelConfig *pcfg, const uint8 bit)
{
    if (pcfg->CanNmPduCbvPosition != CANNM_PDU_OFF) {
        pcfg->CanNmTxPduRef->SduDataPtr[pcfg->CanNmPduCbvPosition] |= (1u << bit);
    }
}

static inline void ClearTxPduCbvBit(const CanNm_ChannelConfig *pcfg, const uint8 bit)
{
    if (pcfg->CanNmPduCbvPosition != CANNM_PDU_OFF) {
        pcfg->CanNmTxPduRef->SduDataPtr[pcfg->CanNmPduCbvPosition] &= ~(1u << bit);
    }
}

static inline uint8 GetPduCbvBit(const CanNm_ChannelConfig *pcfg, const PduInfoType *pdu, const uint8 bit)
{
    uint8 cbv = 0;
    if (pcfg->CanNmPduCbvPosition != CANNM_PDU_OFF) {
        cbv = ((pdu->SduDataPtr[pcfg->CanNmPduCbvPosition] >> bit) & 0x01u);
    }

    return cbv;
}

#if (CANNM_PASSIVE_MODE_ENABLED == FALSE)

static inline void SentPostProcessing(CanNm_ChannelContextType *pctx, const CanNm_ChannelConfig *pcfg, Std_ReturnType result)
{
    if (pctx->ImmediateNmTransmissions > 0) {
        pctx->ImmediateNmTransmissions--;
        if (E_OK == result) {
            uticker_start(&pctx->MsgCycleTmr, (pctx->ImmediateNmTransmissions > 0) ?
                                                pcfg->CanNmImmediateNmCycleTime : pcfg->CanNmMsgCycleTime);
        } else {
            /**
             * @note [SWS_CanNm_00335] If a transmission request to CanIf fails(E_NOT_OK is returned),
             *       CanNm shall retry the transmission request in the next main function.
             *       Afterwards CanNm shall continue transmitting NM PDU susing the CanNmMsgCycleTime.
             */
            uticker_start(&pctx->MsgCycleTmr, 1);
        }
    } else {
        uticker_start(&pctx->MsgCycleTmr, pcfg->CanNmMsgCycleTime);
    }
}

static void CanNm_SendMessage(CanNm_ChannelContextType *pctx)
{
    if (pctx->TransmissionEnabled) {
        const CanNm_ChannelConfig *pcfg = (const CanNm_ChannelConfig *)pctx->userData;
        Std_ReturnType result = E_OK;

#if (CANNM_IMMEDIATE_TX_CONF_ENABLED == TRUE)
        /*! Note: If CanNmImmediateTxconfEnabled is enabled it is assumed that each Net-
         *  work Management PDU transmission request results in a successful Network Management PDU transmission.
         */
        CanNmIf_Transmit(pcfg->CanNmTxPduId, pcfg->CanNmTxPduRef);
        uticker_start(&pctx->NmTimeoutTmr, pcfg->CanNmTimeoutTime);

        if (!pctx->NetworkModeConfirmed) {
            pctx->NetworkModeConfirmed = 1;
            Nm_NetworkModeConfirmation(pcfg->nmNetworkHandle);
        }
#else
        result = CanNmIf_Transmit(pcfg->CanNmTxPduId, pcfg->CanNmTxPduRef);
        if (result == E_OK) {
            uticker_start(&pctx->TxTimeoutTmr, pcfg->CanNmMsgTimeoutTime);
        }
#endif

        SentPostProcessing(pctx, pcfg, result);
    }
}
#endif

static void BusSleep_onTransition(CanNm_ChannelContextType *pctx, StateActionType action)
{
    const CanNm_ChannelConfig *pcfg = (const CanNm_ChannelConfig *)pctx->userData;

    if (STA_ACTION_ENTER == action) {
        pctx->State = NM_STATE_BUS_SLEEP;
        Nm_BusSleepMode(pcfg->nmNetworkHandle);

#if (CANNM_STATE_CHANGE_IND_ENABLED == TRUE)
        Nm_StateChangeNotification(pcfg->nmNetworkHandle, NM_STATE_PREPARE_BUS_SLEEP, NM_STATE_BUS_SLEEP);
#endif
    }
}

static const HsmState *BusSleep_HandleEvent(CanNm_ChannelContextType *pctx)
{
    const HsmState *target = NULL_PTR;

    if (pctx->Requested || pctx->PassiveStartUp) {
        target = GetCanNmFsmState(NM_STATE_REPEAT_MESSAGE); // 主动请求/被动唤醒→进入重复发送状态
    } else {
        if (GET_EVT_FLAGS(pctx, EVT_RX_INDICATION)) { // 检测到接收事件标志
            U_SAFE_AREA0 {
                CLR_EVT_FLAGS(pctx, EVT_RX_INDICATION); // 清除事件标志（避免重复处理）
            }

            const CanNm_ChannelConfig *pcfg = (const CanNm_ChannelConfig *)pctx->userData; // 实际指向的是config数组
            Nm_NetworkStartIndication(pcfg->nmNetworkHandle); // 通知上层：网络开始激活，这是个回调函数，在Lcfg中供调用，这里发送的是句柄编号

            /* transition to RMS as soon as possible */
            if (pctx->Requested || pctx->PassiveStartUp) {
                target = GetCanNmFsmState(NM_STATE_REPEAT_MESSAGE);
            }
        }
    }

    return target;
}


static void PrepareBusSleep_onTransition(CanNm_ChannelContextType *pctx, StateActionType action)
{
    const CanNm_ChannelConfig *pcfg = (const CanNm_ChannelConfig *)pctx->userData;

    if (STA_ACTION_ENTER == action) {
        pctx->State = NM_STATE_PREPARE_BUS_SLEEP;
        uticker_start(&pctx->WaitBusSleepTmr, pcfg->CanNmWaitBusSleepTime);
        Nm_PrepareBusSleepMode(pcfg->nmNetworkHandle);

#if (CANNM_STATE_CHANGE_IND_ENABLED == TRUE)
        Nm_StateChangeNotification(pcfg->nmNetworkHandle, NM_STATE_READY_SLEEP, NM_STATE_PREPARE_BUS_SLEEP);
#endif
    }
}

static const HsmState *PrepareBusSleep_HandleEvent(CanNm_ChannelContextType *pctx)
{
    const HsmState *target = NULL_PTR;

    if (pctx->Requested || pctx->PassiveStartUp || GET_EVT_FLAGS(pctx, EVT_RX_INDICATION)) {
        target = GetCanNmFsmState(NM_STATE_REPEAT_MESSAGE);

        if (GET_EVT_FLAGS(pctx, EVT_RX_INDICATION)) {
            U_SAFE_AREA0 {
                CLR_EVT_FLAGS(pctx, EVT_RX_INDICATION);
            }
        }
    } else {
        if (uticker_is_expired(&pctx->WaitBusSleepTmr)) {
            target = GetCanNmFsmState(NM_STATE_BUS_SLEEP);
        }
    }

    return target;
}

static void NetworkMode_onTransition(CanNm_ChannelContextType *pctx, StateActionType action)
{
    const CanNm_ChannelConfig *pcfg = (const CanNm_ChannelConfig *)pctx->userData;

    if (STA_ACTION_ENTER == action) {
        if (pctx->Requested && pcfg->CanNmActiveWakeupBitEnabled) {
            SetTxPduCbvBit(pcfg, CBV_BIT_ACTIVE_WAKEUP);
        }

#if (CANNM_GLOBAL_PN_SUPPORT == TRUE)
        if (pcfg->CanNmPnEnabled) {
            SetTxPduCbvBit(pcfg, CBV_BIT_PARTIAL_NETWORK_INFORMATION);
        }
#endif
        U_SAFE_AREA0 {
            pctx->PassiveStartUp = 0;
            CLR_EVT_ALL_FLAGS(pctx);
        }

        Nm_StateType nmPrevState = pctx->State;
        pctx->State = NM_STATE_REPEAT_MESSAGE;
        uticker_start(&pctx->NmTimeoutTmr, pcfg->CanNmTimeoutTime);
#if (CANNM_STATE_CHANGE_IND_ENABLED == TRUE)
        Nm_StateChangeNotification(pcfg->nmNetworkHandle, nmPrevState, NM_STATE_REPEAT_MESSAGE);
#else
        U_UNUSED(nmPrevState);
#endif
        Nm_NetworkMode(pcfg->nmNetworkHandle);
    } else {
        pctx->NetworkModeConfirmed = 0;
        uticker_stop(&pctx->NmTimeoutTmr);
        if (pcfg->CanNmActiveWakeupBitEnabled) {
            ClearTxPduCbvBit(pcfg, CBV_BIT_ACTIVE_WAKEUP);
        }
    }
}

static const HsmState *NetworkMode_HandleEvent(CanNm_ChannelContextType *pctx)
{
#define NMMODE_EVT_FLAGS (EVT_RX_INDICATION | EVT_TX_CONFIRMATION | EVT_NETWORK_CTRL | EVT_COMMUNICATION_CTRL)
    const HsmState *target = NULL_PTR;
    const CanNm_ChannelConfig *pcfg = (const CanNm_ChannelConfig *)pctx->userData;
    uint8 flag = GET_EVT_FLAGS(pctx, NMMODE_EVT_FLAGS);

    if (flag) {
        U_SAFE_AREA0 {
            CLR_EVT_FLAGS(pctx, flag);
        }
    }

    if ((flag & (EVT_RX_INDICATION | EVT_TX_CONFIRMATION)) || uticker_is_expired(&pctx->NmTimeoutTmr)) {
        if (pctx->TransmissionEnabled) {
            uticker_start(&pctx->NmTimeoutTmr, pcfg->CanNmTimeoutTime);
        }
    }

#if (CANNM_PASSIVE_MODE_ENABLED == FALSE)
    if (flag & EVT_NETWORK_CTRL) {
        if (pctx->Requested) {
#if (CANNM_GLOBAL_PN_SUPPORT == TRUE)
            if (pcfg->CanNmPnHandleMultipleNetworkRequests) {
                target = GetCanNmFsmState(NM_STATE_REPEAT_MESSAGE);
            } else
#endif
            {
                if (pctx->State == NM_STATE_READY_SLEEP) {
                    target = GetCanNmFsmState(NM_STATE_NORMAL_OPERATION);
                }
            }
        } else {
            if (pctx->State == NM_STATE_NORMAL_OPERATION) {
                target = GetCanNmFsmState(NM_STATE_READY_SLEEP);
            }
        }
    }
#endif

#if (CANNM_COM_CONTROL_ENABLED == TRUE)
    if (flag & EVT_COMMUNICATION_CTRL) {
        if (pctx->TransmissionEnabled) {
            uticker_start(&pctx->MsgCycleTmr, 0);
            uticker_start(&pctx->NmTimeoutTmr, pcfg->CanNmTimeoutTime);
        } else {
            uticker_stop(&pctx->MsgCycleTmr);
            uticker_stop(&pctx->NmTimeoutTmr);
        }
    }
#endif

#if ((CANNM_PASSIVE_MODE_ENABLED == FALSE) && (CANNM_IMMEDIATE_TX_CONF_ENABLED == FALSE))
    if (flag & EVT_TX_CONFIRMATION) {
        uticker_stop(&pctx->TxTimeoutTmr);

        if (!pctx->NetworkModeConfirmed) {
            pctx->NetworkModeConfirmed = 1;
            Nm_NetworkModeConfirmation(pcfg->nmNetworkHandle);
        }
    } else {
        if (uticker_is_expired(&pctx->TxTimeoutTmr)) {
            uticker_stop(&pctx->TxTimeoutTmr);
            Nm_TxTimeoutException(pcfg->nmNetworkHandle);
        }
    }
#endif

    if (flag & EVT_RX_INDICATION) {
        if (GetPduCbvBit(pcfg, pcfg->CanNmRxPduRef, CBV_BIT_REPEAT_MESSAGE_REQUEST)) {
            if ((pctx->State == NM_STATE_NORMAL_OPERATION) || (pctx->State == NM_STATE_READY_SLEEP)) {
                target = GetCanNmFsmState(NM_STATE_REPEAT_MESSAGE);
            }
        }
    }

    return target;
}

static void RepeatMessage_onTransition(CanNm_ChannelContextType *pctx, StateActionType action)
{
    const CanNm_ChannelConfig *pcfg = (const CanNm_ChannelConfig *)pctx->userData;

    if (STA_ACTION_ENTER == action) {
        if (GET_EVT_FLAGS(pctx, EVT_REPEAT_MSG_REQUEST)) {
            U_SAFE_AREA0 {
                CLR_EVT_FLAGS(pctx, EVT_REPEAT_MSG_REQUEST);
            }

            if (pcfg->CanNmNodeDetectionEnabled) {
                SetTxPduCbvBit(pcfg, CBV_BIT_REPEAT_MESSAGE_REQUEST);
            }
        }

#if (CANNM_PASSIVE_MODE_ENABLED == FALSE)
        if (pctx->Requested && (pcfg->CanNmImmediateNmTransmissions > 0)) {
            pctx->ImmediateNmTransmissions = pcfg->CanNmImmediateNmTransmissions;
            CanNm_SendMessage(pctx); /*! The transmission of the first NM PDU shall be triggered as soon as possible */
        } else {
            uticker_start(&pctx->MsgCycleTmr, pcfg->CanNmMsgCycleOffset);
        }
#endif

        uticker_start(&pctx->RepeatMessageTmr, pcfg->CanNmRepeatMessageTime);
        Nm_StateType nmPrevState = pctx->State;
        pctx->State = NM_STATE_REPEAT_MESSAGE;

#if (CANNM_STATE_CHANGE_IND_ENABLED == TRUE)
        if (NM_STATE_REPEAT_MESSAGE != nmPrevState) {
            Nm_StateChangeNotification(pcfg->nmNetworkHandle, nmPrevState, NM_STATE_REPEAT_MESSAGE);
        }
#else
        U_UNUSED(nmPrevState);
#endif
    } else {
        pctx->ImmediateNmTransmissions = 0;
        if (pcfg->CanNmNodeDetectionEnabled) {
            ClearTxPduCbvBit(pcfg, CBV_BIT_REPEAT_MESSAGE_REQUEST);
        }
    }
}

static const HsmState *RepeatMessage_HandleEvent(CanNm_ChannelContextType *pctx)
{
    const HsmState *target = NULL_PTR;

    if (uticker_is_expired(&pctx->RepeatMessageTmr)) {
        uticker_stop(&pctx->RepeatMessageTmr);
        target = GetCanNmFsmState(pctx->Requested ? NM_STATE_NORMAL_OPERATION : NM_STATE_READY_SLEEP);
    } else {
#if (CANNM_PASSIVE_MODE_ENABLED == FALSE)
        if (uticker_is_expired(&pctx->MsgCycleTmr)) {
            CanNm_SendMessage(pctx);
        }
#endif
    }

    return target;
}

static void NormalOperation_onTransition(CanNm_ChannelContextType *pctx, StateActionType action)
{
    const CanNm_ChannelConfig *pcfg = (const CanNm_ChannelConfig *)pctx->userData;

    if (STA_ACTION_ENTER == action) {
        Nm_StateType nmPrevState = pctx->State;
        pctx->State = NM_STATE_NORMAL_OPERATION;

#if (CANNM_PASSIVE_MODE_ENABLED == FALSE)
        if (NM_STATE_READY_SLEEP == nmPrevState) {
            CanNm_SendMessage(pctx); /* @SWS_CanNm_00006 ... the transmission of NM PDUs shall be started immediately. */
        }
#endif

#if (CANNM_STATE_CHANGE_IND_ENABLED == TRUE)
        Nm_StateChangeNotification(pcfg->nmNetworkHandle, nmPrevState, NM_STATE_NORMAL_OPERATION);
#endif
    }
}

static const HsmState *NormalOperation_HandleEvent(CanNm_ChannelContextType *pctx)
{
    const HsmState *target = NULL_PTR;

    if (GET_EVT_FLAGS(pctx, EVT_REPEAT_MSG_REQUEST)) {
        target = GetCanNmFsmState(NM_STATE_REPEAT_MESSAGE);
    } else {
#if (CANNM_PASSIVE_MODE_ENABLED == FALSE)
        if (uticker_is_expired(&pctx->MsgCycleTmr)) {
            CanNm_SendMessage(pctx);
        }
#endif
    }

    return target;
}


static void ReadySleep_onTransition(CanNm_ChannelContextType *pctx, StateActionType action)
{
    const CanNm_ChannelConfig *pcfg = (const CanNm_ChannelConfig *)pctx->userData;

    if (STA_ACTION_ENTER == action) {
#if (CANNM_STATE_CHANGE_IND_ENABLED == TRUE)
        Nm_StateType nmPrevState = pctx->State;
        pctx->State = NM_STATE_READY_SLEEP;
        Nm_StateChangeNotification(pcfg->nmNetworkHandle, nmPrevState, NM_STATE_READY_SLEEP);
#else
        pctx->State = NM_STATE_READY_SLEEP;
#endif
    }
}

static const HsmState *ReadySleep_HandleEvent(CanNm_ChannelContextType *pctx)
{
    const HsmState *target = NULL_PTR;

    if (GET_EVT_FLAGS(pctx, EVT_REPEAT_MSG_REQUEST)) {
        target = GetCanNmFsmState(NM_STATE_REPEAT_MESSAGE);
    } else {
        if (uticker_is_expired(&pctx->NmTimeoutTmr)) {
            target = GetCanNmFsmState(NM_STATE_PREPARE_BUS_SLEEP);
        }
    }

    return target;
}


#if (CANNM_GLOBAL_PN_SUPPORT == TRUE)
Std_ReturnType CanNm_PnFilterAlgorithm(const CanNm_ChannelConfig *pcfg, const PduInfoType *PduInfoPtr)
{
    Std_ReturnType relevant = E_NOT_OK;

    if (GetPduCbvBit(pcfg, PduInfoPtr, CBV_BIT_PARTIAL_NETWORK_INFORMATION)) {
        for (uint8 i = 0; i < pcfg->NmPncVectorLength; ++i) {
            if (((pcfg->NmPncVectorOffset + i) < PduInfoPtr->SduLength)
                && (PduInfoPtr->SduDataPtr[pcfg->NmPncVectorOffset + i] & pcfg->NmPnFilterMaskByte[i])) {
                relevant = E_OK;
                break;
            }
        }
    }

    return relevant;
}
#endif

/*********************************END OF FILE*********************************/
