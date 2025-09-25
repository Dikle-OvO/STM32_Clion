#include <stdint.h>
#include <stdbool.h>
#include "uds_port.h"
//#include "ecual_can.h"
#include "ecual_adc.h"
#include "diag_can.h"
#include "cantypes.h"
#include "userStorage.h"
#include "dtc_user.h"
#include "ate_uds_data.h"
#include "DidStorage.h"
#include "uds_security.h"
#include "fsl_adapter_flash.h"
#include "main.h"
#include "yf_log.h"
#include "Dem.h"
#include "uds_server.h"

#define UDS_SERVER_IN_BOOT (0u) /* 1:BOOT; 0: APP */

#if UDS_SERVER_IN_BOOT

#define MAX_DOWNLOAD_BlockLength (4082)
uint8_t FlashDrive_buffer[FLASH_DRV_RAM_ADDR_USE_SIZE];

typedef struct {
    bool WriteFinger_flag;

    bool Routine_preConditionsCheck_flag;
    bool Routine_earseMemory_flag;
    bool Routine_dependenceCheck_flag;

    bool Routine_appMemoryCheck_flag;
    bool Routine_driverMemoryCheck_flag;

    void const *DriveAddr;
    void *DriverCurWriteAddr;
    uint32_t DriveSize;
    uint32_t DriveCRC;
    bool DriveRequestDownload;
    bool DriveDownloadComplete;

    void const *AppAddr;
    void *AppCurWriteAddr;
    uint32_t AppSize;
    uint32_t AppCRC;
    bool AppRequestDownload;
    bool AppDownloadComplete;

    uint32_t appBackupStartOffset;

} OTA_PARA;
static OTA_PARA Ota_Para;

static void OtaDrivePara_Init(void *addr, const uint32_t size)
{
    CRC32_Init();
    Ota_Para.DriveCRC = 0xFFFFFFFF;
    Ota_Para.DriveAddr = addr;
    Ota_Para.DriverCurWriteAddr = Ota_Para.DriveAddr;
    Ota_Para.DriveSize = size;
    Ota_Para.DriveRequestDownload = true;

    Ota_Para.AppAddr = NULL;
    Ota_Para.AppCurWriteAddr = NULL;
    Ota_Para.AppSize = 0;
    Ota_Para.AppRequestDownload = false;
    Ota_Para.AppCRC = 0xFFFFFFFF;
}

static void OtaAppPara_Init(void *addr, const uint32_t size)
{
    CRC32_Init();
    Ota_Para.AppCRC = 0xFFFFFFFF;
    Ota_Para.AppAddr = addr;
    Ota_Para.AppCurWriteAddr = Ota_Para.AppAddr;
    Ota_Para.AppSize = size;
    Ota_Para.AppRequestDownload = true;
}

static uint8_t _0x34_RequestDownload(UDSServer_t *srv, const void *data)
{
    UDSRequestDownloadArgs_t *r = (UDSRequestDownloadArgs_t *)data;
    uint8_t ret = kPositiveResponse;
    log_debug("request down load: startAddress=0x%x,length=%d\n", (uint32_t)r->addr, r->size);
    if (srv->sessionType != kProgrammingSession) {
        ret = kServiceNotSupportedInActiveSession;
    } else if (Ota_Para.WriteFinger_flag == false) {
        ret = kUploadDownloadNotAccepted;
    } else if (srv->securityLevel != SECURITY_LEVEL_FBL) {
        ret = kSecurityAccessDenied;
    } else if ((uint32_t)r->addr >= FLASH_DRV_RAM_ADDR_START && (uint32_t)r->addr < FLASH_DRV_RAM_ADDR_END
               && r->size <= FLASH_DRV_RAM_ADDR_USE_SIZE) {
        // request download flash drive
        OtaDrivePara_Init(r->addr, r->size);
        r->maxNumberOfBlockLength = MAX_DOWNLOAD_BlockLength;
    } else if ((uint32_t)r->addr == APP_PROGRAM_START_ADDR && (uint32_t)r->size < APP_PROGRAM_SIZE_MAX) {
        // // request download app
        if (Ota_Para.Routine_earseMemory_flag == false) {
            ret = kUploadDownloadNotAccepted;
        } else {
            OtaAppPara_Init(r->addr, r->size);
            r->maxNumberOfBlockLength = MAX_DOWNLOAD_BlockLength;
        }
    } else {
        ret = kRequestOutOfRange;
    }
    return ret;
}

