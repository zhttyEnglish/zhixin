/**
 * @file     sc_sns_ctrl.h
 * @brief    SENSOR回调控制
 * @version  1.0.0
 * @since    1.0.0
 * @author   陶友水<taoyoushui@sgitg.sgcc.com.cn>
 * @date     2021-11-12 创建文件
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

#ifndef __SC_SNS_CTRL_H__
#define __SC_SNS_CTRL_H__

#include "sc_type.h"
#include "sc_comm_3a.h"
#include "sc_isp_debug.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct scISP_SNS_STATE_S
{
    SC_BOOL     bInit;                  /* SC_TRUE: Sensor init */
    SC_BOOL     bSyncInit;              /* SC_TRUE: Sync Reg init */

    SC_U8       u8ImgMode;
    SC_U8       u8Hdr;                  /* SC_TRUE: HDR enbale */

    WDR_MODE_E  enWDRMode;

    ISP_SNS_REGS_INFO_S astRegsInfo[2]; /* [0]: Sensor reg info of cur-frame; [1]: Sensor reg info of pre-frame */

    SC_U32      au32FL[2];              /* [0]: FullLines of cur-frame; [1]: Pre FullLines of pre-frame */
    SC_U32      u32FLStd;               /* FullLines std */
    SC_U32      u32FLMid;
    SC_U32      u32FLShort;
    SC_U32      au32WDRIntTime[4];      /* [0]: Short exposure time; [1]: Long exposure time; */
} ISP_SNS_STATE_S;

typedef enum scISP_SNS_MIRRORFLIP_TYPE_E
{
    ISP_SNS_NORMAL      = 0,
    ISP_SNS_MIRROR      = 1,
    ISP_SNS_FLIP        = 2,
    ISP_SNS_MIRROR_FLIP = 3,
    ISP_SNS_BUTT
} ISP_SNS_MIRRORFLIP_TYPE_E;

typedef struct scISP_SNS_OBJ_S
{
    SC_S32  (*pfnRegisterCallback)(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib);
    SC_S32  (*pfnUnRegisterCallback)(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib);
    SC_S32  (*pfnSetBusInfo)(VI_PIPE ViPipe, ISP_SNS_COMMBUS_U unSNSBusInfo);
    SC_VOID (*pfnStandby)(VI_PIPE ViPipe);
    SC_VOID (*pfnRestart)(VI_PIPE ViPipe);
    SC_VOID (*pfnMirrorFlip)(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip);
    SC_S32  (*pfnWriteReg)(VI_PIPE ViPipe, SC_S32 s32Addr, SC_S32 s32Data);
    SC_S32  (*pfnReadReg)(VI_PIPE ViPipe, SC_S32 s32Addr);
    SC_S32  (*pfnSetInit)(VI_PIPE ViPipe, ISP_INIT_ATTR_S *pstInitAttr);
} ISP_SNS_OBJ_S;

#define CMOS_CHECK_POINTER(ptr)\
    do {\
        if (SC_NULL == ptr)\
        {\
            printf("Pointer is null!\n");\
            return SC_FAILURE;\
        }\
    }while(0)

#define CMOS_CHECK_POINTER_VOID(ptr)\
    do {\
        if (SC_NULL == ptr)\
        {\
            printf("Pointer is null!\n");\
            return;\
        }\
    }while(0)

#define SENSOR_FREE(ptr)\
    do{\
        if (SC_NULL != ptr)\
        {\
            free(ptr);\
            ptr = SC_NULL;\
        }\
    } while (0)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __SC_SNS_CTRL_H__ */
