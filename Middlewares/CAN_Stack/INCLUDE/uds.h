#ifndef __UDS_H__
#define __UDS_H__

#include "isotp.h"
#include <stdbool.h>

/** 服务标志(service mark) */
#define SER_MARK_BROADCAST 0x01u /** bit0  服务支持广播方式的请求(功能寻址：一对多) 0:不支持，1:支持*/
#define SER_MARK_SPRM 0x02u      /** bit1  Suppress-positive response messaget  0:不支持，1:支持*/

#define UDS_SESS_DEF (1 << 0) /*bit 0*/
#define UDS_SESS_PRG (1 << 1) /*bit 1*/
#define UDS_SESS_EXT (1 << 2) /*bit 2*/
#define UDS_SESS_ALL (UDS_SESS_DEF | UDS_SESS_PRG | UDS_SESS_EXT)

#define SUPPRESS_POSITIVE_RESP_BIT ((uint8_t)0x80) /** 子功能 -- bit7 为一时，表示抑制肯定响应，不需要发送肯定响应 */

enum UDSServerEvent
{
    UDS_SRV_EVT_DiagSessCtrl = 0,         // 0: UDSDiagSessCtrlArgs_t *
    UDS_SRV_EVT_EcuReset,                 // 1: UDSECUResetArgs_t *
    UDS_SRV_EVT_ClrDiagInfo,              // 2: UDSGroupOfDTCArgs_t *
    UDS_SRV_EVT_RepNumOfDTCByStaMask,     // 3: UDSRepNumOfDTCByStaMaskArgs_t *
    UDS_SRV_EVT_RepDTCByStaMask,          // 4: UDSRepDTCByStaMaskArgs_t *
    UDS_SRV_EVT_RepDTCSnapId,             // 5: UDSRepDTCSnapIdArgs_t *
    UDS_SRV_EVT_RepDTCSnapRecByDTCNum,    // 6: UDDTCSnapRecByDTCNumArgs_t *
    UDS_SRV_EVT_RepDTCExtDataRecByDTCNum, // 7: UDSRepDTCExtDataRecByDTCNumArgs_t *
    UDS_SRV_EVT_RepAllSupDTC,             // 8: UDSRepAllSupDTCArgs_t *
    UDS_SRV_EVT_ReadDataByIdent,          // 9: UDSRDBIArgs_t *
    UDS_SRV_EVT_ReadMemByAddr,            // 10: UDSReadMemByAddrArgs_t *
    UDS_SRV_EVT_CommCtrl,                 // 11: UDSCommCtrlArgs_t *
    UDS_SRV_EVT_SecAccessRequestSeed,     // 12: UDSSecAccessRequestSeedArgs_t *
    UDS_SRV_EVT_SecAccessValidateKey,     // 13: UDSSecAccessValidateKeyArgs_t *
    UDS_SRV_EVT_WriteDataByIdent,         // 14: UDSWDBIArgs_t *
    UDS_SRV_EVT_RoutineCtrl,              // 15: UDSRoutineCtrlArgs_t*
    UDS_SRV_EVT_RequestDownload,          // 16: UDSRequestDownloadArgs_t*
    UDS_SRV_EVT_RequestUpload,            // 17: UDSRequestUploadArgs_t *
    UDS_SRV_EVT_TransferData,             // 18: UDSTransferDataArgs_t *
    UDS_SRV_EVT_RequestTransferExit,      // 19: UDSRequestTransferExitArgs_t *
    UDS_SRV_EVT_ControlDTC,               // 20: uint8_t
    UDS_SRV_EVT_SessionTimeout,           // 21: NULL
    UDS_SRV_EVT_DoScheduledReset,         // 22: enum UDSEcuResetType *
    UDS_SRV_EVT_Err,                      // 23: UDSErr_t *
    UDS_EVT_IDLE,                         // 24
    UDS_EVT_RESP_RECV,                    // 25
};

typedef int UDSServerEvent_t;
typedef UDSServerEvent_t UDSEvent_t;

