#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "uds.h"
#include "uds_server.h"

#include "can_drv.h"
#include "fsl_debug_console.h"
/* TODO: insert other include files here. */
#include "yf_log.h"
#include "ecual_lptmr.h"

#define ECU_TX_ID 0x7BA
#define ECU_RX_PHY_ID 0x73A
#define ECU_RX_FUN_ID 0x7DF

#define UDS_TP_MTU_PHY (4095)
#define UDS_TP_MTU_FUN (64)
#define UDS_TP_MTU_SEND (128)
uint8_t send_buf[UDS_TP_MTU_SEND];
uint8_t phy_recv_buf[UDS_TP_MTU_PHY];
uint8_t fun_recv_buf[UDS_TP_MTU_FUN];

const uint8_t YFSescurityLevelTab[] = {1, 9};
const uint8_t YFSescurityFBL[] = {9};
const uint8_t YFSescurityAPP[] = {1};

/** 获取数组元素的个数 */
#ifndef GET_ARRAY_NUM
#define GET_ARRAY_NUM(array) (sizeof(array) / sizeof((array)[0]))
#endif

static const UdsServiceInfo_S UdsSvrServicesTab[] = {
    /** sid;
     *  mark;  >> bit0: 广播；bit1: 子功能。 @see SER_MARK_...
     *
     *  { *sessionSupportTable, *securityLevelTable,   sessionTableSize, securityTableSize }
     */
    {
        kSID_DIAGNOSTIC_SESSION_CONTROL,    /** DiagnosticSessionControl */
        SER_MARK_BROADCAST | SER_MARK_SPRM, /** 支持广播、抑制正响应 */
        UDS_SESS_ALL,                       /*Session: 01/02/03;*/
        {NULL, 0}                           /**   SecurityAccessLevel: No-need */
    },
    {
        kSID_ECU_RESET,                     /** EcuReset */
        SER_MARK_BROADCAST | SER_MARK_SPRM, /** 支持广播、抑制正响应 */
        UDS_SESS_ALL,                       /*Session: 01/02/03;*/
        {NULL, 0}                           /**   SecurityAccessLevel: No-need */
    },
    {
        kSID_SECURITY_ACCESS,        /** SecurityAccess */
        0,                           /** 不支持广播、抑制正响应 */
        UDS_SESS_PRG | UDS_SESS_EXT, /*Session: 02/03;*/
        {NULL, 0}                    /**   SecurityAccessLevel: No-need */
    },
    {
        kSID_TESTER_PRESENT,                /** TesterPresent */
        SER_MARK_BROADCAST | SER_MARK_SPRM, /** 支持广播、抑制正响应 */
        UDS_SESS_ALL,                       /*Session: 01/02/03;*/
        {NULL, 0}                           /**   SecurityAccessLevel: No-need */
    },
    {
        kSID_READ_DATA_BY_IDENTIFIER, /** ReadDataByIdentifier */
        SER_MARK_BROADCAST,           /** 支持广播 */
        UDS_SESS_ALL,                 /*Session: 01/02/03;*/
        {NULL, 0}                     /**   SecurityAccessLevel: No-need */
    },
    {
        kSID_WRITE_DATA_BY_IDENTIFIER,                                       /** WriteDataByIdentifier */
        0,                                                                   /** 不支持广播、抑制正响应 */
        UDS_SESS_PRG | UDS_SESS_EXT,                                         /*Session: 02/03;*/
        {(uint8_t *)YFSescurityLevelTab, GET_ARRAY_NUM(YFSescurityLevelTab)} /**   SecurityAccessLevel: APP & FBL */
    },
    {
        kSID_ROUTINE_CONTROL,                                                /** RoutineControl */
        0,                                                                   /** 不支持广播、抑制正响应 */
        UDS_SESS_PRG | UDS_SESS_EXT,                                         /*Session: 02/03;*/
        {(uint8_t *)YFSescurityLevelTab, GET_ARRAY_NUM(YFSescurityLevelTab)} /**   SecurityAccessLevel: APP & FBL */
    },
    {
        kSID_REQUEST_DOWNLOAD,                                     /** RequestDownload */
        0,                                                         /** 不支持广播、抑制正响应 */
        UDS_SESS_PRG,                                              /*Session: 02;*/
        {(uint8_t *)YFSescurityFBL, GET_ARRAY_NUM(YFSescurityFBL)} /**   SecurityAccessLevel:  FBL */
    },
    {
        kSID_TRANSFER_DATA,                                        /** TransferData */
        0,                                                         /** 不支持广播、抑制正响应 */
        UDS_SESS_PRG,                                              /*Session: 02;*/
        {(uint8_t *)YFSescurityFBL, GET_ARRAY_NUM(YFSescurityFBL)} /**   SecurityAccessLevel:  FBL */
    },
    {
        kSID_REQUEST_TRANSFER_EXIT,                                /** RequestTransferExit */
        0,                                                         /** 不支持广播、抑制正响应 */
        UDS_SESS_PRG,                                              /*Session: 02;*/
        {(uint8_t *)YFSescurityFBL, GET_ARRAY_NUM(YFSescurityFBL)} /**   SecurityAccessLevel:  FBL */
    },

};

uint8_t uds_send_can(const uint32_t arbitration_id, const uint8_t *data, const uint8_t size, int timeout)
{
    // return API_CANDrv_SendFrame(arbitration_id, size, data, ECUAL_MAILBOX_DIAG, 0) == 1 ? 0 : 1;
    PduR_Send(&gPriPduRLink, Module_UDS, arbitration_id, 0, data, size, 100);
    return ISOTP_RET_OK;
}

UDSInitPara udsPara = {
    .bufCfg = {
        .ta = ECU_TX_ID,
        .sa_phy = ECU_RX_PHY_ID,
        .sa_func = ECU_RX_FUN_ID,
        .send_buf = send_buf,
        .send_buf_size = UDS_TP_MTU_SEND,
        .phy_recv_buf = phy_recv_buf,
        .phy_recv_buf_size = UDS_TP_MTU_PHY,
        .fun_recv_buf = fun_recv_buf,
        .fun_recv_buf_size = UDS_TP_MTU_FUN,
    },
    .bspCallbackCfg = {
        .uds_send_can = uds_send_can,
        .uds_get_ms = get_cur_ms,
        .print_func = PRINTF,
    },
    .serviceCfg = {
        .udsSvrTab = &UdsSvrServicesTab,
        .udsSvrNumber = GET_ARRAY_NUM(UdsSvrServicesTab),
        .udsserver_event_callback = udsserver_event_callback,
        .userRegisterService = userRegisterService,
        .getSecurutyAuthFailCount = getSecurutyAuthFailCount,
        .setSecurutyAuthFailCount = setSecurutyAuthFailCount,
        .UdsSecuritySeedLen = 4,
        .SecurityMaxAttemptsCount = 3,
        .p2_server_ms = 50,
        .p2_star_server_ms = 5000,
        .power_down_time_ms = 10,
        .s3_server = 5000,
        .auth_fail_delay_ms = 10000,
    },
    .tpCfg = {
        .As_ms = 70,
        .Bs_ms = 150,
        .Ar_ms = 70,
        .Cr_ms = 150,
        .BlockSize = 0,
        .STmin = 0,
        .max_WFT_Number = 0,
        .frame_padding_char = 0xCC,
        .canfd = 0,
    }
};
