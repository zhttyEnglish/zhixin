/* vi: set sw=4 ts=4 et: */
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>

#include "sample_comm.h"

const SC_U8 g_SOI[2] = {0xFF, 0xD8};
const SC_U8 g_EOI[2] = {0xFF, 0xD9};
static pthread_t gs_VencPid;
static pthread_t gs_VencQpmapPid;
static SAMPLE_VENC_GETSTREAM_PARA_S gs_stPara;
static SAMPLE_VENC_QPMAP_SENDFRAME_PARA_S stQpMapSendFramePara;

static SC_S32 gs_s32SnapCnt = 0;
SC_CHAR *DstBuf = NULL;
#define TEMP_BUF_LEN 8
#define MAX_THM_SIZE (64*1024)

#ifdef __READ_ALL_FILE__
static SC_S32 FileTrans_GetThmFromJpg(SC_CHAR *JPGPath, SC_U32 *DstSize)
{
    SC_S32 s32RtnVal = 0;
    FILE *fpJpg = NULL;
    SC_CHAR tempbuf[TEMP_BUF_LEN] = {0};
    SC_S32 bufpos = 0;
    SC_CHAR startflag[2] = {0xff, 0xd8};
    SC_S32 startpos = 0;
    SC_CHAR endflag[2] = {0xff, 0xd9};
    SC_S32 endpos = 0;
    fpJpg = fopen(JPGPath, "rb");
    SC_CHAR *pszFile = NULL;
    SC_S32 fd = 0;
    SC_S32 s32I = 0;
    struct stat stStat;
    memset(&stStat, 0, sizeof(struct stat));
    if (NULL == fpJpg)
    {
        printf("file %s not exist!\n", JPGPath);
        return SC_FAILURE;
    }
    else
    {
        fd = fileno(fpJpg);
        fstat(fd, &stStat);
        pszFile = (SC_CHAR *)malloc(stStat.st_size);
        if ((NULL == pszFile) || (stStat.st_size < 6))
        {
            fclose(fpJpg);
            printf("memory malloc fail!\n");
            return SC_FAILURE;
        }

        if (fread(pszFile, stStat.st_size, 1, fpJpg) <= 0)
        {
            fclose(fpJpg);
            free(pszFile);
            printf("fread jpeg src fail!\n");
            return SC_FAILURE;
        }

        fclose(fpJpg);
        SC_U16 u16THMLen = 0;
        u16THMLen = (pszFile[4] << 8) + pszFile[5];
        while (s32I < stStat.st_size)
        {
            tempbuf[bufpos] = pszFile[s32I++];
            if (bufpos > 0)
            {
                if (0 == memcmp(tempbuf + bufpos - 1, startflag, sizeof(startflag)))
                {
                    startpos = s32I - 2;
                    if (startpos < 0)
                    {
                        startpos = 0;
                    }
                }
                if (0 == memcmp(tempbuf + bufpos - 1, endflag, sizeof(endflag)))
                {
                    if (u16THMLen == s32I)
                    {
                        endpos = s32I;
                        break;
                    }
                    else
                    {
                        endpos = s32I;
                        break;
                    }
                }
            }
            bufpos++;
            if (bufpos == (TEMP_BUF_LEN - 1))
            {
                if (tempbuf[bufpos - 1] != 0xFF)
                {
                    bufpos = 0;
                }

            }
            else if (bufpos > (TEMP_BUF_LEN - 1))
            {
                bufpos = 0;
            }

        }

    }
    if (endpos - startpos <= 0)
    {
        free(pszFile);
        printf("get .thm 11 fail!\n");
        return SC_FAILURE;
    }

    if (endpos - startpos >= stStat.st_size)
    {
        free(pszFile);
        printf("NO DCF info, get .thm 22 fail!\n");
        return SC_FAILURE;
    }

    SC_CHAR *temp = pszFile + startpos;
    if(MAX_THM_SIZE < (endpos - startpos))
    {
        printf("Thm is too large than MAX_THM_SIZE, get .thm 33 fail!\n");
        return SC_FAILURE;
    }

    SC_CHAR *cDstBuf = (SC_CHAR *)malloc(endpos - startpos);
    if (NULL == cDstBuf)
    {
        printf("memory malloc fail!\n");
        return SC_FAILURE;
    }

    memcpy(cDstBuf, temp, endpos - startpos);

    DstBuf = cDstBuf;
    *DstSize = endpos - startpos;
    free(pszFile);

    return SC_SUCCESS;
}

#else
static SC_S32 FileTrans_GetThmFromJpg(SC_CHAR *JPGPath, SC_U32 *DstSize)
{
    SC_CHAR tempbuf[TEMP_BUF_LEN] = {0};
    SC_S32 bufpos = 0;
    SC_CHAR startflag[2] = {0xff, 0xd8};
    SC_S32 startpos = 0;
    SC_CHAR endflag[2] = {0xff, 0xd9};
    SC_S32 endpos = 0;
    SC_BOOL bStartMatch = SC_FALSE;

    SC_S32 fd = 0;
    struct stat stStat;
    memset(&stStat, 0, sizeof(struct stat));

    FILE *fpJpg = NULL;
    fpJpg = fopen(JPGPath, "rb");
    if (NULL == fpJpg)
    {
        printf("file %s not exist!\n", JPGPath);
        return SC_FAILURE;
    }
    else
    {
        fd = fileno(fpJpg);
        fstat(fd, &stStat);

        while (!feof(fpJpg))
        {
            tempbuf[bufpos] = getc(fpJpg);
            if (bufpos > 0)
            {
                if (0 == memcmp(tempbuf + bufpos - 1, startflag, sizeof(startflag)))
                {
                    startpos = ftell(fpJpg) - 2;
                    if (startpos < 0)
                    {
                        startpos = 0;
                    }
                    bStartMatch = SC_TRUE;
                }
                if (0 == memcmp(tempbuf + bufpos - 1, endflag, sizeof(endflag)))
                {
                    endpos = ftell(fpJpg);
                    if(SC_TRUE == bStartMatch)
                    {
                        break;
                    }
                }
            }
            bufpos++;

            if (bufpos == (TEMP_BUF_LEN - 1))
            {
                if (tempbuf[bufpos - 1] != 0xFF)
                {
                    bufpos = 0;
                }
            }
            else if (bufpos > (TEMP_BUF_LEN - 1))
            {
                if (tempbuf[bufpos - 1] == 0xFF)
                {
                    tempbuf[0] = 0xFF;
                    bufpos = 1;
                }
                else
                {
                    bufpos = 0;
                }
            }
        }
    }

    if (endpos - startpos <= 0)
    {
        printf("get .thm 11 fail!\n");
        fclose(fpJpg);
        return SC_FAILURE;
    }

    if (endpos - startpos > MAX_THM_SIZE)
    {
        printf("Thm is too large than MAX_THM_SIZE, get .thm 22 fail!\n");
        fclose(fpJpg);
        return SC_FAILURE;
    }

    if (endpos - startpos >= stStat.st_size)
    {
        printf("NO DCF info, get .thm 33 fail!\n");
        fclose(fpJpg);
        return SC_FAILURE;
    }

    SC_CHAR *cDstBuf = (SC_CHAR *)malloc(endpos - startpos);
    if (NULL == cDstBuf)
    {
        printf("memory malloc fail!\n");
        fclose(fpJpg);
        return SC_FAILURE;
    }

    fseek(fpJpg, (long)startpos, SEEK_SET);
    *DstSize = fread(cDstBuf, 1, endpos - startpos, fpJpg);
    if(*DstSize != (endpos - startpos))
    {
        free(cDstBuf);
        printf("fread fail!\n");
        fclose(fpJpg);
        return SC_FAILURE;
    }

    DstBuf = cDstBuf;
    fclose(fpJpg);

    return SC_SUCCESS;
}
#endif

/******************************************************************************
* function : Set venc memory location
******************************************************************************/
SC_S32 SAMPLE_COMM_VENC_MemConfig(SC_VOID)
{
    SC_S32 i = 0;
    SC_S32 s32Ret;
    SC_CHAR *pcMmzName;
    MPP_CHN_S stMppChnVENC;
    /* group, venc max chn is 64*/
    for (i = 0; i < 64; i++)
    {
        stMppChnVENC.enModId = SC_ID_VENC;
        stMppChnVENC.s32DevId = 0;
        stMppChnVENC.s32ChnId = i;
        pcMmzName = NULL;

        /*XXX __no_sup venc*/
        s32Ret = SC_MPI_SYS_SetMemConfig(&stMppChnVENC, pcMmzName);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SC_MPI_SYS_SetMemConfig with %#x!\n", s32Ret);
            return SC_FAILURE;
        }
    }
    return SC_SUCCESS;
}