typedef enum
{
    UDS_ERR = -1,                 // 通用错误
    UDS_OK = 0,                   // 成功
    UDS_ERR_TIMEOUT,              // 请求超时
    UDS_ERR_NEG_RESP,             // 否定响应
    UDS_ERR_DID_MISMATCH,         // 响应DID对不上期待的DID
    UDS_ERR_SID_MISMATCH,         // 请求和响应SID对不上
    UDS_ERR_SUBFUNCTION_MISMATCH, // 请求和响应SubFunction对不上
    UDS_ERR_TPORT,                // 传输层错误
    UDS_ERR_FILE_IO,              // 文件IO错误
    UDS_ERR_RESP_TOO_SHORT,       // 响应太短
    UDS_ERR_BUFSIZ,               // 缓冲器不够大
    UDS_ERR_INVALID_ARG,          // 参数不对、没发
    UDS_ERR_BUSY,                 // 正在忙、没发
} UDSErr_t;

typedef enum
{
    UDSSeqStateDone = 0,
    UDSSeqStateRunning = 1,
    UDSSeqStateGotoNext = 2,
} UDSSeqState_t;

enum UDSDiagnosticSessionType
{
    kDefaultSession = 0x01,
    kProgrammingSession = 0x02,
    kExtendedDiagnostic = 0x03,
    kSafetySystemDiagnostic = 0x04,
};

typedef enum
{
    kPositiveResponse = 0,
    kGeneralReject = 0x10,
    kServiceNotSupported = 0x11,
    kSubFunctionNotSupported = 0x12,
    kIncorrectMessageLengthOrInvalidFormat = 0x13,
    kResponseTooLong = 0x14,
    kBusyRepeatRequest = 0x21,
    kConditionsNotCorrect = 0x22,
    kRequestSequenceError = 0x24,
    kNoResponseFromSubnetComponent = 0x25,
    kFailurePreventsExecutionOfRequestedAction = 0x26,
    kRequestOutOfRange = 0x31,
    kSecurityAccessDenied = 0x33,
    kInvalidKey = 0x35,
    kExceedNumberOfAttempts = 0x36,
    kRequiredTimeDelayNotExpired = 0x37,
    kUploadDownloadNotAccepted = 0x70,
    kTransferDataSuspended = 0x71,
    kGeneralProgrammingFailure = 0x72,
    kWrongBlockSequenceCounter = 0x73,
    kRequestCorrectlyReceived_ResponsePending = 0x78,
    kSubFunctionNotSupportedInActiveSession = 0x7E,
    kServiceNotSupportedInActiveSession = 0x7F,
    kRpmTooHigh = 0x81,
    kRpmTooLow = 0x82,
    kEngineIsRunning = 0x83,
    kEngineIsNotRunning = 0x84,
    kEngineRunTimeTooLow = 0x85,
    kTemperatureTooHigh = 0x86,
    kTemperatureTooLow = 0x87,
    kVehicleSpeedTooHigh = 0x88,
    kVehicleSpeedTooLow = 0x89,
    kThrottlePedalTooHigh = 0x8A,
    kThrottlePedalTooLow = 0x8B,
    kTransmissionRangeNotInNeutral = 0x8C,
    kTransmissionRangeNotInGear = 0x8D,
    kISOSAEReserved = 0x8E,
    kBrakeSwitchNotClosed = 0x8F,
    kShifterLeverNotInPark = 0x90,
    kTorqueConverterClutchLocked = 0x91,
    kVoltageTooHigh = 0x92,
    kVoltageTooLow = 0x93,
    kSubModuleNotRespond = 0xF0, // YuanFeng Custom Nrc
} UDS_NRC;

/**
 * @brief LEV_RT_
 * @addtogroup ecuReset_0x11
 */
enum UDSECUResetType
{
    kHardReset = 1,
    kKeyOffOnReset = 2,
    kSoftReset = 3,
    kEnableRapidPowerShutDown = 4,
    kDisableRapidPowerShutDown = 5,
};

typedef uint8_t UDSECUReset_t;

/**
 * @addtogroup securityAccess_0x27
 */
enum UDSSecurityAccessType
{
    kRequestSeed = 0x01,
    kSendKey = 0x02,
};

/**
 * @addtogroup communicationControl_0x28
 */
enum UDSCommunicationControlType
{
    kEnableRxAndTx = 0,
    kEnableRxAndDisableTx = 1,
    kDisableRxAndEnableTx = 2,
    kDisableRxAndTx = 3,
};

/**
 * @addtogroup communicationControl_0x28
 */
