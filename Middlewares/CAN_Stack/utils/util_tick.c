/******************************************************************************
 * @file   : util_tick.c
 * @brief  :
 * @author : yangtl
 * @date   : 2025/05/09 21:22:20
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#include <stddef.h>
#include "util_tick.h"

boolean uticker_start(uticker *ticker, utick msec)
{
    ticker->tick = util_get_ticks() + msec;
    ticker->started = TRUE;

    return TRUE;
}

boolean uticker_stop(uticker *ticker)
{
    if (ticker->started) {
        ticker->started = FALSE;
    }

    return TRUE;
}

boolean uticker_is_started(uticker *ticker)
{
    return ticker->started;
}

boolean uticker_is_expired(uticker *ticker)
{
    boolean expired = FALSE;

    if (ticker->started) {
        expired = U_TIME_AFTER_EQ(util_get_ticks(), ticker->tick);
    }

    return expired;
}

/*********************************END OF FILE*********************************/
