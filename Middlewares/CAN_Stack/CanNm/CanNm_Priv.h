/******************************************************************************
 * @file   : CanNm_Priv.h
 * @brief  :
 * @author : yangtl
 * @date   : 2025/05/06 16:52:29
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#ifndef CANNM_PRIV_H
#define CANNM_PRIV_H

/*! @addtogroup CanNm
 *  @{
 */

#include "CanNm.h"
#include "Nm_Cbk.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EVT_NETWORK_CTRL        0x01u
#define EVT_COMMUNICATION_CTRL  0x02u
#define EVT_REPEAT_MSG_REQUEST  0x04u
#define EVT_RX_INDICATION       0x10u
#define EVT_TX_CONFIRMATION     0x20u

#define GET_EVT_FLAGS(ptx, evt) ((ptx)->flags & (evt))
#define SET_EVT_FLAGS(ptx, evt) ((ptx)->flags |= (evt))
#define CLR_EVT_FLAGS(ptx, evt) ((ptx)->flags &= ~(evt))
#define CLR_EVT_ALL_FLAGS(ptx)  ((ptx)->flags = 0)

typedef enum {
    STA_ACTION_ENTER,
    STA_ACTION_EXIT
} StateActionType;

typedef struct HsmState HsmState;

struct HsmState {
    void (*onTransition)(CanNm_ChannelContextType *pctx, StateActionType action);
    const HsmState *(*handleEvent)(CanNm_ChannelContextType *pctx);
    const HsmState *parent;
};

void Hsm_DispatchEvent(CanNm_ChannelContextType *pctx);

#if (CANNM_GLOBAL_PN_SUPPORT == TRUE)
Std_ReturnType CanNm_PnFilterAlgorithm(const CanNm_ChannelConfig *pcfg, const PduInfoType *PduInfoPtr);
#endif

#ifdef __cplusplus
}
#endif

/*! @}*/

#endif /* CANNM_PRIV_H */
/*********************************END OF FILE*********************************/