enum UDSCommunicationType
{
    kNormalCommunicationMessages = 0x1,
    kNetworkManagementCommunicationMessages = 0x2,
    kNetworkManagementCommunicationMessagesAndNormalCommunicationMessages = 0x3,
};

/**
 * @addtogroup routineControl_0x31
 */
enum RoutineControlType
{
    kStartRoutine = 1,
    kStopRoutine = 2,
    kRequestRoutineResults = 3,
};

/**
 * @addtogroup controlDTCSetting_0x85
 */
enum DTCSettingType
{
    kDTCSettingON = 0x01,
    kDTCSettingOFF = 0x02,
};

// ISO-14229-1:2013 Table 2
#define UDS_MAX_DIAGNOSTIC_SERVICES 0x7F

#define UDS_RESPONSE_SID_OF(request_sid) (request_sid + 0x40)
#define UDS_REQUEST_SID_OF(response_sid) (response_sid - 0x40)

#define UDS_NEG_RESP_LEN 3U
#define UDS_0X10_REQ_LEN 2U
#define UDS_0X10_RESP_LEN 6U
#define UDS_0X11_REQ_MIN_LEN 2U
#define UDS_0X11_RESP_BASE_LEN 2U
#define UDS_0X14_REQ_MIN_LEN 4U
#define UDS_0X14_RESP_LEN 1U
#define UDS_0X1901_REQ_LEN 3U
#define UDS_0X1901_RESP_LEN 6U
#define UDS_0X1902_REQ_LEN 3U
#define UDS_0X1903_REQ_LEN 2U
#define UDS_0X1904_REQ_BASE_LEN 6U
#define UDS_0X1906_REQ_BASE_LEN 6U
#define UDS_0X190A_REQ_BASE_LEN 2U
#define UDS_0X23_REQ_MIN_LEN 4U
#define UDS_0X23_RESP_BASE_LEN 1U
#define UDS_0X22_RESP_BASE_LEN 1U
#define UDS_0X27_REQ_BASE_LEN 2U
#define UDS_0X27_RESP_BASE_LEN 2U
#define UDS_0X28_REQ_BASE_LEN 3U
#define UDS_0X28_REQ_MIN_LEN 2U
#define UDS_0X28_RESP_LEN 2U
#define UDS_0X2E_REQ_BASE_LEN 3U
#define UDS_0X2E_REQ_MIN_LEN 4U
#define UDS_0X2E_RESP_LEN 3U
#define UDS_0X31_REQ_MIN_LEN 4U
#define UDS_0X31_RESP_MIN_LEN 4U
#define UDS_0X34_REQ_BASE_LEN 3U
#define UDS_0X34_RESP_BASE_LEN 2U
#define UDS_0X35_REQ_BASE_LEN 3U
#define UDS_0X35_RESP_BASE_LEN 2U
#define UDS_0X36_REQ_BASE_LEN 2U
#define UDS_0X36_RESP_BASE_LEN 2U
#define UDS_0X37_REQ_BASE_LEN 1U
#define UDS_0X37_RESP_BASE_LEN 1U
#define UDS_0X3E_REQ_MIN_LEN 2U
#define UDS_0X3E_REQ_MAX_LEN 2U
#define UDS_0X3E_RESP_LEN 2U
#define UDS_0X85_REQ_BASE_LEN 2U
#define UDS_0X85_RESP_LEN 2U

