/******************************************************************************
 * @file   : util_platform.c
 * @brief  :
 * @author : yangtl
 * @date   : 2025/05/15 19:23:48
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include "util_platform.h"

#include "main.h"

extern uint32_t global_ms;
extern UART_HandleTypeDef huart1;

int util_printf(const char *fmt, ...)
{
    va_list args;
    static char strbuf[2048];

    va_start(args, fmt);
    if (vsprintf(strbuf, fmt, args) < 0) {
        fprintf(stderr, "Error: vsprintf failed\n");
        va_end(args);
        return -1;
    }
    va_end(args);

    char log_buf[128];
    snprintf(log_buf, sizeof(log_buf), "[%lu] %s", global_ms, strbuf);
    UART_Send_IT(&huart1,log_buf);
    return 0;
}

utick util_get_ticks(void)
{
    return global_ms;
}

uint32 util_enter_critical(void)
{
    // util_printf("enter critical\n");
    return 0;
}

void util_exit_critical(uint32 flags)
{
    // util_printf("exit critical\n");
}

/*********************************END OF FILE*********************************/
