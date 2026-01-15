/**
 * @file     desktop_vo_main.c
 * @brief    桌面背景显示功能
 * @version  1.0.0
 * @since    1.0.0
 * @author  张桂庆<zhangguiqing@sgitg.sgcc.com.cn>
 * @date    2023-7-7 创建文件
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
#include "desktop_vo.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

/******************************************************************************
* function : show usage
******************************************************************************/
static void DeskTopVO_Usage(char *sPrgNm)
{
    printf("Usage : %s <index>\n", sPrgNm);
    printf("index:\n");
    printf("\t0)DESKTOP VO -> HDMI 1080P\n");
    printf("\t1)DESKTOP VO -> MIPI 800x600\n");

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
        DeskTopVO_Usage(argv[0]);
        return SC_FAILURE;
    }

    if (!strncmp(argv[1], "-h", 2))
    {
        DeskTopVO_Usage(argv[0]);
        return SC_SUCCESS;
    }

    signal(SIGINT, DeskTopVO_HandleSig);
    signal(SIGTERM, DeskTopVO_HandleSig);

    s32Index = atoi(argv[1]);
    switch (s32Index)
    {
    case 0:
        s32Ret = DeskTopVO_Hdmi_1080p();
        break;

    case 1:
        s32Ret = DeskTopVO_Mipi_800x600();
        break;

    default:
        printf("the index %d is invaild!\n", s32Index);
        DeskTopVO_Usage(argv[0]);
        return SC_FAILURE;
    }

    usleep(1000000);
    if (s32Ret == SC_SUCCESS)
    {
        printf("desktop_vo exit success!\n");
    }
    else
    {
        printf("desktop_vo exit abnormally!\n");
    }

    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