enum UDSDiagnosticServiceId
{
    kSID_DIAGNOSTIC_SESSION_CONTROL = 0x10,
    kSID_ECU_RESET = 0x11,
    kSID_CLEAR_DIAGNOSTIC_INFORMATION = 0x14,
    kSID_READ_DTC_INFORMATION = 0x19,
    kSID_READ_DATA_BY_IDENTIFIER = 0x22,
    kSID_READ_MEMORY_BY_ADDRESS = 0x23,
    kSID_READ_SCALING_DATA_BY_IDENTIFIER = 0x24,
    kSID_SECURITY_ACCESS = 0x27,
    kSID_COMMUNICATION_CONTROL = 0x28,
    kSID_READ_PERIODIC_DATA_BY_IDENTIFIER = 0x2A,
    kSID_DYNAMICALLY_DEFINE_DATA_IDENTIFIER = 0x2C,
    kSID_WRITE_DATA_BY_IDENTIFIER = 0x2E,
    kSID_INPUT_CONTROL_BY_IDENTIFIER = 0x2F,
    kSID_ROUTINE_CONTROL = 0x31,
    kSID_REQUEST_DOWNLOAD = 0x34,
    kSID_REQUEST_UPLOAD = 0x35,
    kSID_TRANSFER_DATA = 0x36,
    kSID_REQUEST_TRANSFER_EXIT = 0x37,
    kSID_REQUEST_FILE_TRANSFER = 0x38,
    kSID_WRITE_MEMORY_BY_ADDRESS = 0x3D,
    kSID_TESTER_PRESENT = 0x3E,
    KSID_ATE_UDS = 0x62,
    kSID_ACCESS_TIMING_PARAMETER = 0x83,
    kSID_SECURED_DATA_TRANSMISSION = 0x84,
    kSID_CONTROL_DTC_SETTING = 0x85,
    kSID_RESPONSE_ON_EVENT = 0x86,
};

/** Supported session level and security access level */
typedef struct
{
    uint8_t *securityLevelTable; /** table of Security Access Level: NULL means no security access! */
    uint16_t securityTableSize;  /** securityLevelTable[securityTableSize] */
} UdsSecurityLevel;

typedef struct
{
    uint8_t sid;
    uint8_t mark; /** bit0: 广播；bit1: 子功能。 @see SER_MARK_... */
    uint8_t sessionSupportMask;
    UdsSecurityLevel securitySupport;
} UdsServiceInfo_S;

enum UDSTpStatusFlags
{
    UDS_TP_IDLE = 0x00000000,
    UDS_TP_SEND_IN_PROGRESS = 0x00000001,
    UDS_TP_RECV_COMPLETE = 0x00000002,
};

typedef uint32_t UDSTpStatus_t;

typedef enum
{
    UDS_A_MTYPE_DIAG = 0,
    UDS_A_MTYPE_REMOTE_DIAG,
    UDS_A_MTYPE_SECURE_DIAG,
    UDS_A_MTYPE_SECURE_REMOTE_DIAG,
} UDS_A_Mtype_t;

typedef enum
{
    UDS_A_TA_TYPE_PHYSICAL = 0, // unicast (1:1)
    UDS_A_TA_TYPE_FUNCTIONAL,   // multicast
} UDS_A_TA_Type_t;

typedef uint8_t UDSTpAddr_t;

/**
 * @brief Service data unit (SDU)
 * @details data interface between the application layer and the transport layer
 */
typedef struct
{
    UDS_A_Mtype_t A_Mtype;     // message type (diagnostic, remote diagnostic, secure diagnostic, secure
                               // remote diagnostic)
    uint16_t A_SA;             // application source address
    uint16_t A_TA;             // application target address
    UDS_A_TA_Type_t A_TA_Type; // application target address type (physical or functional)
    uint16_t A_AE;             // application layer remote address
} UDSSDU_t;

/**
 * @brief Server request context
 */
typedef struct
{
    uint8_t *recv_buf;
    uint8_t *send_buf;
    uint16_t recv_len;
    uint16_t send_len;
    uint16_t send_buf_size;
    uint16_t recv_buf_size;
    UDSSDU_t info;
    bool isInprogress;
    bool isRecvComplete;
} UDSReq_t;

typedef struct
{
    uint8_t sw_major_version;
    uint8_t sw_minor_version;
    uint8_t sw_patch_version;
} UDS_VersionInfoType;

typedef struct UDSServer UDSServer_t;

typedef UDS_NRC (*UDSService)(UDSServer_t *srv, UDSReq_t *r);
typedef ISOTP_RET (*UDSSendCanFunPtr)(const uint32_t arbitration_id, const uint8_t *data, const uint8_t size);
typedef uint32_t (*UDSGetMsFunPtr)(void);
typedef void (*UDSPrintFuncPtr)(const char *format, ...);
typedef void (*UDSlockPtr)(void);
typedef UDS_NRC (*UDSEventCallbackPtr)(UDSServer_t *srv, int evt, const void *data);
#ifdef UDS_CONFIG_SECURITY_DELAY
typedef bool (*getSecurutyAuthFailCountPtr)(uint8_t *FailAuthCounter);
typedef bool (*setSecurutyAuthFailCountPtr)(uint8_t FailAuthCounter);
#endif
typedef UDSService (*UDSuserRegisterServicePtr)(uint8_t sid);