static uint8_t _0x36_TransferData(UDSServer_t *srv, const void *data)
{
    // 该服务分两次调用，首次调用发送0x78，第二次才执行写入操作
    UDSTransferDataArgs_t *r = (UDSTransferDataArgs_t *)data;
    uint8_t ret = kPositiveResponse;
    uint16_t curVoltage = ECUAL_ADC_GetBatteryVoltage();
    if (srv->sessionType != kProgrammingSession) {
        ret = kServiceNotSupportedInActiveSession;
    } else if (r->len > r->maxRespLen) {
        ret = kIncorrectMessageLengthOrInvalidFormat;
    } else if (srv->securityLevel != SECURITY_LEVEL_FBL) {
        ret = kSecurityAccessDenied;
    } else if (curVoltage < 8500) {
        log_warning("voltage :%d is too low to update.\n", curVoltage);
        ret = kVoltageTooLow;
    } else if (curVoltage > 16500) {
        log_warning("voltage :%d is too high to update.\n", curVoltage);
        ret = kVoltageTooHigh;
        /*flash driver*/
    } else if (Ota_Para.DriveRequestDownload == true && Ota_Para.AppRequestDownload == false) {
        log_debug("FlashDriveCurWriteAddr = 0x%x,writeLen = %d\n", (uint32_t)Ota_Para.DriverCurWriteAddr, r->len);
        uint32_t offset = (uint32_t)(Ota_Para.DriverCurWriteAddr - Ota_Para.DriveAddr);
        memcpy(FlashDrive_buffer + offset, r->data, r->len);
        Ota_Para.DriveCRC = CRC32_Calc(Ota_Para.DriveCRC, r->data, r->len);
        Ota_Para.DriverCurWriteAddr += r->len;
        if (Ota_Para.DriverCurWriteAddr == Ota_Para.DriveAddr + Ota_Para.DriveSize) {
            Ota_Para.DriveCRC = ~Ota_Para.DriveCRC;
            Ota_Para.DriveDownloadComplete = true;
        }
        /*app*/
    } else if (Ota_Para.DriveDownloadComplete == true && Ota_Para.AppRequestDownload == true) {
        detectP2TimeAndResponsePending(srv);
        uint8_t *inputDataAddr = r->data;
        uint16_t inputDataLen = r->len;
        static uint8_t localRemainBytes[FLASH_PHRASE_SIZE] = {0};
        static uint8_t localRemainByteSize = 0;

        if (localRemainByteSize > 0) {
            /*如果本地数组存在未写入的字节，优先填满*/
            if (r->len > FLASH_PHRASE_SIZE - localRemainByteSize) {
                memcpy(&localRemainBytes[localRemainByteSize], r->data, FLASH_PHRASE_SIZE - localRemainByteSize);
                HAL_FlashProgram((uint32_t)Ota_Para.AppCurWriteAddr + Ota_Para.appBackupStartOffset, FLASH_PHRASE_SIZE,
                                 localRemainBytes);
                log_debug("AppCurWriteAddr = 0x%x,writeLen = %d\n",
                          (uint32_t)Ota_Para.AppCurWriteAddr + Ota_Para.appBackupStartOffset, FLASH_PHRASE_SIZE);
                Ota_Para.AppCRC = CRC32_Calc(Ota_Para.AppCRC, localRemainBytes, FLASH_PHRASE_SIZE);
                Ota_Para.AppCurWriteAddr += FLASH_PHRASE_SIZE;
                inputDataAddr = r->data + (FLASH_PHRASE_SIZE - localRemainByteSize);
                inputDataLen = r->len - (FLASH_PHRASE_SIZE - localRemainByteSize);
                localRemainByteSize = 0;
            } else {
                /*如果新传入的数组不够填满*/
                memcpy(&localRemainBytes[localRemainByteSize], r->data, r->len);
                memset(localRemainBytes + localRemainByteSize + r->len, 0XFF,
                       FLASH_PHRASE_SIZE - localRemainByteSize - r->len);
                HAL_FlashProgram((uint32_t)Ota_Para.AppCurWriteAddr + Ota_Para.appBackupStartOffset, FLASH_PHRASE_SIZE,
                                 localRemainBytes);
                log_debug("AppCurWriteAddr = 0x%x,writeLen = %d\n",
                          (uint32_t)Ota_Para.AppCurWriteAddr + Ota_Para.appBackupStartOffset, FLASH_PHRASE_SIZE);
                Ota_Para.AppCRC = CRC32_Calc(Ota_Para.AppCRC, localRemainBytes, localRemainByteSize + r->len);
                Ota_Para.AppCurWriteAddr += localRemainByteSize + r->len;
                inputDataAddr = r->data + r->len;
                inputDataLen = 0;
                localRemainByteSize = 0;
            }
        }

        uint32_t extraBytes = (inputDataLen) % FLASH_PHRASE_SIZE;
        uint32_t alignedLength = (inputDataLen)-extraBytes;

        if (extraBytes > 0) {
            memcpy(localRemainBytes, inputDataAddr + alignedLength, extraBytes);
            memset(localRemainBytes + extraBytes, 0xFF, FLASH_PHRASE_SIZE - extraBytes);
            localRemainByteSize = extraBytes;
        }

        log_debug("AppCurWriteAddr = 0x%x,writeLen = %d\n",
                  (uint32_t)Ota_Para.AppCurWriteAddr + Ota_Para.appBackupStartOffset, alignedLength);

        if (alignedLength > 0) {
            if (kStatus_HAL_Flash_Success
                == HAL_FlashProgram((uint32_t)Ota_Para.AppCurWriteAddr + Ota_Para.appBackupStartOffset, alignedLength,
                                    inputDataAddr)) {
                Ota_Para.AppCRC = CRC32_Calc(Ota_Para.AppCRC, inputDataAddr, alignedLength);
                Ota_Para.AppCurWriteAddr += alignedLength;
            } else {
                log_error("Programming error,address=0x%x,size=0x%x\n",
                          (uint32_t)Ota_Para.AppCurWriteAddr + Ota_Para.appBackupStartOffset, alignedLength);
                ret = kGeneralProgrammingFailure;
            }
        }

        if (Ota_Para.AppCurWriteAddr + localRemainByteSize == Ota_Para.AppAddr + Ota_Para.AppSize) {
            HAL_FlashProgram((uint32_t)Ota_Para.AppCurWriteAddr + Ota_Para.appBackupStartOffset, FLASH_PHRASE_SIZE,
                             localRemainBytes);
            log_debug("AppCurWriteAddr = 0x%x,writeLen = %d\n",
                      (uint32_t)Ota_Para.AppCurWriteAddr + Ota_Para.appBackupStartOffset, localRemainByteSize);
            Ota_Para.AppCRC = CRC32_Calc(Ota_Para.AppCRC, localRemainBytes, localRemainByteSize);
            Ota_Para.AppCRC = ~Ota_Para.AppCRC;
            Ota_Para.AppDownloadComplete = true;
        }
    }
    return ret;
}
static uint8_t _0x37_RequestTransferExit(UDSServer_t *srv, const void *data)
{
    UDSRequestTransferExitArgs_t *r = (UDSRequestTransferExitArgs_t *)data;
    uint8_t ret = kPositiveResponse;
    uint32_t CrcValue = 0, CrcValueInput = 0;
    if (srv->sessionType != kProgrammingSession) {
        ret = kServiceNotSupportedInActiveSession;
    } else if (r->len != 4) {
        ret = kIncorrectMessageLengthOrInvalidFormat;
    } else if (srv->securityLevel != SECURITY_LEVEL_FBL) {
        ret = kSecurityAccessDenied;
    } else {
        if (Ota_Para.DriveDownloadComplete == true && Ota_Para.AppDownloadComplete == false) {
            CrcValue = Ota_Para.DriveCRC;
        } else if (Ota_Para.DriveDownloadComplete == true && Ota_Para.AppDownloadComplete == true) {
            CrcValue = Ota_Para.AppCRC;
        }
        CrcValueInput = r->data[0] << 24;
        CrcValueInput |= r->data[1] << 16;
        CrcValueInput |= r->data[2] << 8;
        CrcValueInput |= r->data[3];
        log_debug("CrcInput = 0x%x,CrcMcu = 0x%x\n", CrcValueInput, CrcValue);
        if (CrcValueInput == CrcValue) {
            r->copyResponse(srv, r->data, r->len);
        } else {
            ret = kRequestSequenceError;
        }
    }
    return ret;
}

