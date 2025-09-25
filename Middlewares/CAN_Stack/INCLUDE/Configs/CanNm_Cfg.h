/******************************************************************************
 * @file   : CanNm_Cfg.h
 * @brief  : The Pre-Compiled time config header file of the CanNm module
 * @author : yangtl
 * @date   : 2025/04/23 14:18:44
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#ifndef CANNM_CFG_H
#define CANNM_CFG_H

/*! @addtogroup CanNm
 *  @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pre-processor switch for enabling version info API support.
 */
#define CANNM_VERSION_INFO_API                       TRUE

/**
 * @brief Pre-processor switch for enabling support of the Passive Mode.
 */
#define CANNM_PASSIVE_MODE_ENABLED                   FALSE

/**
 * @brief Enable/disable the immediate tx confirmation.
 * @note dependency: CanNmImmediateTxconfEnabled shall not be enabled if CanNmPasiveModeEnabled is enabled.
 */
#define CANNM_IMMEDIATE_TX_CONF_ENABLED              TRUE

/**
 * @brief Pre-processor switch for enabling user data support.
 */
#define CANNM_USER_DATA_ENABLED                      TRUE

/**
 * @brief Pre-processor switch for enabling the Communication Control support.
 * @note dependency: If (CanNmPassiveModeEnabled == False) then Equal(NmComControlEnabled) else Equal(False)
 */
#define CANNM_COM_CONTROL_ENABLED                    TRUE

/**
 * @brief Pre-processor switch for enabling the CAN NM state change notification.
 */
#define CANNM_STATE_CHANGE_IND_ENABLED               TRUE

/**
 * @brief Pre-processor switch for enabling remote sleep indication support.
 * @note dependency: If (CanNmPassiveModeEnabled == False) then Equal(NmRemoteSleepIndEnabled) else Equal(False)
 */
#define CANNM_REMOTE_SLEEP_IND_ENABLED               FALSE

/**
 * @brief Pre-processor switch for enabling partial networking support globally.
 */
#define CANNM_GLOBAL_PN_SUPPORT                      FALSE

/**
 * @brief Switches the development error detection and notification on or off.
 */
#define CANNM_DEV_ERROR_DETECT                       FALSE

#define CANNM_PDU_RX_INDICATION_ENABLED              FALSE

/**
 * @brief Pre-processor switch for enabling busload reduction support.
 * @note dependency: CanNmBusLoadReductionEnabled = false if CanNmPassiveMode
 *       Enabled == true or CanNmGlobalPnSupport == true
 */
#define CANNM_BUS_LOAD_REDUCTION_ENABLED             FALSE

/**
 * @brief Pre-processor switch for enabling bus synchronization support. This feature is required
 *        for gateway nodes only.
 * @note dependency: calculationFormula = If (CanNmPassiveModeEnabled == False) then
 *       Equal(NmBusSynchronizationEnabled) else Equal(False)
 */
#define CANNM_BUS_SYNCHRONIZATION_ENABLED            FALSE

#define CANNM_COM_USER_DATA_SUPPORT                  FALSE

#define CANNM_COORDINATOR_SYNC_SUPPORT               FALSE

#define CANNM_DYNAMIC_PNC_TO_CHANNEL_MAPPING_SUPPORT FALSE

#define CANNM_IMMEDIATE_RESTART_ENABLED              FALSE

#ifdef __cplusplus
}
#endif

/*! @}*/

#endif /* CANNM_CFG_H */
/*********************************END OF FILE*********************************/
