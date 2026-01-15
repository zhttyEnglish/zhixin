#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sc_common.h"
#include "sc_comm_video.h"
#include "sc_comm_sys.h"
#include "sc_comm_vi.h"
#include "sc_comm_isp.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_isp.h"

#define MAX_FRM_CNT     25
#define MAX_FRM_WIDTH   8192

#define ALIGN_BACK(x, a)              ((a) * (((x) / (a))))

void Usage(void)
{
    printf(
        "\n"
        "*************************************************\n"
        "Usage: ./vi_bayerdump [ViPipe] [nbit] [FrmCnt] [ByteAlign]\n"
        "ViPipe: \n"
        "   0:ViPipe0 ~ 1:ViPipe 1\n"
        "nbit: \n"
        "   The bit num to be dump\n"
        "FrmCnt: \n"
        "   the count of frame to be dump\n"
        "ByteAlign: \n"
        "   Whether convert to Byte align, default is 1\n"
        "e.g : ./vi_bayerdump  0 12 2 1\n"
        "*************************************************\n"
        "\n");
    exit(1);
}

SC_S32 GetDumpPipe(VI_PIPE ViPipe, VI_DEV_BIND_PIPE_S *pstDevBindPipe)
{
    pstDevBindPipe->u32Num = 1;
    pstDevBindPipe->PipeId[0] = ViPipe;

    return SC_SUCCESS;
}

static inline SC_S32 BitWidth2PixelFormat(SC_U32 u32Nbit, PIXEL_FORMAT_E *penPixelFormat)
{
    PIXEL_FORMAT_E enPixelFormat;

    if (8 == u32Nbit)
    {
        enPixelFormat = PIXEL_FORMAT_RGB_BAYER_8BPP;
    }
    else if (10 == u32Nbit)
    {
        enPixelFormat = PIXEL_FORMAT_RGB_BAYER_10BPP;
    }
    else if (12 == u32Nbit)
    {
        enPixelFormat = PIXEL_FORMAT_RGB_BAYER_12BPP;
    }
    else if (14 == u32Nbit)
    {
        enPixelFormat = PIXEL_FORMAT_RGB_BAYER_14BPP;
    }
    else if (16 == u32Nbit)
    {
        enPixelFormat = PIXEL_FORMAT_RGB_BAYER_16BPP;
    }
    else
    {
        enPixelFormat = PIXEL_FORMAT_RGB_BAYER_16BPP;
    }

    *penPixelFormat = enPixelFormat;
    return SC_SUCCESS;
}

SC_S32 ConvertBitPixel(SC_U8 *pu8Data, SC_U32 u32DataNum, SC_U32 u32BitWidth, SC_U16 *pu16OutData)
{
    SC_S32 i, u32Tmp, s32OutCnt;
    SC_U32 u32Val;
    SC_U64 u64Val;
    SC_U8 *pu8Tmp = pu8Data;

    s32OutCnt = 0;
    switch(u32BitWidth)
    {
    case 10:
        /* 4 pixels consist of 5 bytes  */
        u32Tmp = u32DataNum / 4;

        for (i = 0; i < u32Tmp; i++)
        {
            /* byte4 byte3 byte2 byte1 byte0 */
            pu8Tmp = pu8Data + 5 * i;
            u64Val = pu8Tmp[0] + ((SC_U32)pu8Tmp[1] << 8) + ((SC_U32)pu8Tmp[2] << 16) +
                ((SC_U32)pu8Tmp[3] << 24) + ((SC_U64)pu8Tmp[4] << 32);

            pu16OutData[s32OutCnt++] = u64Val & 0x3ff;
            pu16OutData[s32OutCnt++] = (u64Val >> 10) & 0x3ff;
            pu16OutData[s32OutCnt++] = (u64Val >> 20) & 0x3ff;
            pu16OutData[s32OutCnt++] = (u64Val >> 30) & 0x3ff;
        }
        break;
    case 12:
        /* 2 pixels consist of 3 bytes  */
        u32Tmp = u32DataNum / 2;

        for (i = 0; i < u32Tmp; i++)
        {
            /* byte2 byte1 byte0 */
            pu8Tmp = pu8Data + 3 * i;
            u32Val = pu8Tmp[0] + (pu8Tmp[1] << 8) + (pu8Tmp[2] << 16);
            pu16OutData[s32OutCnt++] = u32Val & 0xfff;
            pu16OutData[s32OutCnt++] = (u32Val >> 12) & 0xfff;
        }
        break;
    case 14:
        /* 4 pixels consist of 7 bytes  */
        u32Tmp = u32DataNum / 4;

        for (i = 0; i < u32Tmp; i++)
        {
            pu8Tmp = pu8Data + 7 * i;
            u64Val = pu8Tmp[0] + ((SC_U32)pu8Tmp[1] << 8) + ((SC_U32)pu8Tmp[2] << 16) +
                ((SC_U32)pu8Tmp[3] << 24) + ((SC_U64)pu8Tmp[4] << 32) +
                ((SC_U64)pu8Tmp[5] << 40) + ((SC_U64)pu8Tmp[6] << 48);

            pu16OutData[s32OutCnt++] = u64Val & 0x3fff;
            pu16OutData[s32OutCnt++] = (u64Val >> 14) & 0x3fff;
            pu16OutData[s32OutCnt++] = (u64Val >> 28) & 0x3fff;
            pu16OutData[s32OutCnt++] = (u64Val >> 42) & 0x3fff;
        }
        break;
    default:
        fprintf(stderr, "unsuport bitWidth: %d\n", u32BitWidth);
        return -1;
        break;
    }

    return s32OutCnt;
}

