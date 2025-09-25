/******************************************************************************
 * @file   : CanStack_General_Cfg.h
 * @brief  : The Pre-Compiled time config header file of the general module
 * @author : yangtl
 * @date   : 2025/04/21 20:28:32
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#ifndef __CANSTACK_GENERAL_CFG_H__
#define __CANSTACK_GENERAL_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CPU_TYPE
#define CPU_TYPE CPU_TYPE_32
#endif

#ifndef CPU_BIT_ORDER
#define CPU_BIT_ORDER LSB_FIRST
#endif

#ifndef CPU_BYTE_ORDER
#define CPU_BYTE_ORDER LOW_BYTE_FIRST
#endif

#ifdef __cplusplus
}
#endif

#endif /* __CANSTACK_GENERAL_CFG_H__ */
/*********************************END OF FILE*********************************/