typedef struct
{
    IsoTpLink phys_link;
    IsoTpLink func_link;
    uint32_t ta;      /*transfer address*/
    uint32_t sa_phy;  /*Physical Addressing ID*/
    uint32_t sa_func; /*Functional  Addressing ID*/
} UDSISOTpC_t;

/* buff Configuration parameters */
typedef struct
{
    uint32_t ta;            /*transfer address*/
    uint32_t sa_phy;        /*Physical Source Address*/
    uint32_t sa_func;       /*Functional Source Address*/
    uint8_t *send_buf;      /*send buffer*/
    uint16_t send_buf_size; /*send buff size*/
    uint8_t *recv_buf;
    uint16_t recv_buf_size;
} BufCfg;

/*callback function Configuration parameters*/
typedef struct
{
    UDSSendCanFunPtr uds_send_can; /* send can message. should return ISOTP_RET_OK(0) when success.*/
    UDSGetMsFunPtr uds_get_ms;     /* get millisecond */
    UDSPrintFuncPtr print_func;    /* print function*/
    UDSlockPtr lock_tx;
    UDSlockPtr unlock_tx;
} BspCallbackCfg;

/*uds Service Related Configuration Parameters*/
typedef struct
{
    const UdsServiceInfo_S *udsSvrTab;                    /*uds service information including parameters such as : service supports 、 suppresses positive responses 、sessions 、 security levels*/
    uint16_t udsSvrNumber;                                /*number of udsSvrTab*/
    UDSEventCallbackPtr udsserver_event_callback;         /*the callback function of the service implemented in the port*/
    UDSuserRegisterServicePtr userRegisterService;        /*if you need to register the corresponding service you can add this function*/
    uint8_t UdsSecuritySeedLen;                           /*the size of Security Seed(0x27)*/
#ifdef UDS_CONFIG_SECURITY_DELAY
    uint8_t SecurityMaxAttemptsCount;                     /*the service will wait for a delay after the number of errors reaches the SecurityMaxAttemptsCount(0x27)*/
    uint8_t SecurityMaxReqSeedCount;                     /*the service will wait for a delay after the number of errors reaches consecutive "Request Seed" attempts.*/
    getSecurutyAuthFailCountPtr getSecurutyAuthFailCount; /*the function to get Securuty authentication Fail Count(0x27)*/
    setSecurutyAuthFailCountPtr setSecurutyAuthFailCount; /*the function to set Securuty authentication Fail Count(0x27)*/
    uint16_t auth_fail_delay_ms;                          /*Time to wait for the next secure access request, when the service 0x27 negative response code is 0x36*/
#endif
    uint16_t p2_server_ms;                                /*Time betw een client (tester) request and server (ECU) response*/
    uint16_t p2_star_server_ms;                           /*Enhanced timeout for the client to wait after the reception of a negative response message with response code 78 h*/
    uint16_t p2_pending_ms;
    uint16_t s3_server;                                   /*Session timeout; after timeout return to default-session*/
    uint16_t power_down_time_ms;                          /*Interval to perform a reset after receiving a 0x11 reset*/
} ServiceCfg;

struct UDSServer
{
    UDSISOTpC_t tp_cfg;
    /**
     * @brief \~chinese 服务器时间参数（毫秒） \~ Server time constants (milliseconds) \~
     */
    uint16_t p2_ms;              // Default P2_server_max timing supported by the server for
                                 // the activated diagnostic session.
    uint32_t p2_star_ms;         // Enhanced (NRC 0x78) P2_server_max supported by the
                                 // server for the activated diagnostic session.
    uint16_t p2_pending_ms;
    uint16_t s3_ms;              // Session timeout
    uint16_t power_down_time_ms; /*Interval to perform a reset after receiving a 0x11 reset*/
    uint16_t auth_fail_delay_ms; /*Time to wait for the next secure access request, when the service 0x27 negative response code is 0x36*/

    uint8_t ecuResetScheduled;           // nonzero indicates that an ECUReset has been scheduled
    uint32_t ecuResetTimer;              // for delaying resetting until a response
                                         // has been sent to the client
    uint32_t p2_timer;                   // for rate limiting server responses
    uint32_t s3_session_timeout_timer;   // indicates that diagnostic session has timed out
    uint32_t sec_access_auth_fail_timer; // brute-force hardening: rate limit security access
                                         // requests

