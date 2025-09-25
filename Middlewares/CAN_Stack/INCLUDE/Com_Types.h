/******************************************************************************
 * @copyright (C) 2024-20XX, yftech Development Team.
 * @file   : Com_Types.h
 * @brief  : Data types of communication module AUTOSAR CP
 * @author : yangtl
 * @date   : 2024/06/05 20:57:38
 * @version: 0.0.1
 *
 * 2024/06/05  Initial Release
 *****************************************************************************/
#ifndef __COM_TYPES_H__
#define __COM_TYPES_H__

#include "ComStack_Types.h"

typedef void (*Com_CbkTxAckType)(PduIdType PduId);
typedef void (*Com_CbkTxErrType)(PduIdType PduId);
typedef void (*Com_CbkRxAckType)(PduIdType PduId, PduInfoType const *PduInfoPtr);
typedef void (*Com_CbkRxToutType)(PduIdType PduId, PduInfoType const *PduInfoPtr);

typedef uint16_t Com_SignalIdType;

typedef uint16_t Com_SignalGroupIdType;

typedef uint16_t Com_IpduGroupIdType;

#endif /* __COM_TYPES_H__ */
/*********************************END OF FILE*********************************/
