/*
 * @file     sc_debug.h
 * @brief    调试打印输出
 * @version  1.0.1
 * @since    1.0.0
 * @author
 */

/*
 ********************************************************************************************************
 * Copyright 2021, BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
 *             ALL RIGHTS RESERVED
 * Permission is hereby granted to licensees of BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
 * to use or abstract this computer program for the sole purpose of implementing a product based on
 * BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. No other rights to reproduce, use,
 * or disseminate this computer program,whether in part or in whole, are granted.
 * BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. makes no representation or warranties
 * with respect to the performance of this computer program, and specifically disclaims
 * any responsibility for any damages, special or consequential, connected with the use of this program.
 * For details, see http://www.sgitg.sgcc.com.cn/
 ********************************************************************************************************
 */

#ifndef __SC_DEBUG_H__
#define __SC_DEBUG_H__

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "sc_type.h"
#include "sc_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define CONFIGSC_LOG_LEVEL SC_LOGLV_WARN /* 编译时打印级别 */

typedef enum
{
    SC_LOGLV_EMERG = 0,   /* system is unusable                   */
    SC_LOGLV_ALERT,        /* action must be taken immediately     */
    SC_LOGLV_ERR,          /* error conditions                     */
    SC_LOGLV_CRIT,         /* critical conditions                  */
    SC_LOGLV_WARN,         /* warning conditions                   */
    SC_LOGLV_NOTICE,       /* normal but significant condition     */
    SC_LOGLV_INFO,         /* informational                        */
    SC_LOGLV_DEBUG,        /* debug-level messages                 */

    SC_LOGLV_CNT,          /* */
} SC_LOGLV_E;

#define SC_ASSERT(expr)               \
do{                                   \
    if (!(expr)) {                    \
        printf("\nASSERT at:\n"       \
               "  >Function : %s\n"   \
               "  >Line No. : %d\n"   \
               "  >Condition: %s\n",  \
               __FUNCTION__, __LINE__, #expr);\
        assert(0);\
    } \
}while(0)

#define SC_TRACE(enLogLv, enModId, format, args...)  \
    do {                                         \
        sc_trace(enModId, enLogLv,               \
            __FUNCTION__, __LINE__,              \
            format, ##args);              \
    } while (0)

#if (CONFIGSC_LOG_LEVEL >= SC_LOGLV_EMERG)

#define SC_TRACE_EMERG(fmt, args...)\
do {\
    SC_TRACE(SC_LOGLV_EMERG, SC_LOGMOD, fmt, ##args);\
} while (0)\

#else
#define SC_TRACE_EMERG(fmt, args...) do{}while(0)
#endif

#if (CONFIGSC_LOG_LEVEL >= SC_LOGLV_ALERT)

#define SC_TRACE_ALERT(fmt, args...)\
do {\
    SC_TRACE(SC_LOGLV_ALERT, SC_LOGMOD, fmt, ##args);\
} while (0)\

#else
#define SC_TRACE_ALERT(fmt, args...) do{}while(0)
#endif

#if (CONFIGSC_LOG_LEVEL >= SC_LOGLV_CRIT)

#define SC_TRACE_CRIT(fmt, args...)\
do {\
    SC_TRACE(SC_LOGLV_CRIT, SC_LOGMOD, fmt, ##args);\
} while (0)\

#else
#define SC_TRACE_CRIT(fmt, args...) do{}while(0)
#endif

#if (CONFIGSC_LOG_LEVEL >= SC_LOGLV_ERR)

#define SC_TRACE_ERR(fmt, args...)\
do {\
    SC_TRACE(SC_LOGLV_ERR, SC_LOGMOD, fmt, ##args);\
} while (0)\

#else
#define SC_TRACE_ERR(fmt, args...) do{}while(0)
#endif

#if (CONFIGSC_LOG_LEVEL >= SC_LOGLV_WARN)

#define SC_TRACE_WARN(fmt, args...)\
do {\
    SC_TRACE(SC_LOGLV_WARN, SC_LOGMOD, fmt, ##args);\
} while (0)\

#else
#define SC_TRACE_WARN(fmt, args...) do{}while(0)
#endif

#if (CONFIGSC_LOG_LEVEL >= SC_LOGLV_NOTICE)

#define SC_TRACE_NOTICE(fmt, args...)\
do {\
    SC_TRACE(SC_LOGLV_NOTICE, SC_LOGMOD, fmt, ##args);\
} while (0)\

#else
#define SC_TRACE_NOTICE(fmt, args...) do{}while(0)
#endif

#if (CONFIGSC_LOG_LEVEL >= SC_LOGLV_INFO)

#define SC_TRACE_INFO(fmt, args...)\
do {\
    SC_TRACE(SC_LOGLV_INFO, SC_LOGMOD, fmt, ##args);\
} while (0)\

#else
#define SC_TRACE_INFO(fmt, args...) do{}while(0)
#endif

#if (CONFIGSC_LOG_LEVEL >= SC_LOGLV_DEBUG)

#define SC_TRACE_DEBUG(fmt, args...)\
do {\
    SC_TRACE(SC_LOGLV_DEBUG, SC_LOGMOD, fmt, ##args);\
} while (0)\

#else
#define SC_TRACE_DEBUG(fmt, args...) do{}while(0)
#endif

#ifdef DEBUG
#  define SC_PR_DBG(fmt, args...)\
do { \
    fprintf(stderr, "[%s][%d]" fmt, \
        __FUNCTION__, __LINE__,   \
        ##args);   \
} while (0)

#else
#  define SC_PR_DBG(fmt, args...) do{}while(0)
#endif

void sc_trace(
    MOD_ID_E enModId,
    SC_LOGLV_E enLogLv,
    const char *func,
    int line,
    const char *format, ...) ;
void sc_trace_set_lv(MOD_ID_E enModId, SC_LOGLV_E enLogLv);
const char *sc_trace_get_mod_name(MOD_ID_E enModId);

SC_S32 sc_logmpp_read(const char **str);
SC_S32 sc_logmpp_write(char *str);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __SC_DEBUG_H__ */

