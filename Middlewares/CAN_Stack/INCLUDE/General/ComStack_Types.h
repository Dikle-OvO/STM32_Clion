/******************************************************************************
 * @file   : ComStack_Types.h
 * @brief  : <Communication Stack Types AUTOSAR CP R24-11>
 * @author : yangtl
 * @date   : 2025/04/21 21:02:44
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#ifndef COMSTACK_TYPES_H
#define COMSTACK_TYPES_H

/*! @addtogroup General
 *  @{
 */

#include "Std_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16 PduIdType;

typedef uint32 PduLengthType;

typedef struct {
    /*!< Pointer to the meta data (e.g. CAN ID, socket ID, diagnostic addresses)
     * of the PDU, consisting of a sequence of meta data items. The length
     * and type of the meta data items is statically configured for each PDU.
     * Meta data items with more than 8 bits use platform byte order.
     */
    uint8 *MetaDataPtr;

    /*!< Pointer to the SDU (i.e. payload data) of the PDU. The type of this
     * pointer depends on the memory model being used at compile time.
     */
    uint8 *SduDataPtr;

    /*!< Length of the SDU in bytes. */
    PduLengthType SduLength;
} PduInfoType;

typedef uint8 NetworkHandleType;

typedef uint8 PNCHandleType;

/**
 * @brief Specify the parameter to which the value has to be changed (BS or STmin).
 */
typedef enum {
    TP_STMIN, /*!< Separation Time */
    TP_BS,    /*!< Block Size*/
    TP_BC     /*!< The Band width control parameter used in FlexRay transport protocol module */
} TPParameterType;

/**
 * @brief Variables of this type shall be used to store the result of a buffer request.
 */
typedef enum {
    BUFREQ_OK,
    BUFREQ_E_NOT_OK,
    BUFREQ_E_BUSY,
    BUFREQ_E_OVFL
} BufReq_ReturnType;

/**
 * @brief Variables of this type shall be used to store the state of TP buffer.
 */
typedef enum {
    TP_DATACONF,
    TP_DATARETRY,
    TP_CONFPENDING
} TpDataStateType;

/**
 * @brief Variables of this type shall be used to store the information about Tp buffer handling.
 */
typedef struct {
    TpDataStateType TpDataState;
    PduLengthType   TxTpDataCnt;
} RetryInfoType;

#ifdef __cplusplus
}
#endif

/*! @}*/

#endif /* COMSTACK_TYPES_H */
/*********************************END OF FILE*********************************/
