/******************************************************************************
 * @file   : Std_Types.h
 * @brief  : <Standard Types AUTOSAR CP R24-11>
 * @author : yangtl
 * @date   : 2025/04/21 16:23:48
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#ifndef STD_TYPES_H
#define STD_TYPES_H

/*! @addtogroup General
 *  @{
 */

#include "Platform_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Because E_OK is already defined within OSEK, the symbol E_OK has to be
 * shared. To avoid name clashes and redefinition problems, the symbols have to
 * be defined in the following way (approved within implementation):
 */
#ifndef STATUSTYPEDEFINED
#define STATUSTYPEDEFINED
#define E_OK 0x00u
typedef uint8 StatusType; /* OSEK compliance */
#endif
#define E_NOT_OK   0x01u

#define STD_HIGH   0x01u /*!< Physical state 5V or 3.3V */
#define STD_LOW    0x00u /*!< Physical state 0V */

#define STD_ACTIVE 0x01u /*!< Logical state active */
#define STD_IDLE   0x00u /*!< Logical state idle */

#define STD_ON     0x01u
#define STD_OFF    0x00u


#ifndef NULL_PTR
#define NULL_PTR   ((void *)0)
#endif

/**
 * @brief This type can be used as standard API return type which is shared
 * between the RTE and the BSW modules.
 * @note Range: E_OK,
 *              E_NOT_OK,
 *              0x02-0x3F: Available to user specific errors.
 */
typedef uint8 Std_ReturnType;

/**
 * @brief This type shall be used to request the version of a BSW module using
 *        the <Module name>_GetVersionInfo() function.
 */
typedef struct {
    uint16 vendorID;
    uint16 moduleID;
    uint8  sw_major_version;
    uint8  sw_minor_version;
    uint8  sw_patch_version;
} Std_VersionInfoType;

#ifdef __cplusplus
}
#endif


#include "General_Cfg.h"
/*
#define CPU_TYPE            CPU_TYPE_32
#define CPU_BIT_ORDER       LSB_FIRST
#define CPU_BYTE_ORDER      LOW_BYTE_FIRST
*/

/*! @}*/

#endif /* STD_TYPES_H */
/*********************************END OF FILE*********************************/
