/******************************************************************************
 * @file   : Nm_Lcfg.h
 * @brief  : Link time configuration for Network Management (Nm) module
 * @author : yangtl
 * @date   : 2025/05/19 22:30:08
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#ifndef NM_LCFG_H
#define NM_LCFG_H

/*! @addtogroup Nm
 *  @{
 */

#include "Nm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CANNM_CHANNEL_NUMBER    2u
#define CANNM_CHANNEL1          0u
#define CANNM_CHANNEL2          1u

#define CANNM_NODE1_CAN_ID      0x500u
#define CANNM_NODE2_CAN_ID      0x501u

extern const Nm_ConfigType gNmConfigExample;

#ifdef __cplusplus
}
#endif

/*! @}*/

#endif /* NM_LCFG_H */
/*********************************END OF FILE*********************************/
