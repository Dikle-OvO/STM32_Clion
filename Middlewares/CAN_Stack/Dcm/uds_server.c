#include "uds_server.h"
#include "util_tick.h"

/** CanNm Version */
#define UDS_SW_MAJOR_VERSION 1
#define UDS_SW_MINOR_VERSION 0
#define UDS_SW_PATCH_VERSION 5

#define UNUSED(x) (void)(x)
#define UDS_ENABLE_DBG_PRINT 1
#if UDS_ENABLE_DBG_PRINT
#define PRINT_LOG(handle, format, ...)                                                \
    do                                                                                \
    {                                                                                 \
        if ((handle)->print_func)                                                     \
        {                                                                             \
            (handle)->print_func(format, ##__VA_ARGS__); \
        }                                                                             \
    } while (0)

#define PRINT_HEX(handle, data, size)                                  \
    do                                                                 \
    {                                                                  \
        if ((handle)->print_func)                                      \
        {                                                              \
            (handle)->print_func(" Hex: "); \
            for (uint32_t i = 0; i < size; ++i)                        \
            {                                                          \
                (handle)->print_func("%02X ", (data)[i]);              \
            }                                                          \
            (handle)->print_func("\n");                                \
        }                                                              \
    } while (0)

#else
#define PRINT_LOG(handle, format, ...)
#define PRINT_HEX(handle, data, size)
#endif
static UDSServer_t local_srv;

static inline uint8_t NegativeResponse(UDSReq_t *r, uint8_t response_code)
{
    r->send_buf[0] = 0x7F;
    r->send_buf[1] = r->recv_buf[0];
    r->send_buf[2] = response_code;
    r->send_len = UDS_NEG_RESP_LEN;
    return response_code;
}

void detectP2TimeAndResponsePending(UDSServer_t *srv)
{
    uint32_t tempT = srv->uds_get_ms();
    if (srv->p2_timer <= tempT ||((srv->p2_timer - tempT) <= srv->p2_ms))
    {
        /* tp层无缓存，发送0x78后立即发诊断响应会导致丢帧，所以0x78直接发送给dpur,(最优解应该是tp支持队列发送) */
        uint8_t pendbuf[8];
        memset(pendbuf, srv->tp_cfg.phys_link.config->frame_padding_char, 8);
        pendbuf[0] = 0x03;
        pendbuf[1] = 0x7F;
        pendbuf[2] = srv->r.recv_buf[0];
        pendbuf[3] = kRequestCorrectlyReceived_ResponsePending;
        srv->isSendPending = true;
        srv->isNeedResponse = true;
        if(ISOTP_RET_OK != srv->uds_send_can(srv->tp_cfg.phys_link.send_id, pendbuf, 8)){
            srv->isSendPending = false;
            srv->isNeedResponse  = false;
        }
        srv->p2_timer = tempT + srv->p2_pending_ms;
        srv->s3_session_timeout_timer = tempT + srv->s3_ms;
    }
    return;
}

static inline void NoResponse(UDSReq_t *r) { r->send_len = 0; }

static inline bool UDSTimeAfterCheck(uint32_t a, uint32_t b, uint32_t exTime)
{
    bool timeAfter = false;

    if (a > b)
    {
        if (b < exTime)
        {
            if ((a + exTime) > exTime)
            {
                timeAfter = true;
            }
        }
        else
        {
            timeAfter = true;
        }
    }
    else
    {
        if ((b >= exTime) && ((a + exTime) < b))
        {
            timeAfter = true;
        }
    }
    return timeAfter;
}

static uint8_t EmitEvent(UDSServer_t *srv, UDSServerEvent_t evt, void *data)
{
    if (srv->udsserver_event_callback)
    {
        return srv->udsserver_event_callback(srv, evt, data);
    }
    else
    {
        PRINT_LOG(srv, "Unhandled UDSServerEvent %d, srv.udsserver_event_callback not installed!\n", evt);
        return kGeneralReject;
    }
}

static void ResetTransfer(UDSServer_t *srv)
{
    srv->xferBlockSequenceCounter = 1;
    srv->xferLastBlockSequenceCounter = 0xFFFF;
    srv->xferByteCounter = 0;
    srv->xferTotalBytes = 0;
    srv->xferIsActive = false;
}

static void ResetSecurity(UDSServer_t *srv)
{
    srv->securityLevel = 0;
    srv->requestSecurityLevel = 0;
}

static void resetInternalVariable(UDSServer_t *srv)
{
    srv->sessionType = kDefaultSession;
    ResetSecurity(srv);
    ResetTransfer(srv);
}

static uint8_t _0x10_DiagnosticSessionControl(UDSServer_t *srv, UDSReq_t *r)
{
    if (r->recv_len < UDS_0X10_REQ_LEN)
    {
        return NegativeResponse(r, kIncorrectMessageLengthOrInvalidFormat);
    }

    uint8_t sessType = r->recv_buf[1] & 0x7F;

    UDSDiagSessCtrlArgs_t args = {
        .type = sessType,
        .p2_ms = srv->p2_ms,
        .p2_star_ms = srv->p2_star_ms,
    };

    uint8_t err = EmitEvent(srv, UDS_SRV_EVT_DiagSessCtrl, &args);

    if (kPositiveResponse != err)
    {
        return NegativeResponse(r, err);
    }

    srv->sessionType = sessType;
    ResetSecurity(srv);

    switch (sessType)
    {
    case kDefaultSession:
        resetInternalVariable(srv);
        break;
    case kProgrammingSession:
    case kExtendedDiagnostic:
    default:
        srv->s3_session_timeout_timer = srv->uds_get_ms() + srv->s3_ms;
        break;
    }

    r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_DIAGNOSTIC_SESSION_CONTROL);
    r->send_buf[1] = sessType;

    // UDS-1-2013: Table 29
    // resolution: 1ms
    r->send_buf[2] = args.p2_ms >> 8;
    r->send_buf[3] = args.p2_ms;

    // resolution: 10ms
    r->send_buf[4] = (args.p2_star_ms / 10) >> 8;
    r->send_buf[5] = args.p2_star_ms / 10;

    r->send_len = UDS_0X10_RESP_LEN;
    return kPositiveResponse;
}

void resetBegin(UDSServer_t *srv, uint8_t resetType, uint32_t resetTimer)
{
    srv->ecuResetScheduled = resetType;
    srv->ecuResetTimer = srv->uds_get_ms() + resetTimer;
}

static uint8_t _0x11_ECUReset(UDSServer_t *srv, UDSReq_t *r)
{
    uint8_t resetType = r->recv_buf[1] & 0x7F;

    if (r->recv_len < UDS_0X11_REQ_MIN_LEN)
    {
        return NegativeResponse(r, kIncorrectMessageLengthOrInvalidFormat);
    }

    UDSECUResetArgs_t args = {
        .type = resetType,
        .powerDownTimeMillis = srv->power_down_time_ms,
    };

    uint8_t err = EmitEvent(srv, UDS_SRV_EVT_EcuReset, &args);

    if (kPositiveResponse == err)
    {
        resetInternalVariable(srv);
        srv->ecuResetScheduled = resetType;
        srv->ecuResetTimer = srv->uds_get_ms() + args.powerDownTimeMillis;
    }
    else
    {
        return NegativeResponse(r, err);
    }

    r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_ECU_RESET);
    r->send_buf[1] = resetType;

    if (kEnableRapidPowerShutDown == resetType)
    {
        uint32_t powerDownTime = args.powerDownTimeMillis / 1000;
        if (powerDownTime > 255)
        {
            powerDownTime = 255;
        }
        r->send_buf[2] = powerDownTime;
        r->send_len = UDS_0X11_RESP_BASE_LEN + 1;
    }
    else
    {
        r->send_len = UDS_0X11_RESP_BASE_LEN;
    }
    return kPositiveResponse;
}

