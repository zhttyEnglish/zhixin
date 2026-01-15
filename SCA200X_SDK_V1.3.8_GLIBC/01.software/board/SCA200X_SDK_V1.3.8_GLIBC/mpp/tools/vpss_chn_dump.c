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
#include "sc_comm_vb.h"
#include "sc_buffer.h"
#include "mpi_vb.h"
#include "sc_comm_vpss.h"
#include "mpi_vpss.h"
#include "mpi_vgs.h"

#define MAX_FRM_WIDTH   20480

#define VALUE_BETWEEN(x,min,max) (((x)>=(min)) && ((x) <= (max)))

static SC_U32 u32VpssDepthFlag = 0;
static SC_U32 u32SignalFlag = 0;

static VPSS_GRP VpssGrp = 0;
static VPSS_CHN VpssChn = 0;
static SC_U32 u32OrigDepth = 0;

static VIDEO_FRAME_INFO_S g_stFrame;
static char *g_pVBufVirt_Y = NULL;
static char *g_pVBufVirt_C = NULL;
static SC_U32 g_Ysize, g_Csize;
static FILE *g_pfd = NULL;

/*When saving a file,sp420 will be denoted by p420 and sp422 will be denoted by p422 in the name of the file */
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
        printf("SC_MPI_SYS_Mmap Y error!\n");
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
            printf("SC_MPI_SYS_Mmap V error!\n");
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
            printf("SC_MPI_SYS_Mmap U error!\n");
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

static void sample_yuv_10bit_dump(VIDEO_FRAME_S *pVBuf)
{
    #if 0
    unsigned int w, h, k, wy, wuv;
    char *pVBufVirt_Y;
    char *pVBufVirt_C;
    char *pMemContent;
    SC_U64 phy_addr;
    SC_U16  src[MAX_FRM_WIDTH];
    SC_U8  dest[MAX_FRM_WIDTH];
    PIXEL_FORMAT_E  enPixelFormat = pVBuf->enPixelFormat;
    /* When the storage format is a planar format,
     * this variable is used to keep the height of the UV component
     */
    SC_U32 u32UvHeight = 0;
    SC_U32 u32YWidth;

    if (PIXEL_FORMAT_YVU_SEMIPLANAR_420 == enPixelFormat || PIXEL_FORMAT_YUV_SEMIPLANAR_420 == enPixelFormat)
    {
        u32Size = (pVBuf->u32Stride[0]) * (pVBuf->u32Height) * 3 / 2;
        u32UvHeight = pVBuf->u32Height / 2;
    }
    else if (PIXEL_FORMAT_YVU_SEMIPLANAR_422 == enPixelFormat || PIXEL_FORMAT_YUV_SEMIPLANAR_422 == enPixelFormat)
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

        fwrite(src, pVBuf->u32Width * 2, 1, pfd);
    }

    if (PIXEL_FORMAT_YUV_400 != enPixelFormat)
    {
        fflush(pfd);
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

            fwrite(src, pVBuf->u32Width, 1, pfd);
        }

        fflush(pfd);

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

            fwrite(src, pVBuf->u32Width, 1, pfd);
        }
    }

    fflush(pfd);

    fprintf(stderr, "done %d!\n", pVBuf->u32TimeRef);
    fflush(stderr);

    SC_MPI_SYS_Munmap(pUserPageAddr[0], u32Size);
    pUserPageAddr[0] = SC_NULL;
    #endif
}

static SC_S32 VPSS_Restore(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
    SC_S32 s32Ret = SC_FAILURE;
    VPSS_CHN_ATTR_S stChnAttr;
    VPSS_EXT_CHN_ATTR_S stExtChnAttr;

    if (VB_INVALID_POOLID != g_stFrame.u32PoolId)
    {
        s32Ret = SC_MPI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, &g_stFrame);

        if (SC_SUCCESS != s32Ret)
        {
            printf("Release Chn Frame error!!!\n");
        }

        g_stFrame.u32PoolId = VB_INVALID_POOLID;
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

    if (u32VpssDepthFlag)
    {

        if (VpssChn >= VPSS_MAX_PHY_CHN_NUM)
        {
            s32Ret = SC_MPI_VPSS_GetExtChnAttr(VpssGrp, VpssChn, &stExtChnAttr);
        }
        else
        {
            s32Ret = SC_MPI_VPSS_GetChnAttr(VpssGrp, VpssChn, &stChnAttr);
        }

        if (s32Ret != SC_SUCCESS)
        {
            printf("get chn attr error!!!\n");
        }

        if (VpssChn >= VPSS_MAX_PHY_CHN_NUM)
        {
            stExtChnAttr.u32Depth = u32OrigDepth;
            s32Ret = SC_MPI_VPSS_SetExtChnAttr(VpssGrp, VpssChn, &stExtChnAttr);
        }
        else
        {
            stChnAttr.u32Depth = u32OrigDepth;
            s32Ret = SC_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &stChnAttr) ;
        }

        if (s32Ret != SC_SUCCESS)
        {
            printf("set depth error!!!\n");
        }

        u32VpssDepthFlag = 0;
    }

    return SC_SUCCESS;
}

