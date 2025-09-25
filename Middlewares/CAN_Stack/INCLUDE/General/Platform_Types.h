/******************************************************************************
 * @file   : Platform_Types.h
 * @brief  : <Platform Types for Classic Platform AUTOSAR CP R24-11>
 * @author : yangtl
 * @date   : 2025/04/21 16:22:17
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#ifndef PLATFORM_TYPES_H
#define PLATFORM_TYPES_H

/*! @addtogroup General
 *  @{
 */

#if !defined(NO_STDINT_H)
#include <stdbool.h>
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define CPU_TYPE_8      8
#define CPU_TYPE_16     16
#define CPU_TYPE_32     32
#define CPU_TYPE_64     64

#define MSB_FIRST       0
#define LSB_FIRST       1

#define HIGH_BYTE_FIRST 0
#define LOW_BYTE_FIRST  1


#ifndef TRUE
#if defined(NO_STDINT_H)
#define TRUE 1
#else
#define TRUE true
#endif
#endif

#ifndef FALSE
#if defined(NO_STDINT_H)
#define FALSE 0
#else
#define FALSE false
#endif
#endif

#if defined(NO_STDINT_H)
typedef unsigned char boolean;
#else
typedef bool boolean;
#endif

#if defined(NO_STDINT_H)
typedef signed char        sint8;
typedef signed short       sint16;
typedef signed long        sint32;
typedef signed long long   sint64;
typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned long      uint32;
typedef unsigned long long uint64;
typedef unsigned long      uint8_least;
typedef unsigned long      uint16_least;
typedef unsigned long      uint32_least;
typedef signed long        sint8_least;
typedef signed long        sint16_least;
typedef signed long        sint32_least;
#else
typedef int8_t             sint8;
typedef int16_t            sint16;
typedef int32_t            sint32;
typedef int64_t            sint64;
typedef uint8_t            uint8;
typedef uint16_t           uint16;
typedef uint32_t           uint32;
typedef uint64_t           uint64;
typedef uint_least8_t      uint8_least;
typedef uint_least16_t     uint16_least;
typedef uint_least32_t     uint32_least;
typedef int_least8_t       sint8_least;
typedef int_least16_t      sint16_least;
typedef int_least32_t      sint32_least;
#endif

typedef float       float32;
typedef double      float64;
typedef void*       VoidPtr;
typedef const void* ConstVoidPtr;

#ifdef __cplusplus
}
#endif

/*! @}*/

#endif /* PLATFORM_TYPES_H */
/*********************************END OF FILE*********************************/
