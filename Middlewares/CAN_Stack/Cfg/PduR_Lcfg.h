#ifndef __PDUR_CONFIG_H__
#define __PDUR_CONFIG_H__

#include "PduR.h"

#ifndef CFG_IS_CANFD
#define CFG_IS_CANFD true
#endif

enum PduRModuldId {
  PDUR_MID_NM,
  PDUR_MID_UDS,
  PDUR_MID_COM,
  PDUR_MID_TP,
  PDUR_MID_ATE,
  PDUR_MID_XCDTP,
  PDUR_MID_GATEWAY_PUB,
  PDUR_MID_GATEWAY_PRV,
};

enum CanMailBox {
  CAN_MAILBOX_COM,
  CAN_MAILBOX_NM,
  CAN_MAILBOX_UDS,
  CAN_MAILBOX_NUM,
};

extern PduR_LinkType gPduRLink;
void PduR_Config_Init(void);


#endif /* __PDUR_CONFIG_H__ */
