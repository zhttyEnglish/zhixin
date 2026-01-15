#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "sample_comm.h"
#include "mpi_ive.h"
#include "sample_comm_ive.h"
#include "sample_ive_main.h"

#define SAMPLE_IVE_READ_PATH "./data/input/1920_1080_p420.yuv"
#define SAMPLE_IVE_SAVE_PATH "./data/output"
#define SAMPLE_IVE_BITMAP_PATH "./data/input/256_49.bmp"

SC_S32 SAMPLE_IVE_RESZIE()
{
    SAMPLE_COMM_IVE_CheckIveMpiInit();

    SC_S32 s32Ret;
    SC_S32 s32SrcWidth = 1920;
    SC_S32 s32SrcHeight = 1080;
    SC_S32 s32DstWidth = 1280;
    SC_S32 s32DstHeight = 720;

    IVE_CROP_INFO_S sCropInfo;
    sCropInfo.bEnable = SC_FALSE;
    sCropInfo.enCropCoordinate = VI_CROP_ABS_COOR;
    sCropInfo.stCropRect.s32X = 0;
    sCropInfo.stCropRect.s32Y = 0;
    sCropInfo.stCropRect.u32Width = 1280;
    sCropInfo.stCropRect.u32Height = 270;

    IVE_SRC_IMAGE_S stResizeIn = {0};
    stResizeIn.u32Width = s32SrcWidth;
    stResizeIn.u32Height = s32SrcHeight;
    stResizeIn.enType = IVE_IMAGE_TYPE_YUV420P;

    s32Ret = SAMPLE_COMM_IVE_CreateImage(&stResizeIn, stResizeIn.enType, stResizeIn.u32Width, stResizeIn.u32Height);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_IVE_CreateImage err %d!\n", s32Ret);
        return -1;
    }

    FILE *pSrcFile = fopen(SAMPLE_IVE_READ_PATH, "rb");
    s32Ret = SAMPLE_COMM_IVE_ReadFile(&stResizeIn, pSrcFile);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_IVE_ReadFile err %d!\n", s32Ret);
        fclose(pSrcFile);
        return -1;
    }
    fclose(pSrcFile);

    IVE_SRC_IMAGE_S stResizeOut = {0};
    stResizeOut.u32Width = s32DstWidth;
    stResizeOut.u32Height = s32DstHeight;
    stResizeOut.enType = IVE_IMAGE_TYPE_YUV420P;
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&stResizeOut, stResizeOut.enType, stResizeOut.u32Width, stResizeOut.u32Height);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_IVE_CreateImage err %d!\n", s32Ret);
        return -1;
    }

    #if 1
    s32Ret = SC_MPI_IVE_Resize4(&stResizeIn, &stResizeOut, &sCropInfo, 1);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SC_MPI_IVE_Resize4 err %d!\n", s32Ret);
        return -1;
    }
    #else
    IVE_HANDLE hIve;
    IVE_RESIZE_CTRL_S stIveCtrl;
    stIveCtrl.u16Num = 1;
    SC_BOOL bInstance = SC_TRUE;
    s32Ret = SC_MPI_IVE_Resize(&hIve, &stResizeIn, &stResizeOut, &stIveCtrl, bInstance);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SC_MPI_IVE_csc err %d!\n", s32Ret);
    }

    SC_BOOL bBlock = SC_TRUE;
    SC_BOOL bFinish;
    SC_S32 ret = SC_MPI_IVE_Query(hIve, &bFinish, bBlock);
    while (SC_ERR_IVE_QUERY_TIMEOUT == ret)
    {
        usleep(100);
        ret = SC_MPI_IVE_Query(hIve, &bFinish, bBlock);
    }

    #endif

    SC_CHAR s8SrcName[64];
    sprintf(s8SrcName, "p420");
    SC_CHAR s8DstFileName[64] = {0};
    sprintf(s8DstFileName, "%s/IVE_Resize_from_%s_%d_%d_to_%d_%d.yuv", SAMPLE_IVE_SAVE_PATH, s8SrcName, s32SrcWidth,
        s32SrcHeight, s32DstWidth, s32DstHeight);
    FILE *pDstFile = fopen(s8DstFileName, "w+");
    s32Ret = SAMPLE_COMM_IVE_WriteFile(&stResizeOut, pDstFile);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_IVE_WriteFile err %d!\n", s32Ret);
        fclose(pDstFile);
        return -1;
    }
    fclose(pDstFile);

    IVE_MMZ_FREE(stResizeIn.au64PhyAddr[0], stResizeIn.au64VirAddr[0]);
    IVE_MMZ_FREE(stResizeOut.au64PhyAddr[0], stResizeOut.au64VirAddr[0]);

    printf("ive resize ok!\n");

    return 0;
}