/******************************************************************************
* funciton : get file postfix according palyload_type.
******************************************************************************/
SC_S32 SAMPLE_COMM_VENC_GetFilePostfix(PAYLOAD_TYPE_E enPayload, char *szFilePostfix)
{
    if (PT_H264 == enPayload)
    {
        strcpy(szFilePostfix, ".h264");
    }
    else if (PT_H265 == enPayload)
    {
        strcpy(szFilePostfix, ".h265");
    }
    else if (PT_JPEG == enPayload)
    {
        strcpy(szFilePostfix, ".jpg");
    }
    else if (PT_MJPEG == enPayload)
    {
        strcpy(szFilePostfix, ".mjp");
    }
    else
    {
        SAMPLE_PRT("payload type err!\n");
        return SC_FAILURE;
    }
    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VENC_GetGopAttr(VENC_GOP_MODE_E enGopMode, VENC_GOP_ATTR_S *pstGopAttr)
{
    switch(enGopMode)
    {
    case VENC_GOPMODE_NORMALP:
        pstGopAttr->enGopMode  = VENC_GOPMODE_NORMALP;
        pstGopAttr->stNormalP.s32IPQpDelta = 2;
        break;
    case VENC_GOPMODE_SMARTP:
        pstGopAttr->enGopMode  = VENC_GOPMODE_SMARTP;
        pstGopAttr->stSmartP.s32BgQpDelta  = 4;
        pstGopAttr->stSmartP.s32ViQpDelta  = 2;
        pstGopAttr->stSmartP.u32BgInterval =  90;
        break;

    case VENC_GOPMODE_DUALP:
        pstGopAttr->enGopMode  = VENC_GOPMODE_DUALP;
        pstGopAttr->stDualP.s32IPQpDelta  = 4;
        pstGopAttr->stDualP.s32SPQpDelta  = 2;
        pstGopAttr->stDualP.u32SPInterval = 3;
        break;

    case VENC_GOPMODE_BIPREDB:
        pstGopAttr->enGopMode  = VENC_GOPMODE_BIPREDB;
        pstGopAttr->stBipredB.s32BQpDelta  = -2;
        pstGopAttr->stBipredB.s32IPQpDelta = 3;
        pstGopAttr->stBipredB.u32BFrmNum   = 2;
        break;

    default:
        SAMPLE_PRT("not support the gop mode !\n");
        return SC_FAILURE;
        break;
    }
    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VENC_Getdcfinfo(char *SrcJpgPath, char *DstThmPath)
{
    SC_S32 s32RtnVal = SC_SUCCESS;
    SC_CHAR JPGSrcPath[FILE_NAME_LEN] = {0};
    SC_CHAR JPGDesPath[FILE_NAME_LEN] = {0};
    SC_U32 DstSize = 0;
    snprintf(JPGSrcPath, sizeof(JPGSrcPath), "%s", SrcJpgPath);
    snprintf(JPGDesPath, sizeof(JPGDesPath), "%s", DstThmPath);

    s32RtnVal = FileTrans_GetThmFromJpg(JPGSrcPath, &DstSize);
    if ((SC_SUCCESS != s32RtnVal) || (0 == DstSize))
    {
        printf("fail to get thm\n");
        return SC_FAILURE;
    }

    FILE *fpTHM = fopen(JPGDesPath, "w");
    if (SC_NULL == fpTHM)
    {
        printf("file to create file %s\n", JPGDesPath);
        return SC_FAILURE;
    }

    SC_U32 u32WritenSize = 0;
    while (u32WritenSize < DstSize)
    {
        s32RtnVal = fwrite(DstBuf + u32WritenSize, 1, DstSize, fpTHM);
        if (s32RtnVal <= 0)
        {
            printf("fail to wirte file, rtn=%d\n", s32RtnVal);
            break;
        }

        u32WritenSize += s32RtnVal;
    }

    if (fpTHM)
    {
        fclose(fpTHM);
        fpTHM = 0;
    }

    if(NULL != DstBuf)
    {
        free(DstBuf);
        DstBuf = NULL;
    }

    return 0;
}

/******************************************************************************
* funciton : save stream
******************************************************************************/
SC_S32 SAMPLE_COMM_VENC_SaveStream(FILE *pFd, VENC_STREAM_S *pstStream)
{
    SC_S32 i;

    int offs = 0;
    for (i = 0; i < pstStream->u32PackCount; i++)
    {
#if 1
        fwrite(pstStream->pstPack[i].pu8Addr + pstStream->pstPack[i].u32Offset,
                1, pstStream->pstPack[i].u32Len - pstStream->pstPack[i].u32Offset, pFd);
        fflush(pFd);
        fdatasync(fileno(pFd));
#else
        static char __buffer[0x400000];
        memcpy(__buffer + offs, pstStream->pstPack[i].pu8Addr + pstStream->pstPack[i].u32Offset,
                pstStream->pstPack[i].u32Len - pstStream->pstPack[i].u32Offset);
        offs += pstStream->pstPack[i].u32Len - pstStream->pstPack[i].u32Offset;
        printf("XXXXXXXXXXXXXXXXXXXXXX...\n");


#endif
    }

    return SC_SUCCESS;
}

/******************************************************************************
* funciton : the process of physical address retrace
******************************************************************************/
SC_S32 SAMPLE_COMM_VENC_SaveStream_PhyAddr(FILE *pFd, VENC_STREAM_BUF_INFO_S *pstStreamBuf, VENC_STREAM_S *pstStream)
{
    SC_U32 i, j;
    SC_U64 u64SrcPhyAddr;
    SC_U32 u32Left;
    SC_S32 s32Ret = 0;

    for(i = 0; i < pstStream->u32PackCount; i++)
    {
        for(j = 0; j < MAX_TILE_NUM; j++)
        {
            if((pstStream->pstPack[i].u64PhyAddr > pstStreamBuf->u64PhyAddr[j]) && \
                (pstStream->pstPack[i].u64PhyAddr <= pstStreamBuf->u64PhyAddr[j] + pstStreamBuf->u64BufSize[j]))
                break;
        }

        if(pstStream->pstPack[i].u64PhyAddr + pstStream->pstPack[i].u32Len >=
            pstStreamBuf->u64PhyAddr[j] + pstStreamBuf->u64BufSize[j])
        {
            if (pstStream->pstPack[i].u64PhyAddr + pstStream->pstPack[i].u32Offset >=
                pstStreamBuf->u64PhyAddr[j] + pstStreamBuf->u64BufSize[j])
            {
                /* physical address retrace in offset segment */
                u64SrcPhyAddr = pstStreamBuf->u64PhyAddr[j] +
                    ((pstStream->pstPack[i].u64PhyAddr + pstStream->pstPack[i].u32Offset) -
                        (pstStreamBuf->u64PhyAddr[j] + pstStreamBuf->u64BufSize[j]));

                s32Ret = fwrite ((void *)(size_t)u64SrcPhyAddr, pstStream->pstPack[i].u32Len - pstStream->pstPack[i].u32Offset, 1, pFd);
                if(s32Ret < 0)
                {
                    SAMPLE_PRT("fwrite err %d\n", s32Ret);
                    return s32Ret;
                }
            }
            else
            {
                /* physical address retrace in data segment */
                u32Left = (pstStreamBuf->u64PhyAddr[j] + pstStreamBuf->u64BufSize[j]) - pstStream->pstPack[i].u64PhyAddr;

                s32Ret = fwrite((void *)(size_t)(pstStream->pstPack[i].u64PhyAddr + pstStream->pstPack[i].u32Offset),
                        u32Left - pstStream->pstPack[i].u32Offset, 1, pFd);
                if(s32Ret < 0)
                {
                    SAMPLE_PRT("fwrite err %d\n", s32Ret);
                    return s32Ret;
                }

                s32Ret = fwrite((void *)(size_t)pstStreamBuf->u64PhyAddr[j], pstStream->pstPack[i].u32Len - u32Left, 1, pFd);
                if(s32Ret < 0)
                {
                    SAMPLE_PRT("fwrite err %d\n", s32Ret);
                    return s32Ret;
                }
            }
        }
        else
        {
            /* physical address retrace does not happen */
            s32Ret = fwrite ((void *)(size_t)(pstStream->pstPack[i].u64PhyAddr + pstStream->pstPack[i].u32Offset),
                    pstStream->pstPack[i].u32Len - pstStream->pstPack[i].u32Offset, 1, pFd);
            if(s32Ret < 0)
            {
                SAMPLE_PRT("fwrite err %d\n", s32Ret);
                return s32Ret;
            }
        }
        fflush(pFd);
    }
    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VENC_Create(VENC_CHN VencChn, PAYLOAD_TYPE_E enType,
    PIC_SIZE_E enSize, SAMPLE_RC_E enRcMode, SC_U32 u32Profile,
    VENC_GOP_ATTR_S *pstGopAttr)
{
    SC_S32 s32Ret;
    SIZE_S stPicSize;
    VENC_CHN_ATTR_S        stVencChnAttr;
    VENC_ATTR_JPEG_S       stJpegAttr;
    SAMPLE_VI_CONFIG_S     stViConfig;
    SC_U32                 u32FrameRate;
    SC_U32                 u32StatTime;
    SC_U32                 u32Gop = 30;

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSize, &stPicSize);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Get picture size failed!\n");
        return SC_FAILURE;
    }

    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);
    if(SAMPLE_SNS_TYPE_BUTT == stViConfig.astViInfo[0].stSnsInfo.enSnsType)
    {
        SAMPLE_PRT("Not set SENSOR%d_TYPE !\n", 0);
        return SC_FALSE;
    }
    s32Ret = SAMPLE_COMM_VI_GetFrameRateBySensor(stViConfig.astViInfo[0].stSnsInfo.enSnsType, &u32FrameRate);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_GetFrameRateBySensor failed!\n");
        return s32Ret;
    }

    /******************************************
     step 1:  Create Venc Channel
    ******************************************/
    stVencChnAttr.stVencAttr.enType          = enType;
    stVencChnAttr.stVencAttr.u32MaxPicWidth  = stPicSize.u32Width;
    stVencChnAttr.stVencAttr.u32MaxPicHeight = stPicSize.u32Height;
    stVencChnAttr.stVencAttr.u32PicWidth     = stPicSize.u32Width;/*the picture width*/
    stVencChnAttr.stVencAttr.u32PicHeight    = stPicSize.u32Height;/*the picture height*/
    stVencChnAttr.stVencAttr.u32BufSize      = stPicSize.u32Width * stPicSize.u32Height * 2;/*stream buffer size*/
    stVencChnAttr.stVencAttr.u32Profile      = u32Profile;
    stVencChnAttr.stVencAttr.bByFrame        = SC_TRUE;/*get stream mode is slice mode or frame mode?*/

    if(VENC_GOPMODE_SMARTP == pstGopAttr->enGopMode)
    {
        u32StatTime = pstGopAttr->stSmartP.u32BgInterval / u32Gop;
    }
    else
    {
        u32StatTime = 1;
    }

    switch (enType)
    {
    case PT_H265:
    {
        if (SAMPLE_RC_CBR == enRcMode)
        {
            VENC_H265_CBR_S    stH265Cbr;

            stVencChnAttr.stRcAttr.enRcMode = SC_VENC_RC_MODE_H265CBR;
            stH265Cbr.u32Gop            = u32Gop;
            stH265Cbr.u32StatTime       = u32StatTime; /* stream rate statics time(s) */
            stH265Cbr.u32SrcFrameRate   = u32FrameRate; /* input (vi) frame rate */
            stH265Cbr.fr32DstFrameRate  = u32FrameRate; /* target frame rate */
            switch (enSize)
            {
            case PIC_720P:
                stH265Cbr.u32BitRate = 1024 * 2 + 1024 * u32FrameRate / 30;
                break;
            case PIC_1080P:
                stH265Cbr.u32BitRate = 1024 * 2 + 2048 * u32FrameRate / 30;
                break;
            case PIC_2592x1944:
                stH265Cbr.u32BitRate = 1024 * 3 + 3072 * u32FrameRate / 30;
                break;
            case PIC_3840x2160:
                stH265Cbr.u32BitRate = 1024 * 5  + 5120 * u32FrameRate / 30;
                break;
            case PIC_4000x3000:
                stH265Cbr.u32BitRate = 1024 * 10 + 5120 * u32FrameRate / 30;
                break;
            case PIC_7680x4320:
                stH265Cbr.u32BitRate = 1024 * 20 + 5120 * u32FrameRate / 30;
                break;
            default :
                stH265Cbr.u32BitRate = 1024 * 15 + 2048 * u32FrameRate / 30;
                break;
            }
            memcpy(&stVencChnAttr.stRcAttr.stH265Cbr, &stH265Cbr, sizeof(VENC_H265_CBR_S));
        }
        else if (SAMPLE_RC_FIXQP == enRcMode)
        {
            VENC_H265_FIXQP_S    stH265FixQp;

            stVencChnAttr.stRcAttr.enRcMode = SC_VENC_RC_MODE_H265FIXQP;
            stH265FixQp.u32Gop              = 30;
            stH265FixQp.u32SrcFrameRate     = u32FrameRate;
            stH265FixQp.fr32DstFrameRate    = u32FrameRate;
            stH265FixQp.u32IQp              = 25;
            stH265FixQp.u32PQp              = 30;
            stH265FixQp.u32BQp              = 32;
            memcpy(&stVencChnAttr.stRcAttr.stH265FixQp, &stH265FixQp, sizeof(VENC_H265_FIXQP_S));
        }
        else if (SAMPLE_RC_VBR == enRcMode)
        {
            VENC_H265_VBR_S    stH265Vbr;

            stVencChnAttr.stRcAttr.enRcMode = SC_VENC_RC_MODE_H265VBR;
            stH265Vbr.u32Gop           = u32Gop;
            stH265Vbr.u32StatTime      = u32StatTime;
            stH265Vbr.u32SrcFrameRate  = u32FrameRate;
            stH265Vbr.fr32DstFrameRate = u32FrameRate;
            switch (enSize)
            {
            case PIC_720P:
                stH265Vbr.u32MaxBitRate = 1024 * 2 + 1024 * u32FrameRate / 30;
                break;
            case PIC_1080P:
                stH265Vbr.u32MaxBitRate = 1024 * 2 + 2048 * u32FrameRate / 30;
                break;
            case PIC_2592x1944:
                stH265Vbr.u32MaxBitRate = 1024 * 3 + 3072 * u32FrameRate / 30;
                break;
            case PIC_3840x2160:
                stH265Vbr.u32MaxBitRate = 1024 * 5  + 5120 * u32FrameRate / 30;
                break;
            case PIC_4000x3000:
                stH265Vbr.u32MaxBitRate = 1024 * 10 + 5120 * u32FrameRate / 30;
                break;
            case PIC_7680x4320:
                stH265Vbr.u32MaxBitRate = 1024 * 20 + 5120 * u32FrameRate / 30;
                break;
            default :
                stH265Vbr.u32MaxBitRate    = 1024 * 15 + 2048 * u32FrameRate / 30;
                break;
            }
            memcpy(&stVencChnAttr.stRcAttr.stH265Vbr, &stH265Vbr, sizeof(VENC_H265_VBR_S));
        }
        else if(SAMPLE_RC_AVBR == enRcMode)
        {
            VENC_H265_AVBR_S    stH265AVbr;

            stVencChnAttr.stRcAttr.enRcMode = SC_VENC_RC_MODE_H265AVBR;
            stH265AVbr.u32Gop         = u32Gop;
            stH265AVbr.u32StatTime    = u32StatTime;
            stH265AVbr.u32SrcFrameRate  = u32FrameRate;
            stH265AVbr.fr32DstFrameRate = u32FrameRate;
            switch (enSize)
            {
            case PIC_720P:
                stH265AVbr.u32MaxBitRate = 1024 * 2 + 1024 * u32FrameRate / 30;
                break;
            case PIC_1080P:
                stH265AVbr.u32MaxBitRate = 1024 * 2 + 2048 * u32FrameRate / 30;
                break;
            case PIC_2592x1944:
                stH265AVbr.u32MaxBitRate = 1024 * 3 + 3072 * u32FrameRate / 30;
                break;
            case PIC_3840x2160:
                stH265AVbr.u32MaxBitRate = 1024 * 5  + 5120 * u32FrameRate / 30;
                break;
            case PIC_4000x3000:
                stH265AVbr.u32MaxBitRate = 1024 * 10 + 5120 * u32FrameRate / 30;
                break;
            case PIC_7680x4320:
                stH265AVbr.u32MaxBitRate = 1024 * 20 + 5120 * u32FrameRate / 30;
                break;
            default :
                stH265AVbr.u32MaxBitRate    = 1024 * 15 + 2048 * u32FrameRate / 30;
                break;
            }
            memcpy(&stVencChnAttr.stRcAttr.stH265AVbr, &stH265AVbr, sizeof(VENC_H265_AVBR_S));
        }
        else if(SAMPLE_RC_QPMAP == enRcMode)
        {
            VENC_H265_QPMAP_S    stH265QpMap;

            stVencChnAttr.stRcAttr.enRcMode = SC_VENC_RC_MODE_H265QPMAP;
            stH265QpMap.u32Gop           = u32Gop;
            stH265QpMap.u32StatTime      = u32StatTime;
            stH265QpMap.u32SrcFrameRate  = u32FrameRate;
            stH265QpMap.fr32DstFrameRate = u32FrameRate;
            stH265QpMap.enQpMapMode      = SC_VENC_RC_QPMAP_MODE_MEANQP;
            memcpy(&stVencChnAttr.stRcAttr.stH265QpMap, &stH265QpMap, sizeof(VENC_H265_QPMAP_S));
        }
        else
        {
            SAMPLE_PRT("%s,%d,enRcMode(%d) not support\n", __FUNCTION__, __LINE__, enRcMode);
            return SC_FAILURE;
        }
        stVencChnAttr.stVencAttr.stAttrH265e.bRcnRefShareBuf = SC_FALSE;
    }
    break;
    case PT_H264:
    {
        if (SAMPLE_RC_CBR == enRcMode)
        {
            VENC_H264_CBR_S    stH264Cbr;

            stVencChnAttr.stRcAttr.enRcMode = SC_VENC_RC_MODE_H264CBR;
            stH264Cbr.u32Gop                = u32Gop; /*the interval of IFrame*/
            stH264Cbr.u32StatTime           = u32StatTime; /* stream rate statics time(s) */
            stH264Cbr.u32SrcFrameRate       = u32FrameRate; /* input (vi) frame rate */
            stH264Cbr.fr32DstFrameRate      = u32FrameRate; /* target frame rate */
            switch (enSize)
            {
            case PIC_720P:
                stH264Cbr.u32BitRate = 1024 * 2  + 1024 * u32FrameRate / 30;
                break;
            case PIC_1080P:
                stH264Cbr.u32BitRate = 1024 * 2  + 2048 * u32FrameRate / 30;
                break;
            case PIC_2592x1944:
                stH264Cbr.u32BitRate = 1024 * 3  + 3072 * u32FrameRate / 30;
                break;
            case PIC_3840x2160:
                stH264Cbr.u32BitRate = 1024 * 5 + 5120 * u32FrameRate / 30;
                break;
            case PIC_4000x3000:
                stH264Cbr.u32BitRate = 1024 * 12 + 5120 * u32FrameRate / 30;
                break;
            case PIC_7680x4320:
                stH264Cbr.u32BitRate = 1024 * 10 + 5120 * u32FrameRate / 30;
                break;
            default :
                stH264Cbr.u32BitRate = 1024 * 15 + 2048 * u32FrameRate / 30;
                break;
            }

            memcpy(&stVencChnAttr.stRcAttr.stH264Cbr, &stH264Cbr, sizeof(VENC_H264_CBR_S));
        }
        else if (SAMPLE_RC_FIXQP == enRcMode)
        {
            VENC_H264_FIXQP_S    stH264FixQp;

            stVencChnAttr.stRcAttr.enRcMode = SC_VENC_RC_MODE_H264FIXQP;
            stH264FixQp.u32Gop           = 30;
            stH264FixQp.u32SrcFrameRate  = u32FrameRate;
            stH264FixQp.fr32DstFrameRate = u32FrameRate;
            stH264FixQp.u32IQp           = 25;
            stH264FixQp.u32PQp           = 30;
            stH264FixQp.u32BQp           = 32;
            memcpy(&stVencChnAttr.stRcAttr.stH264FixQp, &stH264FixQp, sizeof(VENC_H264_FIXQP_S));
        }
        else if (SAMPLE_RC_VBR == enRcMode)
        {
            VENC_H264_VBR_S    stH264Vbr;

            stVencChnAttr.stRcAttr.enRcMode = SC_VENC_RC_MODE_H264VBR;
            stH264Vbr.u32Gop           = u32Gop;
            stH264Vbr.u32StatTime      = u32StatTime;
            stH264Vbr.u32SrcFrameRate  = u32FrameRate;
            stH264Vbr.fr32DstFrameRate = u32FrameRate;
            switch (enSize)
            {
            case PIC_720P:
                stH264Vbr.u32MaxBitRate = 1024 * 2   + 1024 * u32FrameRate / 30;
                break;
            case PIC_1080P:
                stH264Vbr.u32MaxBitRate = 1024 * 2   + 2048 * u32FrameRate / 30;
                break;
            case PIC_2592x1944:
                stH264Vbr.u32MaxBitRate = 1024 * 3   + 3072 * u32FrameRate / 30;
                break;
            case PIC_3840x2160:
                stH264Vbr.u32MaxBitRate = 1024 * 5   + 5120 * u32FrameRate / 30;
                break;
            case PIC_4000x3000:
                stH264Vbr.u32MaxBitRate = 1024 * 10  + 5120 * u32FrameRate / 30;
                break;
            case PIC_7680x4320:
                stH264Vbr.u32MaxBitRate = 1024 * 20  + 5120 * u32FrameRate / 30;
                break;
            default :
                stH264Vbr.u32MaxBitRate = 1024 * 15  + 2048 * u32FrameRate / 30;
                break;
            }
            memcpy(&stVencChnAttr.stRcAttr.stH264Vbr, &stH264Vbr, sizeof(VENC_H264_VBR_S));
        }
        else if (SAMPLE_RC_AVBR == enRcMode)
        {
            VENC_H264_VBR_S    stH264AVbr;

            stVencChnAttr.stRcAttr.enRcMode = SC_VENC_RC_MODE_H264AVBR;
            stH264AVbr.u32Gop           = u32Gop;
            stH264AVbr.u32StatTime      = u32StatTime;
            stH264AVbr.u32SrcFrameRate  = u32FrameRate;
            stH264AVbr.fr32DstFrameRate = u32FrameRate;
            switch (enSize)
            {
            case PIC_720P:
                stH264AVbr.u32MaxBitRate = 1024 * 2   + 1024 * u32FrameRate / 30;
                break;
            case PIC_1080P:
                stH264AVbr.u32MaxBitRate = 1024 * 2   + 2048 * u32FrameRate / 30;
                break;
            case PIC_2592x1944:
                stH264AVbr.u32MaxBitRate = 1024 * 3   + 3072 * u32FrameRate / 30;
                break;
            case PIC_3840x2160:
                stH264AVbr.u32MaxBitRate = 1024 * 5   + 5120 * u32FrameRate / 30;
                break;
            case PIC_4000x3000:
                stH264AVbr.u32MaxBitRate = 1024 * 10  + 5120 * u32FrameRate / 30;
                break;
            case PIC_7680x4320:
                stH264AVbr.u32MaxBitRate = 1024 * 20  + 5120 * u32FrameRate / 30;
                break;
            default :
                stH264AVbr.u32MaxBitRate = 1024 * 15  + 2048 * u32FrameRate / 30;
                break;
            }
            memcpy(&stVencChnAttr.stRcAttr.stH264AVbr, &stH264AVbr, sizeof(VENC_H264_AVBR_S));
        }
        else if(SAMPLE_RC_QPMAP == enRcMode)
        {
            VENC_H264_QPMAP_S    stH264QpMap;

            stVencChnAttr.stRcAttr.enRcMode = SC_VENC_RC_MODE_H264QPMAP;
            stH264QpMap.u32Gop           = u32Gop;
            stH264QpMap.u32StatTime      = u32StatTime;
            stH264QpMap.u32SrcFrameRate  = u32FrameRate;
            stH264QpMap.fr32DstFrameRate = u32FrameRate;
            memcpy(&stVencChnAttr.stRcAttr.stH264QpMap, &stH264QpMap, sizeof(VENC_H264_QPMAP_S));
        }
        else
        {
            SAMPLE_PRT("%s,%d,enRcMode(%d) not support\n", __FUNCTION__, __LINE__, enRcMode);
            return SC_FAILURE;
        }
        stVencChnAttr.stVencAttr.stAttrH264e.bRcnRefShareBuf = SC_FALSE;
    }
    break;
    case PT_MJPEG:
    {
        if (SAMPLE_RC_FIXQP == enRcMode)
        {
            VENC_MJPEG_FIXQP_S stMjpegeFixQp;

            stVencChnAttr.stRcAttr.enRcMode = SC_VENC_RC_MODE_MJPEGFIXQP;
            stMjpegeFixQp.u32Qfactor        = 95;
            stMjpegeFixQp.u32SrcFrameRate    = u32FrameRate;
            stMjpegeFixQp.fr32DstFrameRate   = u32FrameRate;

            memcpy(&stVencChnAttr.stRcAttr.stMjpegFixQp, &stMjpegeFixQp, sizeof(VENC_MJPEG_FIXQP_S));
        }
        else if (SAMPLE_RC_CBR == enRcMode)
        {
            VENC_MJPEG_CBR_S stMjpegeCbr;

            stVencChnAttr.stRcAttr.enRcMode = SC_VENC_RC_MODE_MJPEGCBR;
            stMjpegeCbr.u32StatTime         = u32StatTime;
            stMjpegeCbr.u32SrcFrameRate     = u32FrameRate;
            stMjpegeCbr.fr32DstFrameRate    = u32FrameRate;
            switch (enSize)
            {
            case PIC_720P:
                stMjpegeCbr.u32BitRate = 1024 * 5  + 1024 * u32FrameRate / 30;
                break;
            case PIC_1080P:
                stMjpegeCbr.u32BitRate = 1024 * 8  + 2048 * u32FrameRate / 30;
                break;
            case PIC_2592x1944:
                stMjpegeCbr.u32BitRate = 1024 * 20 + 3072 * u32FrameRate / 30;
                break;
            case PIC_3840x2160:
                stMjpegeCbr.u32BitRate = 1024 * 25 + 5120 * u32FrameRate / 30;
                break;
            case PIC_4000x3000:
                stMjpegeCbr.u32BitRate = 1024 * 30 + 5120 * u32FrameRate / 30;
                break;
            case PIC_7680x4320:
                stMjpegeCbr.u32BitRate = 1024 * 40 + 5120 * u32FrameRate / 30;
                break;
            default :
                stMjpegeCbr.u32BitRate = 1024 * 20 + 2048 * u32FrameRate / 30;
                break;
            }

            memcpy(&stVencChnAttr.stRcAttr.stMjpegCbr, &stMjpegeCbr, sizeof(VENC_MJPEG_CBR_S));
        }
        else if ((SAMPLE_RC_VBR == enRcMode) || (SAMPLE_RC_AVBR == enRcMode))
        {
            VENC_MJPEG_VBR_S   stMjpegVbr;

            if(SAMPLE_RC_AVBR == enRcMode)
            {
                SAMPLE_PRT("Mjpege not support AVBR, so change rcmode to VBR!\n");
            }

            stVencChnAttr.stRcAttr.enRcMode = SC_VENC_RC_MODE_MJPEGVBR;
            stMjpegVbr.u32StatTime      = u32StatTime;
            stMjpegVbr.u32SrcFrameRate  = u32FrameRate;
            stMjpegVbr.fr32DstFrameRate = 5;

            switch (enSize)
            {
            case PIC_720P:
                stMjpegVbr.u32MaxBitRate = 1024 * 5 + 1024 * u32FrameRate / 30;
                break;
            case PIC_1080P:
                stMjpegVbr.u32MaxBitRate = 1024 * 8 + 2048 * u32FrameRate / 30;
                break;
            case PIC_2592x1944:
                stMjpegVbr.u32MaxBitRate = 1024 * 20 + 3072 * u32FrameRate / 30;
                break;
            case PIC_3840x2160:
                stMjpegVbr.u32MaxBitRate = 1024 * 25 + 5120 * u32FrameRate / 30;
                break;
            case PIC_4000x3000:
                stMjpegVbr.u32MaxBitRate    = 1024 * 30 + 5120 * u32FrameRate / 30;
                break;
            case PIC_7680x4320:
                stMjpegVbr.u32MaxBitRate = 1024 * 40 + 5120 * u32FrameRate / 30;
                break;
            default :
                stMjpegVbr.u32MaxBitRate = 1024 * 20 + 2048 * u32FrameRate / 30;
                break;
            }

            memcpy(&stVencChnAttr.stRcAttr.stMjpegVbr, &stMjpegVbr, sizeof(VENC_MJPEG_VBR_S));
        }
        else
        {
            SAMPLE_PRT("cann't support other mode(%d) in this version!\n", enRcMode);
            return SC_FAILURE;
        }
    }
    break;

    case PT_JPEG:
        stJpegAttr.bSupportDCF     = SC_FALSE;
        stJpegAttr.stMPFCfg.u8LargeThumbNailNum = 0;
        stJpegAttr.enReceiveMode = VENC_PIC_RECEIVE_SINGLE;
        memcpy(&stVencChnAttr.stVencAttr.stAttrJpege, &stJpegAttr, sizeof(VENC_ATTR_JPEG_S));
        break;
    default:
        SAMPLE_PRT("cann't support this enType (%d) in this version!\n", enType);
        return SC_ERR_VENC_NOT_SUPPORT;
    }

    if(PT_MJPEG == enType || PT_JPEG == enType )
    {
        stVencChnAttr.stGopAttr.enGopMode  = VENC_GOPMODE_NORMALP;
        stVencChnAttr.stGopAttr.stNormalP.s32IPQpDelta = 0;
    }
    else
    {
        memcpy(&stVencChnAttr.stGopAttr, pstGopAttr, sizeof(VENC_GOP_ATTR_S));
        if((VENC_GOPMODE_BIPREDB == pstGopAttr->enGopMode) && (PT_H264 == enType))
        {
            if(0 == stVencChnAttr.stVencAttr.u32Profile)
            {
                stVencChnAttr.stVencAttr.u32Profile = 1;

                SAMPLE_PRT("H.264 base profile not support BIPREDB, so change profile to main profile!\n");
            }
        }

        if((SC_VENC_RC_MODE_H264QPMAP == stVencChnAttr.stRcAttr.enRcMode)
            || (SC_VENC_RC_MODE_H265QPMAP == stVencChnAttr.stRcAttr.enRcMode))
        {
            if(VENC_GOPMODE_ADVSMARTP == pstGopAttr->enGopMode)
            {
                stVencChnAttr.stGopAttr.enGopMode = VENC_GOPMODE_SMARTP;

                SAMPLE_PRT("advsmartp not support QPMAP, so change gopmode to smartp!\n");
            }
        }
    }

    s32Ret = SC_MPI_VENC_CreateChn(VencChn, &stVencChnAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VENC_CreateChn [%d] faild with %#x! ===\n", \
            VencChn, s32Ret);
        return s32Ret;
    }

    return SC_SUCCESS;
}

