/**
 * @file hal_type.h
 * @brief  hal 基础数据类型定义
 * @author SmartChip Software Team
 * @version 0.0.1
 * @date 2021/04/21
 * @license 2021-2025, SmartChip. Co., Ltd.
**/

#ifndef __HAL_TYPE_H__
#define __HAL_TYPE_H__
#include <stdint.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/*----------------------------------------------*
 * The common data type, will be used in the whole project.*
 *----------------------------------------------*/
#ifndef SC_U8
typedef unsigned char           SC_U8;
#endif
#ifndef SC_U16
typedef unsigned short          SC_U16;
#endif
#ifndef SC_U32
typedef unsigned int            SC_U32;
#endif
#ifndef SC_S8
typedef signed char             SC_S8;
#endif
#ifndef SC_S16
typedef short                   SC_S16;
#endif
#ifndef SC_S32
typedef int                     SC_S32;
#endif
#ifndef SC_FLOAT
typedef float                   SC_FLOAT;
#endif
#ifndef SC_DOUBLE
typedef double                  SC_DOUBLE;
#endif
#ifndef SC_U64
typedef unsigned long long      SC_U64;
#endif
#ifndef SC_S64
typedef long long               SC_S64;
#endif
#ifndef SC_CHAR
typedef char                    SC_CHAR;
#endif
#ifndef SC_UCHAR
typedef unsigned char           SC_UCHAR;
#endif
#ifndef SC_HANDLE
typedef unsigned int            SC_HANDLE;
#endif

#ifndef SC_VOID
#define SC_VOID                 void
#endif

#if 0
//on 64bit platform
#ifndef SC_INTPTR
typedef intptr_t     SC_INTPTR;
#endif
#ifndef SC_UINTPTR
typedef uintptr_t    SC_UINTPTR;
#endif
#endif
/*----------------------------------------------*
 * const defination                             *
 *----------------------------------------------*/
typedef enum {
	SC_FALSE = 0,
	SC_TRUE  = 1,
} SC_BOOL;

#ifndef NULL
#define NULL        0L
#endif

#define SC_NULL         0L
#define SC_SUCCESS      0
#define SC_FAILURE      (-1)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __SC_TYPE_H__ */