static uint8_t RoutineControl_earseMemory(UDSServer_t *srv, const UDSRoutineCtrlArgs_t *r)
{
    // 该服务分两次调用，首次调用发送0x78，第二次才执行擦除操作
    uint32_t eraseAddr = 0xFFFFFFFF;
    uint32_t eraselength = 0xFFFFFFFF;
    uint8_t ret = kPositiveResponse;
    uint8_t respChar = 1;
    // 校验报文长度
    if (r->len != 8) {
        ret = kIncorrectMessageLengthOrInvalidFormat;
    }

    if (Ota_Para.Routine_driverMemoryCheck_flag == false) {
        ret = kConditionsNotCorrect;
    }
    if (SECURITY_LEVEL_FBL != srv->securityLevel && SECURITY_LEVEL_APP != srv->securityLevel) {
        ret = kSecurityAccessDenied;
    } else {
        detectP2TimeAndResponsePending(srv);
        // log_hex(r->optionRecord,8);
        // memcpy(&eraseAddr, r->optionRecord, 4);
        // memcpy(&eraselength, r->optionRecord + 4, 4);;
        eraseAddr = r->optionRecord[0] << 24;
        eraseAddr |= r->optionRecord[1] << 16;
        eraseAddr |= r->optionRecord[2] << 8;
        eraseAddr |= r->optionRecord[3];
        eraselength = r->optionRecord[4] << 24;
        eraselength |= r->optionRecord[5] << 16;
        eraselength |= r->optionRecord[6] << 8;
        eraselength |= r->optionRecord[7];
        log_info("Request: eraseAddr=0x%x, eraselength=0x%x \n", eraseAddr, eraselength);
        eraselength += (8 * 1024 - eraselength % (8 * 1024));
        if (eraseAddr == APP_PROGRAM_START_ADDR && eraselength <= APP_PROGRAM_SIZE_MAX) {
            // 如果app小于备份区大小，则使用AB分区备份。否则直接写入
            if (eraselength > APP_BACKUP_SIZE_MAX) {
                log_info("%s", "App size is too large to backup!\n");
                Ota_Para.appBackupStartOffset = 0;
            } else {
                Ota_Para.appBackupStartOffset = APP_BACKUP_SIZE_MAX;
                log_info("Backup app using AB partition, backup address : 0x%x!\n ",
                         Ota_Para.appBackupStartOffset + APP_PROGRAM_START_ADDR);
            }
            if (kStatus_HAL_Flash_Success
                == HAL_FlashEraseSector(Ota_Para.appBackupStartOffset + eraseAddr, eraselength)) {
                log_info("Reality: eraseAddr=0x%x, eraselength=0x%x \n", Ota_Para.appBackupStartOffset + eraseAddr,
                         eraselength);
                Ota_Para.Routine_earseMemory_flag = true;
            } else {
                log_error("Erase error,address=0x%x,size=0x%x\n", Ota_Para.appBackupStartOffset + eraseAddr,
                          eraselength);
                ret = kGeneralProgrammingFailure;
            }
        } else {
            log_warning("%s", "App size is too large to update!\n");
            ret = kRequestOutOfRange;
        }
    }
    return ret;
}

static uint32_t calculateChecksum(uint32_t init, const uint8_t *data, uint32_t size)
{
    uint32_t checksum = init;

    for (uint32_t i = 0; i < size; i += 4) {
        checksum += *(volatile uint32_t *)(data + i);
    }

    return checksum;
}

static bool sumCheck(uint32_t addr, uint32_t size)
{
    uint32_t curIdx = 0, blocksize = 0;
    uint32_t sum = 0;
    int ret = true;
    uint8_t appBuf[1024];

    while (curIdx < size) {
        if (size - curIdx >= 1024) {
            blocksize = 1024;
        } else {
            blocksize = size - curIdx;
        }
        if (kStatus_HAL_Flash_Success != HAL_FlashRead(addr + curIdx, blocksize, appBuf)) {
            ret = false;
            break;
        } else {
            sum = calculateChecksum(sum, appBuf, blocksize);
            curIdx += blocksize;
        }
    }

    if ((true == ret) && (sum != 0)) {
        ret = false;
    }

    return ret;
}

static bool isFlashDriveValid()
{
    bool ret = false;
    uint32_t flashMagic, flashSize, sum = 0;
    flashMagic = *(uint32_t *)(FlashDrive_buffer + FLASH_DRV_MAGIC_OFFSET);
    flashSize = *(uint32_t *)(FlashDrive_buffer + FLASH_DRV_SIZE_OFFSET);

    if (flashMagic == FLASH_DRIVE_MAGIC && (calculateChecksum(sum, FlashDrive_buffer, flashSize) == 0)) {
        ret = true;
    }
    return ret;
}

bool isBootValid(uint32_t BootAddr)
{
    bool ret = false;
    uint32_t BootMagic, bootSize;
    uint8_t BootMagicBuff[4], bootSizeBuff[4];
    if (kStatus_HAL_Flash_Success == HAL_FlashRead(BootAddr + BOOT_SIZE_OFFSET, sizeof(bootSizeBuff), bootSizeBuff)) {
        bootSize = *(uint32_t *)(bootSizeBuff);
        if (kStatus_HAL_Flash_Success
            == HAL_FlashRead(BootAddr + BOOT_MAGIC_OFFSET, sizeof(BootMagicBuff), BootMagicBuff)) {
            BootMagic = *(uint32_t *)(BootMagicBuff);
            if (BootMagic == BOOT_MAGIC && bootSize <= BOOT_MAX_SIZE && sumCheck(BootAddr, bootSize) == true) {
                ret = true;
            }
        }
    }
    return ret;
}

bool isAppValid(uint32_t appStartAddr)
{
    bool ret = false;
    uint32_t appMagic, appSize;
    uint8_t appMagicBuff[4], appSizeBuff[4];
    if (kStatus_HAL_Flash_Success == HAL_FlashRead(appStartAddr + APP_MAGIC_OFFSET, sizeof(appMagicBuff), appMagicBuff)
        && kStatus_HAL_Flash_Success
               == HAL_FlashRead(appStartAddr + APP_SIZE_OFFSET, sizeof(appSizeBuff), appSizeBuff)) {
        appMagic = *(uint32_t *)(appMagicBuff);
        appSize = *(uint32_t *)(appSizeBuff);
        if (appMagic == APP_MAGIC && appSize > 0 && appSize <= APP_PROGRAM_SIZE_MAX
            && sumCheck(appStartAddr, appSize) == true) {
            ret = true;
        }
    }
    return ret;
}

bool isAppBootVaild(uint32_t appStartAddr, uint32_t appBootSize)
{
    /*如果app后跟有新的boot，校验boot的完整性*/
    bool ret = false;
    uint32_t appSize;
    uint8_t appMagicBuff[4], appSizeBuff[4];
    if (kStatus_HAL_Flash_Success == HAL_FlashRead(appStartAddr + APP_SIZE_OFFSET, sizeof(appSizeBuff), appSizeBuff)) {
        appSize = *(uint32_t *)(appSizeBuff);
        if (appSize == appBootSize) {
            ret = true;
        } else {
            ret = isBootValid(appStartAddr + appSize);
        }
    }
    return ret;
}

