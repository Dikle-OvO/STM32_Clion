/******************************************************************************
 * @file   : CanNm_Types.h
 * @brief  : Data types of CAN Network Management AUTOSAR CP
 * @author : yangtl
 * @date   : 2025/04/28 16:16:16
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#ifndef CANNM_TYPES_H
#define CANNM_TYPES_H

/*! @addtogroup CanNm
 *  @{
 */
#pragma anon_unions
#include "CanNm_Cfg.h"
#include "NmStack_types.h"
#include "util_tick.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CANNM_PDU_BYTE_0 = 0x00,
    CANNM_PDU_BYTE_1 = 0x01,
    CANNM_PDU_OFF = 0xFF
} CanNm_PduPositionType;

/**
 * @brief This container contains the channel specific configuration parameter of the CanNm.
 */
typedef struct {
    NetworkHandleType     nmNetworkHandle; //网络句柄号
#if (CANNM_PASSIVE_MODE_ENABLED == FALSE)
    /** NOTE: If CanNmPnHandleMultipleNetworkRequests == True" then "CanNmImmediateNmTransmissions > 0;
     *        CanNmRepeatMessageTime > CanNmImmediateNmTransmissions * CanNmImmediateNmCycleTime
     */
    uint8                 CanNmImmediateNmTransmissions;
    uint16                CanNmImmediateNmCycleTime;
    uint16                CanNmMsgCycleOffset; /* CanNmMsgCycleOffset < CanNmMsgCycleTime */
    uint16                CanNmMsgCycleTime;
    uint16                CanNmMsgTimeoutTime; /* CanNmMsgTimeoutTime < CanNmMsgCycleTime */
    uint16                CanNmMsgReducedTime; /* 0.5 * CanNmMsgCycleTime <= CanNmMsgReducedTime < CanNmMsgCycleTime */
#endif
    uint16                CanNmTimeoutTime; /* CanNmTimeoutTime > CanNmMsgCycleTime */
    uint16                CanNmWaitBusSleepTime;
    uint16                CanNmRepeatMessageTime;
#if (CANNM_REMOTE_SLEEP_IND_ENABLED == TRUE)
    uint16                CanNmRemoteSleepIndTime;
#endif
    CanNm_PduPositionType CanNmPduNidPosition;
    CanNm_PduPositionType CanNmPduCbvPosition;
    uint8                 CanNmNodeId;
    boolean               CanNmNodeIdEnabled;
    boolean               CanNmActiveWakeupBitEnabled;
    boolean               CanNmNodeDetectionEnabled;
    boolean               CanNmRepeatMsgIndEnabled;
    boolean               CanNmBusLoadReductionActive;
    boolean               CanNmSynchronizedPncShutdownEnabled;
    boolean               CanNmDynamicPncToChannelMappingEnabled;
#if (CANNM_GLOBAL_PN_SUPPORT == TRUE)
    boolean               CanNmPnEnabled;
    boolean               CanNmAllNmMessagesKeepAwake;
    boolean               CanNmPnHandleMultipleNetworkRequests;

    /*! @note NmPncBitVectorOffset == Number of <Bus>Nm Sytem Bytes OR
     *        NmPncBitVectorOffset + NmPncBitVectorLength == NM PduLength.
     */
    uint8                 NmPncVectorOffset;
    uint8                 NmPncVectorLength;
    const uint8          *NmPnFilterMaskByte;
#endif

#if 0 /* TODO */
    uint8   CanNmCarWakeUpBitPosition;
    uint8   CanNmCarWakeUpBytePosition;
    boolean CanNmCarWakeUpFilterEnabled;
    uint8   CanNmCarWakeUpFilterNodeId;
    boolean CanNmCarWakeUpRxEnabled;
#endif

    /* CanNmRxPdu */
    struct {
        PduIdType          CanNmRxPduId;
        const PduInfoType *CanNmRxPduRef;
    };

    /* CanNmTxPdu */
    struct {
        PduIdType          CanNmTxPduId;
        const PduInfoType *CanNmTxPduRef;
    };

    /* CanNmUserDataTxPdu */
} CanNm_ChannelConfig;

typedef struct {
    void *userData; // 实际上是指向 CanNlm_ChannelConfig 的指针

    union {
        uticker MsgCycleTmr;
        uticker WaitBusSleepTmr;
    };

    uticker RepeatMessageTmr;
    uticker NmTimeoutTmr;
#if (CANNM_IMMEDIATE_TX_CONF_ENABLED != TRUE)
    uticker TxTimeoutTmr;
#endif
    uint8        ImmediateNmTransmissions;
    Nm_StateType State; // 状态

    /*! event flags and status */
    uint8 flags;
    struct {
        uint8 Requested : 1;
        uint8 PassiveStartUp : 1;
        uint8 TransmissionEnabled : 1;

        /*! At least one NM message was successfully sent. */
        uint8 NetworkModeConfirmed : 1;
    };
} CanNm_ChannelContextType;

typedef struct {
    CanNm_ChannelContextType  *pChannelContexts;
    const CanNm_ChannelConfig *pChannelConfigs;
    uint8                      NumberOfChannels;
} CanNm_ConfigType;

#ifdef __cplusplus
}
#endif

/*! @}*/

#endif /* CANNM_TYPES_H */
/*********************************END OF FILE*********************************/