/******************************************************************************
* funciton : Start venc stream mode
* note      : rate control parameter need adjust, according your case.
******************************************************************************/
SC_S32 SAMPLE_COMM_VENC_Start(VENC_CHN VencChn, PAYLOAD_TYPE_E enType,
    PIC_SIZE_E enSize, SAMPLE_RC_E enRcMode, SC_U32 u32Profile,
    VENC_GOP_ATTR_S *pstGopAttr)
{
    SC_S32 s32Ret;
    VENC_RECV_PIC_PARAM_S  stRecvParam;

    /******************************************
     step 1:  Creat Encode Chnl
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_Create(VencChn, enType, enSize, enRcMode, u32Profile, pstGopAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VENC_Create faild with%#x! \n", s32Ret);
        return SC_FAILURE;
    }
    /******************************************
     step 2:  Start Recv Venc Pictures
    ******************************************/
    stRecvParam.s32RecvPicNum = -1;
    s32Ret = SC_MPI_VENC_StartRecvFrame(VencChn, &stRecvParam);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VENC_StartRecvPic faild with%#x! \n", s32Ret);
        return SC_FAILURE;
    }
    return SC_SUCCESS;
}

/******************************************************************************
* funciton : Stop venc ( stream mode -- H264, MJPEG )
******************************************************************************/
SC_S32 SAMPLE_COMM_VENC_Stop(VENC_CHN VencChn)
{
    SC_S32 s32Ret;
    /******************************************
     step 1:  Stop Recv Pictures
    ******************************************/
    s32Ret = SC_MPI_VENC_StopRecvFrame(VencChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VENC_StopRecvPic vechn[%d] failed with %#x!\n", \
            VencChn, s32Ret);
        return SC_FAILURE;
    }    /******************************************
     step 2:  Distroy Venc Channel
    ******************************************/
    s32Ret = SC_MPI_VENC_DestroyChn(VencChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VENC_DestroyChn vechn[%d] failed with %#x!\n", \
            VencChn, s32Ret);
        return SC_FAILURE;
    }
    return SC_SUCCESS;
}

