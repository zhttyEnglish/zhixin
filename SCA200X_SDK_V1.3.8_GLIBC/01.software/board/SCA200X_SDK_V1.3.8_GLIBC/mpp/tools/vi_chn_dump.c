#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "sc_math.h"
#include "sc_common.h"
#include "sc_comm_video.h"
#include "sc_comm_sys.h"
#include "mpi_sys.h"
#include "sc_buffer.h"
#include "sc_comm_vb.h"
#include "mpi_vb.h"
#include "sc_comm_vi.h"
#include "mpi_vi.h"
#include "mpi_vgs.h"

#define MAX_FRM_WIDTH   8192

#define VALUE_BETWEEN(x,min,max) (((x)>=(min)) && ((x) <= (max)))

static SC_U32 u32ViDepthFlag = 0;
static SC_U32 u32SignalFlag = 0;

static VI_PIPE ViPipe = 0;
static VI_CHN ViChn = 0;
static SC_U32 u32OrigDepth = 0;
static VIDEO_FRAME_INFO_S stFrame;

static VGS_HANDLE hHandle = -1;
static SC_U32  u32BlkSize = 0;

static SC_CHAR *pUserPageAddr[3] = {SC_NULL, SC_NULL, SC_NULL};
static SC_U32 g_Ysize, g_Vsize, g_Usize;

static FILE *pfd = SC_NULL;

/*When saving a file,sp420 will be denoted by p420 and sp422 will be denoted by p422 in the name of the file */
static void sample_yuv_8bit_dump(VIDEO_FRAME_S *pVBuf, FILE *pfd)
{
    unsigned int h;
    char *pVBufVirt_Y;
    char *pVBufVirt_V;
    char *pVBufVirt_U;
    char *pMemContent;

    SC_U64 phy_addr;
    PIXEL_FORMAT_E  enPixelFormat = pVBuf->enPixelFormat;
    SC_U32 u32UvHeight = 0;

    if(enPixelFormat != PIXEL_FORMAT_YVU_PLANAR_420)
    {
        return;
    }

    g_Ysize = pVBuf->u32Stride[0] * pVBuf->u32Height;
    u32UvHeight = pVBuf->u32Height / 2;
    g_Vsize = pVBuf->u32Stride[1] * u32UvHeight;
    g_Usize = pVBuf->u32Stride[2] * u32UvHeight;

    phy_addr = pVBuf->u64PhyAddr[0];
    //printf("phy_addr:%x, size:%d\n", phy_addr, size);
    pUserPageAddr[0] = (SC_CHAR *) SC_MPI_SYS_Mmap(phy_addr, g_Ysize);
    phy_addr = pVBuf->u64PhyAddr[1];
    pUserPageAddr[1] = (SC_CHAR *) SC_MPI_SYS_Mmap(phy_addr, g_Usize);
    phy_addr = pVBuf->u64PhyAddr[2];
    pUserPageAddr[2] = (SC_CHAR *) SC_MPI_SYS_Mmap(phy_addr, g_Vsize);
    if (SC_NULL == pUserPageAddr[0]
        || SC_NULL == pUserPageAddr[1]
        || SC_NULL == pUserPageAddr[2])
    {
        return;
    }
    //printf("stride: %d,%d\n",pVBuf->u32Stride[0],pVBuf->u32Stride[1] );
    //printf("__stride:%d %d %d\n", pVBuf->u32Stride[0], pVBuf->u32Stride[1], pVBuf->u32Stride[2]);
    pVBufVirt_Y = pUserPageAddr[0];
    pVBufVirt_U = pUserPageAddr[1];
    pVBufVirt_V = pUserPageAddr[2];

    /* save Y ----------------------------------------------------------------*/
    fprintf(stderr, "saving......Y......");
    fflush(stderr);

    for (h = 0; h < pVBuf->u32Height; h++)
    {
        pMemContent = pVBufVirt_Y + h * pVBuf->u32Stride[0];
        fwrite(pMemContent, pVBuf->u32Width, 1, pfd);
    }
    fflush(pfd);
    /* save U ----------------------------------------------------------------*/
    fprintf(stderr, "U......");
    fflush(stderr);

    for (h = 0; h < u32UvHeight; h++)
    {
        pMemContent = pVBufVirt_U + h * pVBuf->u32Stride[1];
        fwrite(pMemContent, pVBuf->u32Width / 2, 1, pfd);
    }
    fflush(pfd);

    /* save V ----------------------------------------------------------------*/
    fprintf(stderr, "V......");
    fflush(stderr);

    for (h = 0; h < u32UvHeight; h++)
    {
        pMemContent = pVBufVirt_V + h * pVBuf->u32Stride[2];
        fwrite(pMemContent, pVBuf->u32Width / 2, 1, pfd);
    }
    fflush(pfd);

    fprintf(stderr, "done %d!\n", pVBuf->u32TimeRef);
    fflush(stderr);

    SC_MPI_SYS_Munmap(pUserPageAddr[0], g_Ysize);
    SC_MPI_SYS_Munmap(pUserPageAddr[1], g_Vsize);
    SC_MPI_SYS_Munmap(pUserPageAddr[2], g_Usize);
    pUserPageAddr[0] = SC_NULL;
    pUserPageAddr[1] = SC_NULL;
    pUserPageAddr[2] = SC_NULL;
}

