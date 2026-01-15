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
#include "sample_venc.h"
#include "mpi_sys.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_VENC_Usage(char *sPrgNm)
{
    printf("Usage : %s [index] \n", sPrgNm);
    printf("index:\n");
    printf("\t  0) H.265e@1080P@30fps. from yuv file\n");
    printf("\t  1) H.265e@1080P@30fps.\n");
    printf("\t  2) H.265e@1080P@30fps + H.265e@1080P@30fps.\n");
    printf("\t  3) Jpeg@1080P.\n");
    printf("\t  4) ModParam@75Hz.\n");
    printf("\t  5) Set ChnAttr.\n");
    return;
}

/******************************************************************************
* function    : main()
* Description : main
******************************************************************************/
int main(int argc, char *argv[])
{
    SC_S32 s32Index;
    SC_S32 s32Ret = SC_FAILURE;

    if (argc < 2 || argc > 2)
    {
        printf("test->02");
        SAMPLE_VENC_Usage(argv[0]);
        return SC_FAILURE;
    }

    if (!strncmp(argv[1], "-h", 2))
    {
        SAMPLE_VENC_Usage(argv[0]);
        return SC_SUCCESS;
    }

    signal(SIGINT, SAMPLE_VENC_HandleSig);
    signal(SIGTERM, SAMPLE_VENC_HandleSig);

    s32Index = atoi(argv[1]);
    switch (s32Index)
    {
    case 0:
        s32Ret = SAMPLE_VENC_FILE_H264();
        break;
    case 1:
        s32Ret = SAMPLE_VENC_H265();
        break;
    case 2:
        s32Ret = SAMPLE_VENC_2CHN();
        break;
    case 3:
        s32Ret = SAMPLE_VENC_Jpeg();
        break;
    case 4:
        s32Ret = SAMPLE_VENC_MODPARAM();
        break;
    case 5:
        s32Ret = SAMPLE_VENC_SET_CHN_ATTR();
        break;

    default:
        printf("the index is invaild!\n");
        SAMPLE_VENC_Usage(argv[0]);
        return SC_FAILURE;
    }

    if (SC_SUCCESS == s32Ret)
        SAMPLE_PRT("program exit normally!\n");
    else
        SAMPLE_PRT("program exit abnormally!\n");

    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