static uint8_t _0x14_ClearDiagnosticInformation(UDSServer_t *srv, UDSReq_t *r)
{
    if (r->recv_len < UDS_0X14_REQ_MIN_LEN)
    {
        return NegativeResponse(r, kIncorrectMessageLengthOrInvalidFormat);
    }

    UDSGroupOfDTCArgs_t args = {
        .HighByte = r->recv_buf[1],
        .MidByte = r->recv_buf[2],
        .LowByte = r->recv_buf[3],
    };

    uint8_t err = EmitEvent(srv, UDS_SRV_EVT_ClrDiagInfo, &args);

    if (kPositiveResponse != err)
    {
        return NegativeResponse(r, err);
    }

    r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_CLEAR_DIAGNOSTIC_INFORMATION);
    r->send_len = UDS_0X14_RESP_LEN;

    return kPositiveResponse;
}

static uint8_t safe_copy(UDSServer_t *srv, const void *src, uint16_t count)
{
    if (srv == NULL)
    {
        return kGeneralReject;
    }
    UDSReq_t *r = (UDSReq_t *)&srv->r;
    if (count <= r->send_buf_size - r->send_len)
    {
        memmove(r->send_buf + r->send_len, src, count);
        r->send_len += count;
        return kPositiveResponse;
    }
    return kResponseTooLong;
}

static uint8_t _0x1901_ReportNumberOfDTCByStatusMask(UDSServer_t *srv, UDSReq_t *r)
{

    UDSRepNumOfDTCByStaMaskArgs_t args = {
        .DTCFormatIdentifier = 0,
        .DTCStatusMask = r->recv_buf[2],
    };

    uint8_t err = EmitEvent(srv, UDS_SRV_EVT_RepNumOfDTCByStaMask, &args);

    if (kPositiveResponse != err)
    {
        return NegativeResponse(r, err);
    }

    r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_READ_DTC_INFORMATION);
    r->send_buf[1] = 0x1;
    r->send_buf[2] = args.DTCStatusAvailabilityMask;
    r->send_buf[3] = args.DTCFormatIdentifier;
    r->send_buf[4] = args.DTCCountHighByte;
    r->send_buf[5] = args.DTCCountLowByte;
    r->send_len = UDS_0X1901_RESP_LEN;

    return kPositiveResponse;
}

static uint8_t _0x1902_ReportDTCByStatusMask(UDSServer_t *srv, UDSReq_t *r)
{

    UDSRepDTCByStaMaskArgs_t args = {
        .DTCStatusMask = r->recv_buf[2],
        .DTCStatusAvailabilityMask = 0,
        .copy = safe_copy,
    };
    /*Optional output data starting at the 3 byte*/
    r->send_len = 3;

    uint8_t err = EmitEvent(srv, UDS_SRV_EVT_RepDTCByStaMask, &args);

    if (kPositiveResponse != err)
    {
        return NegativeResponse(r, err);
    }
    r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_READ_DTC_INFORMATION);
    r->send_buf[1] = 0x2;
    r->send_buf[2] = args.DTCStatusAvailabilityMask;

    return kPositiveResponse;
}

static uint8_t _0x1903_ReportDTCSnapshotIdentification(UDSServer_t *srv, UDSReq_t *r)
{
    UNUSED(srv);
    UNUSED(r);
    return kSubFunctionNotSupported;
}
static uint8_t _0x1904_ReportDTCSnapshotRecordByDTCNumber(UDSServer_t *srv, UDSReq_t *r)
{

    UDSDTCSnapExtRecByDTCNumArgs_t args = {
        .DTCHighByte = r->recv_buf[2],
        .DTCMiddleByte = r->recv_buf[3],
        .DTCLowByte = r->recv_buf[4],
        .DTCRecordNumber = r->recv_buf[5],
        .copy = safe_copy,
    };

    /*Optional output data starting at the 6 byte*/
    r->send_len = 6;

    uint8_t err = EmitEvent(srv, UDS_SRV_EVT_RepDTCSnapRecByDTCNum, &args);

    if (kPositiveResponse != err)
    {
        return NegativeResponse(r, err);
    }
    r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_READ_DTC_INFORMATION);
    r->send_buf[1] = 0x4;
    r->send_buf[2] = args.DTCHighByte;
    r->send_buf[3] = args.DTCMiddleByte;
    r->send_buf[4] = args.DTCLowByte;
    r->send_buf[5] = args.StausOfDTC;

    return kPositiveResponse;
}

static uint8_t _0x1906_ReportDTCExternalDataRecordByDTCNumber(UDSServer_t *srv, UDSReq_t *r)
{

    UDSDTCSnapExtRecByDTCNumArgs_t args = {
        .DTCHighByte = r->recv_buf[2],
        .DTCMiddleByte = r->recv_buf[3],
        .DTCLowByte = r->recv_buf[4],
        .DTCRecordNumber = r->recv_buf[5],
        .copy = safe_copy,
    };
    /*Optional output data starting at the 6 byte*/
    r->send_len = 6;

    uint8_t err = EmitEvent(srv, UDS_SRV_EVT_RepDTCExtDataRecByDTCNum, &args);

    if (kPositiveResponse != err)
    {
        return NegativeResponse(r, err);
    }
    r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_READ_DTC_INFORMATION);
    r->send_buf[1] = 0x6;
    r->send_buf[2] = args.DTCHighByte;
    r->send_buf[3] = args.DTCMiddleByte;
    r->send_buf[4] = args.DTCLowByte;
    r->send_buf[5] = args.StausOfDTC;
    return kPositiveResponse;
}
static uint8_t _0x190A_ReportAllSupportedDTC(UDSServer_t *srv, UDSReq_t *r)
{
    UDSRepAllSupDTCArgs_t args = {
        .copy = safe_copy,
    };

    /*Optional output data starting at the 3 byte*/
    r->send_len = 3;

    uint8_t err = EmitEvent(srv, UDS_SRV_EVT_RepAllSupDTC, &args);

    if (kPositiveResponse != err)
    {
        return NegativeResponse(r, err);
    }

    r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_READ_DTC_INFORMATION);
    r->send_buf[1] = 0x0A;
    r->send_buf[2] = args.DTCStatusAvailabilityMask;

    return kPositiveResponse;
}

static uint8_t _0x19_ReadDTCInformation(UDSServer_t *srv, UDSReq_t *r)
{
    uint8_t ret = kPositiveResponse;
    uint8_t subFunc = r->recv_buf[1] & 0x7F;

    if (r->recv_len < 2)
    {
        return NegativeResponse(r, kIncorrectMessageLengthOrInvalidFormat);
    }
    switch (subFunc)
    {
    case 0x01:
        ret = _0x1901_ReportNumberOfDTCByStatusMask(srv, r);
        break;
    case 0x02:
        ret = _0x1902_ReportDTCByStatusMask(srv, r);
        break;
    case 0x03:
        ret = _0x1903_ReportDTCSnapshotIdentification(srv, r);
        break;
    case 0x04:
        ret = _0x1904_ReportDTCSnapshotRecordByDTCNumber(srv, r);
        break;
    case 0x06:
        ret = _0x1906_ReportDTCExternalDataRecordByDTCNumber(srv, r);
        break;
    case 0x0A:
        ret = _0x190A_ReportAllSupportedDTC(srv, r);
        break;

    default:
        return NegativeResponse(r, kSubFunctionNotSupported);
        break;
    }

    if (kPositiveResponse != ret)
    {
        return NegativeResponse(r, ret);
    }

    return ret;
}