static SC_S32 VI_Restore(VI_PIPE Pipe, VI_CHN Chn)
{
    SC_S32 s32Ret = SC_FAILURE;
    VI_CHN_ATTR_S stChnAttr;
    VI_EXT_CHN_ATTR_S stExtChnAttr;

    if(VB_INVALID_POOLID != stFrame.u32PoolId)
    {
        s32Ret = SC_MPI_VI_ReleaseChnFrame(Pipe, Chn, &stFrame);
        if(SC_SUCCESS != s32Ret)
        {
            printf("Release Chn Frame error!!!\n");
        }
        stFrame.u32PoolId = VB_INVALID_POOLID;
    }

    if(SC_NULL != pUserPageAddr[0])
    {
        SC_MPI_SYS_Munmap(pUserPageAddr[0], g_Ysize);
        pUserPageAddr[0] = SC_NULL;
    }

    if(SC_NULL != pUserPageAddr[1])
    {
        SC_MPI_SYS_Munmap(pUserPageAddr[1], g_Vsize);
        pUserPageAddr[1] = SC_NULL;
    }

    if(SC_NULL != pUserPageAddr[2])
    {
        SC_MPI_SYS_Munmap(pUserPageAddr[2], g_Usize);
        pUserPageAddr[2] = SC_NULL;
    }

    if(pfd)
    {
        fclose(pfd);
        pfd = SC_NULL;
    }

    if(u32ViDepthFlag)
    {
        if(Chn > VI_MAX_PHY_CHN_NUM - 1)
        {
            s32Ret = SC_MPI_VI_GetExtChnAttr(Pipe, Chn, &stExtChnAttr);

            if (SC_SUCCESS != s32Ret)
            {
                printf("get chn attr error!!!\n");
                return SC_FAILURE;
            }

            stExtChnAttr.u32Depth = u32OrigDepth;
            s32Ret = SC_MPI_VI_SetExtChnAttr(Pipe, Chn, &stExtChnAttr);

            if (SC_SUCCESS != s32Ret)
            {
                printf("set chn attr error!!!\n");
                return SC_FAILURE;
            }
        }
        else
        {
            s32Ret = SC_MPI_VI_GetChnAttr(Pipe, Chn, &stChnAttr);

            if (SC_SUCCESS != s32Ret)
            {
                printf("get chn attr error!!!\n");
                return SC_FAILURE;
            }

            stChnAttr.u32Depth = u32OrigDepth;
            s32Ret = SC_MPI_VI_SetChnAttr(Pipe, Chn, &stChnAttr);

            if (SC_SUCCESS != s32Ret)
            {
                printf("set chn attr error!!!\n");
                return SC_FAILURE;
            }
        }
        u32ViDepthFlag = 0;
    }

    return SC_SUCCESS;
}