int SampleSaveCompressParam(SC_VOID *pCmpParam, SC_U32 u32Size, FILE *pfd)
{
    fwrite(pCmpParam, u32Size, 1, pfd);
    fflush(pfd);
    return SC_SUCCESS;
}

int SampleSaveUncompressRaw(VIDEO_FRAME_S *pVBuf, SC_U32 u32Nbit, FILE *pfd, SC_U32 u32ByteAlign)
{
    SC_U32 u32H;
    SC_U16 *pu16Data = NULL;
    SC_U64 phy_addr, size;
    SC_U8 *pUserPageAddr[2];
    SC_U8  *pu8Data;
    PIXEL_FORMAT_E enPixelFormat = PIXEL_FORMAT_BUTT;

    BitWidth2PixelFormat(u32Nbit, &enPixelFormat);
    if (enPixelFormat != pVBuf->enPixelFormat)
    {
        fprintf(stderr, "NoCmp: invalid pixel format:%d, u32Nbit: %d\n", pVBuf->enPixelFormat, u32Nbit);
        return SC_FAILURE;
    }

    size = (pVBuf->u32Stride[0]) * (pVBuf->u32Height);
    phy_addr = pVBuf->u64PhyAddr[0];

    pUserPageAddr[0] = (SC_U8 *) SC_MPI_SYS_Mmap(phy_addr, size);
    if (NULL == pUserPageAddr[0])
    {
        return SC_FAILURE;
    }

    pu8Data = pUserPageAddr[0];
    if ((8 != u32Nbit) && (16 != u32Nbit))
    {
        pu16Data = (SC_U16 *)malloc(pVBuf->u32Width * 2U);
        if (NULL == pu16Data)
        {
            fprintf(stderr, "alloc memory failed\n");
            SC_MPI_SYS_Munmap(pUserPageAddr[0], size);
            pUserPageAddr[0] = NULL;
            return SC_FAILURE;
        }
    }

    /* save Y ----------------------------------------------------------------*/
    fprintf(stderr, "saving......dump data......u32Stride[0]: %d, width: %d\n", pVBuf->u32Stride[0], pVBuf->u32Width);
    fflush(stderr);

    for (u32H = 0; u32H < pVBuf->u32Height; u32H++)
    {
        if (8 == u32Nbit)
        {
            fwrite(pu8Data, pVBuf->u32Width, 1, pfd);
        }
        else if (16 == u32Nbit)
        {
            fwrite(pu8Data, pVBuf->u32Width, 2, pfd);
            fflush(pfd);
        }
        else
        {
            if (1 == u32ByteAlign)
            {
                ConvertBitPixel(pu8Data, pVBuf->u32Width, u32Nbit, pu16Data);
                fwrite(pu16Data, pVBuf->u32Width, 2, pfd);
            }
            else
            {
                if (0 == ((pVBuf->u32Width * u32Nbit) % 8))
                {
                    fwrite(pu8Data, pVBuf->u32Width * u32Nbit / 8, 1, pfd); //-- pVBuf->u32Width * u32Nbit / 8
                }
                else
                {
                    fwrite(pu8Data, ((pVBuf->u32Width * u32Nbit) / 8 + 8), 1, pfd);
                }

            }
        }
        pu8Data += pVBuf->u32Stride[0];
    }
    fflush(pfd);

    fprintf(stderr, "u32Nbit_%d done u32TimeRef: %d, u32ByteAlign: %d!\n", u32Nbit, pVBuf->u32TimeRef, u32ByteAlign);
    fflush(stderr);

    if (NULL != pu16Data)
    {
        free(pu16Data);
    }

    SC_MPI_SYS_Munmap(pUserPageAddr[0], size);
    pUserPageAddr[0] = NULL;

    return SC_SUCCESS;
}

