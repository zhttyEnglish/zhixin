/**
 * @file     sample_vo_main.c
 * @brief    vo模块示例代码的main
 * @version  1.0.0
 * @since    1.0.0
 * @author  张桂庆<zhangguiqing@sgitg.sgcc.com.cn>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "sc_common.h"
#include "sample_vo.h"
#include "mpi_sys.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_VO_Usage(char *sPrgNm)
{
    printf("Usage : %s <index>\n", sPrgNm);
    printf("index:\n");
    printf("\t 0)USERPIC -> VO ->FILE + MIPILCD_1024x600.\n");
    printf("\t 1)USERPIC -> VO ->FILE + HDMI_1920x1080.\n");
    printf("\t 2)USERPIC -> VO ->VI + HDMI_1920x1080.\n");
    printf("\t 3)USERPIC -> VO ->FILE + MIPILCD_800x1280.\n");
    printf("\t 4)USERPIC -> VO ->VI + MIPILCD_800x1280.\n");
    printf("\t 5)USERPIC -> VO ->FILE + MIPILCD_480x640.\n");
    printf("\t 6)USERPIC -> VO ->VI + MIPILCD_480x640.\n");
    printf("\t 7)USERPIC -> VO ->FILE + MIPILCD_720x1280.\n");
    printf("\t 8)USERPIC -> VO ->VI + MIPILCD_720x1280.\n");
    printf("\t 9)USERPIC -> VO ->FILE + MIPIRGBLCD_720x1280.\n");
    printf("\t10)USERPIC -> VO ->VI + MIPIRGBLCD_720x1280.\n");
    printf("\t11)USERPIC -> VO ->VI + (MIPI + RGB) or HDMI loop\n");

    printf("\t If you have any questions, please look at readme.txt!\n");
    return;
}

/******************************************************************************
* function    : main()
* Description : main
******************************************************************************/
int main(int argc, char *argv[])
{
    SC_S32 s32Ret = SC_FAILURE;
    SC_S32 s32Index;

    if (argc < 2 || argc > 2)
    {
        SAMPLE_VO_Usage(argv[0]);
        return SC_FAILURE;
    }

    if (!strncmp(argv[1], "-h", 2))
    {
        SAMPLE_VO_Usage(argv[0]);
        return SC_SUCCESS;
    }
    signal(SIGINT, SAMPLE_VO_HandleSig);
    signal(SIGTERM, SAMPLE_VO_HandleSig);

    SAMPLE_VO_StartThread();

    s32Index = atoi(argv[1]);
    switch (s32Index)
    {
    case 0:
        s32Ret = SAMPLE_VO_MIPILCD_1024x600();
        break;
    case 1:
        s32Ret = SAMPLE_VO_HDMI_1920x1080();
        break;
    case 2:
        s32Ret = SAMPLE_VI_VO();
        break;
    case 3:
        s32Ret = SAMPLE_VO_MIPILCD_800x1280();
        break;
    case 4:
        s32Ret = SAMPLE_VI_MIPILCD_800x1280();
        break;
    case 5:
        s32Ret = SAMPLE_VO_MIPILCD_480x640();
        break;
    case 6:
        s32Ret = SAMPLE_VI_MIPILCD_480x640();
        break;
    case 7:
        s32Ret = SAMPLE_VO_MIPILCD_720x1280();
        break;
    case 8:
        s32Ret = SAMPLE_VI_MIPILCD_720x1280();
        break;
    case 9:
        s32Ret = SAMPLE_VO_MIPIRGBLCD_720x1280();
        break;
    case 10:
        s32Ret = SAMPLE_VI_MIPIRGBLCD_720x1280();
        break;
    case 11:
        s32Ret = SAMPLE_VI_MIPIRGB_HDMI_LOOP();
        break;
    default:
        SAMPLE_PRT("the index %d is invaild!\n", s32Index);
        SAMPLE_VO_Usage(argv[0]);
        return SC_FAILURE;
    }

    usleep(1000000);
    if (s32Ret == SC_SUCCESS)
    {
        SAMPLE_PRT("sample_vo exit success!\n");
    }
    else
    {
        SAMPLE_PRT("sample_vo exit abnormally!\n");
    }

    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
