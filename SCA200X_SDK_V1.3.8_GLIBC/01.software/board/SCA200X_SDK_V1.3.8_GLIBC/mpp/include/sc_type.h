/**
 * @file     sc_type.h
 * @brief    基础类型定义
 * @version  1.0.0
 * @since    1.0.0
 * @author  陶友水<taoyoushui@sgitg.sgcc.com.cn>
 * @date    2021-07-15 创建文件
 */
/************************************************************
 *@note
    Copyright 2021, BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
                   ALL RIGHTS RESERVED
    Permission is hereby granted to licensees of  BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. to use
    or abstract this computer program for the sole purpose of implementing a product based on BEIJIING SMARTCHIP
    MICROELECTRONICS TECHNOLOGY CO., LTD. No other rights to reproduce, use, or disseminate this computer program,
    whether in part or in whole, are granted. BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. makes no
    representation or warranties with respect to the performance of this computer program, and specifically disclaims
    any responsibility for any damages, special or consequential, connected with the use of this program.
　　For details, see http://www.sgitg.sgcc.com.cn/
**********************************************************/

#ifndef __SC_TYPE_H__
#define __SC_TYPE_H__

#ifdef __KERNEL__

    #include <linux/types.h>
#else

    #include <stdint.h>
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

typedef unsigned char           SC_UCHAR;
typedef unsigned char           SC_U8;
typedef unsigned short          SC_U16;
typedef unsigned int            SC_U32;
typedef unsigned long           SC_UL;
typedef SC_UL                   SC_ULONG;
typedef uintptr_t               SC_UINTPTR_T;

typedef char                    SC_CHAR;
typedef signed char             SC_S8;
typedef short                   SC_S16;
typedef int                     SC_S32;
typedef long                    SC_SL;

typedef float                   SC_FLOAT;
typedef double                  SC_DOUBLE;

typedef unsigned long long      SC_U64;
typedef long long               SC_S64;

typedef unsigned long           SC_SIZE_T;
typedef unsigned long           SC_LENGTH_T;
typedef unsigned long int       SC_PHYS_ADDR_T;

typedef unsigned int            SC_HANDLE;
typedef void                   *SC_PVOID;

/*--u16bit--------------------------------------------*/
typedef unsigned short          SC_U0Q16;

/*----------------------------------------------*
 * const defination                             *
 *----------------------------------------------*/
typedef enum
{
    SC_FALSE = 0,
    SC_TRUE  = 1,
} SC_BOOL;

#ifndef NULL
#define NULL                0L
#endif

#define SC_NULL                 0L
#define SC_SUCCESS              0
#define SC_FAILURE              (-1)
#define SC_NOSUPPORT            (-128)

#define SC_VOID                 void

typedef unsigned char           sc_uchar;
typedef unsigned char           sc_u8;
typedef unsigned short          sc_u16;
typedef unsigned int            sc_u32;
typedef unsigned long           sc_ulong;

typedef char                    sc_char;
typedef signed char             sc_s8;
typedef short                   sc_s16;
typedef int                     sc_s32;
typedef long                    sc_slong;

typedef float                   sc_float;
typedef double                  sc_double;

typedef void                    sc_void;

typedef unsigned long long      sc_u64;
typedef long long               sc_s64;

typedef unsigned long           sc_size_t;
typedef unsigned long           sc_length_t;

typedef sc_u32                  sc_handle;

typedef SC_BOOL                 SC_bool;
typedef SC_UINTPTR_T            sc_uintptr_t;

#define SC_UNUSED(x)            ((x) = (x))

/**

define of SC_HANDLE :
bit31                                                           bit0
  |<----   16bit --------->|<---   8bit    --->|<---  8bit   --->|
  |--------------------------------------------------------------|
  |      SC_MOD_ID_E       |  mod defined data |     chnID       |
  |--------------------------------------------------------------|

mod defined data: private data define by each module(for example: sub-mod id), usually, set to 0.
*/

#define SC_HANDLE_MAKEHANDLE(mod, privatedata, chnid)  (sc_handle)( (((mod)& 0xffff) << 16) | ((((privatedata)& 0xff) << 8) ) | (((chnid) & 0xff)) )
#define SC_HANDLE_GET_MODID(handle)     (((handle) >> 16) & 0xffff)
#define SC_HANDLE_GET_PriDATA(handle)   (((handle) >> 8) & 0xff)
#define SC_HANDLE_GET_CHNID(handle)     (((handle)) & 0xff)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __SC_TYPE_H__ */

