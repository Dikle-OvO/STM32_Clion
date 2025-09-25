/******************************************************************************
 * @file   : Nm_Cbk.h
 * @brief  : Header of Network Management Call-back notifications
 * @author : yangtl
 * @date   : 2025/04/28 11:49:21
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#ifndef NM_CBK_H
#define NM_CBK_H

/*! @addtogroup Nm
 *  @{
 */

#include "Nm_Cfg.h"
#include "NmStack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************  Standard Call-back notifications **************/

/**
 * @brief      Notification that a NM-message has been received in the Bus-Sleep Mode,
 *             what indicates that some nodes in the network have already entered the Network Mode.
 * @param[in]  nmNetworkHandle Identification of the NM-channel
 * @retval     None
 */
extern void Nm_NetworkStartIndication(NetworkHandleType nmNetworkHandle);

/**
 * @brief      Notification that the network management has entered Network Mode.
 * @param[in]  nmNetworkHandle Identification of the NM-channel
 * @retval     None
 */
extern void Nm_NetworkMode(NetworkHandleType nmNetworkHandle);

/**
 * @brief      Notification that the network management has entered Bus-Sleep Mode.
 * @param[in]  nmNetworkHandle Identification of the NM-channel
 * @retval     None
 */
extern void Nm_BusSleepMode(NetworkHandleType nmNetworkHandle);

/**
 * @brief      Notification that the network management has entered Prepare Bus-Sleep Mode.
 * @param[in]  nmNetworkHandle Identification of the NM-channel
 * @retval     None
 */
extern void Nm_PrepareBusSleepMode(NetworkHandleType nmNetworkHandle);

/**
 * @brief      Notification that the network management has entered Synchronize Mode.
 * @param[in]  nmNetworkHandle Identification of the NM-channel
 * @retval     None
 */
extern void Nm_SynchronizeMode(NetworkHandleType nmNetworkHandle);

/**
 * @brief      Notification that the network management has detected that all other nodes on the network are
 *             ready to enter Bus-Sleep Mode.
 * @param[in]  nmNetworkHandle Identification of the NM-channel
 * @retval     None
 */
extern void Nm_RemoteSleepIndication(NetworkHandleType nmNetworkHandle);

/**
 * @brief      Notification that the network management has detected that not all other nodes on the network
 *             are longer ready to enter Bus-Sleep Mode.
 * @param[in]  nmNetworkHandle Identification of the NM-channel
 * @retval     None
 */
extern void Nm_RemoteSleepCancellation(NetworkHandleType nmNetworkHandle);

/************** Non-Standard Call-back notifications **************/

/**
 * @brief      Notification that at least one NM message was successfully sent.
 * @param[in]  nmNetworkHandle Identification of the NM-channel
 * @retval     None
 */
extern void Nm_NetworkModeConfirmation(NetworkHandleType nmNetworkHandle);

/************** Extra Call-back notifications **************/

/**
 * @brief      Notification that a NM message has been received.
 * @note       This function is only available if NmPduRxIndicationEnabled is set to TRUE.
 * @param[in]  nmNetworkHandle Identification of the NM-channel
 * @retval     None
 */
extern void Nm_PduRxIndication(NetworkHandleType nmNetworkHandle);

/**
 * @brief      Notification that the state of the lower layer <Bus>Nm has changed.
 * @note       This function is only available if NmStateChangeIndEnabled is set to TRUE.
 * @param[in]  nmNetworkHandle Identification of the NM-channel
 * @retval     None
 */
extern void Nm_StateChangeNotification(NetworkHandleType nmNetworkHandle, Nm_StateType nmPreviousState, Nm_StateType nmCurrentState);

/**
 * @brief      Service to indicate that an NM message with set Repeat Message Re- quest Bit has been received.
 *             This is needed for node detection and the Dynamic PNC-to-channel-mapping feature.
 * @param[in]  nmNetworkHandle Identification of the NM-channel
 * @retval     None
 */
extern void Nm_RepeatMessageIndication(NetworkHandleType nmNetworkHandle, boolean pnLearningBitSet);

/**
 * @brief      Service to indicate that an attempt to send an NM message failed.
 * @param[in]  nmNetworkHandle Identification of the NM-channel
 * @retval     None
 */
extern void Nm_TxTimeoutException(NetworkHandleType nmNetworkHandle);

/**
 * @brief      This function is called by a <Bus>Nm to indicate reception of a CWU request.
 * @param[in]  nmNetworkHandle Identification of the NM-channel
 * @retval     None
 */
extern void Nm_CarWakeUpIndication(NetworkHandleType nmChannelHandle);


#ifdef __cplusplus
}
#endif

/*! @}*/

#endif /* NM_CBK_H */
/*********************************END OF FILE*********************************/
