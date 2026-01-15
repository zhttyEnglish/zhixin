#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "sc_common.h"
#include "sc_comm_video.h"
#include "sc_comm_sys.h"
#include "mpi_sys.h"
#include "sc_comm_vb.h"
#include "mpi_vb.h"
#include "sc_comm_vo.h"
#include "mpi_vo.h"
#include "mpi_vgs.h"
#include "sc_math.h"

#define MAX_FRM_WIDTH   8192

static VIDEO_FRAME_INFO_S g_stFrame;
static char *g_pVBufVirt_Y = NULL;
static char *g_pVBufVirt_C = NULL;
static SC_U32 g_Ysize, g_Csize;
static FILE *g_pfd = NULL;

static void usage(void)
{
    printf(
        "\n"
        "*************************************************\n"
        "Usage: ./vo_screen_dump [VoLayer] [Frmcnt].\n"
        "1)VoLayer: \n"
        "   Which layer to be dumped\n"
        "   Default: 0\n"
        "2)FrmCnt: \n"
        "   The count of frame to be dumped\n"
        "   Default: 1\n"
        "*)Example:\n"
        "   e.g : ./vo_screen_dump 0 1 (dump one YUV)\n"
        "*************************************************\n"
        "\n");
}

/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void VOU_TOOL_HandleSig(SC_S32 signo)
{
    if (SIGINT == signo || SIGTERM == signo)
    {
        if (0 != g_stFrame.stVFrame.u64PhyAddr[0])
        {
            SC_MPI_VO_ReleaseScreenFrame(0, &g_stFrame);
            memset(&g_stFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
        }

        if (NULL != g_pVBufVirt_Y)
        {
            SC_MPI_SYS_Munmap(g_pVBufVirt_Y, g_Ysize);
            g_pVBufVirt_Y = NULL;
        }

        if (NULL != g_pVBufVirt_C)
        {
            SC_MPI_SYS_Munmap(g_pVBufVirt_C, g_Csize);
            g_pVBufVirt_C = NULL;
        }

        if (NULL != g_pfd)
        {
            fclose(g_pfd);
            g_pfd = NULL;
        }

        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

/*  save to yvu420p  for 8bit*/
static void sample_yuv_8bit_dump(VIDEO_FRAME_S *pVBuf)
{
    unsigned int h;
    char *pMemContent;
    PIXEL_FORMAT_E  enPixelFormat = pVBuf->enPixelFormat;
    /* When the storage format is a planar format,
     * this variable is used to keep the height of the UV component
     */
    SC_U32 u32UvHeight = 0;

    g_Ysize = (pVBuf->u32Stride[0]) * (pVBuf->u32Height);

    if (PIXEL_FORMAT_YVU_PLANAR_420 == enPixelFormat)
    {
        g_Csize = (pVBuf->u32Stride[1]) * (pVBuf->u32Height) / 2;
        u32UvHeight = pVBuf->u32Height / 2;
    }
    else
    {
        fprintf(stderr, "no support video format(%d)\n", enPixelFormat);
        return;
    }

    g_pVBufVirt_Y = (SC_CHAR *) SC_MPI_SYS_Mmap(pVBuf->u64PhyAddr[0], g_Ysize);
    if (NULL == g_pVBufVirt_Y)
    {
        return;
    }

    /* save Y ----------------------------------------------------------------*/
    fprintf(stderr, "saving......Y......");
    fflush(stderr);

    for (h = 0; h < pVBuf->u32Height; h++)
    {
        pMemContent = g_pVBufVirt_Y + h * pVBuf->u32Stride[0];
        fwrite(pMemContent, pVBuf->u32Width, 1, g_pfd);
    }
    fflush(g_pfd);
    SC_MPI_SYS_Munmap(g_pVBufVirt_Y, g_Ysize);
    g_pVBufVirt_Y = NULL;

    if (PIXEL_FORMAT_YVU_PLANAR_420 == enPixelFormat)
    {
        g_pVBufVirt_C = (SC_CHAR *) SC_MPI_SYS_Mmap(pVBuf->u64PhyAddr[1], g_Csize);
        if (NULL == g_pVBufVirt_C)
        {
            return;
        }

        /* save V ----------------------------------------------------------------*/
        fprintf(stderr, "V......");
        fflush(stderr);

        for (h = 0; h < u32UvHeight; h++)
        {
            pMemContent = g_pVBufVirt_C + h * pVBuf->u32Stride[1];

            fwrite(pMemContent, pVBuf->u32Width / 2, 1, g_pfd);
        }
        fflush(g_pfd);
        SC_MPI_SYS_Munmap(g_pVBufVirt_C, g_Csize);
        g_pVBufVirt_C = NULL;

        g_pVBufVirt_C = (SC_CHAR *) SC_MPI_SYS_Mmap(pVBuf->u64PhyAddr[2], g_Csize);
        if (NULL == g_pVBufVirt_C)
        {
            return;
        }

        /* save U ----------------------------------------------------------------*/
        fprintf(stderr, "U......");
        fflush(stderr);

        for (h = 0; h < u32UvHeight; h++)
        {
            pMemContent = g_pVBufVirt_C + h * pVBuf->u32Stride[2];

            fwrite(pMemContent, pVBuf->u32Width / 2, 1, g_pfd);
        }
        fflush(g_pfd);
        SC_MPI_SYS_Munmap(g_pVBufVirt_C, g_Csize);
        g_pVBufVirt_C = NULL;
    }

    fprintf(stderr, "done %d!\n", pVBuf->u32TimeRef);
    fflush(stderr);
}

/*  save to yvu420p  for 10bit*/
static void sample_yuv_10bit_dump(VIDEO_FRAME_S *pVBuf)
{
    #if 0
    unsigned int w, h, k, wy, wuv;
    char *pVBufVirt_Y;
    char *pVBufVirt_C;
    char *pMemContent;
    //unsigned char TmpBuff[MAX_FRM_WIDTH];                //If this value is too small and the image is big, this memory may not be enough

    SC_U64 phy_addr;
    SC_U16  src[MAX_FRM_WIDTH];
    SC_U8  dest[MAX_FRM_WIDTH];

    PIXEL_FORMAT_E  enPixelFormat = pVBuf->enPixelFormat;
    /* When the storage format is a planar format,
     * this variable is used to keep the height of the UV component
     */
    SC_U32 u32UvHeight = 0;
    //SC_U32 u32UvWidth;
    SC_U32 u32YWidth;

    if (PIXEL_FORMAT_YVU_SEMIPLANAR_420 == enPixelFormat)
    {
        u32Size = (pVBuf->u32Stride[0]) * (pVBuf->u32Height) * 3 / 2;
        u32UvHeight = pVBuf->u32Height / 2;
    }
    else if (PIXEL_FORMAT_YVU_SEMIPLANAR_422 == enPixelFormat)
    {
        u32Size = (pVBuf->u32Stride[0]) * (pVBuf->u32Height) * 2;
        u32UvHeight = pVBuf->u32Height;
    }
    else if (PIXEL_FORMAT_YUV_400 == enPixelFormat)
    {
        u32Size = (pVBuf->u32Stride[0]) * (pVBuf->u32Height);
        u32UvHeight = pVBuf->u32Height;
    }

    u32YWidth = (pVBuf->u32Width * 10 + 7) / 8;

    phy_addr = pVBuf->u64PhyAddr[0];
    pUserPageAddr[0] = (SC_CHAR *) SC_MPI_SYS_Mmap(phy_addr, u32Size);

    if (SC_NULL == pUserPageAddr[0])
    {
        return;
    }

    pVBufVirt_Y = pUserPageAddr[0];
    pVBufVirt_C = pVBufVirt_Y + (pVBuf->u32Stride[0]) * (pVBuf->u32Height);

    /* save Y ----------------------------------------------------------------*/
    fprintf(stderr, "saving......Y......");
    fflush(stderr);

    for (h = 0; h < pVBuf->u32Height; h++)
    {
        pMemContent = pVBufVirt_Y + h * pVBuf->u32Stride[0];
        wy = 0;

        for (w = 0; w < u32YWidth - 1; w++)
        {
            dest[w] = *pMemContent;
            dest[w + 1] = *(pMemContent + 1);
            k = wy % 4;

            switch (k)
            {
            case 0:
                src[wy] = (((SC_U16)(dest[w]))) + (((dest[w + 1]) & 0x3) << 8);
                break;

            case 1:
                src[wy] = ((((SC_U16)(dest[w])) & 0xfc) >> 2) + (((SC_U16)(dest[w + 1]) & 0xf) << 6);
                break;

            case 2:
                src[wy] = ((((SC_U16)(dest[w])) & 0xf0) >> 4) + (((SC_U16)(dest[w + 1]) & 0x3f) << 4);
                break;

            case 3:
                src[wy] = ((((SC_U16)(dest[w])) & 0xc0) >> 6) + ((SC_U16)(dest[w + 1]) << 2);
                w++;
                pMemContent += 1;
                break;
            }

            pMemContent += 1;
            wy++;
        }

        fwrite(src, pVBuf->u32Width * 2, 1, g_pfd);
    }

    if (PIXEL_FORMAT_YUV_400 != enPixelFormat)
    {
        fflush(g_pfd);
        /* save U ----------------------------------------------------------------*/
        fprintf(stderr, "U......");
        fflush(stderr);

        for (h = 0; h < u32UvHeight; h++)
        {
            pMemContent = pVBufVirt_C + h * pVBuf->u32Stride[1];

            //pMemContent += 1;
            wy = 0;
            wuv = 0;

            for (w = 0; w < u32YWidth - 1; w++)
            {
                dest[w] = *pMemContent;
                dest[w + 1] = *(pMemContent + 1);
                k = wuv % 4;

                switch (k)
                {
                case 0:
                    // src[wy] = (((SC_U16)(dest[w]))) + (((dest[w+1])&0x3)<<8);
                    break;

                case 1:
                    src[wy] = ((((SC_U16)(dest[w])) & 0xfc) >> 2) + (((SC_U16)(dest[w + 1]) & 0xf) << 6);
                    wy++;
                    break;

                case 2:
                    //src[wy] = ((((SC_U16)(dest[w]))&0xf0)>>4) + (((SC_U16)(dest[w+1])&0x3f)<<4);
                    break;

                case 3:
                    src[wy] = ((((SC_U16)(dest[w])) & 0xc0) >> 6) + ((SC_U16)(dest[w + 1]) << 2);
                    wy++;
                    w++;
                    pMemContent += 1;
                    break;
                }

                wuv++;
                pMemContent += 1;

            }

            fwrite(src, pVBuf->u32Width, 1, g_pfd);
        }

        fflush(g_pfd);

        /* save V ----------------------------------------------------------------*/
        fprintf(stderr, "V......");
        fflush(stderr);

        for (h = 0; h < u32UvHeight; h++)
        {
            pMemContent = pVBufVirt_C + h * pVBuf->u32Stride[1];
            wy = 0;
            wuv = 0;

            for (w = 0; w < u32YWidth - 1; w++)
            {
                dest[w] = *pMemContent;
                dest[w + 1] = *(pMemContent + 1);
                k = wuv % 4;

                switch (k)
                {
                case 0:
                    src[wy] = (((SC_U16)(dest[w]))) + (((dest[w + 1]) & 0x3) << 8);
                    wy++;
                    break;

                case 1:
                    //src[wy] = ((((SC_U16)(dest[w]))&0xfc)>>2) + (((SC_U16)(dest[w+1])&0xf)<<6);
                    break;

                case 2:
                    src[wy] = ((((SC_U16)(dest[w])) & 0xf0) >> 4) + (((SC_U16)(dest[w + 1]) & 0x3f) << 4);
                    wy++;
                    break;

                case 3:
                    //src[wy] = ((((SC_U16)(dest[w]))&0xc0)>>6) + ((SC_U16)(dest[w+1])<<2);
                    w++;
                    pMemContent += 1;
                    break;
                }

                pMemContent += 1;
                wuv ++;
            }

            fwrite(src, pVBuf->u32Width, 1, g_pfd);
        }
    }

    fflush(g_pfd);

    fprintf(stderr, "done %d!\n", pVBuf->u32TimeRef);
    fflush(stderr);

    SC_MPI_SYS_Munmap(pUserPageAddr[0], u32Size);
    pUserPageAddr[0] = SC_NULL;

    #endif

}

static SC_CHAR *sample_get_dyrg(DYNAMIC_RANGE_E enDyRg, SC_CHAR *pszYuvDyRg)
{
    switch(enDyRg)
    {
    case DYNAMIC_RANGE_SDR8:
        snprintf(pszYuvDyRg, 10, "SDR8");
        break;
    case DYNAMIC_RANGE_SDR10:
        snprintf(pszYuvDyRg, 10, "SDR10");
        break;
    case DYNAMIC_RANGE_HDR10:
        snprintf(pszYuvDyRg, 10, "HDR10");
        break;
    case DYNAMIC_RANGE_HLG:
        snprintf(pszYuvDyRg, 10, "HLG");
        break;
    case DYNAMIC_RANGE_SLF:
        snprintf(pszYuvDyRg, 10, "SLF");
        break;
    default:
        snprintf(pszYuvDyRg, 10, "SDR8");
        break;
    }
    return pszYuvDyRg;
}

/*
* Name : SAMPLE_MISC_VoDump
* Note : MISC,miscellaneous
*/
static SC_S32 SAMPLE_MISC_VoDump(VO_LAYER VoLayer, SC_S32 s32Cnt)
{
    SC_S32 i, s32Ret;
    SC_CHAR szYuvName[128];
    SC_CHAR szPixFrm[10];
    SC_CHAR szYuvDyRg[10];

    /* Get Frame to make file name*/
    s32Ret = SC_MPI_VO_GetScreenFrame(VoLayer, &g_stFrame, 0);
    if (SC_SUCCESS != s32Ret)
    {
        printf("SC_MPI_VO(%d)_GetScreenFrame errno %#x\n", VoLayer, s32Ret);
        return -1;
    }

    /* make file name */
    if (PIXEL_FORMAT_YVU_SEMIPLANAR_420 == g_stFrame.stVFrame.enPixelFormat)
    {
        snprintf(szPixFrm, 10, "P420");
    }
    else if (PIXEL_FORMAT_YVU_SEMIPLANAR_422 == g_stFrame.stVFrame.enPixelFormat)
    {
        snprintf(szPixFrm, 10, "P422");
    }
    else if (PIXEL_FORMAT_YVU_PLANAR_420 == g_stFrame.stVFrame.enPixelFormat)
    {
        snprintf(szPixFrm, 10, "P420");
    }
    else
    {
        return -1;
    }

    snprintf(szYuvName, 128, "./vo_layer%d_%ux%u_%s_%s_%d.yuv", VoLayer,
        g_stFrame.stVFrame.u32Width,
        g_stFrame.stVFrame.u32Height,
        szPixFrm,
        sample_get_dyrg(g_stFrame.stVFrame.enDynamicRange, szYuvDyRg),
        s32Cnt);

    printf("Dump YUV frame of vo(%d) to file: \"%s\"\n", VoLayer, szYuvName);

    SC_MPI_VO_ReleaseScreenFrame(VoLayer, &g_stFrame);

    /* open file */
    g_pfd = fopen(szYuvName, "wb");

    if (NULL == g_pfd)
    {
        printf("open file failed:%s!\n", strerror(errno));
        return -1;
    }

    /* get VO frame  */
    for (i = 0; i < s32Cnt; i++)
    {
        s32Ret = SC_MPI_VO_GetScreenFrame(VoLayer, &g_stFrame, 0);
        if (SC_SUCCESS != s32Ret)
        {
            printf("get vo(%d) frame err\n", VoLayer);
            printf("only get %d frame\n", i);
            break;
        }

        /* save VO frame to file */
        if (DYNAMIC_RANGE_SDR8 == g_stFrame.stVFrame.enDynamicRange)
        {
            sample_yuv_8bit_dump(&g_stFrame.stVFrame);
        }
        else
        {
            sample_yuv_10bit_dump(&g_stFrame.stVFrame);
        }

        /* release frame after using */
        s32Ret = SC_MPI_VO_ReleaseScreenFrame(VoLayer, &g_stFrame);
        memset(&g_stFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
        if (SC_SUCCESS != s32Ret)
        {
            printf("Release vo(%d) frame err\n", VoLayer);
            printf("only get %d frame\n", i);
            break;
        }
    }

    memset(&g_stFrame, 0, sizeof(VIDEO_FRAME_INFO_S));

    fclose(g_pfd);

    return 0;
}
/*
* Name : vo_screen_dump
* Desc : Dump screen frame.
*/
SC_S32 main(int argc, char *argv[])
{
    VO_LAYER VoLayer = 0;
    SC_S32 s32FrmCnt = 1;

    printf("\nNOTICE: This tool only can be used for TESTING !!!\n");
    printf("\tTo see more usage, please enter: ./vo_screen_dump -h\n\n");

    if (argc > 3)
    {
        printf("Too many parameters!\n");
        return SC_FAILURE;
    }

    /* VO video layer ID*/
    if (argc > 1)
    {
        if (!strncmp(argv[1], "-h", 2))
        {
            usage();
            exit(SC_SUCCESS);
        }
        VoLayer = atoi(argv[1]);
    }

    /* need Frmcnt*/
    if (argc > 2)
    {
        s32FrmCnt = atoi(argv[2]);
        if (s32FrmCnt <= 0)
        {
            printf("The Frmcnt(%d) is wrong!\n", s32FrmCnt);
            return SC_FAILURE;
        }
    }

    signal(SIGINT, VOU_TOOL_HandleSig);
    signal(SIGTERM, VOU_TOOL_HandleSig);

    SAMPLE_MISC_VoDump(VoLayer, s32FrmCnt);

    return SC_SUCCESS;
}