void VPSS_Chn_Dump_HandleSig(SC_S32 signo)
{
    if (u32SignalFlag)
    {
        exit(-1);
    }

    if (SIGINT == signo || SIGTERM == signo)
    {
        u32SignalFlag++;
        VPSS_Restore(VpssGrp, VpssChn);
        u32SignalFlag--;
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }

    exit(-1);
}

SC_VOID SAMPLE_MISC_VpssDump(VPSS_GRP Grp, VPSS_CHN Chn, SC_U32 u32FrameCnt)
{
    SC_CHAR szYuvName[128];
    SC_CHAR szPixFrm[10];
    SC_U32  u32Cnt = u32FrameCnt;
    SC_U32  u32Depth = 1;
    SC_S32  s32MilliSec = -1;
    SC_S32  s32Times = 10;
    SC_S32  s32Ret;
    VPSS_CHN_ATTR_S stChnAttr;
    VPSS_EXT_CHN_ATTR_S stExtChnAttr;

    if (Chn > VPSS_CHN3)
    {
        s32Ret = SC_MPI_VPSS_GetExtChnAttr(Grp, Chn, &stExtChnAttr);
        u32OrigDepth = stExtChnAttr.u32Depth;
    }
    else
    {
        s32Ret = SC_MPI_VPSS_GetChnAttr(Grp, Chn, &stChnAttr);
        u32OrigDepth = stChnAttr.u32Depth;
    }

    if (s32Ret != SC_SUCCESS)
    {
        printf("get chn attr error!!!\n");
        return;
    }

    if (Chn > VPSS_CHN3)
    {
        stExtChnAttr.u32Depth = u32Depth;
        s32Ret = SC_MPI_VPSS_SetExtChnAttr(Grp, Chn, &stExtChnAttr);
    }
    else
    {
        stChnAttr.u32Depth = u32Depth;
        s32Ret = SC_MPI_VPSS_SetChnAttr(Grp, Chn, &stChnAttr) ;
    }

    if (s32Ret != SC_SUCCESS)
    {
        printf("set depth error!!!\n");
        VPSS_Restore(Grp, Chn);
        return;
    }

    u32VpssDepthFlag = 1;

    memset(&g_stFrame, 0, sizeof(g_stFrame));
    g_stFrame.u32PoolId = VB_INVALID_POOLID;

    while (SC_MPI_VPSS_GetChnFrame(Grp, Chn, &g_stFrame, s32MilliSec) != SC_SUCCESS)
    {
        s32Times--;

        if (0 >= s32Times)
        {
            printf("get frame error for 10 times,now exit !!!\n");
            VPSS_Restore(Grp, Chn);
            return;
        }

        usleep(40000);
    }

    #if 0
    if ((VIDEO_FORMAT_LINEAR != g_stFrame.stVFrame.enVideoFormat)
        || (COMPRESS_MODE_NONE != g_stFrame.stVFrame.enCompressMode))
    {
        printf("only support linear frame dump!\n");
        SC_MPI_VPSS_ReleaseChnFrame(Grp, Chn, &g_stFrame);
        g_stFrame.u32PoolId = VB_INVALID_POOLID;
        return;
    }
    #endif

    switch (g_stFrame.stVFrame.enPixelFormat)
    {
    case PIXEL_FORMAT_YVU_SEMIPLANAR_420:
    case PIXEL_FORMAT_YUV_SEMIPLANAR_420:
        snprintf(szPixFrm, 10, "P420");
        break;

    case PIXEL_FORMAT_YVU_SEMIPLANAR_422:
    case PIXEL_FORMAT_YUV_SEMIPLANAR_422:
        snprintf(szPixFrm, 10, "P422");
        break;

    default:
        snprintf(szPixFrm, 10, "P420");
        break;
    }

    /* make file name */
    snprintf(szYuvName, 128, "./vpss_grp%d_chn%d_%dx%d_%s_%d.yuv", Grp, Chn,
        g_stFrame.stVFrame.u32Width, g_stFrame.stVFrame.u32Height, szPixFrm, u32Cnt);
    printf("Dump YUV frame of vpss chn %d  to file: \"%s\"\n", Chn, szYuvName);
    fflush(stdout);

    s32Ret = SC_MPI_VPSS_ReleaseChnFrame(Grp, Chn, &g_stFrame);

    if (SC_SUCCESS != s32Ret)
    {
        printf("Release frame error ,now exit !!!\n");
        VPSS_Restore(Grp, Chn);
        return;
    }

    g_stFrame.u32PoolId = VB_INVALID_POOLID;
    /* open file */
    g_pfd = fopen(szYuvName, "wb");

    if (SC_NULL == g_pfd)
    {
        printf("open file failed:%s!\n", strerror(errno));
        VPSS_Restore(Grp, Chn);
        return;
    }

    /* get frame  */
    while (u32Cnt--)
    {
        if (SC_MPI_VPSS_GetChnFrame(Grp, Chn, &g_stFrame, s32MilliSec) != SC_SUCCESS)
        {
            printf("Get frame fail \n");
            usleep(1000);
            continue;
        }

        if (DYNAMIC_RANGE_SDR8 == g_stFrame.stVFrame.enDynamicRange)
        {
            sample_yuv_8bit_dump(&g_stFrame.stVFrame);
        }
        else
        {
            sample_yuv_10bit_dump(&g_stFrame.stVFrame);
        }

        printf("Get frame %d!!\n", u32Cnt);
        /* release frame after using */
        s32Ret = SC_MPI_VPSS_ReleaseChnFrame(Grp, Chn, &g_stFrame);

        if (SC_SUCCESS != s32Ret)
        {
            printf("Release frame error ,now exit !!!\n");
            VPSS_Restore(Grp, Chn);
            return;
        }

        g_stFrame.u32PoolId = VB_INVALID_POOLID;
    }

    VPSS_Restore(Grp, Chn);
    return;
}
static void usage(void)
{
    printf(
        "\n"
        "*************************************************\n"
        "Usage: ./vpss_chn_dump [VpssGrp] [VpssChn] [FrmCnt]\n"
        "1)VpssGrp: \n"
        "   Vpss group id\n"
        "2)VpssChn: \n"
        "   vpss chn id\n"
        "3)FrmCnt: \n"
        "   the count of frame to be dump\n"
        "*)Example:\n"
        "   e.g : ./vpss_chn_dump 0 0 1\n"
        "   e.g : ./vpss_chn_dump 1 4 2\n"
        "*************************************************\n"
        "\n");
}

