/******************************************************************************
 * @file   : util_tick.h
 * @brief  : Utility tick functions for the CAN-Stack.
 * @author : yangtl
 * @date   : 2025/05/09 21:22:11
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#ifndef UTIL_TICK_H
#define UTIL_TICK_H

/*! @addtogroup util
 *  @{
 */

#include "util_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef U_TIME_AFTER_EQ
#define U_TIME_AFTER_EQ(a, b) ((long)((a) - (b)) >= 0)
#endif

typedef struct {
    utick tick;
    boolean started;
} uticker;

static inline void utick_reset(utick *tick, utick timeout)
{
    *tick = util_get_ticks() + timeout;
}

static inline boolean utick_is_expired(utick *tick)
{
    return U_TIME_AFTER_EQ(util_get_ticks(), *tick);
}

boolean uticker_start(uticker *ticker, utick tick);
boolean uticker_stop(uticker *ticker);
boolean uticker_is_started(uticker *ticker);
boolean uticker_is_expired(uticker *ticker);

#ifdef __cplusplus
}
#endif

/*! @}*/

#endif /* UTIL_TICK_H */
/*********************************END OF FILE*********************************/
