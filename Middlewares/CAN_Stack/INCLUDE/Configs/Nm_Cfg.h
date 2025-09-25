/******************************************************************************
 * @file   : Nm_Cfg.h
 * @brief  : The Pre-Compiled time config header file of the Nm module
 * @author : yangtl
 * @date   : 2025/04/28 14:22:01
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#ifndef NM_CFG_H
#define NM_CFG_H

/*! @addtogroup Nm
 *  @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/*! @note Only one of NM_PROTOCOL_AUTOSAR and NM_PROTOCOL_OSEK can be defined, not both.
 *  If none of them are defined, it defaults to NM_PROTOCOL_AUTOSAR.
 */
/* #define NM_PROTOCOL_AUTOSAR */
/* #define NM_PROTOCOL_OSEK */

#ifdef __cplusplus
}
#endif

/*! @}*/

#endif /* NM_CFG_H */
/*********************************END OF FILE*********************************/