/******************************************************************************
* funciton : Start snap
******************************************************************************/

SC_S32 SAMPLE_COMM_VENC_SnapStart(VENC_CHN VencChn, SIZE_S *pstSize, SC_BOOL bSupportDCF)
{
    SC_S32 s32Ret;
    VENC_CHN_ATTR_S stVencChnAttr;
    /******************************************
     step 1:  Create Venc Channel
    ******************************************/
    stVencChnAttr.stVencAttr.enType = PT_JPEG;
    stVencChnAttr.stVencAttr.u32MaxPicWidth     = pstSize->u32Width;
    stVencChnAttr.stVencAttr.u32MaxPicHeight    = pstSize->u32Height;
    stVencChnAttr.stVencAttr.u32PicWidth        = pstSize->u32Width;
    stVencChnAttr.stVencAttr.u32PicHeight       = pstSize->u32Height;
    stVencChnAttr.stVencAttr.u32BufSize         = pstSize->u32Width * pstSize->u32Height * 2;
    stVencChnAttr.stVencAttr.bByFrame           = SC_TRUE;/*get stream mode is field mode  or frame mode*/
    stVencChnAttr.stVencAttr.stAttrJpege.bSupportDCF = bSupportDCF;
    //stVencChnAttr.stVencAttr.stAttrJpege.bSupportXMP = SC_FALSE;
    stVencChnAttr.stVencAttr.stAttrJpege.stMPFCfg.u8LargeThumbNailNum = 0;
    stVencChnAttr.stVencAttr.stAttrJpege.enReceiveMode = VENC_PIC_RECEIVE_SINGLE;

    s32Ret = SC_MPI_VENC_CreateChn(VencChn, &stVencChnAttr);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VENC_CreateChn [%d] faild with %#x!\n", \
            VencChn, s32Ret);
        return s32Ret;
    }
    return SC_SUCCESS;
}