static uint8_t RoutineControl_downMemoryCheck(UDSServer_t *srv, const UDSRoutineCtrlArgs_t *r)
{
    uint8_t ret = kPositiveResponse;
    uint8_t respChar = 1;
    if (r->len != 0) {
        ret = kIncorrectMessageLengthOrInvalidFormat;
    } else if (SECURITY_LEVEL_FBL != srv->securityLevel && SECURITY_LEVEL_APP != srv->securityLevel) {
        ret = kSecurityAccessDenied;
    } else if (Ota_Para.DriveDownloadComplete == false) {
        ret = kConditionsNotCorrect;
    } else {
        if (isFlashDriveValid()) {
            Ota_Para.Routine_driverMemoryCheck_flag = true;
            respChar = 0;
        } else {
            respChar = 1;
        }
        r->copyStatusRecord(srv, &respChar, sizeof(respChar));
    }
    return ret;
}

static uint8_t RoutineControl_dependenceCheck(UDSServer_t *srv, const UDSRoutineCtrlArgs_t *r)
{
    uint8_t ret = kPositiveResponse;
    uint8_t respChar = 1;
    if (r->len != 0) {
        ret = kIncorrectMessageLengthOrInvalidFormat;
    } else if (SECURITY_LEVEL_FBL != srv->securityLevel && SECURITY_LEVEL_APP != srv->securityLevel) {
        ret = kSecurityAccessDenied;
    } else if (Ota_Para.AppDownloadComplete == false) {
        ret = kConditionsNotCorrect;
    } else {
        if (isAppValid(APP_PROGRAM_START_ADDR + Ota_Para.appBackupStartOffset)
            && isAppBootVaild(APP_PROGRAM_START_ADDR + Ota_Para.appBackupStartOffset, Ota_Para.AppSize)) {
            if (Ota_Para.appBackupStartOffset == 0) {
                // 刷写计数器加1
                BootSuccessCountUp();
                respChar = 0;
                log_debug("%s", "The app in Zone A is valid\n");
            } else if (Ota_Para.appBackupStartOffset == APP_BACKUP_SIZE_MAX) {
                log_debug("%s", "The app backed up by Zone B is valid\n");
                detectP2TimeAndResponsePending(srv);
                if (kStatus_HAL_Flash_Success == HAL_FlashEraseSector(APP_PROGRAM_START_ADDR, APP_BACKUP_SIZE_MAX)) {
                    detectP2TimeAndResponsePending(srv);
                    uint32_t backupSize = APP_BACKUP_SIZE_MAX + (16 - APP_BACKUP_SIZE_MAX % 16);
                    if (kStatus_HAL_Flash_Success
                        == HAL_FlashProgram(APP_PROGRAM_START_ADDR, backupSize,
                                            (uint8_t *)(APP_PROGRAM_START_ADDR + Ota_Para.appBackupStartOffset))) {
                        // 刷写计数器加1
                        BootSuccessCountUp();
                        respChar = 0;
                        log_debug("%s", "Copying apps backed up from zone B to zone A SUCCESSED\n");
                    } else {
                        log_warning("%s", "Copying apps backed up from zone B to zone A FAILED\n");
                    }
                } else {
                    log_error("Erase error,address=0x%x,size=0x%x\n", APP_PROGRAM_START_ADDR, APP_BACKUP_SIZE_MAX);
                }
            }
        } else {
            HAL_FlashEraseSector((uint8_t *)(APP_PROGRAM_START_ADDR + Ota_Para.appBackupStartOffset), 8 * 1024);
            log_warning("%s", "app check fail, OTA fail\n");
        }
        r->copyStatusRecord(srv, &respChar, sizeof(respChar));
    }
    return ret;
}

#else
typedef struct {
    bool WriteFinger_flag;

    bool Routine_preConditionsCheck_flag;
    bool Routine_earseMemory_flag;
    bool Routine_dependenceCheck_flag;

    bool Routine_appMemoryCheck_flag;
    bool Routine_driverMemoryCheck_flag;

} OTA_PARA;
static OTA_PARA Ota_Para;

static uint8_t _0x14_ClrDiagInfo(UDSServer_t *srv, const void *data)
{
    UDSGroupOfDTCArgs_t *r = (UDSGroupOfDTCArgs_t *)data;
    uint8_t nrc = kPositiveResponse;
    uint32 dtc;
    Dem_ReturnClearDTCType result;
    dtc = BYTES_TO_DTC(r->HighByte, r->MidByte, r->LowByte);
    result = Dem_ClearDTC(dtc, DEM_DTC_KIND_ALL_DTCS, DEM_DTC_ORIGIN_PRIMARY_MEMORY);

    switch (result) {
    case DEM_CLEAR_OK:
        break;
    case DEM_CLEAR_FAILED:
        nrc = kGeneralProgrammingFailure;
        break;
    default:
        nrc = kRequestOutOfRange;
        break;
    }
    return nrc;
}

static uint8_t _0x1901_RepNumOfDTCByStaMask(UDSServer_t *srv, const void *data)
{
    UDSRepNumOfDTCByStaMaskArgs_t *r = (UDSRepNumOfDTCByStaMaskArgs_t *)data;
    uint8_t nrc = kPositiveResponse;
    Dem_ReturnSetDTCFilterType setDtcFilterResult;

    setDtcFilterResult = Dem_SetDTCFilter(r->DTCStatusMask, DEM_DTC_KIND_ALL_DTCS, DEM_DTC_ORIGIN_PRIMARY_MEMORY,
                                          DEM_FILTER_WITH_SEVERITY_NO, VALUE_IS_NOT_USED, DEM_FILTER_FOR_FDC_NO);

    if (DEM_FILTER_ACCEPTED == setDtcFilterResult) {
        Std_ReturnType result;
        uint16 numberOfFilteredDtc;
        Dem_ReturnGetNumberOfFilteredDTCType getNumerResult;
        uint8 dtcStatusMask = 0;
        getNumerResult = Dem_GetNumberOfFilteredDtc(&numberOfFilteredDtc);
        if (DEM_NUMBER_OK == getNumerResult) {
            result = Dem_GetDTCStatusAvailabilityMask(&dtcStatusMask);
            r->DTCStatusAvailabilityMask = dtcStatusMask;
            r->DTCFormatIdentifier = Dem_GetTranslationType();
            r->DTCCountHighByte = (numberOfFilteredDtc >> 8);
            r->DTCCountLowByte = (numberOfFilteredDtc & 0xFF);
        } else {
            // nrc = kRequestOutOfRange;
        }
    } else {
        // nrc = kRequestOutOfRange;
    }

    return nrc;
}

