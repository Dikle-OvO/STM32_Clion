/******************************************************************************
 * @file   : util_macro.h
 * @brief  : Utility macros for the CAN-Stack.
 * @author : yangtl
 * @date   : 2025/05/13 14:36:26
 * @version: 0.1
 *
 * @copyright (C) 2025, yftech Development Team.
 *****************************************************************************/
#ifndef UTIL_MACRO_H
#define UTIL_MACRO_H

/*! @addtogroup util
 *  @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#define U_UNUSED(a) (void)(a)

#ifndef U_MIN
#define U_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef U_MAX
#define U_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#define U_COUNTOF(a)                     (sizeof(a) / sizeof((a)[0])) //

#define U_OFFSETOF(type, member)         (size_t)(&((type *)0)->member)

/**
 * @brief cast a member of a structure out to the containing structure
 * @param[in] ptr the pointer to the member.
 * @param[in] type the type of the container struct this is embedded in.
 * @param[in] member the name of the member within the struct.
 *
 * WARNING: any const qualifier of "ptr" is lost.
 */
#define U_CONTAINEROF(ptr, type, member) (type *)((char *)(ptr) - U_OFFSETOF(type, member))

#ifdef __cplusplus
}
#endif

/*! @}*/

#endif /* UTIL_MACRO_H */
/*********************************END OF FILE*********************************/