static uint8_t _0x22_ReadDataByIdentifier(UDSServer_t *srv, UDSReq_t *r)
{
    uint8_t numDIDs;
    uint16_t dataId = 0;
    uint8_t ret = kPositiveResponse;
    r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_READ_DATA_BY_IDENTIFIER);
    r->send_len = 1;

    if (0 != (r->recv_len - 1) % sizeof(uint16_t))
    {
        return NegativeResponse(r, kIncorrectMessageLengthOrInvalidFormat);
    }

    numDIDs = r->recv_len / sizeof(uint16_t);
    /* UDS-1 2013 Figure 20 Key 1 */
#if XIAOMI
    if (0 == numDIDs || numDIDs > 10)
    {
        return NegativeResponse(r, kIncorrectMessageLengthOrInvalidFormat);
    }
#else
    if (0 == numDIDs)
    {
        return NegativeResponse(r, kIncorrectMessageLengthOrInvalidFormat);
    }
#endif

    for (int did = 0; did < numDIDs; did++)
    {
        uint16_t idx = 1 + did * 2;
        dataId = (r->recv_buf[idx] << 8) + r->recv_buf[idx + 1];

        if (r->send_len + 3 > r->send_buf_size)
        {
            return NegativeResponse(r, kResponseTooLong);
        }
        uint8_t *copylocation = r->send_buf + r->send_len;
        copylocation[0] = dataId >> 8;
        copylocation[1] = dataId;
        r->send_len += 2;

        UDSRDBIArgs_t args = {
            .dataId = dataId,
            .copy = safe_copy,
        };

        ret = EmitEvent(srv, UDS_SRV_EVT_ReadDataByIdent, &args);

#if XIAOMI/* 非最后一个DID，则忽略0x31 */
        if((ret == kRequestOutOfRange) && (did != (numDIDs - 1)))
        {
            r->send_len -= 2;
            continue;
        }
#endif
        if (kPositiveResponse != ret)
        {
            return NegativeResponse(r, ret);
        }
    }

    return kPositiveResponse;
}

/**
 * @brief decode the addressAndLengthFormatIdentifier that appears in ReadMemoryByAddress (0x23),
 * DynamicallyDefineDataIdentifier (0x2C), RequestDownload (0X34)
 *
 * @param srv
 * @param buf pointer to addressAndDataLengthFormatIdentifier in recv_buf
 * @param memoryAddress the decoded memory address
 * @param memorySize the decoded memory size
 * @return uint8_t
 */
static uint8_t decodeAddressAndLength(UDSReq_t *r, uint8_t *const buf, void **memoryAddress, uint32_t *memorySize)
{
    uintptr_t tmp = 0;
    *memoryAddress = 0;
    *memorySize = 0;

    // assert(buf >= r->recv_buf && buf <= r->recv_buf + sizeof(r->recv_buf));

    if (r->recv_len < 3)
    {
        return NegativeResponse(r, kIncorrectMessageLengthOrInvalidFormat);
    }

    uint8_t memorySizeLength = (buf[0] & 0xF0) >> 4;
    uint8_t memoryAddressLength = buf[0] & 0x0F;

    if (memorySizeLength == 0 || memorySizeLength > sizeof(uint32_t))
    {
        return NegativeResponse(r, kRequestOutOfRange);
    }

    if (memoryAddressLength == 0 || memoryAddressLength > sizeof(uint32_t))
    {
        return NegativeResponse(r, kRequestOutOfRange);
    }

    if (buf + memorySizeLength + memoryAddressLength > r->recv_buf + r->recv_len)
    {
        return NegativeResponse(r, kIncorrectMessageLengthOrInvalidFormat);
    }

    for (int byteIdx = 0; byteIdx < memoryAddressLength; byteIdx++)
    {
        long long unsigned int byte = buf[1 + byteIdx];
        uint8_t shiftBytes = memoryAddressLength - 1 - byteIdx;
        tmp |= byte << (8 * shiftBytes);
    }
    *memoryAddress = (void *)tmp;

    for (int byteIdx = 0; byteIdx < memorySizeLength; byteIdx++)
    {
        uint8_t byte = buf[1 + memoryAddressLength + byteIdx];
        uint8_t shiftBytes = memorySizeLength - 1 - byteIdx;
        *memorySize |= (uint32_t)byte << (8 * shiftBytes);
    }
    return kPositiveResponse;
}

static uint8_t _0x23_ReadMemoryByAddress(UDSServer_t *srv, UDSReq_t *r)
{
    uint8_t ret = kPositiveResponse;
    void *address = 0;
    uint32_t length = 0;

    if (r->recv_len < UDS_0X23_REQ_MIN_LEN)
    {
        return NegativeResponse(r, kIncorrectMessageLengthOrInvalidFormat);
    }

    ret = decodeAddressAndLength(r, &r->recv_buf[1], &address, &length);
    if (kPositiveResponse != ret)
    {
        return NegativeResponse(r, ret);
    }

    UDSReadMemByAddrArgs_t args = {
        .memAddr = address,
        .memSize = length,
        .copy = safe_copy,
    };

    r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_READ_MEMORY_BY_ADDRESS);
    r->send_len = UDS_0X23_RESP_BASE_LEN;
    ret = EmitEvent(srv, UDS_SRV_EVT_ReadMemByAddr, &args);
    if (kPositiveResponse != ret)
    {
        return NegativeResponse(r, ret);
    }
    if (r->send_len != UDS_0X23_RESP_BASE_LEN + length)
    {
        return kGeneralProgrammingFailure;
    }
    return kPositiveResponse;
}
#ifdef UDS_CONFIG_SECURITY_DELAY
static inline bool SecurityAccessDelayIsEnable(UDSServer_t *srv)
{
    return ((srv->SecurityMaxAttemptsCount > 0) && (srv->auth_fail_delay_ms > 0));
}

void SecurityInit(UDSServer_t *srv)
{
    uint8_t count;
    if (SecurityAccessDelayIsEnable(srv))
    {
        srv->getSecurutyAuthFailCount(&count);
        if (count > srv->SecurityMaxAttemptsCount)
        {
            count = srv->SecurityMaxAttemptsCount;
        }
        srv->SecurityFailedAuthCount = count;
        if (srv->SecurityFailedAuthCount >= srv->SecurityMaxAttemptsCount)
        {
            srv->sec_access_auth_fail_timer = srv->uds_get_ms() + srv->auth_fail_delay_ms;
        }
    }
}

static bool SecurityAccessDelayHandle(UDSServer_t *srv)
{
    bool delay_active = false;
    if (SecurityAccessDelayIsEnable(srv))
    {
        if (srv->SecurityFailedAuthCount >= srv->SecurityMaxAttemptsCount)
        {
            srv->SecurityFailedAuthCount = srv->SecurityMaxAttemptsCount;
            srv->sec_access_auth_fail_timer = srv->uds_get_ms() + srv->auth_fail_delay_ms;
            delay_active = true;
        }
        detectP2TimeAndResponsePending(srv);
        srv->setSecurutyAuthFailCount(srv->SecurityFailedAuthCount);
    }
    return delay_active;
}

