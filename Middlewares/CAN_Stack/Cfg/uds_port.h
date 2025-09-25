#ifndef __UDS_PORT_H__
#define __UDS_PORT_H__

#include "uds.h"

/*uds必须要实现的函数*/
UDS_NRC udsserver_event_callback(UDSServer_t *srv, int evt, const void *data);
bool getSecurutyAuthFailCount(uint8_t *FailAuthCounter);
bool setSecurutyAuthFailCount(uint8_t FailAuthCounter);
UDSService userRegisterService(uint8_t sid);

#endif
