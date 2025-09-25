/******************************************************************************
 * @file   : Nm_Lcfg.c
 * @brief  : Link time configuration for Network Management (Nm) module
 * @author : yangtl
 * @date   : 2025/05/19 22:31:19
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#include "Nm_Lcfg.h"

#define MSEC2TICK(a) (a)

static uint8 canNmRxBuf[CANNM_CHANNEL_NUMBER][8];
static uint8 canNmTxBuf[CANNM_CHANNEL_NUMBER][8];

static const PduInfoType canNmRxPdu[CANNM_CHANNEL_NUMBER] = {
    {
        .SduDataPtr = canNmRxBuf[0],
        .SduLength = sizeof(canNmRxBuf[0]),
    },
#if CANNM_CHANNEL_NUMBER > 1
    {
        .SduDataPtr = canNmRxBuf[1],
        .SduLength = sizeof(canNmRxBuf[1]),
    },
#endif
};

static const PduInfoType canNmTxPdu[CANNM_CHANNEL_NUMBER] = {
    {
        .SduDataPtr = canNmTxBuf[0],
        .SduLength = sizeof(canNmTxBuf[0]),
    },
#if CANNM_CHANNEL_NUMBER > 1
    {
        .SduDataPtr = canNmTxBuf[1],
        .SduLength = sizeof(canNmTxBuf[1]),
    },
#endif
};

static const CanNm_ChannelConfig canNmChannelConfigs[CANNM_CHANNEL_NUMBER] = {
    [CANNM_CHANNEL1] = {
        .nmNetworkHandle = CANNM_CHANNEL1,
        .CanNmWaitBusSleepTime = MSEC2TICK(2000),
        .CanNmTimeoutTime = MSEC2TICK(2000),
        .CanNmRepeatMessageTime = MSEC2TICK(1500),
        .CanNmImmediateNmCycleTime = MSEC2TICK(50),
        .CanNmImmediateNmTransmissions = 20,
        .CanNmMsgCycleOffset = 0,
        .CanNmMsgCycleTime = MSEC2TICK(500),

        .CanNmPduNidPosition = CANNM_PDU_BYTE_0,
        .CanNmPduCbvPosition = CANNM_PDU_BYTE_1,
        .CanNmNodeId = CANNM_NODE1_CAN_ID & 0xFFu,
        .CanNmNodeIdEnabled = TRUE,
        .CanNmActiveWakeupBitEnabled = TRUE,
        .CanNmNodeDetectionEnabled = TRUE,

        .CanNmRxPduId = CANNM_CHANNEL1,
        .CanNmRxPduRef = &canNmRxPdu[CANNM_CHANNEL1],
        .CanNmTxPduId = CANNM_CHANNEL1,
        .CanNmTxPduRef = &canNmTxPdu[CANNM_CHANNEL1],

#if (CANNM_GLOBAL_PN_SUPPORT == TRUE)
        .CanNmPnEnabled = TRUE,
        .CanNmAllNmMessagesKeepAwake = FALSE,
        .CanNmPnHandleMultipleNetworkRequests = FALSE,

        .NmPncVectorOffset = 4,
        .NmPncVectorLength = 4,
        .NmPnFilterMaskByte = (const uint8[]) {
            0xCF, 0x00, 0x00, 0x00
        },
#endif
    },
#if CANNM_CHANNEL_NUMBER > 1
    [CANNM_CHANNEL2] = {
        .nmNetworkHandle = CANNM_CHANNEL2,
        .CanNmWaitBusSleepTime = MSEC2TICK(2000),
        .CanNmTimeoutTime = MSEC2TICK(2000),
        .CanNmRepeatMessageTime = MSEC2TICK(1500),
        .CanNmImmediateNmCycleTime = MSEC2TICK(50),
        .CanNmImmediateNmTransmissions = 20,
        .CanNmMsgCycleOffset = 0,
        .CanNmMsgCycleTime = MSEC2TICK(500),

        .CanNmPduCbvPosition = CANNM_PDU_BYTE_1,
        .CanNmPduNidPosition = CANNM_PDU_BYTE_0,
        .CanNmNodeId = CANNM_NODE2_CAN_ID & 0xFFu,
        .CanNmNodeIdEnabled = TRUE,
        .CanNmActiveWakeupBitEnabled = TRUE,
        .CanNmNodeDetectionEnabled = TRUE,

        .CanNmRxPduId = CANNM_CHANNEL2,
        .CanNmRxPduRef = &canNmRxPdu[CANNM_CHANNEL2],
        .CanNmTxPduId = CANNM_CHANNEL2,
        .CanNmTxPduRef = &canNmTxPdu[CANNM_CHANNEL2],

#if (CANNM_GLOBAL_PN_SUPPORT == TRUE)
        .CanNmPnEnabled = TRUE,
        .CanNmAllNmMessagesKeepAwake = FALSE,
        .CanNmPnHandleMultipleNetworkRequests = FALSE,

        .NmPncVectorOffset = 4,
        .NmPncVectorLength = 4,
        .NmPnFilterMaskByte = (const uint8[]) {
            0x03, 0x00, 0x00, 0x00
        },
#endif
    },
#endif
};

static CanNm_ChannelContextType canNmChannelContexts[CANNM_CHANNEL_NUMBER]; // 通道上下文
// typedef CanNm_ConfigType Nm_ConfigType;
const Nm_ConfigType gNmConfigExample = {
    .pChannelContexts = canNmChannelContexts,
    .pChannelConfigs = canNmChannelConfigs,
    .NumberOfChannels = CANNM_CHANNEL_NUMBER,
};

/*********************************END OF FILE*********************************/