int SampleBayerDump(VIDEO_FRAME_S *pVBuf, SC_U32 u32Nbit, FILE *pfd, SC_U32 u32ByteAlign)
{
    return SampleSaveUncompressRaw(pVBuf, u32Nbit, pfd, u32ByteAlign);
}

char *CompressModeToString(COMPRESS_MODE_E enCompressMode)
{
    if(COMPRESS_MODE_NONE == enCompressMode)
    {
        return "CMP_NONE";
    }
    else if(COMPRESS_MODE_SEG == enCompressMode)
    {
        return "CMP_SEG";
    }
    else if(COMPRESS_MODE_DPCM_6BITS == enCompressMode)
    {
        return "DPCM_6BITS";
    }
    else if(COMPRESS_MODE_DPCM_8BITS == enCompressMode)
    {
        return "DPCM_8BITS";
    }
    else if(COMPRESS_MODE_DPCM_10BITS == enCompressMode)
    {
        return "DPCM_10BITS";
    }
    else
    {
        return "CMP_XXX";
    }
}

char *IspBayerFormatToString(ISP_BAYER_FORMAT_E  enBayer)
{
    if(SC_BAYER_RGGB == enBayer)
    {
        return "RGGB";
    }
    else if(SC_BAYER_GRBG == enBayer)
    {
        return "GRBG";
    }
    else if(SC_BAYER_GBRG == enBayer)
    {
        return "GBRG";
    }
    else if(SC_BAYER_BGGR == enBayer)
    {
        return "BGGR";
    }
    else
    {
        return "RGGB";
    }
}

SC_S32 DumpLinearBayer(VI_PIPE ViPipe, SC_U32 u32Nbit, COMPRESS_MODE_E enCompressMode, SC_U32 u32Cnt,
    SC_U32 u32ByteAlign, SC_U32 u32RatioShow)
{
    SC_S32                 s32Ret        = SC_SUCCESS;
    int                    i, j;
    SC_CHAR                szYuvName[256] = {0};
    FILE                  *pfd = NULL;
    SC_S32                 s32MilliSec = 4000;
    SC_U64                 u64IspInfoPhyAddr = 0;
    VI_CMP_PARAM_S         stCmpPara;
    ISP_FRAME_INFO_S      *pstIspFrameInfo;
    ISP_PUB_ATTR_S         stPubAttr;
    VIDEO_FRAME_INFO_S     astFrame;
    VB_SUPPLEMENT_CONFIG_S stSupplementConfig;
    PIXEL_FORMAT_E         enPixelFormat;
    int                    errCnt = 0;

    BitWidth2PixelFormat(u32Nbit, &enPixelFormat);

    while (1)
    {
        if (SC_SUCCESS != SC_MPI_VI_GetPipeFrame(ViPipe, &astFrame, s32MilliSec))
        {
            printf("Linear: get vi Pipe %d frame err\n", ViPipe);
            break;
        }

        if ((astFrame.stVFrame.enCompressMode == enCompressMode)
            && (astFrame.stVFrame.enPixelFormat == enPixelFormat))
        {
            break;
        }
        else
        {
            errCnt++;
            if(errCnt > 5)
            {
                printf("Linear:  need compressMode:%d pixelFormat:%d real compressMode:%d pixelFormat:%d \n",
                    enCompressMode, enPixelFormat,
                    astFrame.stVFrame.enCompressMode,
                    astFrame.stVFrame.enPixelFormat);
                SC_MPI_VI_ReleasePipeFrame(ViPipe, &astFrame);
                return SC_FAILURE;
            }
        }
    }

    /* make file name */
    snprintf(szYuvName, 256, "./vi_Pipe_%d_%d_%d_%d_%dbits_%s_%d_%d.raw", ViPipe,
        astFrame.stVFrame.u32Width,  astFrame.stVFrame.u32Height,
        u32Cnt, u32Nbit, CompressModeToString(astFrame.stVFrame.enCompressMode), u32ByteAlign, u32RatioShow);

    SC_MPI_VI_ReleasePipeFrame(ViPipe, &astFrame);

    /* open file */
    pfd = fopen(szYuvName, "wb");

    if (NULL == pfd)
    {
        printf("open file failed:%s!\n", strerror(errno));

        goto end;
    }

    for (j = 0; j < u32Cnt; j++)
    {
        if (SC_SUCCESS != SC_MPI_VI_GetPipeFrame(ViPipe, &astFrame, s32MilliSec))
        {
            printf("Linear: get vi Pipe %d frame err\n", ViPipe);
            break;
        }

        /* save VI frame to file */
        SampleBayerDump(&astFrame.stVFrame, u32Nbit, pfd, u32ByteAlign);

        /* release frame after using */
        SC_MPI_VI_ReleasePipeFrame(ViPipe, &astFrame);
    }

    fclose(pfd);

    return SC_SUCCESS;

end:
    return SC_FAILURE;
}