/******************************************************************************
* funciton : Stop snap
******************************************************************************/
SC_S32 SAMPLE_COMM_VENC_SnapStop(VENC_CHN VencChn)
{
    SC_S32 s32Ret;
    s32Ret = SC_MPI_VENC_StopRecvFrame(VencChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VENC_StopRecvPic vechn[%d] failed with %#x!\n", VencChn, s32Ret);
        return SC_FAILURE;
    }
    s32Ret = SC_MPI_VENC_DestroyChn(VencChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VENC_DestroyChn vechn[%d] failed with %#x!\n", VencChn, s32Ret);
        return SC_FAILURE;
    }
    return SC_SUCCESS;
}

/******************************************************************************
* funciton : snap process
******************************************************************************/

SC_S32 SAMPLE_COMM_VENC_SnapProcess(VENC_CHN VencChn, SC_U32 SnapCnt, SC_BOOL bSaveJpg, SC_BOOL bSaveThm)
{
    struct timeval TimeoutVal;
    fd_set read_fds;
    SC_S32 s32VencFd;
    VENC_CHN_STATUS_S stStat;
    VENC_STREAM_S stStream;
    SC_S32 s32Ret;
    VENC_RECV_PIC_PARAM_S  stRecvParam;
    SC_U32 i;

    /******************************************
     step 2:  Start Recv Venc Pictures
    ******************************************/
    stRecvParam.s32RecvPicNum = SnapCnt;
    s32Ret = SC_MPI_VENC_StartRecvFrame(VencChn, &stRecvParam);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VENC_StartRecvPic faild with%#x!\n", s32Ret);
        return SC_FAILURE;
    }
    /******************************************
     step 3:  recv picture
    ******************************************/
    s32VencFd = SC_MPI_VENC_GetFd(VencChn);
    if (s32VencFd < 0)
    {
        SAMPLE_PRT("SC_MPI_VENC_GetFd faild with%#x!\n", s32VencFd);
        return SC_FAILURE;
    }

    for(i = 0; i < SnapCnt; i++)
    {
        FD_ZERO(&read_fds);
        FD_SET(s32VencFd, &read_fds);
        TimeoutVal.tv_sec  = 10;
        TimeoutVal.tv_usec = 0;
        s32Ret = select(s32VencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            SAMPLE_PRT("snap select failed!\n");
            return SC_FAILURE;
        }
        else if (0 == s32Ret)
        {
            SAMPLE_PRT("snap time out!\n");
            return SC_FAILURE;
        }
        else
        {
            if (FD_ISSET(s32VencFd, &read_fds))
            {
                s32Ret = SC_MPI_VENC_QueryStatus(VencChn, &stStat);
                if (s32Ret != SC_SUCCESS)
                {
                    SAMPLE_PRT("SC_MPI_VENC_QueryStatus failed with %#x!\n", s32Ret);
                    return SC_FAILURE;
                }
                /*******************************************************
                suggest to check both u32CurPacks and u32LeftStreamFrames at the same time,for example:
                 if(0 == stStat.u32CurPacks || 0 == stStat.u32LeftStreamFrames)
                 {                SAMPLE_PRT("NOTE: Current  frame is NULL!\n");
                    return SC_SUCCESS;
                 }
                 *******************************************************/
                if (0 == stStat.u32CurPacks)
                {
                    SAMPLE_PRT("NOTE: Current  frame is NULL!\n");
                    return SC_SUCCESS;
                }
                stStream.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
                if (NULL == stStream.pstPack)
                {
                    SAMPLE_PRT("malloc memory failed!\n");
                    return SC_FAILURE;
                }
                stStream.u32PackCount = stStat.u32CurPacks;
                s32Ret = SC_MPI_VENC_GetStream(VencChn, &stStream, -1);
                if (SC_SUCCESS != s32Ret)
                {
                    SAMPLE_PRT("SC_MPI_VENC_GetStream failed with %#x!\n", s32Ret);

                    free(stStream.pstPack);
                    stStream.pstPack = NULL;
                    return SC_FAILURE;
                }
                if(bSaveJpg || bSaveThm)
                {
                    char acFile[FILE_NAME_LEN]    = {0};
                    FILE *pFile;

                    snprintf(acFile, FILE_NAME_LEN, "snap_%d.jpg", gs_s32SnapCnt);
                    pFile = fopen(acFile, "wb");
                    if (pFile == NULL)
                    {
                        SAMPLE_PRT("open file err\n");

                        free(stStream.pstPack);
                        stStream.pstPack = NULL;
                        return SC_FAILURE;
                    }

                    s32Ret = SAMPLE_COMM_VENC_SaveStream(pFile, &stStream);
                    if (SC_SUCCESS != s32Ret)
                    {
                        SAMPLE_PRT("save snap picture failed!\n");

                        free(stStream.pstPack);
                        stStream.pstPack = NULL;

                        fclose(pFile);
                        return SC_FAILURE;
                    }

                    if(bSaveThm)
                    {
                        char acFile_dcf[FILE_NAME_LEN]  = {0};
                        snprintf(acFile_dcf, FILE_NAME_LEN, "snap_thm_%d.jpg", gs_s32SnapCnt);
                        s32Ret = SAMPLE_COMM_VENC_Getdcfinfo(acFile, acFile_dcf);
                        if (SC_SUCCESS != s32Ret)
                        {
                            SAMPLE_PRT("save thm picture failed!\n");

                            free(stStream.pstPack);
                            stStream.pstPack = NULL;

                            fclose(pFile);
                            return SC_FAILURE;
                        }
                    }

                    fclose(pFile);
                    gs_s32SnapCnt++;
                }

                s32Ret = SC_MPI_VENC_ReleaseStream(VencChn, &stStream);
                if (SC_SUCCESS != s32Ret)
                {
                    SAMPLE_PRT("SC_MPI_VENC_ReleaseStream failed with %#x!\n", s32Ret);

                    free(stStream.pstPack);
                    stStream.pstPack = NULL;

                    return SC_FAILURE;
                }

                free(stStream.pstPack);
                stStream.pstPack = NULL;
            }
        }
    }
    /******************************************
     step 4:  stop recv picture
    ******************************************/
    s32Ret = SC_MPI_VENC_StopRecvFrame(VencChn);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VENC_StopRecvPic failed with %#x!\n",  s32Ret);
        return SC_FAILURE;
    }
    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VENC_SaveJpeg(VENC_CHN VencChn, SC_U32 SnapCnt)
{
    struct timeval TimeoutVal;
    fd_set read_fds;
    SC_S32 s32VencFd;
    VENC_CHN_STATUS_S stStat;
    VENC_STREAM_S stStream;
    SC_S32 s32Ret;
    SC_U32 i;

    s32VencFd = SC_MPI_VENC_GetFd(VencChn);
    if (s32VencFd < 0)
    {
        SAMPLE_PRT("SC_MPI_VENC_GetFd faild with%#x!\n", s32VencFd);
        return SC_FAILURE;
    }

    for(i = 0; i < SnapCnt; i++)
    {
        FD_ZERO(&read_fds);
        FD_SET(s32VencFd, &read_fds);
        TimeoutVal.tv_sec  = 10;
        TimeoutVal.tv_usec = 0;
        s32Ret = select(s32VencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            SAMPLE_PRT("snap select failed!\n");
            return SC_FAILURE;
        }
        else if (0 == s32Ret)
        {
            SAMPLE_PRT("snap time out!\n");
            return SC_FAILURE;
        }
        else
        {
            if (FD_ISSET(s32VencFd, &read_fds))
            {
                s32Ret = SC_MPI_VENC_QueryStatus(VencChn, &stStat);
                if (s32Ret != SC_SUCCESS)
                {
                    SAMPLE_PRT("SC_MPI_VENC_QueryStatus failed with %#x!\n", s32Ret);
                    return SC_FAILURE;
                }
                /*******************************************************
                suggest to check both u32CurPacks and u32LeftStreamFrames at the same time,for example:
                 if(0 == stStat.u32CurPacks || 0 == stStat.u32LeftStreamFrames)
                 {                SAMPLE_PRT("NOTE: Current  frame is NULL!\n");
                    return SC_SUCCESS;
                 }
                 *******************************************************/
                if (0 == stStat.u32CurPacks)
                {
                    SAMPLE_PRT("NOTE: Current  frame is NULL!\n");
                    return SC_SUCCESS;
                }
                stStream.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
                if (NULL == stStream.pstPack)
                {
                    SAMPLE_PRT("malloc memory failed!\n");
                    return SC_FAILURE;
                }
                stStream.u32PackCount = stStat.u32CurPacks;
                s32Ret = SC_MPI_VENC_GetStream(VencChn, &stStream, -1);
                if (SC_SUCCESS != s32Ret)
                {
                    SAMPLE_PRT("SC_MPI_VENC_GetStream failed with %#x!\n", s32Ret);

                    free(stStream.pstPack);
                    stStream.pstPack = NULL;
                    return SC_FAILURE;
                }
                if(1)
                {
                    char acFile[FILE_NAME_LEN]    = {0};
                    FILE *pFile;

                    snprintf(acFile, FILE_NAME_LEN, "snap_%d.jpg", gs_s32SnapCnt);
                    pFile = fopen(acFile, "wb");
                    if (pFile == NULL)
                    {
                        SAMPLE_PRT("open file err\n");

                        free(stStream.pstPack);
                        stStream.pstPack = NULL;
                        return SC_FAILURE;
                    }

                    s32Ret = SAMPLE_COMM_VENC_SaveStream(pFile, &stStream);
                    if (SC_SUCCESS != s32Ret)
                    {
                        SAMPLE_PRT("save snap picture failed!\n");

                        free(stStream.pstPack);
                        stStream.pstPack = NULL;

                        fclose(pFile);
                        return SC_FAILURE;
                    }

                    fclose(pFile);
                    gs_s32SnapCnt++;
                }

                s32Ret = SC_MPI_VENC_ReleaseStream(VencChn, &stStream);
                if (SC_SUCCESS != s32Ret)
                {
                    SAMPLE_PRT("SC_MPI_VENC_ReleaseStream failed with %#x!\n", s32Ret);

                    free(stStream.pstPack);
                    stStream.pstPack = NULL;

                    return SC_FAILURE;
                }

                free(stStream.pstPack);
                stStream.pstPack = NULL;
            }
        }
    }

    return SC_SUCCESS;
}

