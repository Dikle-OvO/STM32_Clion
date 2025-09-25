#ifndef __UDS_SERVER_H__
#define __UDS_SERVER_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "uds.h"
#include "isotp.h"

/********************** Initialization parameters, see the corresponding structure for specific parameter information ***********************/
typedef struct
{
    BufCfg bufCfg;
    BspCallbackCfg bspCallbackCfg;
    ServiceCfg serviceCfg;
    const isotp_configPara *tpCfg;
} UDSInitPara;

/****************************************************** Interface for external calls to uds ***********************************************/

/**
 * @fn      UDSServerInit()
 * @brief   Initialises the UDSServer library.
 * @param   UDSInitPara [IN] The parameters used to initialize the library
 * @retval  UDSErr_t 
 */
UDSErr_t UDSServerInit(UDSInitPara *udaPara);

/**
 * @fn      UDSServerReceiveCAN()
 * @brief   Receives Can Data From The Driver Layer.
 * @param   identifier [IN] The Id Of The Driver Layer Data
 * @param   data [IN] The data passed to uds services.
 * @param   size [IN] The size of the data.
 * @retval None
 */
void UDSServerReceiveCAN(uint32_t identifier, uint8_t *data, uint8_t len);

/**
 * @fn      UDSServerPoll()
 * @brief   Polling function; call this function periodically to handle timeouts, handle diagnostic services and send responses, etc.
 * @retval None
 */
void UDSServerPoll(void);

/**
 * @fn     UDSServerGetVersion()
 * @brief  This service returns the version information of this module.
 * @param  versioninfo  [OUT] Pointer to where to store the version information of this module
 * @retval None
 */
void UDSServerGetVersion(UDS_VersionInfoType *versioninfo);

/**
 * @fn     setUDSSession()
 * @brief  Set Uds Session Level.
 * @param  session  [IN] Session Level, Currently Supported :0x01、0x02 And 0x03
 * @retval None
 */
void setUDSSession(uint8_t session);

/**
 * @fn     UDSSendConfirm()
 * @brief  check the sending status of the previous frame, must set it success or fail before send next frame.
 * @param  result      ISOTP_SEND_RESULT_SUCCESS,ISOTP_SEND_RESULT_FAIL
 * @retval None
 */
void UDSSendConfirm(IsoTpSendResultTypes result);
/****************************************************** Interface for port calls to uds ***********************************************/

/**
 * @fn     detectP2TimeAndResponsePending()
 * @brief  Detect p2 and p2star time and automatically send 0x78
 * @param  srv  [IN] uds instance pointer
 * @retval None
 * @note   The first time the service calls this function it must send 0x78, converting the p2 time to p2star. after that it sends 0x78 after the p2star*(3/10) timeout.
 */
void detectP2TimeAndResponsePending(UDSServer_t *srv); /*response 0x78 & update p2_star & update s3*/

/**
 * @fn     UdsServerWithSubFuncNotSuppressPosRsp()
 * @brief  Set this sub-service not to support suppression of positive responses
 * @param  srv  [IN] uds instance pointer
 * @retval None
 * @note   For master services that support Suppress Positive Response, but child services that do not support Suppress Positive Response.For example: 0x10 0x02.
 */
void UdsServerWithSubFuncNotSuppressPosRsp(UDSServer_t *srv);

/**
 * @fn     resetBegin()
 * @brief  Delayed reset function, after resetTimer timeout call UDS_SRV_EVT_DoScheduledReset callback in port
 * @param  srv  [IN] uds instance pointer
 * @param  resetType  [IN] Reset type, which is passed to the UDS_SRV_EVT_DoScheduledReset callback
 * @param  resetTimer  [IN] Reset timeout
 * @retval None
 * @note   Applicable to the case of app jump to boot, qpp may need to perform other reset operations before switching the programming session, need to wait for the delay.
 */
void resetBegin(UDSServer_t *srv, uint8_t resetType, uint32_t resetTimer);

/**
 * @fn     UdsServerSetSecurityAccess()
 * @brief  whether security authentication is performed when the uds service is executed
 * @param  isEnable  [IN] True: enable security authentication, False: disable security authentication
 * @retval None 
 */
void UdsServerSetSecurityAccess(bool isEnable);

/**
 * @fn     UdsServerSetSecurityLevel()
 * @brief  unlock 27 service levels to the specified level. note: 27 services do not need to be called separately
 * @param  level  [IN] level
 * @retval None 
 */
void UdsServerSetSecurityLevel(uint8_t level);

#endif