void VI_Chn_Dump_HandleSig(SC_S32 signo)
{
    if(u32SignalFlag)
    {
        exit(-1);
    }

    if (SIGINT == signo || SIGTERM == signo)
    {
        u32SignalFlag++;
        VI_Restore(ViPipe, ViChn);
        u32SignalFlag--;
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

SC_VOID *SAMPLE_MISC_ViDump(VI_PIPE Pipe, VI_CHN Chn, SC_U32 u32FrameCnt, SC_U32 u32ByteAlign)
{
    SC_CHAR szYuvName[128];
    SC_CHAR szPixFrm[10];
    SC_CHAR szDynamicRange[10];
    SC_CHAR szVideoFrm[12];
    SC_U32 u32Cnt = u32FrameCnt;
    SC_U32 u32Depth = 2;
    SC_S32 s32MilliSec = -1;
    SC_S32 s32Times = 10;
    SC_S32 s32Ret;
    VI_CHN_ATTR_S stChnAttr;
    VI_EXT_CHN_ATTR_S stExtChnAttr;

    if(Chn > VI_MAX_PHY_CHN_NUM - 1)
    {
        s32Ret = SC_MPI_VI_GetExtChnAttr(Pipe, Chn, &stExtChnAttr);

        if (SC_SUCCESS != s32Ret)
        {
            printf("get chn attr error!!!\n");
            return (SC_VOID *) - 1;
        }

        u32OrigDepth = stExtChnAttr.u32Depth;

        stExtChnAttr.u32Depth = u32Depth;
        s32Ret = SC_MPI_VI_SetExtChnAttr(Pipe, Chn, &stExtChnAttr);

        if (SC_SUCCESS != s32Ret)
        {
            printf("set chn attr error!!!\n");
            return (SC_VOID *) - 1;
        }
    }
    else
    {
        s32Ret = SC_MPI_VI_GetChnAttr(Pipe, Chn, &stChnAttr);

        if (SC_SUCCESS != s32Ret)
        {
            printf("get chn attr error!!!\n");
            return (SC_VOID *) - 1;
        }

        u32OrigDepth = stChnAttr.u32Depth;
        stChnAttr.u32Depth = u32Depth;
        s32Ret = SC_MPI_VI_SetChnAttr(Pipe, Chn, &stChnAttr);

        if (SC_SUCCESS != s32Ret)
        {
            printf("set chn attr error!!!\n");
            return (SC_VOID *) - 1;
        }
    }

    u32ViDepthFlag = 1;

    memset(&stFrame, 0, sizeof(stFrame));
    stFrame.u32PoolId = VB_INVALID_POOLID;
    while (SC_MPI_VI_GetChnFrame(Pipe, Chn, &stFrame, s32MilliSec) != SC_SUCCESS)
    {
        s32Times--;
        if(0 >= s32Times)
        {
            printf("get frame error for 10 times,now exit !!!\n");
            VI_Restore(Pipe, Chn);
            return (SC_VOID *) - 1;
        }
        usleep(40000);
    }

    switch (stFrame.stVFrame.enPixelFormat)
    {
    case PIXEL_FORMAT_YVU_PLANAR_420:
        snprintf(szPixFrm, 10, "yvu420p");
        break;
    case PIXEL_FORMAT_YVU_SEMIPLANAR_420:
        snprintf(szPixFrm, 10, "P420");
        break;
    case PIXEL_FORMAT_YVU_SEMIPLANAR_422:
        snprintf(szPixFrm, 10, "P422");
        break;
    case PIXEL_FORMAT_YUV_400:
        snprintf(szPixFrm, 10, "P400");
        break;
    default:
        snprintf(szPixFrm, 10, "--");
        break;
    }

    switch (stFrame.stVFrame.enVideoFormat)
    {
    case VIDEO_FORMAT_LINEAR:
        snprintf(szVideoFrm, sizeof(szVideoFrm), "linear");
        break;
    case VIDEO_FORMAT_TILE_64x16:
        snprintf(szVideoFrm, sizeof(szVideoFrm), "tile_64X16");
        break;
    case VIDEO_FORMAT_TILE_16x8:
        snprintf(szVideoFrm, sizeof(szVideoFrm), "tile_16X8");
        break;
    default:
        snprintf(szVideoFrm, sizeof(szVideoFrm), "--");
        break;
    }

    switch (stFrame.stVFrame.enDynamicRange)
    {
    case DYNAMIC_RANGE_SDR8:
        snprintf(szDynamicRange, 10, "SDR8");
        break;

    case DYNAMIC_RANGE_SDR10:
        snprintf(szDynamicRange, 10, "SDR10");
        break;

    case DYNAMIC_RANGE_HDR10:
        snprintf(szDynamicRange, 10, "HDR10");
        break;

    case DYNAMIC_RANGE_XDR:
        snprintf(szDynamicRange, 10, "XDR");
        break;

    case DYNAMIC_RANGE_HLG:
        snprintf(szDynamicRange, 10, "HLG");
        break;

    case DYNAMIC_RANGE_SLF:
        snprintf(szDynamicRange, 10, "SLF");
        break;

    default:
        snprintf(szDynamicRange, 10, "--");
        break;
    }

    /* make file name */
    snprintf(szYuvName, 128, "./vi_pipe%d_chn%d_w%d_h%d_%s_%s_%s_%d_%d_%u.yuv", Pipe, Chn,
        stFrame.stVFrame.u32Width, stFrame.stVFrame.u32Height,
        szPixFrm, szVideoFrm, szDynamicRange, u32Cnt, u32ByteAlign, stFrame.stVFrame.u64PTS/1000);
    printf("Dump YUV frame of vi chn %d  to file: \"%s\"\n", Chn, szYuvName);
    fflush(stdout);

    s32Ret = SC_MPI_VI_ReleaseChnFrame(Pipe, Chn, &stFrame);
    if(SC_SUCCESS != s32Ret)
    {
        printf("Release frame error ,now exit !!!\n");
        VI_Restore(Pipe, Chn);
        return (SC_VOID *) - 1;
    }

    stFrame.u32PoolId = VB_INVALID_POOLID;
    /* open file */
    pfd = fopen(szYuvName, "wb");

    if (SC_NULL == pfd)
    {
        printf("open file failed:%s!\n", strerror(errno));
        VI_Restore(Pipe, Chn);
        return (SC_VOID *) - 1;
    }

    /* get frame  */
    while (u32Cnt--)
    {
        if (SC_MPI_VI_GetChnFrame(Pipe, Chn, &stFrame, s32MilliSec) != SC_SUCCESS)
        {
            printf("Get frame fail \n");
            usleep(1000);
            continue;
        }

        if(DYNAMIC_RANGE_SDR8 == stFrame.stVFrame.enDynamicRange)
        {
            sample_yuv_8bit_dump(&stFrame.stVFrame, pfd);
        }

        /* release frame after using */
        s32Ret = SC_MPI_VI_ReleaseChnFrame(Pipe, Chn, &stFrame);
        if(SC_SUCCESS != s32Ret)
        {
            printf("Release frame error ,now exit !!!\n");
            VI_Restore(Pipe, Chn);
            return (SC_VOID *) - 1;
        }

        stFrame.u32PoolId = VB_INVALID_POOLID;
    }

    VI_Restore(Pipe, Chn);
    return (SC_VOID *)0;
}
static void usage(void)
{
    printf(
        "\n"
        "**********************************************************\n"
        "Usage: ./vi_chn_dump [ViPipe] [ViChn] [FrmCnt] [ByteAlign]\n"
        "1)ViPipe: \n"
        "   Vi pipe id\n"
        "2)ViChn: \n"
        "   vi chn id\n"
        "3)FrmCnt: \n"
        "   the count of frame to be dump\n"
        "4)ByteAlign: \n"
        "   Whether convert to Byte align , default is 1\n"
        "*)Example:\n"
        "   e.g : ./vi_chn_dump 0 0 1 1\n"
        "   e.g : ./vi_chn_dump 1 4 2 0\n"
        "**********************************************************\n"
        "\n");
}

SC_S32 main(int argc, char *argv[])
{
    SC_U32 u32FrmCnt = 1;
    SC_U32 u32ByteAlign = 1;

    printf("\nNOTICE: This tool only can be used for TESTING !!!\n");
    printf("\tTo see more usage, please enter: ./vi_chn_dump -h\n\n");
    if (argc > 1)
    {
        if (!strncmp(argv[1], "-h", 2))
        {
            usage();
            exit(SC_SUCCESS);
        }
    }

    if (argc < 5)
    {
        usage();
        exit(SC_SUCCESS);
    }

    ViPipe = atoi(argv[1]);
    if (!VALUE_BETWEEN(ViPipe, 0, VI_MAX_PIPE_NUM - 1))
    {
        printf("pipe id must be [0,%d]!!!!\n\n", VI_MAX_PIPE_NUM - 1);
        return -1;
    }

    ViChn = atoi(argv[2]);/* chn id*/
    if (!VALUE_BETWEEN(ViChn, 0, VI_MAX_CHN_NUM - 1))
    {
        printf("chn id must be [0,%d]!!!!\n\n", VI_MAX_CHN_NUM - 1);
        return -1;
    }

    u32ViDepthFlag = 0;
    u32SignalFlag = 0;
    pUserPageAddr[0] = SC_NULL;
    pUserPageAddr[1] = SC_NULL;
    pUserPageAddr[2] = SC_NULL;
    stFrame.u32PoolId = VB_INVALID_POOLID;
    u32OrigDepth = 0;
    hHandle = -1;
    u32BlkSize = 0;
    g_Ysize = 0;
    g_Vsize = 0;
    g_Usize = 0;
    pfd = SC_NULL;

    signal(SIGINT, VI_Chn_Dump_HandleSig);
    signal(SIGTERM, VI_Chn_Dump_HandleSig);

    u32FrmCnt = atoi(argv[3]);/* frame count*/
    u32ByteAlign = atoi(argv[4]);/* Byte align type*/

    SAMPLE_MISC_ViDump(ViPipe, ViChn, u32FrmCnt, u32ByteAlign);

    return SC_SUCCESS;
}