uint8_t SvrUser_GetDTCByMask(uint8_t status_mask, uint8_t *p_outbuf, uint16_t bufsize, uint16_t *p_length)
{
    uint8_t nrc = kPositiveResponse;
    Dem_ReturnSetDTCFilterType setDtcFilterResult =
        Dem_SetDTCFilter(status_mask, DEM_DTC_KIND_ALL_DTCS, DEM_DTC_ORIGIN_PRIMARY_MEMORY, DEM_FILTER_WITH_SEVERITY_NO,
                         VALUE_IS_NOT_USED, DEM_FILTER_FOR_FDC_NO);

    if (DEM_FILTER_ACCEPTED == setDtcFilterResult) {
        uint8 dtcStatusMask = 0;
        Dem_ReturnGetNextFilteredDTCType getNextFilteredDtcResult;
        uint32 dtc;
        Dem_EventStatusExtendedType dtcStatus;
        Std_ReturnType result;
        uint16 currIndex = 0;

        Dem_GetDTCStatusAvailabilityMask(&dtcStatusMask);

        if (0 != dtcStatusMask) {
            getNextFilteredDtcResult = Dem_GetNextFilteredDTC(&dtc, &dtcStatus);
            while (DEM_FILTERED_OK == getNextFilteredDtcResult) {
                if (bufsize < currIndex + 4) {
                    nrc = kResponseTooLong;
                    break;
                }
                p_outbuf[currIndex++] = DTC_HIGH_BYTE(dtc);
                p_outbuf[currIndex++] = DTC_MID_BYTE(dtc);
                p_outbuf[currIndex++] = DTC_LOW_BYTE(dtc);
                p_outbuf[currIndex++] = dtcStatus;
                getNextFilteredDtcResult = Dem_GetNextFilteredDTC(&dtc, &dtcStatus);
            }
        }
        *p_length = currIndex;
    } else {
        nrc = kResponseTooLong;
    }
    return nrc;
}

static uint8_t _0x1902_RepDTCByStaMask(UDSServer_t *srv, const void *data)
{
    UDSRepDTCByStaMaskArgs_t *r = (UDSRepDTCByStaMaskArgs_t *)data;
    uint8_t nrc = kPositiveResponse;
    uint8_t tempArry[255];
    uint16_t index;
    uint16_t length = 0;
    Dem_GetDTCStatusAvailabilityMask(&r->DTCStatusAvailabilityMask);
    nrc = SvrUser_GetDTCByMask(r->DTCStatusMask, tempArry, 255, &length);
    if (nrc == kPositiveResponse) {
        r->copy(srv, tempArry, length);
    }
    return nrc;
}

static uint8_t _0x1904_RepDTCSnapRecByDTCNum(UDSServer_t *srv, const void *data)
{
    UDSDTCSnapExtRecByDTCNumArgs_t *r = (UDSDTCSnapExtRecByDTCNumArgs_t *)data;
    uint8_t nrc = kPositiveResponse;

    // 1. Only consider Negative Response 0x10

    Dem_DTCKindType DtcType = 0;
    Dem_DTCOriginType DtcOrigin = 0;
    uint32 DtcNumber = 0;
    // srv->tp_cfg.send_buf_size;
    uint8_t tempArry[255];
    uint8 AvailableBufSize = 255;
    uint8 RecNumOffset = 0;
    uint16 index = 0;
    uint16 EventIndex = 0;
    uint16 FFIdNumber = 0;
    Dem_ReturnGetFreezeFrameDataByDTCType GetFFbyDtcReturnCode = DEM_GET_FFDATABYDTC_WRONG_DTC;
    Dem_ReturnGetStatusOfDTCType GetStatusOfDtc = DEM_STATUS_OK;
    Dem_EventParameterType *pEventParaTemp = NULL;
    uint8 startRecNum, endRecNum, recNum;

    switch (r->DTCRecordNumber) {
    case 0xFF: // Report all Extended Data Records
        startRecNum = 0x00;
        endRecNum = 0xFE;
        break;
    default:
        startRecNum = r->DTCRecordNumber;
        endRecNum = startRecNum;
        break;
    }
    DtcNumber = BYTES_TO_DTC(r->DTCHighByte, r->DTCMiddleByte, r->DTCLowByte);
    DtcType = DEM_FREEZE_FRAME_NON_OBD;
    // DtcOrigin = pEventParaTemp->EventClass->EventDestination[?];
    //  now use DEM_DTC_ORIGIN_PRIMARY_MEMORY as default.
    DtcOrigin = DEM_DTC_ORIGIN_PRIMARY_MEMORY;
    boolean foundValidRecordNumber = FALSE;
    GetStatusOfDtc = Dem_GetStatusOfDTC(DtcNumber, DtcType, DtcOrigin, &r->StausOfDTC); /** @req DEM212 */
    if (GetStatusOfDtc == DEM_STATUS_OK) {
        for (recNum = startRecNum; recNum <= endRecNum; recNum++) {
            Dem_GetNumberOfDidInFreezeFrame(DtcNumber, recNum, &FFIdNumber);
            if (FFIdNumber > 0) {
                AvailableBufSize = sizeof(tempArry) - (RecNumOffset + 2);
                GetFFbyDtcReturnCode = Dem_GetFreezeFrameDataByDTC(DtcNumber, DtcType, DtcOrigin, recNum,
                                                                   &tempArry[RecNumOffset + 2], &AvailableBufSize);
                if (GetFFbyDtcReturnCode == DEM_GET_FFDATABYDTC_OK) {
                    foundValidRecordNumber = TRUE;
                    if (AvailableBufSize > 0) {
                        tempArry[RecNumOffset++] = recNum;
                        tempArry[RecNumOffset++] = FFIdNumber;
                        RecNumOffset = RecNumOffset + AvailableBufSize;
                    }
                } else {
                    break;
                }
            }
        }
    }
    r->copy(srv, tempArry, RecNumOffset);
    // Negative response
    if (!foundValidRecordNumber) {
        nrc = kRequestOutOfRange;
    }
    return nrc;
}

