/******************************************************************************
 * @file   : NmStack_types.h
 * @brief  : Data types of Network Management Stack AUTOSAR CP
 * @author : yangtl
 * @date   : 2025/04/23 14:52:26
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#ifndef NMSTACK_TYPES_H
#define NMSTACK_TYPES_H

#include "ComStack_Types.h"

/*! @addtogroup Nm
 *  @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Operational modes of the network management.
 */
typedef enum {
    NM_MODE_BUS_SLEEP,
    NM_MODE_PREPARE_BUS_SLEEP,
    NM_MODE_SYNCHRONIZE,
    NM_MODE_NETWORK,
} Nm_ModeType;

/**
 * @brief States of the network management state machine.
 */
typedef enum {
    NM_STATE_UNINIT,
    NM_STATE_BUS_SLEEP,
    NM_STATE_PREPARE_BUS_SLEEP,
    NM_STATE_READY_SLEEP,
    NM_STATE_NORMAL_OPERATION,
    NM_STATE_REPEAT_MESSAGE,
    NM_STATE_SYNCHRONIZE,
    NM_STATE_OFFLINE,
} Nm_StateType;

typedef enum {
    NM_BUSNM_CANNM,        /*!< CAN NM type                             */
    NM_BUSNM_FRNM,         /*!< FR NM type                              */
    NM_BUSNM_UDPNM,        /*!< UDP NM type                             */
    NM_BUSNM_GENERICNM,    /*!< Generic NM type                         */
    NM_BUSNM_J1939NM,      /*!< SAE J1939 NM type (address claiming)    */
    NM_BUSNM_LOCALNM,      /*!< Local NM Type                           */
    NM_BUSNM_UNDEF = 0xFF, /*!< NM type undefined; it shall be defined as FFh */
} Nm_BusNmType;

#ifdef __cplusplus
}
#endif

/*! @}*/

#endif /* NMSTACK_TYPES_H */
/*********************************END OF FILE*********************************/
