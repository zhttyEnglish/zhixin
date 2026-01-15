#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

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
#include "sample_ive_main.h"
#include "sample_comm_ive.h"

#define SAMPLE_IVE_READ_PATH "./data/input/1920_1080_p420.yuv"
#define SAMPLE_IVE_SAVE_PATH "./data/output"
#define SAMPLE_IVE_BITMAP_PATH "./data/input/256_49.bmp"

SC_S32 SAMPLE_IVE_CSC()
{
    SC_S32 s32Ret;
    SC_S32 s32SrcWidth = 1920;
    SC_S32 s32SrcHeight = 1080;

    IVE_SRC_IMAGE_S stCSCIn = {0};
    stCSCIn.u32Width = s32SrcWidth;
    stCSCIn.u32Height = s32SrcHeight;
    stCSCIn.enType = IVE_IMAGE_TYPE_YUV420P;

    s32Ret = SAMPLE_COMM_IVE_CreateImage(&stCSCIn, stCSCIn.enType, stCSCIn.u32Width, stCSCIn.u32Height);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_IVE_CreateImage err %d!\n", s32Ret);
    }

    FILE *pSrcFile = fopen(SAMPLE_IVE_READ_PATH, "rb");
    s32Ret = SAMPLE_COMM_IVE_ReadFile(&stCSCIn, pSrcFile);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_IVE_ReadFile err %d!\n", s32Ret);
    }
    fclose(pSrcFile);
    IVE_SRC_IMAGE_S stCSCOut = {0};
    stCSCOut.u32Width = s32SrcWidth;
    stCSCOut.u32Height = s32SrcHeight;
    stCSCOut.enType = IVE_IMAGE_TYPE_U8C3_PACKAGE;
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&stCSCOut, stCSCOut.enType, stCSCOut.u32Width, stCSCOut.u32Height);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_IVE_CreateImage err %d!\n", s32Ret);
    }

    IVE_HANDLE hIve;
    IVE_CSC_CTRL_S stIveCscCtrl;
    stIveCscCtrl.enMode = IVE_CSC_MODE_VIDEO_BT601_YUV2RGB;
    SC_BOOL bInstance = SC_TRUE;
    s32Ret = SC_MPI_IVE_CSC(&hIve, &stCSCIn, &stCSCOut, &stIveCscCtrl, bInstance);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SC_MPI_IVE_csc err %d!\n", s32Ret);
    }

    SC_BOOL bBlock = SC_TRUE;
    SC_BOOL bFinish;
    SC_S32 ret = SC_MPI_IVE_Query(hIve, &bFinish, bBlock);
    while (SC_ERR_IVE_QUERY_TIMEOUT == ret)
    {
        usleep(100000);
        ret = SC_MPI_IVE_Query(hIve, &bFinish, bBlock);
    }

    SC_CHAR s8SrcName[64];
    sprintf(s8SrcName, "p420");
    SC_CHAR s8DstFileName[64] = {0};
    sprintf(s8DstFileName, "%s/IVE_csc_from_%s_%d_%d_to_rgb.rgb", SAMPLE_IVE_SAVE_PATH, s8SrcName, s32SrcWidth,
        s32SrcHeight);
    FILE *pDstFile = fopen(s8DstFileName, "wb+");
    s32Ret = SAMPLE_COMM_IVE_WriteFile(&stCSCOut, pDstFile);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SAMPLE_COMM_IVE_WriteFile err %d!\n", s32Ret);
    }
    fclose(pDstFile);

    IVE_MMZ_FREE(stCSCIn.au64PhyAddr[0], stCSCIn.au64VirAddr[0]);
    IVE_MMZ_FREE(stCSCOut.au64PhyAddr[0], stCSCOut.au64VirAddr[0]);
    printf("ive csc ok!\n");

    return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