static uint8_t _0x1906_RepDTCExtRecByDTCNum(UDSServer_t *srv, const void *data)
{
    UDSDTCSnapExtRecByDTCNumArgs_t *r = (UDSDTCSnapExtRecByDTCNumArgs_t *)data;
    uint8_t nrc = kPositiveResponse;
    Dem_DTCOriginType dtcOrigin = DEM_DTC_ORIGIN_PRIMARY_MEMORY;
    Dem_DTCKindType dtcKind = DEM_DTC_KIND_ALL_DTCS;
    uint8_t tempArry[255];
    uint8 startRecNum;
    uint8 endRecNum;

    switch (r->DTCRecordNumber) {
    case 0xFF: // Report all Extended Data Records
        startRecNum = 0x0;
        endRecNum = 0xFE;
        break;
    default:
        startRecNum = r->DTCRecordNumber;
        endRecNum = startRecNum;
        break;
    }
    Dem_ReturnGetStatusOfDTCType getStatusOfDtcResult;
    uint32 dtc;
    Dem_EventStatusExtendedType statusOfDtc;

    dtc = BYTES_TO_DTC(r->DTCHighByte, r->DTCMiddleByte, r->DTCLowByte);
    getStatusOfDtcResult = Dem_GetStatusOfDTC(dtc, dtcKind, dtcOrigin, &statusOfDtc);
    if (getStatusOfDtcResult == DEM_STATUS_OK) {
        Dem_ReturnGetExtendedDataRecordByDTCType getExtendedDataRecordByDtcResult;
        uint8 recNum;
        uint16 recLength;
        uint16 txIndex = 0;
        boolean foundValidRecordNumber = FALSE;
        r->StausOfDTC = statusOfDtc;

        for (recNum = startRecNum; recNum <= endRecNum; recNum++) {
            recLength = 255 - (txIndex + 1);
            getExtendedDataRecordByDtcResult =
                Dem_GetExtendedDataRecordByDTC(dtc, dtcKind, dtcOrigin, recNum, &tempArry[txIndex + 1], &recLength);
            if (getExtendedDataRecordByDtcResult == DEM_RECORD_OK) {
                foundValidRecordNumber = TRUE;
                if (recLength > 0) {
                    tempArry[txIndex++] = recNum;
                    txIndex += recLength;
                }
            }
        }
        r->copy(srv, tempArry, txIndex);

        if (!foundValidRecordNumber) {
            nrc = kRequestOutOfRange;
        }
    } else {
        nrc = kRequestOutOfRange;
    }
    return nrc;
}

static uint8_t _0x190A_RepAllSupDTC(UDSServer_t *srv, const void *data)
{
    uint8_t nrc = kPositiveResponse;
    UDSRepAllSupDTCArgs_t *r = (UDSRepAllSupDTCArgs_t *)data;
    uint8_t tempArry[512];
    Dem_ReturnSetDTCFilterType setDtcFilterResult;

    setDtcFilterResult = Dem_SetDTCFilter(DEM_DTC_STATUS_MASK_ALL, DEM_DTC_KIND_ALL_DTCS, DEM_DTC_ORIGIN_PRIMARY_MEMORY,
                                          DEM_FILTER_WITH_SEVERITY_NO, VALUE_IS_NOT_USED, DEM_FILTER_FOR_FDC_NO);

    if (setDtcFilterResult == DEM_FILTER_ACCEPTED) {
        uint8 dtcStatusMask = 0;
        Dem_ReturnGetNextFilteredDTCType getNextFilteredDtcResult;
        uint32 dtc;
        Dem_EventStatusExtendedType dtcStatus;
        Std_ReturnType result;
        uint16 currIndex = 0;

        result = Dem_GetDTCStatusAvailabilityMask(&dtcStatusMask);
        if (result != E_OK) {
            dtcStatusMask = 0;
        }
        r->DTCStatusAvailabilityMask = dtcStatusMask;

        if (0 != dtcStatusMask) {
            getNextFilteredDtcResult = Dem_GetNextFilteredDTC(&dtc, &dtcStatus);
            while (getNextFilteredDtcResult == DEM_FILTERED_OK) {
                tempArry[currIndex++] = DTC_HIGH_BYTE(dtc);
                tempArry[currIndex++] = DTC_MID_BYTE(dtc);
                tempArry[currIndex++] = DTC_LOW_BYTE(dtc);
                tempArry[currIndex++] = dtcStatus;
                getNextFilteredDtcResult = Dem_GetNextFilteredDTC(&dtc, &dtcStatus);
            }

            if (getNextFilteredDtcResult != DEM_FILTERED_NO_MATCHING_DTC) {
                nrc = kRequestOutOfRange;
            }
        }

        r->copy(srv, tempArry, currIndex);
    } else {
        nrc = kRequestOutOfRange;
    }
    return nrc;
}
static uint8_t _0x28_CommunicationControl(UDSServer_t *srv, const void *data)
{
    UDSCommCtrlArgs_t *r = (UDSCommCtrlArgs_t *)data;
    uint8_t ret = kPositiveResponse;

    switch (r->ctrlType) {
    case kEnableRxAndTx:
        if (r->commType == kNormalCommunicationMessages) {
            CanComSendEnableAll(0, true);
        } else if (r->commType == kNetworkManagementCommunicationMessages) {
            CanNm_EnableCommunication(0);
        } else if (r->commType == kNetworkManagementCommunicationMessagesAndNormalCommunicationMessages) {
            CanNm_EnableCommunication(0);
            CanComSendEnableAll(0, true);
        } else {
            ret = kRequestOutOfRange;
        }
        break;
    case kDisableRxAndTx:
        if (r->commType == kNormalCommunicationMessages) {
            CanComSendEnableAll(0, false);
        } else if (r->commType == kNetworkManagementCommunicationMessages) {
            CanNm_DisableCommunication(0);
        } else if (r->commType == kNetworkManagementCommunicationMessagesAndNormalCommunicationMessages) {
            CanNm_DisableCommunication(0);
            CanComSendEnableAll(0, false);
        } else {
            ret = kRequestOutOfRange;
        }
        break;
    case kEnableRxAndDisableTx:
    case kDisableRxAndEnableTx:
    default:
        ret = kSubFunctionNotSupported;
        break;
    }
    return ret;
}

static uint8_t _0x85_ControlDTC(UDSServer_t *srv, const void *data)
{
    UDSDtcSettingType r = *(UDSDtcSettingType *)data;
    uint8_t ret = kPositiveResponse;
    uint8_t dtc_off;
    switch (r) {
    case kDTCSettingON:
        Dem_EnableDTCStorage(DEM_DTC_GROUP_ALL_DTCS, DEM_DTC_KIND_ALL_DTCS);
        break;
    case kDTCSettingOFF:
        Dem_DisableDTCStorage(DEM_DTC_GROUP_ALL_DTCS, DEM_DTC_KIND_ALL_DTCS);
        break;
    default:
        ret = kSubFunctionNotSupported;
        break;
    }
    return ret;
}
#endif

static uint8_t session_timeout(UDSServer_t *srv, const void *data)
{
    log_warning("%s\n", "session timeout!");
    return kPositiveResponse;
}