#if 0
#define QpMapBufNum 8
SC_VOID *SAMPLE_COMM_QpmapSendFrameProc(SC_VOID *p)
{
    SC_U32 i, j, VeChnCnt;
    SC_S32 s32Ret;
    VIDEO_FRAME_INFO_S  *pstVideoFrame;
    USER_FRAME_INFO_S   stFrame[QpMapBufNum];
    SAMPLE_VENC_QPMAP_SENDFRAME_PARA_S *pstPara;
    //SC_U32 u32AlignPicHeight ;
    //SC_U32 u32AlignPicWidth  ;

    SC_U32   u32QpMapSize;
    SC_U64   u64QpMapPhyAddr[QpMapBufNum];
    SC_VOID *pQpMapVirAddr[QpMapBufNum];
    SC_U32 u32QpMapSizeHeight ;
    SC_U32 u32QpMapSizeWidth  ;
    SC_U8 *pVirAddr;
    SC_U64 u64PhyAddr;
    SC_U8 *pVirAddrTemp;

    SC_U32   u32SkipWeightHeight_H264 ;
    SC_U32   u32SkipWeightWidth_H264  ;
    SC_U32   u32SkipWeightSize_H264;
    SC_U64   u64SkipWeightPhyAddr_H264[QpMapBufNum];
    SC_VOID *pSkipWeightVirAddr_H264[QpMapBufNum];

    SC_U32   u32SkipWeightHeight_H265 ;
    SC_U32   u32SkipWeightWidth_H265  ;
    SC_U32   u32SkipWeightSize_H265;
    SC_U64   u64SkipWeightPhyAddr_H265[QpMapBufNum];
    SC_VOID *pSkipWeightVirAddr_H265[QpMapBufNum];

    VPSS_CHN_ATTR_S  stChnAttr;

    pstPara = (SAMPLE_VENC_QPMAP_SENDFRAME_PARA_S *)p;

    //u32AlignPicWidth  = (pstPara->stSize.u32Height + 15)/16;
    //u32AlignPicHeight = (pstPara->stSize.u32Height + 15)/16;

    /* qpmap */
    u32QpMapSizeWidth  = (pstPara->stSize.u32Width + 511) / 512 * 32;
    u32QpMapSizeHeight = (pstPara->stSize.u32Height + 15) / 16;
    u32QpMapSize      = u32QpMapSizeWidth * u32QpMapSizeHeight;
    s32Ret = SC_MPI_SYS_MmzAlloc(&u64PhyAddr, (void **)&pVirAddr, NULL, SC_NULL, u32QpMapSize * QpMapBufNum);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_SYS_MmzAlloc err:0x%x", s32Ret);
        return NULL;
    }

    for(i = 0; i < QpMapBufNum; i++)
    {
        u64QpMapPhyAddr[i] = u64PhyAddr + i * u32QpMapSize;
        pQpMapVirAddr[i]   = pVirAddr + i * u32QpMapSize;
    }

    /* skipweight h.264 */
    u32SkipWeightWidth_H264  = (pstPara->stSize.u32Width + 511) / 512 * 16;
    u32SkipWeightHeight_H264 = (pstPara->stSize.u32Height + 15) / 16;
    u32SkipWeightSize_H264   = u32SkipWeightWidth_H264 * u32SkipWeightHeight_H264;
    s32Ret = SC_MPI_SYS_MmzAlloc(&u64PhyAddr, (void **)&pVirAddr, NULL, SC_NULL, u32SkipWeightSize_H264 * QpMapBufNum);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_SYS_MmzAlloc err:0x%x", s32Ret);
        SC_MPI_SYS_MmzFree(u64QpMapPhyAddr[0], pQpMapVirAddr[0]);
        return NULL;
    }

    for(i = 0; i < QpMapBufNum; i++)
    {
        u64SkipWeightPhyAddr_H264[i] = u64PhyAddr + i * u32SkipWeightSize_H264;
        pSkipWeightVirAddr_H264[i]   = pVirAddr + i * u32SkipWeightSize_H264;
    }

    /* skipweight h.265 */
    u32SkipWeightWidth_H265  = (pstPara->stSize.u32Width + 2047) / 2048 * 16;
    u32SkipWeightHeight_H265 = (pstPara->stSize.u32Height + 63) / 64;
    u32SkipWeightSize_H265   = u32SkipWeightWidth_H265 * u32SkipWeightHeight_H265;
    s32Ret = SC_MPI_SYS_MmzAlloc(&u64PhyAddr, (void **)&pVirAddr, NULL, SC_NULL, u32SkipWeightSize_H265 * QpMapBufNum);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_SYS_MmzAlloc err:0x%x", s32Ret);
        SC_MPI_SYS_MmzFree(u64QpMapPhyAddr[0], pQpMapVirAddr[0]);
        SC_MPI_SYS_MmzFree(u64SkipWeightPhyAddr_H264[0], pSkipWeightVirAddr_H264[0]);
        return NULL;
    }
    for(i = 0; i < QpMapBufNum; i++)
    {
        u64SkipWeightPhyAddr_H265[i] = u64PhyAddr + i * u32SkipWeightSize_H265;
        pSkipWeightVirAddr_H265[i]   = pVirAddr + i * u32SkipWeightSize_H265;
    }

    s32Ret = SC_MPI_VPSS_GetChnAttr(pstPara->VpssGrp, pstPara->VpssChn, &stChnAttr);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VPSS_GetChnAttr err:0x%x", s32Ret);

        return NULL;
    }

    stChnAttr.u32Depth = 3;
    s32Ret = SC_MPI_VPSS_SetChnAttr(pstPara->VpssGrp, pstPara->VpssChn, &stChnAttr);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_VPSS_SetChnAttr err:0x%x", s32Ret);

        return NULL;
    }

    i = 0;
    while(SC_TRUE == pstPara->bThreadStart)
    {
        pstVideoFrame = &stFrame[i].stUserFrame;
        s32Ret = SC_MPI_VPSS_GetChnFrame(pstPara->VpssGrp, pstPara->VpssChn, pstVideoFrame, 1000);
        if(SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SC_MPI_VPSS_GetChnFrame err:0x%x\n", s32Ret);
            continue;
        }

        pVirAddrTemp = (SC_U8 *)pQpMapVirAddr[i];
        for(j = 0; j < u32QpMapSize; j++)
        {
            *pVirAddrTemp = 0x5E;
            pVirAddrTemp++;
        }

        pVirAddrTemp = (SC_U8 *)pSkipWeightVirAddr_H264[i];
        for(j = 0; j < u32SkipWeightSize_H264; j++)
        {
            *pVirAddrTemp = 0x88;
            pVirAddrTemp++;
        }

        pVirAddrTemp = (SC_U8 *)pSkipWeightVirAddr_H265[i];
        for(j = 0; j < u32SkipWeightSize_H265; j++)
        {
            *pVirAddrTemp = 0x88;
            pVirAddrTemp++;
        }

        for(VeChnCnt = 0; VeChnCnt < pstPara->s32Cnt; VeChnCnt++)
        {
            VENC_CHN_ATTR_S stChnAttr;
            SC_MPI_VENC_GetChnAttr(pstPara->VeChn[VeChnCnt], &stChnAttr);
            if(PT_H264 == stChnAttr.stVencAttr.enType)
            {
                stFrame[i].stUserRcInfo.bSkipWeightValid = 1;
                stFrame[i].stUserRcInfo.u64SkipWeightPhyAddr = u64SkipWeightPhyAddr_H264[i];
            }
            else if(PT_H265 == stChnAttr.stVencAttr.enType)
            {
                stFrame[i].stUserRcInfo.bSkipWeightValid = 1;
                stFrame[i].stUserRcInfo.u64SkipWeightPhyAddr = u64SkipWeightPhyAddr_H265[i];
            }
            else
            {
                continue;
            }

            stFrame[i].stUserRcInfo.bQpMapValid     = 1;
            stFrame[i].stUserRcInfo.u64QpMapPhyAddr = u64QpMapPhyAddr[i];
            stFrame[i].stUserRcInfo.u32BlkStartQp   = 30;
            stFrame[i].stUserRcInfo.enFrameType = VENC_FRAME_TYPE_NONE;

            s32Ret = SC_MPI_VENC_SendFrameEx(pstPara->VeChn[VeChnCnt], &stFrame[i], -1);
            if(SC_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("SC_MPI_VENC_SendFrame err:0x%x\n", s32Ret);
                break;
            }
        }
        if(SC_SUCCESS != s32Ret)
        {
            s32Ret = SC_MPI_VPSS_ReleaseChnFrame(pstPara->VpssGrp, pstPara->VpssChn, pstVideoFrame);
            if(SC_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("SC_MPI_VPSS_ReleaseChnFrame err:0x%x", s32Ret);
                goto err_out;
            }
            continue;
        }

        s32Ret = SC_MPI_VPSS_ReleaseChnFrame(pstPara->VpssGrp, pstPara->VpssChn, pstVideoFrame);
        if(SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SC_MPI_VPSS_ReleaseChnFrame err:0x%x", s32Ret);
            goto err_out;
        }

        i++;
        if(i >= QpMapBufNum)
        {
            i = 0;
        }
    }
err_out:
    s32Ret = SC_MPI_SYS_MmzFree(u64QpMapPhyAddr[0], pQpMapVirAddr[0]);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_SYS_MmzFree err:0x%x", s32Ret);
        return NULL;
    }

    s32Ret =  SC_MPI_SYS_MmzFree(u64SkipWeightPhyAddr_H264[0], pSkipWeightVirAddr_H264[0]);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_SYS_MmzFree err:0x%x", s32Ret);
        return NULL;
    }

    s32Ret = SC_MPI_SYS_MmzFree(u64SkipWeightPhyAddr_H265[0], pSkipWeightVirAddr_H265[0]);
    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_SYS_MmzFree err:0x%x", s32Ret);
        return NULL;
    }

    return NULL;
}
SC_S32 SAMPLE_COMM_VENC_QpmapSendFrame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VENC_CHN VeChn[], SC_S32 s32Cnt,
    SIZE_S stSize)
{
    SC_S32 i;

    stQpMapSendFramePara.bThreadStart = SC_TRUE;
    stQpMapSendFramePara.VpssGrp = VpssGrp;
    stQpMapSendFramePara.VpssChn = VpssChn;
    stQpMapSendFramePara.s32Cnt  = s32Cnt;
    stQpMapSendFramePara.stSize  = stSize;
    for(i = 0; i < s32Cnt; i++)
    {
        stQpMapSendFramePara.VeChn[i] = VeChn[i];
    }

    return pthread_create(&gs_VencQpmapPid, 0, SAMPLE_COMM_QpmapSendFrameProc, (SC_VOID *)&stQpMapSendFramePara);
}
#endif