static void SecurityAccessDelayTimerExpired(UDSServer_t *srv)
{
    if (SecurityAccessDelayIsEnable(srv))
    {
        if (srv->SecurityFailedAuthCount >= srv->SecurityMaxAttemptsCount)
        {
            srv->SecurityFailedAuthCount--;
            detectP2TimeAndResponsePending(srv);
            srv->setSecurutyAuthFailCount(srv->SecurityFailedAuthCount);
        }
    }
}

static void SecurityAccessDelayClear(UDSServer_t *srv)
{
    if (srv->SecurityFailedAuthCount != 0)
    {
        srv->SecurityFailedAuthCount = 0;
        detectP2TimeAndResponsePending(srv);
        srv->setSecurutyAuthFailCount(0);
    }
}
#endif
static uint8_t _0x27_SecurityAccess(UDSServer_t *srv, UDSReq_t *r)
{
    uint8_t subFunction = r->recv_buf[1] & 0x7F;
    uint8_t response = kPositiveResponse;
    r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_SECURITY_ACCESS);
    r->send_buf[1] = subFunction;
    r->send_len = UDS_0X27_RESP_BASE_LEN;
    if (r->recv_len < UDS_0X27_REQ_BASE_LEN)
    {
        return NegativeResponse(r, kIncorrectMessageLengthOrInvalidFormat);
    }

    // Even: sendKey
    if (0 == subFunction % 2)
    {
        uint8_t requestedLevel = subFunction - 1;
        UDSSecAccessValidateKeyArgs_t args = {
            .level = requestedLevel,
            .key = &r->recv_buf[UDS_0X27_REQ_BASE_LEN],
            .len = r->recv_len - UDS_0X27_REQ_BASE_LEN,
        };

        response = EmitEvent(srv, UDS_SRV_EVT_SecAccessValidateKey, &args);
        /*NRC24 顺序不正确应在service里进行，但该响应码优先级较低*/
        if (srv->requestSecurityLevel != requestedLevel)
        {
            if (kInvalidKey == response || kPositiveResponse == response)
            {
                response = kRequestSequenceError;
            }
        }

        srv->requestSecurityLevel = 0;
        if (kPositiveResponse != response)
        {
#ifdef UDS_CONFIG_SECURITY_DELAY
            if (kInvalidKey == response)
            {
                srv->SecurityFailedAuthCount++;
                if(SecurityAccessDelayHandle(srv))
                {
                    response = kExceedNumberOfAttempts;
                }
            }
#endif
            return NegativeResponse(r, response);
        }
#ifdef UDS_CONFIG_SECURITY_DELAY
    SecurityAccessDelayClear(srv);
#endif
        srv->securityLevel = requestedLevel;
        r->send_len = UDS_0X27_RESP_BASE_LEN;
    }

    // Odd: requestSeed
    else
    {
        /* If a server supports security, but the requested security level is already unlocked when
        a SecurityAccess ‘requestSeed’ message is received, that server shall respond with a
        SecurityAccess ‘requestSeed’ positive response message service with a seed value equal to
        zero (0). The server shall never send an all zero seed for a given security level that is
        currently locked. The client shall use this method to determine if a server is locked for a
        particular security level by checking for a non-zero seed.
        */
#ifdef UDS_CONFIG_SECURITY_DELAY
        if (!(UDSTimeAfterCheck(srv->uds_get_ms(), srv->sec_access_auth_fail_timer, srv->auth_fail_delay_ms)))
        {
            return NegativeResponse(r, kRequiredTimeDelayNotExpired);
        }
        SecurityAccessDelayTimerExpired(srv);
#endif

        UDSSecAccessRequestSeedArgs_t args = {
            .level = subFunction,
            .dataRecord = &r->recv_buf[UDS_0X27_REQ_BASE_LEN],
            .len = r->recv_len - UDS_0X27_REQ_BASE_LEN,
            .copySeed = safe_copy,
        };

        response = EmitEvent(srv, UDS_SRV_EVT_SecAccessRequestSeed, &args);

        if (kPositiveResponse != response)
        {
            srv->requestSecurityLevel = 0;
            return NegativeResponse(r, response);
        }
        if (r->send_len <= UDS_0X27_RESP_BASE_LEN)
        { // no data was copied
            return NegativeResponse(r, kGeneralProgrammingFailure);
        }

        if (!srv->SecurityAccessMask || subFunction == srv->securityLevel)
        {
            // Table 52 sends a response of length 2. Use a preprocessor define if this needs
            // customizing by the user.
            UDSReq_t *r = (UDSReq_t *)&srv->r;
            r->send_len = UDS_0X27_RESP_BASE_LEN;
            memset(r->send_buf + r->send_len, 0, srv->UdsSecuritySeedLen);
            r->send_len += srv->UdsSecuritySeedLen;
            return kPositiveResponse;
        }

#ifdef UDS_CONFIG_SECURITY_DELAY
        if (subFunction == srv->requestSecurityLevel)
        {
            srv->SecurityFailedAuthCount++;
            if (SecurityAccessDelayHandle(srv))
            {
                srv->requestSecurityLevel = 0;
                return NegativeResponse(r, kRequiredTimeDelayNotExpired);
            }
        }
#endif
        srv->requestSecurityLevel = subFunction;
    }
    return kPositiveResponse;
}

static uint8_t _0x28_CommunicationControl(UDSServer_t *srv, UDSReq_t *r)
{
    uint8_t controlType = r->recv_buf[1] & 0x7F;
    uint8_t communicationType = r->recv_buf[2];

    if (r->recv_len < UDS_0X28_REQ_MIN_LEN)
    {
        return NegativeResponse(r, kIncorrectMessageLengthOrInvalidFormat);
    }

    UDSCommCtrlArgs_t args = {
        .ctrlType = controlType,
        .commType = communicationType,
    };

    uint8_t err = EmitEvent(srv, UDS_SRV_EVT_CommCtrl, &args);
    if (kPositiveResponse != err)
    {
        return NegativeResponse(r, err);
    }

    r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_COMMUNICATION_CONTROL);
    r->send_buf[1] = controlType;
    r->send_len = UDS_0X28_RESP_LEN;
    return kPositiveResponse;
}

static uint8_t _0x2E_WriteDataByIdentifier(UDSServer_t *srv, UDSReq_t *r)
{
    uint16_t dataLen = 0;
    uint16_t dataId = 0;
    uint8_t err = kPositiveResponse;

    /* UDS-1 2013 Figure 21 Key 1 */
    if (r->recv_len < UDS_0X2E_REQ_MIN_LEN)
    {
        return NegativeResponse(r, kIncorrectMessageLengthOrInvalidFormat);
    }

    dataId = (r->recv_buf[1] << 8) + r->recv_buf[2];
    dataLen = r->recv_len - UDS_0X2E_REQ_BASE_LEN;

    UDSWDBIArgs_t args = {
        .dataId = dataId,
        .data = &r->recv_buf[UDS_0X2E_REQ_BASE_LEN],
        .len = dataLen,
    };

    err = EmitEvent(srv, UDS_SRV_EVT_WriteDataByIdent, &args);
    if (kPositiveResponse != err)
    {
        return NegativeResponse(r, err);
    }

    r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_WRITE_DATA_BY_IDENTIFIER);
    r->send_buf[1] = dataId >> 8;
    r->send_buf[2] = dataId;
    r->send_len = UDS_0X2E_RESP_LEN;
    return kPositiveResponse;
}