static uint8_t _0x10_Session_Ctrl(UDSServer_t *srv, const void *data)
{
    uint8_t ret = kPositiveResponse;
    UDSDiagSessCtrlArgs_t *r = (UDSDiagSessCtrlArgs_t *)data;
    if (r->type == kProgrammingSession) {
        // 扩展会话请求不支持抑制正响应
        UdsServerWithSubFuncNotSuppressPosRsp(srv);
    }

    if ((srv->sessionType == kDefaultSession && r->type == kProgrammingSession)
        || (srv->sessionType == kProgrammingSession && r->type == kExtendedDiagnostic)) {
        ret = kSubFunctionNotSupportedInActiveSession;
    } else if (srv->sessionType == kExtendedDiagnostic && r->type == kProgrammingSession
               && Ota_Para.Routine_preConditionsCheck_flag == false) {
        ret = kConditionsNotCorrect;
    } else {
        switch (r->type) {
        case kDefaultSession:
            memset(&Ota_Para, 0, sizeof(Ota_Para));
#if UDS_SERVER_IN_BOOT
            ECUAL_Reset();
#endif
            break;
        case kProgrammingSession:
#if UDS_SERVER_IN_BOOT
            BootTryCountUp();
#else
            detectP2TimeAndResponsePending(srv);
            writeUpdateReqflag(PROGRAM_UPDATE_REQ_FLAG);
            resetBegin(srv, kHardReset, 5);
            ret = kRequestCorrectlyReceived_ResponsePending;
#endif
            break;
        default:
            break;
        }
    }
    return ret;
}

static uint8_t _0x11_Diag_EcuReset(UDSServer_t *srv, const void *data)
{
    /*service :reset: session、 security、 Communication、transfer*/
    UDSECUResetArgs_t *r = (UDSECUResetArgs_t *)data;
    uint8_t ret = kPositiveResponse;
    switch (r->type) {
    case kHardReset:
        ret = kPositiveResponse;
        break;
    case kKeyOffOnReset:
    case kSoftReset:
    default:
        ret = kSubFunctionNotSupported;
        break;
    }
    return ret;
}

static uint8_t DoScheduledReset(UDSServer_t *srv, const void *data)
{
    uint8_t ret = kPositiveResponse;
    log_info("%s", "ECU reset!\n");
    switch (*(uint8_t *)data) {
    case kHardReset:
    case kKeyOffOnReset:
    case kSoftReset:
        ECUAL_Reset();
    default:
        ret = kSubFunctionNotSupported;
        break;
    }
    return ret;
}

static uint8_t _0x22_ReadDataByIdent(UDSServer_t *srv, const void *data)
{
    UDSRDBIArgs_t *r = (UDSRDBIArgs_t *)data;
    uint8_t tempData[128], didLen;
    uint8_t ret = kPositiveResponse;
    if (!DID_isVaildRW(r->dataId, DID_READ)) {
        ret = kRequestOutOfRange;
    } else if (!DID_isValidRWSession(r->dataId, DID_READ, srv->sessionType)) {
        ret = kSubFunctionNotSupportedInActiveSession;
    } else if (!DID_isValidRWSecurity(r->dataId, DID_READ,
                                      srv->securityLevel == SECURITY_LEVEL_FBL ? DID_SECU_FBL : DID_SECU_APP)) {
        ret = kSecurityAccessDenied;
    } else {
        if (DID_read(r->dataId, tempData, &didLen)) {
            // log_hex(tempData, didLen);
            r->copy(srv, tempData, didLen);
        } else {
            ret = kRequestOutOfRange;
        }
    }
    return ret;
}

static uint8_t _0x2E_WriteDataByIdent(UDSServer_t *srv, const void *data)
{
    UDSWDBIArgs_t *r = (UDSWDBIArgs_t *)data;
    uint8_t ret = kPositiveResponse;
    if (!DID_isVaildRW(r->dataId, DID_WRITE)) {
        ret = kRequestOutOfRange;
    } else if (getDidLength(r->dataId) != r->len) {
        ret = kIncorrectMessageLengthOrInvalidFormat;
    } else if (!DID_isValidRWSession(r->dataId, DID_WRITE, srv->sessionType)) {
        ret = kSubFunctionNotSupportedInActiveSession;
    } else if (!DID_isValidRWSecurity(r->dataId, DID_WRITE,
                                      srv->securityLevel == SECURITY_LEVEL_FBL ? DID_SECU_FBL : DID_SECU_APP)) {
        ret = kSecurityAccessDenied;
    } else if (!DID_isValidFormat(r->dataId, r->data, r->len)) {
        ret = kRequestOutOfRange;
    } else {
        detectP2TimeAndResponsePending(srv);
        if (DID_write(r->dataId, r->data, r->len)) {
            log_hex(r->data, r->len);
            switch (r->dataId) {
            case 0xF184:
                Ota_Para.WriteFinger_flag = true;
                break;
            default:
                break;
            }
        } else {
            ret = kGeneralProgrammingFailure;
        }
    }
    return ret;
}

static uint8_t _0x27_SecurityGetSeed(UDSServer_t *srv, const void *data)
{
    UDSSecAccessRequestSeedArgs_t *r = (UDSSecAccessRequestSeedArgs_t *)data;
    uint8_t ret = kPositiveResponse;
    uint8_t FailAuthCount = 0;

    if (srv->sessionType == kDefaultSession) {
        ret = kServiceNotSupportedInActiveSession;
    } else if (r->len != 0) {
        ret = kIncorrectMessageLengthOrInvalidFormat;
    } else if (SECURITY_LEVEL_FBL != r->level && SECURITY_LEVEL_APP != r->level) {
        ret = kSubFunctionNotSupported;
    } else if (srv->sessionType != kExtendedDiagnostic && r->level == SECURITY_LEVEL_APP
               || srv->sessionType != kProgrammingSession && r->level == SECURITY_LEVEL_FBL) {
        ret = kSubFunctionNotSupportedInActiveSession;
    } else {
        uint8_t *seed;
        if (srv->requestSecurityLevel == r->level) {
            seed = getSeed(false);
        } else {
            seed = getSeed(true);
        }
        log_hex(seed, SECURITY_SEED_SIZE);
        r->copySeed(srv, seed, SECURITY_SEED_SIZE);
    }
    return ret;
}