    /**
     * @brief UDS-1-2013: Table 407 - 0x36 TransferData Supported negative
     * response codes requires that the server keep track of whether the
     * transfer is active
     */
    bool xferIsActive;
    // UDS-1-2013: 14.4.2.3, Table 404: The blockSequenceCounter parameter
    // value starts at 0x01
    uint8_t xferBlockSequenceCounter;
    uint16_t xferLastBlockSequenceCounter;
    uint32_t xferTotalBytes;      // total transfer size in bytes requested by the client
    uint32_t xferByteCounter;     // total number of bytes transferred
    uint32_t xferBlockLength;     // block length (convenience for the TransferData API)
    uint8_t sessionType;          // diagnostic session type (0x10)
    uint8_t securityLevel;        // SecurityAccess (0x27) level
    uint8_t requestSecurityLevel; // SecurityAccess (0x27) level
    uint8_t UdsSecuritySeedLen;
    uint8_t SecurityMaxAttemptsCount;
    uint8_t SecurityFailedAuthCount;
    uint8_t SecurityMaxReqSeedCount;
    uint8_t SecurityReqSeedCount;
    uint8_t commDisableMask;
    // bool RCRRP;             // set to true when user udsserver_event_callback returns 0x78 and false otherwise
    // bool requestInProgress; // set to true when a request has been processed but the response has
    //                         // not yet been sent
    uint8_t subSvrSupportPRMB;
    bool    SecurityAccessMask; // SecurityAccess (0x27) mask True: enable security authentication, False: disable security authentication
    // bool responsePend;
    // UDS-1 2013 defines the following conditions under which the server does not
    // process incoming requests:
    // - not ready to receive (Table A.1 0x78)
    // - not accepting request messages and not sending responses (9.3.1)
    //
    // when this variable is set to true, incoming ISO-TP data will not be processed.
    // bool notReadyToReceive;

    UDSReq_t r;
    UDSSendCanFunPtr uds_send_can;
    UDSGetMsFunPtr uds_get_ms;
    UDSPrintFuncPtr print_func;

    UDSEventCallbackPtr udsserver_event_callback;
#ifdef UDS_CONFIG_SECURITY_DELAY
    getSecurutyAuthFailCountPtr getSecurutyAuthFailCount;
    setSecurutyAuthFailCountPtr setSecurutyAuthFailCount;
#endif
    UDSuserRegisterServicePtr userRegisterService;
    const UdsServiceInfo_S *udsSvrTab;
    uint16_t udsSvrNumber; /** number of sidTable: sidTable[sidNbr] */
    uint8_t isSendPending;
    uint8_t isNeedResponse;
};

typedef struct
{
    const uint8_t type;  /*! requested diagnostic session type (enum UDSDiagnosticSessionType) */
    uint16_t p2_ms;      /*! optional: p2 timing override */
    uint32_t p2_star_ms; /*! optional: p2* timing override */
} UDSDiagSessCtrlArgs_t;

typedef struct
{
    const uint8_t type;           /**< \~chinese 客户端请求的复位类型 \~english reset type requested by client
                                     (enum UDSECUResetType) */
    uint32_t powerDownTimeMillis; /**< when this much time has elapsed after a kPositiveResponse, a
                                     UDS_SRV_EVT_DoScheduledReset will be issued */
} UDSECUResetArgs_t;

typedef struct
{
    const uint8_t HighByte;
    const uint8_t MidByte;
    const uint8_t LowByte;
} UDSGroupOfDTCArgs_t;

typedef struct
{
    const uint8_t DTCStatusMask;
    uint8_t DTCStatusAvailabilityMask;
    uint8_t DTCFormatIdentifier;
    uint8_t DTCCountHighByte;
    uint8_t DTCCountLowByte;
} UDSRepNumOfDTCByStaMaskArgs_t;

typedef struct
{
    const uint8_t DTCStatusMask;
    uint8_t DTCStatusAvailabilityMask;
    uint8_t (*copy)(UDSServer_t *srv, const void *src, uint16_t count); /*! function for copying data */
} UDSRepDTCByStaMaskArgs_t;