/******************************************************************************
* funciton : get stream from each channels and save them
******************************************************************************/
SC_VOID *SAMPLE_COMM_VENC_GetVencStreamProc(SC_VOID *p)
{
    SC_S32 i;
    SC_S32 s32ChnTotal;
    VENC_CHN_ATTR_S stVencChnAttr;
    SAMPLE_VENC_GETSTREAM_PARA_S *pstPara;
    SC_S32 maxfd = 0;
    struct timeval TimeoutVal;
    fd_set read_fds;
    SC_U32 u32PictureCnt[VENC_MAX_CHN_NUM] = {0};
    static SC_S32 VencFd[VENC_MAX_CHN_NUM] = {0};
    SC_CHAR aszFileName[VENC_MAX_CHN_NUM][64];
    FILE *pFile[VENC_MAX_CHN_NUM];
    char szFilePostfix[10];
    VENC_CHN_STATUS_S stStat;
    VENC_STREAM_S stStream;
    SC_S32 s32Ret;
    VENC_CHN VencChn;
    PAYLOAD_TYPE_E enPayLoadType[VENC_MAX_CHN_NUM];
    VENC_STREAM_BUF_INFO_S stStreamBufInfo[VENC_MAX_CHN_NUM];

    prctl(PR_SET_NAME, "GetVencStream", 0, 0, 0);

    pstPara = (SAMPLE_VENC_GETSTREAM_PARA_S *)p;
    s32ChnTotal = pstPara->s32Cnt;
    /******************************************
     step 1:  check & prepare save-file & venc-fd
    ******************************************/
    if (s32ChnTotal >= VENC_MAX_CHN_NUM)
    {
        SAMPLE_PRT("input count invaild\n");
        pthread_exit(NULL);
    }
    for (i = 0; i < s32ChnTotal; i++)
    {
        /* decide the stream file name, and open file to save stream */
        VencChn = pstPara->VeChn[i];
        s32Ret = SC_MPI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SC_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", \
                VencChn, s32Ret);
            pthread_exit(NULL);
        }
        enPayLoadType[i] = stVencChnAttr.stVencAttr.enType;

        s32Ret = SAMPLE_COMM_VENC_GetFilePostfix(enPayLoadType[i], szFilePostfix);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_COMM_VENC_GetFilePostfix [%d] failed with %#x!\n", \
                stVencChnAttr.stVencAttr.enType, s32Ret);
            pthread_exit(NULL);
        }
        if(PT_JPEG != enPayLoadType[i])
        {
            snprintf(aszFileName[i], 32, "stream_chn%d%s", i, szFilePostfix);

            pFile[i] = fopen(aszFileName[i], "wb");
            if (!pFile[i])
            {
                SAMPLE_PRT("open file[%s] failed!\n",
                    aszFileName[i]);
                pthread_exit(NULL);
            }
        }

        /* Set Venc Fd. */
        VencFd[i] = SC_MPI_VENC_GetFd(i);
        if (VencFd[i] < 0)
        {
            SAMPLE_PRT("SC_MPI_VENC_GetFd failed with %#x!\n",
                VencFd[i]);
            pthread_exit(NULL);
        }
        if (maxfd <= VencFd[i])
        {
            maxfd = VencFd[i];
        }

        #if 0
        s32Ret = SC_MPI_VENC_GetStreamBufInfo (i, &stStreamBufInfo[i]);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SC_MPI_VENC_GetStreamBufInfo failed with %#x!\n", s32Ret);
            return (void *)SC_FAILURE;
        }
        #endif
    }

    /******************************************
     step 2:  Start to get streams of each channel.
    ******************************************/
    while (SC_TRUE == pstPara->bThreadStart)
    {
        FD_ZERO(&read_fds);
        for (i = 0; i < s32ChnTotal; i++)
        {
            FD_SET(VencFd[i], &read_fds);
        }

        TimeoutVal.tv_sec  = 2;
        TimeoutVal.tv_usec = 0;//1000 * 200;
        s32Ret = select(maxfd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            SAMPLE_PRT("select failed!\n");
            break;
        }
        else if (s32Ret == 0)
        {
            SAMPLE_PRT("get venc stream time out, exit thread\n");
            continue;
        }
        else
        {
            for (i = 0; i < s32ChnTotal; i++)
            {
                if (FD_ISSET(VencFd[i], &read_fds))
                {
                    /*******************************************************
                     step 2.1 : query how many packs in one-frame stream.
                    *******************************************************/
                    memset(&stStream, 0, sizeof(stStream));

                    s32Ret = SC_MPI_VENC_QueryStatus(i, &stStat);
                    if (SC_SUCCESS != s32Ret)
                    {
                        SAMPLE_PRT("SC_MPI_VENC_QueryStatus chn[%d] failed with %#x!\n", i, s32Ret);
                        break;
                    }

                    /*******************************************************
                    step 2.2 :suggest to check both u32CurPacks and u32LeftStreamFrames at the same time,for example:
                     if(0 == stStat.u32CurPacks || 0 == stStat.u32LeftStreamFrames)
                     {
                        SAMPLE_PRT("NOTE: Current  frame is NULL!\n");
                        continue;
                     }
                    *******************************************************/
                    if(0 == stStat.u32CurPacks)
                    {
                        static int n = 0;
                        //SAMPLE_PRT("NOTE: Current  frame is NULL!\n");
                        SAMPLE_PRT("NOTE: Current  frame is NULL!(%d)\n", ++n);
                        usleep(10 * 1000);
                        continue;
                    }

                    /*******************************************************
                     step 2.3 : malloc corresponding number of pack nodes.
                    *******************************************************/
                    stStream.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
                    if (NULL == stStream.pstPack)
                    {
                        SAMPLE_PRT("malloc stream pack failed!\n");
                        break;
                    }

                    /*******************************************************
                     step 2.4 : call mpi to get one-frame stream
                    *******************************************************/
                    stStream.u32PackCount = stStat.u32CurPacks;
                    s32Ret = SC_MPI_VENC_GetStream(i, &stStream, SC_TRUE);
                    if (SC_SUCCESS != s32Ret)
                    {
                        free(stStream.pstPack);
                        stStream.pstPack = NULL;
                        SAMPLE_PRT("SC_MPI_VENC_GetStream failed with %#x!\n", \
                            s32Ret);
                        break;
                    }

                    /*******************************************************
                     step 2.5 : save frame to file
                    *******************************************************/
                    if(PT_JPEG == enPayLoadType[i])
                    {
                        snprintf(aszFileName[i], 32, "stream_chn%d_%d%s", i, u32PictureCnt[i], szFilePostfix);
                        pFile[i] = fopen(aszFileName[i], "wb");
                        if (!pFile[i])
                        {
                            SAMPLE_PRT("open file err!\n");
                            pthread_exit(NULL);
                        }
                    }
                    if(pstPara->saveFile)
                    {
                        s32Ret = SAMPLE_COMM_VENC_SaveStream(pFile[i], &stStream);
                        if (SC_SUCCESS != s32Ret)
                        {
                            free(stStream.pstPack);
                            stStream.pstPack = NULL;
                            SAMPLE_PRT("save stream failed!\n");
                            break;
                        }
                    }

                    /*******************************************************
                     step 2.6 : release stream
                     *******************************************************/
                    s32Ret = SC_MPI_VENC_ReleaseStream(i, &stStream);
                    if (SC_SUCCESS != s32Ret)
                    {
                        SAMPLE_PRT("SC_MPI_VENC_ReleaseStream failed!\n");
                        free(stStream.pstPack);
                        stStream.pstPack = NULL;
                        break;
                    }

                    /*******************************************************
                     step 2.7 : free pack nodes
                    *******************************************************/
                    free(stStream.pstPack);
                    stStream.pstPack = NULL;
                    u32PictureCnt[i]++;
                    if(PT_JPEG == enPayLoadType[i])
                    {
                        fclose(pFile[i]);
                    }
                }
            }
        }
    }
    /*******************************************************
    * step 3 : close save-file
    *******************************************************/
    for (i = 0; i < s32ChnTotal; i++)
    {
        if(PT_JPEG != enPayLoadType[i])
        {
            fclose(pFile[i]);
        }
    }
    SAMPLE_PRT("%s: thread exit\n", __func__);
    pthread_exit(NULL);
}