static uint8_t _0x31_RoutineControl(UDSServer_t *srv, UDSReq_t *r)
{
    uint8_t err = kPositiveResponse;
    if (r->recv_len < UDS_0X31_REQ_MIN_LEN)
    {
        return NegativeResponse(r, kIncorrectMessageLengthOrInvalidFormat);
    }

    uint8_t routineControlType = r->recv_buf[1] & 0x7F;
    uint16_t routineIdentifier = (r->recv_buf[2] << 8) + r->recv_buf[3];

    UDSRoutineCtrlArgs_t args = {
        .ctrlType = routineControlType,
        .id = routineIdentifier,
        .optionRecord = &r->recv_buf[UDS_0X31_REQ_MIN_LEN],
        .len = r->recv_len - UDS_0X31_REQ_MIN_LEN,
        .copyStatusRecord = safe_copy,
    };

    r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_ROUTINE_CONTROL);
    r->send_buf[1] = routineControlType;
    r->send_buf[2] = routineIdentifier >> 8;
    r->send_buf[3] = routineIdentifier;
    r->send_len = UDS_0X31_RESP_MIN_LEN;

    err = EmitEvent(srv, UDS_SRV_EVT_RoutineCtrl, &args);
    if (kPositiveResponse != err)
    {
        return NegativeResponse(r, err);
    }
    return kPositiveResponse;
}

static uint8_t _0x34_RequestDownload(UDSServer_t *srv, UDSReq_t *r)
{
    uint8_t err;
    void *memoryAddress = 0;
    uint32_t memorySize = 0;

    if (srv->xferIsActive)
    {
        return NegativeResponse(r, kConditionsNotCorrect);
    }

    if (r->recv_len < UDS_0X34_REQ_BASE_LEN)
    {
        return NegativeResponse(r, kIncorrectMessageLengthOrInvalidFormat);
    }

    err = decodeAddressAndLength(r, &r->recv_buf[2], &memoryAddress, &memorySize);
    if (kPositiveResponse != err)
    {
        return NegativeResponse(r, err);
    }

    UDSRequestDownloadArgs_t args = {
        .addr = memoryAddress,
        .size = memorySize,
        .dataFormatIdentifier = r->recv_buf[1],
        .maxNumberOfBlockLength = r->recv_buf_size,
    };

    err = EmitEvent(srv, UDS_SRV_EVT_RequestDownload, &args);

    if (args.maxNumberOfBlockLength < 3)
    {
        PRINT_LOG(srv, "%s", "ERROR: maxNumberOfBlockLength too short\n");
        return NegativeResponse(r, kGeneralProgrammingFailure);
    }

    if (kPositiveResponse != err)
    {
        return NegativeResponse(r, err);
    }

    ResetTransfer(srv);
    srv->xferIsActive = true;
    srv->xferTotalBytes = memorySize;
    srv->xferBlockLength = args.maxNumberOfBlockLength;

    // ISO-14229-1:2013 Table 401:
    uint8_t lengthFormatIdentifier = sizeof(args.maxNumberOfBlockLength) << 4;

    /* ISO-14229-1:2013 Table 396: maxNumberOfBlockLength
    This parameter is used by the requestDownload positive response message to
    inform the client how many data bytes (maxNumberOfBlockLength) to include in
    each TransferData request message from the client. This length reflects the
    complete message length, including the service identifier and the
    data-parameters present in the TransferData request message.
    */
    if (args.maxNumberOfBlockLength > r->recv_buf_size)
    {
        args.maxNumberOfBlockLength = r->recv_buf_size;
    }

    r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_REQUEST_DOWNLOAD);
    r->send_buf[1] = lengthFormatIdentifier;
    for (uint8_t idx = 0; idx < sizeof(args.maxNumberOfBlockLength); idx++)
    {
        uint8_t shiftBytes = sizeof(args.maxNumberOfBlockLength) - 1 - idx;
        uint8_t byte = args.maxNumberOfBlockLength >> (shiftBytes * 8);
        r->send_buf[UDS_0X34_RESP_BASE_LEN + idx] = byte;
    }
    r->send_len = UDS_0X34_RESP_BASE_LEN + sizeof(args.maxNumberOfBlockLength);
    return kPositiveResponse;
}

static uint8_t _0x35_RequestUpload(UDSServer_t *srv, UDSReq_t *r)
{
    uint8_t err;
    void *memoryAddress = 0;
    uint32_t memorySize = 0;

    if (srv->xferIsActive)
    {
        return NegativeResponse(r, kConditionsNotCorrect);
    }

    if (r->recv_len < UDS_0X35_REQ_BASE_LEN)
    {
        return NegativeResponse(r, kIncorrectMessageLengthOrInvalidFormat);
    }

    err = decodeAddressAndLength(r, &r->recv_buf[2], &memoryAddress, &memorySize);
    if (kPositiveResponse != err)
    {
        return NegativeResponse(r, err);
    }

    UDSRequestUploadArgs_t args = {
        .addr = memoryAddress,
        .size = memorySize,
        .dataFormatIdentifier = r->recv_buf[1],
        .maxNumberOfBlockLength = r->recv_buf_size,
    };

    err = EmitEvent(srv, UDS_SRV_EVT_RequestUpload, &args);

    if (args.maxNumberOfBlockLength < 3)
    {
        PRINT_LOG(srv, "%s", "ERROR: maxNumberOfBlockLength too short\n");
        return NegativeResponse(r, kGeneralProgrammingFailure);
    }

    if (kPositiveResponse != err)
    {
        return NegativeResponse(r, err);
    }

    ResetTransfer(srv);
    srv->xferIsActive = true;
    srv->xferTotalBytes = memorySize;
    srv->xferBlockLength = args.maxNumberOfBlockLength;

    uint8_t lengthFormatIdentifier = sizeof(args.maxNumberOfBlockLength) << 4;

    r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_REQUEST_UPLOAD);
    r->send_buf[1] = lengthFormatIdentifier;
    for (uint8_t idx = 0; idx < sizeof(args.maxNumberOfBlockLength); idx++)
    {
        uint8_t shiftBytes = sizeof(args.maxNumberOfBlockLength) - 1 - idx;
        uint8_t byte = args.maxNumberOfBlockLength >> (shiftBytes * 8);
        r->send_buf[UDS_0X35_RESP_BASE_LEN + idx] = byte;
    }
    r->send_len = UDS_0X35_RESP_BASE_LEN + sizeof(args.maxNumberOfBlockLength);
    return kPositiveResponse;
}