#if 0 /* IAR build failed */
typedef struct
{
} UDSRepDTCSnapIdArgs_t;
#endif

typedef struct
{
    const uint8_t DTCHighByte;
    const uint8_t DTCMiddleByte;
    const uint8_t DTCLowByte;
    const uint8_t DTCRecordNumber;
    uint8_t StausOfDTC;
    uint8_t (*copy)(UDSServer_t *srv, const void *src, uint16_t count); /*! function for copying data */
} UDSDTCSnapExtRecByDTCNumArgs_t;

typedef struct
{
    uint8_t DTCStatusAvailabilityMask;
    uint8_t (*copy)(UDSServer_t *srv, const void *src, uint16_t count); /*! function for copying data */
} UDSRepAllSupDTCArgs_t;

typedef struct
{
    const uint16_t dataId;                                              /*! RDBI Data Identifier */
    uint8_t (*copy)(UDSServer_t *srv, const void *src, uint16_t count); /*! function for copying data */
} UDSRDBIArgs_t;

typedef struct
{
    const void *memAddr;
    const uint32_t memSize;
    uint8_t (*copy)(UDSServer_t *srv, const void *src, uint16_t count); /*! function for copying data */
} UDSReadMemByAddrArgs_t;

typedef struct
{
    uint8_t ctrlType; /* enum UDSCommunicationControlType */
    uint8_t commType; /* enum UDSCommunicationType */
} UDSCommCtrlArgs_t;

typedef struct
{
    const uint8_t level;                                                  /*! requested security level */
    const uint8_t *const dataRecord;                                      /*! pointer to request data */
    const uint16_t len;                                                   /*! size of request data */
    uint8_t (*copySeed)(UDSServer_t *srv, const void *src, uint16_t len); /*! function for copying data */
} UDSSecAccessRequestSeedArgs_t;

typedef struct
{
    const uint8_t level;      /*! security level to be validated */
    const uint8_t *const key; /*! key sent by client */
    const uint16_t len;       /*! length of key */
} UDSSecAccessValidateKeyArgs_t;

typedef struct
{
    const uint16_t dataId;     /*! WDBI Data Identifier */
    const uint8_t *const data; /*! pointer to data */
    const uint16_t len;        /*! length of data */
} UDSWDBIArgs_t;

typedef struct
{
    const uint8_t ctrlType;      /*! routineControlType */
    const uint16_t id;           /*! routineIdentifier */
    const uint8_t *optionRecord; /*! optional data */
    const uint16_t len;          /*! length of optional data */
    uint8_t (*copyStatusRecord)(UDSServer_t *srv,
                                const void *src,
                                uint16_t len); /*! function for copying response data */
} UDSRoutineCtrlArgs_t;

typedef struct
{
    const void *addr;                   /*! requested address */
    const uint32_t size;                /*! requested download size */
    const uint8_t dataFormatIdentifier; /*! optional specifier for format of data */
    uint16_t maxNumberOfBlockLength;    /*! optional response: inform client how many data bytes to
                                           send in each    `TransferData` request */
} UDSRequestDownloadArgs_t;

typedef struct
{
    const void *addr;                   /*! requested address */
    const uint32_t size;                /*! requested download size */
    const uint8_t dataFormatIdentifier; /*! optional specifier for format of data */
    uint16_t maxNumberOfBlockLength;    /*! optional response: inform client how many data bytes to
                                           send in each    `TransferData` request */
} UDSRequestUploadArgs_t;

typedef struct
{
    const uint8_t *const data; /*! transfer data */
    const uint16_t len;        /*! transfer data length */
    const uint16_t maxRespLen; /*! don't send more than this many bytes with copyResponse */
    uint8_t (*copyResponse)(UDSServer_t *srv,
                            const void *src,
                            uint16_t len); /*! function for copying transfer data response data (optional) */
} UDSTransferDataArgs_t;

typedef struct
{
    const uint8_t *const data; /*! request data */
    const uint16_t len;        /*! request data length */
    uint8_t (*copyResponse)(UDSServer_t *srv,
                            const void *src,
                            uint16_t len); /*! function for copying response data (optional) */
} UDSRequestTransferExitArgs_t;

typedef const uint8_t UDSDtcSettingType;
typedef uint8_t UDSResetServiceSettingType;
#endif