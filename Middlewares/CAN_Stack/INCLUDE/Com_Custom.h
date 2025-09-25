/******************************************************************************
 * @copyright (C) 2024-20XX, yftech Development Team.
 * @file   : Com_Custom.h
 * @brief  :
 * @author : yangtl
 * @date   : 2024/06/06 15:17:11
 * @version: 0.0.1
 *
 * 2024/06/06  Initial Release
 *****************************************************************************/
#ifndef __COM_CUSTOM_H__
#define __COM_CUSTOM_H__

#include "util_tick.h"

#ifdef __cplusplus
extern "C" {
#endif

#define COM_OPT_CHECK_AVAILABLE                 FALSE

/*
 *  +-----------+-----------------+
 *  +  15 ~ 12  |      11 ~ 0     +
 *  +-----------+-----------------+
 *  +  groupId  |      pduId      +
 *  +-----------+-----------------+
 */
#define COM_PDUID_WITH_GROUP(pduId, groupId)    ((((groupId) & 0x0Fu) << 12u) | (pduId))
#define COM_GET_PDUID(value)                    ((value) & 0xFFFu)
#define COM_GET_GROUPID(value)                  (((value) >> 12u) & 0x0Fu)

#define COM_PDU_DATA_MAX_SIZE                   64u

typedef enum {
    /** basic types */
    COM_MSG_PERIODIC,
    COM_MSG_ON_EVENT,
    COM_MSG_IF_ACTIVE,

    /** combined types */
    COM_MSG_POE, /*!< Periodic and on event */
    COM_MSG_PA,  /*!< Periodic and if active */
} ComMsgType;

typedef enum {
    COM_RX_STA_INIT,       /*!< Already initialized but not enabled for receive monitoring */
    COM_RX_STA_UNKNOWN,    /*!< message not yet received but not timeout after start-up */
    COM_RX_STA_MISSING,    /*!< message has not been received during a certain time window */
    COM_RX_STA_PRESENT,    /*!< correctly received */
} ComRxStatusType;

typedef enum {
    COM_TX_STA_READY,
    COM_TX_STA_GOIING,
    COM_TX_STA_SUCCESS,
    COM_TX_STA_FAILED,
} ComTxStatusType;

/* PduInfoType: MetaDataPtr */
typedef struct {
    uint8_t channel;
    uint32_t canId;
} ComPduMetaDataType;

/* Com Tx Runtime */
typedef struct {
    utick           txTimeout;
    utick           cycleTimeout;
    utick           minDelayTimeout;
    utick           repeatTimeout;
    uint16          repetitionCnt;
    uint8_t         txStatus;

    uint8_t         enabled: 1; /*!< all msg */
    uint8_t         delayed: 1; /*!< combined periodic and on event msg */
    uint8_t         active:  1; /*!< ifActive msg */
} ComTxRteType;

typedef struct {
    ComPduMetaDataType metaData;
    ComMsgType      msgType;

    /** @note time constraints:
     *      1, cycleTime > fastCycleTime > minDelayTime;
     *      2, cycleTime > repeatTime * repetition;
     */

    /** parameter for periodic msg */
    uint16_t        startOffsetP;   /*!< To spread the busload, an individual start offset tOffsetP can be defined for each message. */
    uint16_t        cycleTime;      /*!< use for periodic msg */
    uint16_t        fastCycleTime;  /*!< use for ifActive msg */

    /** parameter for onEvent or ifActive msg */
    uint16_t        repetition;
    uint16_t        repeatTime;
    uint16_t        minDelayTime;   /*!< After a message is sent, there shall be no repeated transmission with the same message for the time tDelay. */
} ComTxMsgInfoType;

typedef struct {
    ComTxMsgInfoType msgInfo;
    PduInfoType      pduInfo;
} ComTxPduConfigType;

typedef struct {
    utick           timeout;
    uint8_t         monitor:    1;
    uint8_t         missing:    1;  /*!< message has not been received during a certain time window */
    uint8_t         present:    1;  /*!< correctly received */
} ComRxRteType;

typedef struct {
    struct {
        ComPduMetaDataType metaData;
        uint16_t timeWindow;
    } msgInfo;

    PduInfoType     pduInfo;
} ComRxPduConfigType;

typedef struct {
    PduIdType PduId;
    void (*pfnSetSignal)(const void *SignalDataPtr);
} ComTxSignalConfigType;

typedef struct {
    PduIdType PduId;
    void (*pfnGetSignal)(void *SignalDataPtr);
} ComRxSignalConfigType;

typedef struct {
    Com_CbkTxAckType cbkTxAck;
    Com_CbkTxErrType cbkTxErr;
    Com_CbkRxAckType cbkRxAck;
    Com_CbkRxToutType cbkRxTout;

    ComTxRteType *pTxRtes; /* pTxRtes[numOfTxPdus] */
    ComRxRteType *pRxRtes; /* pRxRtes[numOfRxPdus] */
    const ComTxPduConfigType *pTxPdus; /* Can be NULL, NULL indicates that no data is sent */
    const ComRxPduConfigType *pRxPdus; /* Can be NULL, NULL indicates that no data is recv */
    const ComTxSignalConfigType *pTxSignal;
    const ComRxSignalConfigType *pRxSignal;
    uint16_t numOfTxPdus;
    uint16_t numOfRxPdus;
    uint16_t numOfTxSignal;
    uint16_t numOfRxSignal;
    boolean inverseOrder;    /*!< Intel: false; Motorola: true */
} ComIPduConfigType;

typedef struct {
    const ComIPduConfigType *pPduGroup;
    uint8_t numOfPduGroup;
    uint32_t cyclePeriod;
} Com_ConfigType;


#ifdef __cplusplus
}
#endif

#endif /* __COM_CUSTOM_H__ */
/*********************************END OF FILE*********************************/
