/**
 * @file     sample_vo_main.c
 * @brief    vo模块示例代码的main
 * @version  1.0.0
 * @since    1.0.0
 * @author
 * @date    2021-10-25 创建文件
 */
/************************************************************
 *@note
    Copyright 2021, BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
                   ALL RIGHTS RESERVED
Permission is hereby granted to licensees of  BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
to use or abstract this computer program for the sole purpose of implementing a product based on  BEIJIING
SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. No other rights to reproduce, use, or disseminate this computer
program,whether in part or in whole, are granted.  BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
makes no representation or warranties with respect to the performance of this computer program, and specifically
disclaims any responsibility for any damages, special or consequential, connected with the use of this program.
　　For details, see http://www.sgitg.sgcc.com.cn/
**********************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "sample_vdec.h"
#include "sample_comm.h"
#include "mpi_sys.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


extern VO_INTF_SYNC_E g_enIntfSync;

/******************************************************************************
* function : show usage
******************************************************************************/
SC_VOID SAMPLE_VDEC_Usage(char *sPrgNm)
{
    printf("\n/************************************/\n");
    printf("Usage : %s <index> <IntfSync >\n", sPrgNm);
    printf("index:\n");
    printf("\t0:  VDEC(H265 PLAYBACK)-VPSS-VO\n");
    printf("\t1:  VDEC(H264 PLAYBACK)-VPSS-VO\n");
    printf("\t2:  VDEC(H264)-FILE\n");
    printf("\t3:  VDEC(H264)-FILE(MULTI-CHANNEL)\n");

    printf("\nIntfSync :\n");
    printf("\t0: VO HDMI   4K@30fps.\n");
    printf("\t1: VO BT1120 1080P@30fps.\n");
    printf("/************************************/\n\n");
}

/******************************************************************************
* function    : main()
* Description : main
******************************************************************************/
/******************************************************************************
* function    : main()
* Description : video vdec sample
******************************************************************************/
int main(int argc, char *argv[])
{
    SC_S32 s32Ret = SC_SUCCESS;

    if ((argc < 2) || (1 != strlen(argv[1])))
    {
        printf("\nInvaild input!  For examples:\n");
        SAMPLE_VDEC_Usage(argv[0]);
        return SC_FAILURE;
    }

    if ((argc > 2) && ('1' == *argv[2]))
    {
        g_enIntfSync = VO_OUTPUT_1080P30;
    }
    else
    {
        g_enIntfSync = VO_OUTPUT_3840x2160_30;
    }

    signal(SIGINT, SAMPLE_VDEC_HandleSig);
    signal(SIGTERM, SAMPLE_VDEC_HandleSig);

    /******************************************
     choose the case
    ******************************************/
    switch (*argv[1])
    {
        case '0':
        {
            s32Ret = SAMPLE_H265_VDEC_VPSS_VO();
            break;
        }
        case '1':
        {
            s32Ret = SAMPLE_H265_VDEC();
            break;
        }
        case '2':
        {
            s32Ret = SAMPLE_H264_VDEC();
            break;
        }
        case '3':
        {
            s32Ret = SAMPLE_H264_VDEC_MULTI_CHN();
            break;
        }
        default :
        {
            SAMPLE_PRT("the index is invaild!\n");
            SAMPLE_VDEC_Usage(argv[0]);
            s32Ret = SC_FAILURE;
            break;
        }
    }

    if (SC_SUCCESS == s32Ret)
    {
        SAMPLE_PRT("program exit normally!\n");
    }
    else
    {
        SAMPLE_PRT("program exit abnormally!\n");
    }

    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