static uint8_t _0x36_TransferData(UDSServer_t *srv, UDSReq_t *r)
{
    uint8_t err = kPositiveResponse;
    uint16_t request_data_len = r->recv_len - UDS_0X36_REQ_BASE_LEN;
    uint8_t blockSequenceCounter = 0;

    if (!srv->xferIsActive)
    {
        return NegativeResponse(r, kRequestSequenceError);
    }

    if (r->recv_len < UDS_0X36_REQ_BASE_LEN)
    {
        err = kIncorrectMessageLengthOrInvalidFormat;
        goto fail;
    }

    blockSequenceCounter = r->recv_buf[1];

    r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_TRANSFER_DATA);
    r->send_buf[1] = blockSequenceCounter;
    r->send_len = UDS_0X36_RESP_BASE_LEN;

    // if (!srv->RCRRP) {
    if (blockSequenceCounter != srv->xferBlockSequenceCounter)
    {
        if (blockSequenceCounter == srv->xferLastBlockSequenceCounter)
        {
            return kPositiveResponse;
        }
        else
        {
            err = kWrongBlockSequenceCounter;
            goto fail;
        }
    }
    else
    {
        srv->xferLastBlockSequenceCounter = srv->xferBlockSequenceCounter++;
    }
    // }

    if (srv->xferByteCounter + request_data_len > srv->xferTotalBytes)
    {
#if XIAOMI
        err = kGeneralProgrammingFailure;
#else
        err = kTransferDataSuspended;
#endif
        goto fail;
    }

    {
        UDSTransferDataArgs_t args = {
            .data = &r->recv_buf[UDS_0X36_REQ_BASE_LEN],
            .len = r->recv_len - UDS_0X36_REQ_BASE_LEN,
            .maxRespLen = srv->xferBlockLength - UDS_0X36_RESP_BASE_LEN,
            .copyResponse = safe_copy,
        };

        err = EmitEvent(srv, UDS_SRV_EVT_TransferData, &args);

        switch (err)
        {
        case kPositiveResponse:
            srv->xferByteCounter += request_data_len;
            return kPositiveResponse;
        case kRequestCorrectlyReceived_ResponsePending:
            return NegativeResponse(r, kRequestCorrectlyReceived_ResponsePending);
        default:
            goto fail;
        }
    }

fail:
    ResetTransfer(srv);
    return NegativeResponse(r, err);
}

static uint8_t _0x37_RequestTransferExit(UDSServer_t *srv, UDSReq_t *r)
{
    uint8_t err = kPositiveResponse;

    if (!srv->xferIsActive)
    {
        return NegativeResponse(r, kRequestSequenceError);
    }

    r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_REQUEST_TRANSFER_EXIT);
    r->send_len = UDS_0X37_RESP_BASE_LEN;

    UDSRequestTransferExitArgs_t args = {
        .data = &r->recv_buf[UDS_0X37_REQ_BASE_LEN],
        .len = r->recv_len - UDS_0X37_REQ_BASE_LEN,
        .copyResponse = safe_copy,
    };

    err = EmitEvent(srv, UDS_SRV_EVT_RequestTransferExit, &args);

    switch (err)
    {
    case kPositiveResponse:
        ResetTransfer(srv);
        return kPositiveResponse;
    case kRequestCorrectlyReceived_ResponsePending:
        return NegativeResponse(r, kRequestCorrectlyReceived_ResponsePending);
    default:
        ResetTransfer(srv);
        return NegativeResponse(r, err);
    }
}

static uint8_t _0x3E_TesterPresent(UDSServer_t *srv, UDSReq_t *r)
{
    if ((r->recv_len < UDS_0X3E_REQ_MIN_LEN)
        || (((r->recv_buf[1] == 0x00) || (r->recv_buf[1] == 0x80)) && (r->recv_len > UDS_0X3E_REQ_MAX_LEN)))
    {
        return NegativeResponse(r, kIncorrectMessageLengthOrInvalidFormat);
    }
    uint8_t zeroSubFunction = r->recv_buf[1];

    switch (zeroSubFunction)
    {
    case 0x00:
    case 0x80:
        srv->s3_session_timeout_timer = srv->uds_get_ms() + srv->s3_ms;
        r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_TESTER_PRESENT);
        r->send_buf[1] = 0x00;
        r->send_len = UDS_0X3E_RESP_LEN;
        return kPositiveResponse;
    default:
        return NegativeResponse(r, kSubFunctionNotSupported);
    }
}

static uint8_t _0x85_ControlDTCSetting(UDSServer_t *srv, UDSReq_t *r)
{
    UDSDtcSettingType dtcSettingType = r->recv_buf[1] & 0x7F;
    if (r->recv_len < UDS_0X85_REQ_BASE_LEN)
    {
        return NegativeResponse(r, kIncorrectMessageLengthOrInvalidFormat);
    }

    uint8_t err = kPositiveResponse;
    err = EmitEvent(srv, UDS_SRV_EVT_ControlDTC, (void *)&dtcSettingType);

    if (err != kPositiveResponse)
    {
        return NegativeResponse(r, err);
    }
    r->send_buf[0] = UDS_RESPONSE_SID_OF(kSID_CONTROL_DTC_SETTING);
    r->send_buf[1] = dtcSettingType;
    r->send_len = UDS_0X85_RESP_LEN;
    return kPositiveResponse;
}

/**
 * @brief Get the internal service handler matching the given SID.
 * @param sid
 * @return pointer to UDSService or NULL if no match
 */
static UDSService getServiceForSID(uint8_t sid)
{
    UDSService userHandle = local_srv.userRegisterService(sid);
    if (userHandle != NULL)
        return userHandle;
    switch (sid)
    {
    case kSID_DIAGNOSTIC_SESSION_CONTROL:
        return (UDSService)_0x10_DiagnosticSessionControl;
    case kSID_ECU_RESET:
        return (UDSService)_0x11_ECUReset;
    case kSID_CLEAR_DIAGNOSTIC_INFORMATION:
        return (UDSService)_0x14_ClearDiagnosticInformation;
    case kSID_READ_DTC_INFORMATION:
        return (UDSService)_0x19_ReadDTCInformation;
    case kSID_READ_DATA_BY_IDENTIFIER:
        return (UDSService)_0x22_ReadDataByIdentifier;
    case kSID_READ_MEMORY_BY_ADDRESS:
        return (UDSService)_0x23_ReadMemoryByAddress;
    case kSID_READ_SCALING_DATA_BY_IDENTIFIER:
        return NULL;
    case kSID_SECURITY_ACCESS:
        return (UDSService)_0x27_SecurityAccess;
    case kSID_COMMUNICATION_CONTROL:
        return (UDSService)_0x28_CommunicationControl;
    case kSID_READ_PERIODIC_DATA_BY_IDENTIFIER:
        return NULL;
    case kSID_DYNAMICALLY_DEFINE_DATA_IDENTIFIER:
        return NULL;
    case kSID_WRITE_DATA_BY_IDENTIFIER:
        return (UDSService)_0x2E_WriteDataByIdentifier;
    case kSID_INPUT_CONTROL_BY_IDENTIFIER:
        return NULL;
    case kSID_ROUTINE_CONTROL:
        return (UDSService)_0x31_RoutineControl;
    case kSID_REQUEST_DOWNLOAD:
        return (UDSService)_0x34_RequestDownload;
    case kSID_REQUEST_UPLOAD:
        return (UDSService)_0x35_RequestUpload;
    case kSID_TRANSFER_DATA:
        return (UDSService)_0x36_TransferData;
    case kSID_REQUEST_TRANSFER_EXIT:
        return (UDSService)_0x37_RequestTransferExit;
    case kSID_REQUEST_FILE_TRANSFER:
        return NULL;
    case kSID_WRITE_MEMORY_BY_ADDRESS:
        return NULL;
    case kSID_TESTER_PRESENT:
        return (UDSService)_0x3E_TesterPresent;
    case kSID_ACCESS_TIMING_PARAMETER:
        return NULL;
    case kSID_SECURED_DATA_TRANSMISSION:
        return NULL;
    case kSID_CONTROL_DTC_SETTING:
        return (UDSService)_0x85_ControlDTCSetting;
    case kSID_RESPONSE_ON_EVENT:
        return NULL;
    default:
        return NULL;
    }
}