SC_S32 main(int argc, char *argv[])
{
    VI_PIPE           ViPipe               = 0;
    SC_S32            s32Ret               = 0;
    SC_U32            i                    = 0;
    SC_U32            u32Nbit              = 12;
    SC_U32            u32FrmCnt            = 1;
    SC_U32            u32RawDepth          = 2;
    SC_U32            u32ByteAlign         = 1;
    SC_U32            u32RatioShow         = 0;
    COMPRESS_MODE_E   enCompressMode       = COMPRESS_MODE_NONE;
    PIXEL_FORMAT_E    enPixFmt;
    VI_DEV_BIND_PIPE_S stDevBindPipe;
    VI_DUMP_ATTR_S    astBackUpDumpAttr[4];
    VI_DUMP_ATTR_S    stDumpAttr;

    printf("\nNOTICE: This tool only can be used for TESTING !!!\n");
    printf("\t To see more usage, please enter: ./vi_bayerdump -h\n\n");

    if (argc > 1)
    {
        if (!strncmp(argv[1], "-h", 2))
        {
            Usage();
            exit(SC_SUCCESS);
        }
        else
        {
            ViPipe = atoi(argv[1]); /* pipe*/
        }
    }

    if (argc > 2)
    {
        u32Nbit = atoi(argv[2]);    /* bit width of raw data:8/10/12/14/16bit */

        if (8 != u32Nbit &&  10 != u32Nbit &&  12 != u32Nbit &&  14 != u32Nbit &&  16 != u32Nbit)
        {
            printf("Can't not support %d bits, only support 8bits,10bits,12bits,14bits,16bits\n", u32Nbit);
            exit(SC_FAILURE);
        }

    }

    if (argc > 3)
    {
        u32FrmCnt = atoi(argv[3]);  /* the frame number */
    }

    if (argc > 4)
    {
        if (atoi(argv[4]) > 0)
        {
            u32ByteAlign = 1 ;
        }
        else
        {
            u32ByteAlign = 0;
        }
    }

    if (1 > u32FrmCnt || MAX_FRM_CNT < u32FrmCnt)
    {
        printf("invalid FrmCnt %d, FrmCnt range from 1 to %d\n", u32FrmCnt, MAX_FRM_CNT);
        exit(SC_FAILURE);
    }

    s32Ret = GetDumpPipe(ViPipe, &stDevBindPipe);

    if (SC_SUCCESS != s32Ret)
    {
        printf("getDumpPipe failed 0x%x!\n", s32Ret);
        return SC_ERR_VI_INVALID_PARA;
    }

    BitWidth2PixelFormat(u32Nbit, &enPixFmt);

    for (i = 0; i < stDevBindPipe.u32Num; i++)
    {
        s32Ret = SC_MPI_VI_GetPipeDumpAttr(stDevBindPipe.PipeId[i], &astBackUpDumpAttr[i]);

        if (SC_SUCCESS != s32Ret)
        {
            printf("Get Pipe %d dump attr failed!\n", ViPipe);
            return s32Ret;
        }

        memcpy(&stDumpAttr, &astBackUpDumpAttr[i], sizeof(VI_DUMP_ATTR_S));
        stDumpAttr.bEnable  = SC_TRUE;
        stDumpAttr.u32Depth = u32RawDepth;
        stDumpAttr.enDumpType = VI_DUMP_TYPE_RAW;

        s32Ret = SC_MPI_VI_SetPipeDumpAttr(stDevBindPipe.PipeId[i], &stDumpAttr);

        if (SC_SUCCESS != s32Ret)
        {
            printf("Set Pipe %d dump attr failed!\n", stDevBindPipe.PipeId[i]);
            return s32Ret;
        }
    }

    DumpLinearBayer(ViPipe, u32Nbit, enCompressMode, u32FrmCnt, u32ByteAlign, u32RatioShow);

    for (i = 0; i < stDevBindPipe.u32Num; i++)
    {
        s32Ret = SC_MPI_VI_SetPipeDumpAttr(stDevBindPipe.PipeId[i], &astBackUpDumpAttr[i]);

        if (SC_SUCCESS != s32Ret)
        {
            printf("Set Pipe %d dump attr failed!\n", ViPipe);
            return s32Ret;
        }
    }

    return s32Ret;
}
