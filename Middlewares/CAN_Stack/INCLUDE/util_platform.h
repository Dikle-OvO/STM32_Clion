/******************************************************************************
 * @file   : util_platform.h
 * @brief  : Utility platform functions for the CAN-Stack.
 * @author : yangtl
 * @date   : 2025/05/13 20:11:17
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#ifndef UTIL_PLATFORM_H
#define UTIL_PLATFORM_H

/*! @addtogroup utils
 *  @{
 */
#include "Std_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32 utick;

extern int util_printf(const char *fmt, ...);

extern utick util_get_ticks(void);

extern uint32 util_enter_critical(void);

extern void util_exit_critical(uint32 flags);

/**
 * @brief U_SAFE_AREA0
 * @attention the code within curly braces cannot use "return" and "break"!
 */
#define U_SAFE_AREA0 for (uint32 flg = util_enter_critical(), done = 0; !done; done = 1, util_exit_critical(flg))

#define U_SAFE_AREA do { uint32_t flg = util_enter_critical(); do
#define U_END_SAFE_AREA while(0); util_exit_critical(flg); } while(0)

#ifdef __cplusplus
}
#endif

/*! @}*/

#endif /* UTIL_PLATFORM_H */
/*********************************END OF FILE*********************************/