/**
 * @brief Call the service if it exists, modifying the response if the spec calls for it.
 * @note see UDS-1 2013 7.5.5 Pseudo code example of server response behavior
 *
 * @param srv
 * @param addressingScheme
 */
/************************************
 * @section other function          *
 ************************************/

/**
 * @fn     UdsLookupService()
 * @brief  查找服务
 * @param  sid          [IN]  service id
 * @param  p_cfg        [IN]  config data, including service table
 * @param  p_service    [OUT] pointer to the found service @note This pointer cannot be NULL!
 * @retval None
 */
bool UdsLookupService(uint8_t sid, UDSServer_t *srv, const UdsServiceInfo_S **p_serviceInfo)
{
    uint16_t i;
    bool found = false;

    for (i = 0; i < srv->udsSvrNumber; i++)
    {
        if (sid == srv->udsSvrTab[i].sid)
        {
            { /* if (p_service) { */
                *p_serviceInfo = &srv->udsSvrTab[i];
                found = true;
            }

            break;
        }
    }

    return found;
}

/**
 * @fn     UdsCheckSessionLevel()
 * @brief  检查会话级别
 * @param  session  [IN] session to be checked
 * @param  p_level  [IN] pointer to the level information @note This pointer cannot be NULL!
 * @retval None
 */
bool UdsCheckSessionLevel(uint8_t session, const UdsServiceInfo_S *p_serviceInfo)
{
    return (p_serviceInfo->sessionSupportMask & (1 << (session - 1)));
}

/**
 * @fn     UdsCheckSecurityLevel()
 * @brief  检查安全访问级别
 * @param  security_level   [IN] security level to be checked
 * @param  p_level          [IN] pointer to the level information @note This pointer cannot be NULL!
 * @retval None
 */
bool UdsCheckSecurityLevel(uint8_t security_level, const UdsServiceInfo_S *p_serviceInfo)
{
    bool ret = false;
    if ((!p_serviceInfo->securitySupport.securityLevelTable) || (0 == p_serviceInfo->securitySupport.securityTableSize))
    {
        ret = true; /** 不需要解锁 "Security Access Level", 因此返回 TRUE. */
    }
    else
    {
        for (int i = 0; i < p_serviceInfo->securitySupport.securityTableSize; i++)
        {
            if (security_level == p_serviceInfo->securitySupport.securityLevelTable[i])
            {
                ret = true;
                break;
            }
        }
    }
    return ret;
}

void UdsServerWithSubFuncNotSuppressPosRsp(UDSServer_t *srv) { srv->subSvrSupportPRMB = false; }

static uint8_t evaluateServiceResponse(UDSServer_t *srv, UDSReq_t *r)
{
    uint8_t response = kPositiveResponse;
    uint8_t sid = r->recv_buf[0];
    UDSService service = getServiceForSID(sid);
    PRINT_LOG(srv, "reveied SID:%0x\n", sid);

    const UdsServiceInfo_S *p_serviceInfo = NULL;
    srv->subSvrSupportPRMB = r->recv_buf[1] & SUPPRESS_POSITIVE_RESP_BIT; /*解决主服务抑制支持但子服务不支持*/
    if (service && srv->udsserver_event_callback && UdsLookupService(sid, srv, &p_serviceInfo))
    {
        if ((r->info.A_TA_Type == UDS_A_TA_TYPE_FUNCTIONAL) && (!(p_serviceInfo->mark & SER_MARK_BROADCAST)))
        {
            /** @note If the server does not support the broadcast mode, it cannot send a reply message. */
            response = NegativeResponse(r, kServiceNotSupported);
        }
        else
        {
            if (UdsCheckSessionLevel(srv->sessionType, p_serviceInfo))
            {
                if (!srv->SecurityAccessMask || UdsCheckSecurityLevel(srv->securityLevel, p_serviceInfo))
                {
                    response = service(srv, r);
                }
                else
                { /* Access not granted, deny this service */
                    response = NegativeResponse(r, kSecurityAccessDenied);
                }
            }
            else
            { /* This service is not supported in the current session */
                response = NegativeResponse(r, kServiceNotSupportedInActiveSession);
            }
        }
    }
    else
    {
        response = NegativeResponse(r, kServiceNotSupported);
    }

    if (!srv->isNeedResponse)
    {
        if (((UDS_A_TA_TYPE_FUNCTIONAL == r->info.A_TA_Type) && ((kServiceNotSupported == response) || (kSubFunctionNotSupported == response) || (kServiceNotSupportedInActiveSession == response) || (kSubFunctionNotSupportedInActiveSession == response) || (kRequestOutOfRange == response))) || (p_serviceInfo && (p_serviceInfo->mark & SER_MARK_SPRM) && (kPositiveResponse == response) && srv->subSvrSupportPRMB))
        {
            NoResponse(r);
        }
    }
    else
    {
        srv->isNeedResponse = false;
    }
    return response;
}

static void tp_rx_callback(uint32_t id, const uint8_t *data, const uint16_t len, void* user_data){
    UDSReq_t *r = &local_srv.r;
    if(!r->isInprogress && (len <= r->recv_buf_size)){
        if(len == 2 && data[0] == 0x3E && data[1] == 0x80){
            local_srv.s3_session_timeout_timer = local_srv.uds_get_ms() + local_srv.s3_ms;
            return;
        }
        memcpy(r->recv_buf, data, len);
        if (id == local_srv.tp_cfg.sa_phy){
            r->info.A_TA_Type = UDS_A_TA_TYPE_PHYSICAL;
        }else if(id == local_srv.tp_cfg.sa_func){
            r->info.A_TA_Type = UDS_A_TA_TYPE_FUNCTIONAL;
        }
        r->recv_len = len;
        r->isRecvComplete = true;
    }else{
        PRINT_LOG(&local_srv, "uds rx error, data[0] %x\n", data[0]);
    }
}

static void tp_rx_ErrorCallback(uint32_t canid, int status, void* user_data)
{
    (void)user_data;
    if ((status == ISOTP_PROTOCOL_RESULT_WRONG_SN) || (status == ISOTP_PROTOCOL_RESULT_TIMEOUT_CR)) {
        local_srv.s3_session_timeout_timer = local_srv.uds_get_ms() + local_srv.s3_ms;
    }
}
// ========================================================================
//                             Public Functions
// ========================================================================

void UDSServerGetVersion(UDS_VersionInfoType *versioninfo)
{
    if (versioninfo)
    {
        (versioninfo)->sw_major_version = UDS_SW_MAJOR_VERSION;
        (versioninfo)->sw_minor_version = UDS_SW_MINOR_VERSION;
        (versioninfo)->sw_patch_version = UDS_SW_PATCH_VERSION;
    }
}
/**
 * @brief \~chinese 初始化服务器 \~english Initialize the server
 *
 * @param srv
 * @param cfg
 * @return int
 */