SC_S32 main(int argc, char *argv[])
{
    SC_U32 u32FrmCnt = 1;

    printf("\nNOTICE: This tool only can be used for TESTING !!!\n");
    printf("\tTo see more usage, please enter: ./vpss_chn_dump -h\n\n");

    if (argc > 1)
    {
        if (!strncmp(argv[1], "-h", 2))
        {
            usage();
            exit(SC_SUCCESS);
        }
    }

    if (argc < 4)
    {
        usage();
        exit(SC_SUCCESS);
    }

    VpssGrp = atoi(argv[1]);

    if (!VALUE_BETWEEN(VpssGrp, 0, VPSS_MAX_GRP_NUM - 1))
    {
        printf("grp id must be [0,%d]!!!!\n\n", VPSS_MAX_GRP_NUM - 1);
        return -1;
    }

    VpssChn = atoi(argv[2]);/* chn id*/

    if (!VALUE_BETWEEN(VpssChn, 0, VPSS_MAX_CHN_NUM - 1))
    {
        printf("chn id must be [0,%d]!!!!\n\n", VPSS_MAX_CHN_NUM - 1);
        return -1;
    }

    u32VpssDepthFlag = 0;
    u32SignalFlag = 0;
    g_stFrame.u32PoolId = VB_INVALID_POOLID;
    u32OrigDepth = 0;

    signal(SIGINT, VPSS_Chn_Dump_HandleSig);
    signal(SIGTERM, VPSS_Chn_Dump_HandleSig);

    u32FrmCnt = atoi(argv[3]);/* frame count*/

    SAMPLE_MISC_VpssDump(VpssGrp, VpssChn, u32FrmCnt);

    return SC_SUCCESS;
}