/******************************************************************************
* funciton : get svc_t stream from h264 channels and save them
******************************************************************************/
SC_VOID *SAMPLE_COMM_VENC_GetVencStreamProc_Svc_t(void *p)
{
    SC_S32 i = 0;
    SC_S32 s32Cnt = 0;
    SC_S32 s32ChnTotal;
    VENC_CHN_ATTR_S stVencChnAttr;
    SAMPLE_VENC_GETSTREAM_PARA_S *pstPara;
    SC_S32 maxfd = 0;
    struct timeval TimeoutVal;
    fd_set read_fds;
    SC_S32 VencFd[VENC_MAX_CHN_NUM];
    SC_CHAR aszFileName[VENC_MAX_CHN_NUM][64];
    FILE *pFile[VENC_MAX_CHN_NUM];
    char szFilePostfix[10];
    VENC_CHN_STATUS_S stStat;
    VENC_STREAM_S stStream;
    SC_S32 s32Ret;
    VENC_CHN VencChn;
    PAYLOAD_TYPE_E enPayLoadType[VENC_MAX_CHN_NUM];
    VENC_STREAM_BUF_INFO_S stStreamBufInfo[VENC_MAX_CHN_NUM];

    pstPara = (SAMPLE_VENC_GETSTREAM_PARA_S *)p;
    s32ChnTotal = pstPara->s32Cnt;

    /******************************************
     step 1:  check & prepare save-file & venc-fd
    ******************************************/
    if (s32ChnTotal >= VENC_MAX_CHN_NUM)
    {
        SAMPLE_PRT("input count invaild\n");
        return NULL;
    }
    for (i = 0; i < s32ChnTotal; i++)
    {
        /* decide the stream file name, and open file to save stream */
        VencChn = i;
        s32Ret = SC_MPI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SC_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", \
                VencChn, s32Ret);
            return NULL;
        }
        enPayLoadType[i] = stVencChnAttr.stVencAttr.enType;

        s32Ret = SAMPLE_COMM_VENC_GetFilePostfix(enPayLoadType[i], szFilePostfix);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_COMM_VENC_GetFilePostfix [%d] failed with %#x!\n", \
                stVencChnAttr.stVencAttr.enType, s32Ret);
            return NULL;
        }

        for (s32Cnt = 0; s32Cnt < 3; s32Cnt++)
        {
            snprintf(aszFileName[i + s32Cnt], 32, "Tid%d%s", i + s32Cnt, szFilePostfix);

            pFile[i + s32Cnt] = fopen(aszFileName[i + s32Cnt], "wb");

            if (!pFile[i + s32Cnt])
            {
                SAMPLE_PRT("open file[%s] failed!\n",
                    aszFileName[i + s32Cnt]);
                return NULL;
            }
        }

        /* Set Venc Fd. */
        VencFd[i] = SC_MPI_VENC_GetFd(i);
        if (VencFd[i] < 0)
        {
            SAMPLE_PRT("SC_MPI_VENC_GetFd failed with %#x!\n",
                VencFd[i]);
            return NULL;
        }
        if (maxfd <= VencFd[i])
        {
            maxfd = VencFd[i];
        }
        s32Ret = SC_MPI_VENC_GetStreamBufInfo (i, &stStreamBufInfo[i]);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SC_MPI_VENC_GetStreamBufInfo failed with %#x!\n", s32Ret);
            return NULL;
        }
    }

    /******************************************
     step 2:  Start to get streams of each channel.
    ******************************************/
    while (SC_TRUE == pstPara->bThreadStart)
    {
        FD_ZERO(&read_fds);
        for (i = 0; i < s32ChnTotal; i++)
        {
            FD_SET(VencFd[i], &read_fds);
        }
        TimeoutVal.tv_sec  = 2;
        TimeoutVal.tv_usec = 0;
        s32Ret = select(maxfd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            SAMPLE_PRT("select failed!\n");
            break;
        }
        else if (s32Ret == 0)
        {
            SAMPLE_PRT("get venc stream time out, exit thread\n");
            continue;
        }
        else
        {
            for (i = 0; i < s32ChnTotal; i++)
            {
                if (FD_ISSET(VencFd[i], &read_fds))
                {
                    /*******************************************************
                    step 2.1 : query how many packs in one-frame stream.
                    *******************************************************/
                    memset(&stStream, 0, sizeof(stStream));
                    s32Ret = SC_MPI_VENC_QueryStatus(i, &stStat);
                    if (SC_SUCCESS != s32Ret)
                    {
                        SAMPLE_PRT("SC_MPI_VENC_Query chn[%d] failed with %#x!\n", i, s32Ret);
                        break;
                    }
                    /*******************************************************
                    step 2.2 :suggest to check both u32CurPacks and u32LeftStreamFrames at the same time,for example:
                     if(0 == stStat.u32CurPacks || 0 == stStat.u32LeftStreamFrames)
                     {                        SAMPLE_PRT("NOTE: Current  frame is NULL!\n");
                        continue;
                     }
                     *******************************************************/
                    if (0 == stStat.u32CurPacks)
                    {
                        SAMPLE_PRT("NOTE: Current  frame is NULL!\n");
                        continue;
                    }
                    /*******************************************************
                     step 2.3 : malloc corresponding number of pack nodes.
                    *******************************************************/
                    stStream.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
                    if (NULL == stStream.pstPack)
                    {
                        SAMPLE_PRT("malloc stream pack failed!\n");
                        break;
                    }
                    /*******************************************************
                     step 2.4 : call mpi to get one-frame stream
                    *******************************************************/
                    stStream.u32PackCount = stStat.u32CurPacks;
                    s32Ret = SC_MPI_VENC_GetStream(i, &stStream, SC_TRUE);
                    if (SC_SUCCESS != s32Ret)
                    {
                        free(stStream.pstPack);
                        stStream.pstPack = NULL;
                        SAMPLE_PRT("SC_MPI_VENC_GetStream failed with %#x!\n", \
                            s32Ret);
                        break;
                    }
                    /*******************************************************
                     step 2.5 : save frame to file
                    *******************************************************/
                    for (s32Cnt = 0; s32Cnt < 3; s32Cnt++)
                    {
                        switch (s32Cnt)
                        {
                        case 0:
                            if (BASE_IDRSLICE == stStream.stH264Info.enRefType ||
                                BASE_PSLICE_REFBYBASE == stStream.stH264Info.enRefType)
                            {
                                s32Ret = SAMPLE_COMM_VENC_SaveStream(pFile[i + s32Cnt], &stStream);
                            }
                            break;
                        case 1:
                            if (BASE_IDRSLICE == stStream.stH264Info.enRefType      ||
                                BASE_PSLICE_REFBYBASE == stStream.stH264Info.enRefType ||
                                BASE_PSLICE_REFBYENHANCE == stStream.stH264Info.enRefType)
                            {
                                s32Ret = SAMPLE_COMM_VENC_SaveStream(pFile[i + s32Cnt], &stStream);
                            }
                            break;
                        case 2:
                            s32Ret = SAMPLE_COMM_VENC_SaveStream(pFile[i + s32Cnt], &stStream);
                            break;
                        }
                        if (SC_SUCCESS != s32Ret)
                        {
                            free(stStream.pstPack);
                            stStream.pstPack = NULL;
                            SAMPLE_PRT("save stream failed!\n");
                            break;
                        }
                    }

                    /*******************************************************
                    step 2.6 : release stream
                    *******************************************************/
                    s32Ret = SC_MPI_VENC_ReleaseStream(i, &stStream);
                    if (SC_SUCCESS != s32Ret)
                    {
                        free(stStream.pstPack);
                        stStream.pstPack = NULL;
                        break;
                    }
                    /*******************************************************
                     step 2.7 : free pack nodes
                    *******************************************************/
                    free(stStream.pstPack);
                    stStream.pstPack = NULL;
                }
            }
        }
    }
    /*******************************************************
     step 3 : close save-file
    *******************************************************/
    for (i = 0; i < s32ChnTotal; i++)
    {
        for (s32Cnt = 0; s32Cnt < 3; s32Cnt++)
        {
            if (pFile[i + s32Cnt])
            {
                fclose(pFile[i + s32Cnt]);
            }
        }
    }
    return NULL;
}

/******************************************************************************
* funciton : start get venc stream process thread
******************************************************************************/
SC_S32 SAMPLE_COMM_VENC_StartGetStream(VENC_CHN VeChn[], SC_S32 s32Cnt)
{
    SC_U32 i;

    gs_stPara.bThreadStart = SC_TRUE;
    gs_stPara.s32Cnt = s32Cnt;
    gs_stPara.saveFile = SC_TRUE;
    for(i = 0; i < s32Cnt; i++)
    {
        gs_stPara.VeChn[i] = VeChn[i];
    }
    return pthread_create(&gs_VencPid, 0, SAMPLE_COMM_VENC_GetVencStreamProc, (SC_VOID *)&gs_stPara);
}

SC_S32 SAMPLE_COMM_VENC_StartGetStreamNoFile(VENC_CHN VeChn[], SC_S32 s32Cnt)
{
    SC_U32 i;

    gs_stPara.bThreadStart = SC_TRUE;
    gs_stPara.s32Cnt = s32Cnt;
    gs_stPara.saveFile = SC_FALSE;
    for(i = 0; i < s32Cnt; i++)
    {
        gs_stPara.VeChn[i] = VeChn[i];
    }
    return pthread_create(&gs_VencPid, 0, SAMPLE_COMM_VENC_GetVencStreamProc, (SC_VOID *)&gs_stPara);
}


/******************************************************************************
* funciton : start get venc svc-t stream process thread
******************************************************************************/
SC_S32 SAMPLE_COMM_VENC_StartGetStream_Svc_t(SC_S32 s32Cnt)
{
    gs_stPara.bThreadStart = SC_TRUE;
    gs_stPara.s32Cnt = s32Cnt;
    return pthread_create(&gs_VencPid, 0, SAMPLE_COMM_VENC_GetVencStreamProc_Svc_t, (SC_VOID *)&gs_stPara);
}

/******************************************************************************
* funciton : stop get venc stream process.
******************************************************************************/
SC_S32 SAMPLE_COMM_VENC_StopGetStream(void)
{
    if (SC_TRUE == gs_stPara.bThreadStart)
    {
        gs_stPara.bThreadStart = SC_FALSE;
        pthread_join(gs_VencPid, 0);
    }
    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VENC_StopSendQpmapFrame(void)
{
    if (SC_TRUE == stQpMapSendFramePara.bThreadStart)
    {
        stQpMapSendFramePara.bThreadStart = SC_FALSE;
        pthread_join(gs_VencQpmapPid, 0);
    }
    return SC_SUCCESS;
}

SC_VOID SAMPLE_COMM_VENC_ReadOneFrame( FILE *fp, SC_U8 *pY, SC_U8 *pU, SC_U8 *pV,
    SC_U32 width, SC_U32 height, SC_U32 stride, SC_U32 stride2)
{
    SC_U8 *pDst;
    SC_U32 u32Row;
    pDst = pY;
    for ( u32Row = 0; u32Row < height; u32Row++ )
    {
        fread( pDst, width, 1, fp );
        pDst += stride;
    }
    pDst = pU;
    for ( u32Row = 0; u32Row < height / 2; u32Row++ )
    {
        fread( pDst, width / 2, 1, fp );
        pDst += stride2;
    }
    pDst = pV;
    for ( u32Row = 0; u32Row < height / 2; u32Row++ )
    {
        fread( pDst, width / 2, 1, fp );
        pDst += stride2;
    }
}

SC_S32 SAMPLE_COMM_VENC_PlanToSemi(SC_U8 *pY, SC_S32 yStride,
    SC_U8 *pU, SC_S32 uStride,
    SC_U8 *pV, SC_S32 vStride,
    SC_S32 picWidth, SC_S32 picHeight)
{
    SC_S32 i;
    SC_U8 *pTmpU, *ptu;
    SC_U8 *pTmpV, *ptv;
    SC_S32 s32HafW = uStride >> 1 ;
    SC_S32 s32HafH = picHeight >> 1 ;
    SC_S32 s32Size = s32HafW * s32HafH;

    pTmpU = malloc( s32Size );
    if(pTmpU == SC_NULL)
    {
        printf("malloc buf failed\n");
        return SC_FAILURE;
    }
    ptu = pTmpU;

    pTmpV = malloc( s32Size );
    if (pTmpV == SC_NULL)
    {
        printf("malloc buf failed\n");
        free( pTmpU );
        return SC_FAILURE;
    }
    ptv = pTmpV;

    memcpy(pTmpU, pU, s32Size);
    memcpy(pTmpV, pV, s32Size);
    for (i = 0; i<s32Size >> 1; i++)
    {
        *pU++ = *pTmpV++;
        *pU++ = *pTmpU++;
    }
    for (i = 0; i<s32Size >> 1; i++)
    {
        *pV++ = *pTmpV++;
        *pV++ = *pTmpU++;
    }
    free( ptu );
    free( ptv );
    return SC_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