UDSErr_t UDSServerInit(UDSInitPara *udsPara)
{
    UDSServer_t *srv = &local_srv;
    memset(srv, 0, sizeof(UDSServer_t));
    srv->tp_cfg.ta = udsPara->bufCfg.ta;
    srv->tp_cfg.sa_phy = udsPara->bufCfg.sa_phy;
    srv->tp_cfg.sa_func = udsPara->bufCfg.sa_func;
    srv->r.send_buf = udsPara->bufCfg.send_buf;
    srv->r.send_buf_size = udsPara->bufCfg.send_buf_size;
    srv->r.recv_buf = udsPara->bufCfg.recv_buf;
    srv->r.recv_buf_size = udsPara->bufCfg.recv_buf_size;

    srv->uds_send_can = udsPara->bspCallbackCfg.uds_send_can;
    srv->uds_get_ms = udsPara->bspCallbackCfg.uds_get_ms;
    srv->print_func = udsPara->bspCallbackCfg.print_func;

    srv->udsSvrTab = udsPara->serviceCfg.udsSvrTab;
    srv->udsSvrNumber = udsPara->serviceCfg.udsSvrNumber;
    srv->udsserver_event_callback = udsPara->serviceCfg.udsserver_event_callback;
    srv->userRegisterService = udsPara->serviceCfg.userRegisterService;
#ifdef UDS_CONFIG_SECURITY_DELAY
    srv->getSecurutyAuthFailCount = udsPara->serviceCfg.getSecurutyAuthFailCount;
    srv->setSecurutyAuthFailCount = udsPara->serviceCfg.setSecurutyAuthFailCount;
    srv->SecurityMaxAttemptsCount = udsPara->serviceCfg.SecurityMaxAttemptsCount;
    srv->SecurityMaxReqSeedCount = udsPara->serviceCfg.SecurityMaxReqSeedCount;
    srv->auth_fail_delay_ms = udsPara->serviceCfg.auth_fail_delay_ms;
#endif
    srv->UdsSecuritySeedLen = udsPara->serviceCfg.UdsSecuritySeedLen;
    srv->p2_ms = udsPara->serviceCfg.p2_server_ms;
    srv->p2_star_ms = udsPara->serviceCfg.p2_star_server_ms;
    srv->p2_pending_ms = udsPara->serviceCfg.p2_pending_ms ? (udsPara->serviceCfg.p2_pending_ms+srv->p2_ms) : (1500+srv->p2_ms);
    srv->power_down_time_ms = udsPara->serviceCfg.power_down_time_ms;
    srv->s3_ms = udsPara->serviceCfg.s3_server;
    srv->SecurityAccessMask = true;
    /*set the initialization default*/
    srv->sessionType = kDefaultSession;
    srv->p2_timer = srv->uds_get_ms() + srv->p2_ms;
    srv->s3_session_timeout_timer = srv->uds_get_ms() + srv->s3_ms;
    srv->sec_access_auth_fail_timer = srv->uds_get_ms();
    srv->securityLevel = 0;
    srv->requestSecurityLevel = 0;

    Isotp_initPara tpInitCfg;
    memset(&tpInitCfg, 0, sizeof(tpInitCfg));
    tpInitCfg.sendid = srv->tp_cfg.ta;
    tpInitCfg.recvid = srv->tp_cfg.sa_phy;
    tpInitCfg.recv_buff_size = udsPara->bufCfg.recv_buf_size;
    tpInitCfg.rx_error_callback = tp_rx_ErrorCallback;
    tpInitCfg.tx_result_callback = NULL;
    tpInitCfg.rx_callback = tp_rx_callback;
    tpInitCfg.uds_get_ms = srv->uds_get_ms;
    tpInitCfg.print_func = srv->print_func;
    tpInitCfg.uds_send_can = srv->uds_send_can;
    tpInitCfg.config = udsPara->tpCfg;
    tpInitCfg.disable_flow_control = false;

    isotp_init_link(&srv->tp_cfg.phys_link, &tpInitCfg);
    tpInitCfg.recvid = srv->tp_cfg.sa_func;
    isotp_init_link(&srv->tp_cfg.func_link, &tpInitCfg);
#ifdef UDS_CONFIG_SECURITY_DELAY
    SecurityInit(srv);
#endif
    return UDS_OK;
}

void UDSServerReceiveCAN(uint32_t identifier, uint8_t *data, uint8_t len)
{
    UDSServer_t *srv = &local_srv;

    if (identifier == srv->tp_cfg.sa_phy)
    {
        isotp_on_can_message(&srv->tp_cfg.phys_link, identifier, data, len);
    }
    else if (identifier == srv->tp_cfg.sa_func)
    {
        if ((data[0] & 0xF0) == 0)
        {
            isotp_on_can_message(&srv->tp_cfg.func_link, identifier, data, len);
        }
    }
}

void setUDSSession(uint8_t session) { local_srv.sessionType = session; }

void UdsServerSetSecurityAccess(bool isEnable){
    local_srv.SecurityAccessMask = isEnable;
}

void UdsServerSetSecurityLevel(uint8_t level)
{
    local_srv.securityLevel = level;
    local_srv.requestSecurityLevel = level;
}

void UDSServerPoll(void)
{
    UDSServer_t *srv = &local_srv;
#ifdef UDS_CONFIG_SECURITY_DELAY
    if(UDSTimeAfterCheck(srv->uds_get_ms(), srv->sec_access_auth_fail_timer, srv->auth_fail_delay_ms))
    {
        if (SecurityAccessDelayIsEnable(srv))
        {
            if (srv->SecurityFailedAuthCount >= srv->SecurityMaxAttemptsCount)
            {
                srv->SecurityFailedAuthCount--;
                PRINT_LOG(&local_srv, "AuthCount-- %d\n", srv->SecurityFailedAuthCount);
                srv->setSecurutyAuthFailCount(srv->SecurityFailedAuthCount);
            }
        }
    }
#endif
    // UDS-1-2013 Figure 38: Session Timeout (S3)
    if (kDefaultSession != srv->sessionType && U_TIME_AFTER_EQ(srv->uds_get_ms(), srv->s3_session_timeout_timer))
    {
        resetInternalVariable(srv);
        EmitEvent(srv, UDS_SRV_EVT_SessionTimeout, NULL);
    }

    isotp_poll(&srv->tp_cfg.phys_link);
    isotp_poll(&srv->tp_cfg.func_link);

    if (srv->ecuResetScheduled){
        if(U_TIME_AFTER_EQ(srv->uds_get_ms(), srv->ecuResetTimer)){
            EmitEvent(srv, UDS_SRV_EVT_DoScheduledReset, &srv->ecuResetScheduled);
        }else{
            return;
        }
    }

    UDSReq_t *r = &srv->r;
    if ((r->isRecvComplete) && (r->recv_len > 0))
    {
        r->isInprogress = true;
        r->isRecvComplete = false;
        srv->p2_timer = srv->uds_get_ms() + srv->p2_ms;
        srv->s3_session_timeout_timer = srv->uds_get_ms() + srv->s3_ms;
        uint8_t response = evaluateServiceResponse(srv, r);
        if (kRequestCorrectlyReceived_ResponsePending == response)
        {
            r->send_len = 0;
        }

        if (r->send_len)
        {
            isotp_send(&srv->tp_cfg.phys_link, r->send_buf, r->send_len);
            r->send_len = 0;
            srv->s3_session_timeout_timer = srv->uds_get_ms() + srv->s3_ms;
        }
    }
    r->isInprogress = false;
}

void UDSSendConfirm(IsoTpSendResultTypes result){
    if(local_srv.isSendPending){
        local_srv.isSendPending = false;
        return;
    }
    isotp_send_confirm(&local_srv.tp_cfg.phys_link, result);
}