static uint8_t _0x27_SecurityCmpKey(UDSServer_t *srv, const void *data)
{
    UDSSecAccessValidateKeyArgs_t *r = (UDSSecAccessValidateKeyArgs_t *)data;
    uint8_t ret = kPositiveResponse;
    if (r->len != SECURITY_KEY_SIZE) {
        ret = kIncorrectMessageLengthOrInvalidFormat;
    } else if (SECURITY_LEVEL_FBL != r->level && SECURITY_LEVEL_APP != r->level) {
        ret = kSubFunctionNotSupported;
    } else if ((srv->sessionType != kExtendedDiagnostic && r->level == SECURITY_LEVEL_APP)
               || (srv->sessionType != kProgrammingSession && r->level == SECURITY_LEVEL_FBL)) {
        ret = kSubFunctionNotSupportedInActiveSession;
    } else {
        if (!compareKey(r->level, r->key, r->len)) {
            ret = kInvalidKey;
        } else {
            ret = kPositiveResponse;
        }
    }
    return ret;
}

static uint8_t RoutineControl_preConditionsCheck(UDSServer_t *srv, const UDSRoutineCtrlArgs_t *r)
{
#if UDS_SERVER_IN_BOOT
    uint8_t ret = kPositiveResponse;
    if (r->len != 0) {
        ret = kIncorrectMessageLengthOrInvalidFormat;
    } else if (SECURITY_LEVEL_FBL != srv->securityLevel && SECURITY_LEVEL_APP != srv->securityLevel) {
        ret = kSecurityAccessDenied;
    } else {
        uint8_t respChar = 0;
        r->copyStatusRecord(srv, &respChar, sizeof(respChar));
        Ota_Para.Routine_preConditionsCheck_flag = true;
        ret = kPositiveResponse;
    }
    return ret;
#else
    uint8_t ret = kPositiveResponse;
    if (r->len != 0) {
        ret = kIncorrectMessageLengthOrInvalidFormat;
    } else if (SECURITY_LEVEL_FBL != srv->securityLevel && SECURITY_LEVEL_APP != srv->securityLevel) {
        ret = kSecurityAccessDenied;
    } else {
        uint8_t respChar = 0;
        if (DIAG_POWER_STATE_NORMAL != DiagBatt_GetPowerState()) {
            respChar = 1;
            r->copyStatusRecord(srv, &respChar, sizeof(respChar));
        } else {
            r->copyStatusRecord(srv, &respChar, sizeof(respChar));
            Ota_Para.Routine_preConditionsCheck_flag = true;
        }
        ret = kPositiveResponse;
    }
    return ret;
#endif
}

static uint8_t RoutineControl_Start(UDSServer_t *srv, const void *data)
{
    UDSRoutineCtrlArgs_t *r = (UDSRoutineCtrlArgs_t *)data;
    uint8_t ret = kPositiveResponse;

    switch (r->id) {
    case 0x0203:
        ret = RoutineControl_preConditionsCheck(srv, data);
        break;
#if UDS_SERVER_IN_BOOT
    case 0x0202:
        ret = RoutineControl_downMemoryCheck(srv, data);
        break;
    case 0xFF00:
        ret = RoutineControl_earseMemory(srv, data);
        break;
    case 0xFF01:
        ret = RoutineControl_dependenceCheck(srv, data);
        break;
#endif
    default:
        ret = kRequestOutOfRange;
        break;
    }
    return ret;
}

static uint8_t _0x31_RoutineCtrl(UDSServer_t *srv, const void *data)
{
    UDSRoutineCtrlArgs_t *r = (UDSRoutineCtrlArgs_t *)data;
    uint8_t ret = kPositiveResponse;
    switch (r->ctrlType) {
    case kStartRoutine:
        ret = RoutineControl_Start(srv, data);
        break;
    case kStopRoutine:
    case kRequestRoutineResults:
    default:
        ret = kSubFunctionNotSupported;
        break;
    }
    return ret;
}

/*用户如果需要重载服务，返回对应函数指针；不重载返回NULL*/
UDSService userRegisterService(uint8_t sid)
{
    switch (sid) {
    case KSID_ATE_UDS:
        return SvrReadAteData;
    default:
        return NULL;
    }
}

uint8_t udsserver_event_callback(UDSServer_t *srv, int evt, const void *data)
{
    uint8_t ret = kPositiveResponse;
    switch (evt) {
    case UDS_SRV_EVT_SessionTimeout:
        ret = session_timeout(srv, data);
        break;
    case UDS_SRV_EVT_DiagSessCtrl:
        ret = _0x10_Session_Ctrl(srv, data);
        break;
    case UDS_SRV_EVT_EcuReset:
        ret = _0x11_Diag_EcuReset(srv, data);
        break;
    case UDS_SRV_EVT_DoScheduledReset:
        ret = DoScheduledReset(srv, data);
        break;
    case UDS_SRV_EVT_ReadDataByIdent:
        ret = _0x22_ReadDataByIdent(srv, data);
        break;
    case UDS_SRV_EVT_WriteDataByIdent:
        ret = _0x2E_WriteDataByIdent(srv, data);
        break;
    case UDS_SRV_EVT_SecAccessRequestSeed:
        ret = _0x27_SecurityGetSeed(srv, data);
        break;
    case UDS_SRV_EVT_SecAccessValidateKey:
        ret = _0x27_SecurityCmpKey(srv, data);
        break;
    case UDS_SRV_EVT_RoutineCtrl:
        ret = _0x31_RoutineCtrl(srv, data);
        break;
#if UDS_SERVER_IN_BOOT
    case UDS_SRV_EVT_RequestDownload:
        ret = _0x34_RequestDownload(srv, data);
        break;
    case UDS_SRV_EVT_TransferData:
        ret = _0x36_TransferData(srv, data);
        break;
    case UDS_SRV_EVT_RequestTransferExit:
        ret = _0x37_RequestTransferExit(srv, data);
        break;
#else
    case UDS_SRV_EVT_ClrDiagInfo:
        ret = _0x14_ClrDiagInfo(srv, data);
        break;
    case UDS_SRV_EVT_RepNumOfDTCByStaMask:
        ret = _0x1901_RepNumOfDTCByStaMask(srv, data);
        break;
    case UDS_SRV_EVT_RepDTCByStaMask:
        ret = _0x1902_RepDTCByStaMask(srv, data);
        break;
    case UDS_SRV_EVT_RepDTCSnapRecByDTCNum:
        ret = _0x1904_RepDTCSnapRecByDTCNum(srv, data);
        break;
    case UDS_SRV_EVT_RepDTCExtDataRecByDTCNum:
        ret = _0x1906_RepDTCExtRecByDTCNum(srv, data);
        break;
    case UDS_SRV_EVT_RepAllSupDTC:
        ret = _0x190A_RepAllSupDTC(srv, data);
        break;
    case UDS_SRV_EVT_CommCtrl:
        ret = _0x28_CommunicationControl(srv, data);
        break;
    case UDS_SRV_EVT_ControlDTC:
        ret = _0x85_ControlDTC(srv, data);
        break;
#endif
    default:
        log_debug("service not support! %d\n", evt);
    }
    return ret;
}
