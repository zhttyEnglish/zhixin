#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/prctl.h>

#include "sample_comm_ive.h"
#include "ivs_md.h"

#define SAMPLE_IVE_MD_IMAGE_NUM 2

typedef struct scSAMPLE_IVE_MD_S
{
    IVE_SRC_IMAGE_S astImg[SAMPLE_IVE_MD_IMAGE_NUM];
    IVE_DST_MEM_INFO_S stBlob;
    MD_ATTR_S stMdAttr;
    SAMPLE_RECT_ARRAY_S stRegion;

} SAMPLE_IVE_MD_S;

static SC_BOOL s_bStopSignal = SC_FALSE;
static pthread_t s_hMdThread = 0;
static SAMPLE_IVE_MD_S s_stMd;
static SAMPLE_IVE_SWITCH_S s_stMdSwitch = {SC_FALSE, SC_TRUE};
static SAMPLE_VI_CONFIG_S s_stViConfig = {0};

static SC_VOID SAMPLE_IVE_Md_Uninit(SAMPLE_IVE_MD_S *pstMd)
{
    SC_S32 i;
    SC_S32 s32Ret = SC_SUCCESS;

    for (i = 0; i < SAMPLE_IVE_MD_IMAGE_NUM; i++)
    {
        IVE_MMZ_FREE(pstMd->astImg[i].au64PhyAddr[0], pstMd->astImg[i].au64VirAddr[0]);
    }

    IVE_MMZ_FREE(pstMd->stBlob.u64PhyAddr, pstMd->stBlob.u64VirAddr);

    s32Ret = SC_IVS_MD_Exit();
    if(s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_IVS_MD_Exit fail,Error(%#x)\n", s32Ret);
        return ;
    }
}

static SC_S32 SAMPLE_IVE_Md_Init(SAMPLE_IVE_MD_S *pstMd, SC_U32 u32Width, SC_U32 u32Height)
{
    SC_S32 s32Ret = SC_SUCCESS;
    SC_S32 i ;
    SC_U32 u32Size;
    SC_U8 u8WndSz;

    for (i = 0; i < SAMPLE_IVE_MD_IMAGE_NUM; i++)
    {
        s32Ret = SAMPLE_COMM_IVE_CreateImage(&pstMd->astImg[i], IVE_IMAGE_TYPE_U8C1, u32Width, u32Height);
        SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, MD_INIT_FAIL,
            "Error(%#x),Create img[%d] image failed!\n", s32Ret, i);
    }
    u32Size = sizeof(IVE_CCBLOB_S);
    s32Ret = SAMPLE_COMM_IVE_CreateMemInfo(&pstMd->stBlob, u32Size);
    SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, MD_INIT_FAIL,
        "Error(%#x),Create blob mem info failed!\n", s32Ret);

    //Set attr info
    pstMd->stMdAttr.enAlgMode = MD_ALG_MODE_BG;
    pstMd->stMdAttr.enSadMode = IVE_SAD_MODE_MB_4X4;
    pstMd->stMdAttr.enSadOutCtrl = IVE_SAD_OUT_CTRL_THRESH;
    pstMd->stMdAttr.u16SadThr = 100 * (1 << 1);//100 * (1 << 2);
    pstMd->stMdAttr.u32Width = u32Width;
    pstMd->stMdAttr.u32Height = u32Height;
    pstMd->stMdAttr.stAddCtrl.u0q16X = 32768;
    pstMd->stMdAttr.stAddCtrl.u0q16Y = 32768;
    pstMd->stMdAttr.stCclCtrl.enMode = IVE_CCL_MODE_4C;
    u8WndSz = ( 1 << (2 + pstMd->stMdAttr.enSadMode));
    pstMd->stMdAttr.stCclCtrl.u16InitAreaThr = u8WndSz * u8WndSz;
    pstMd->stMdAttr.stCclCtrl.u16Step = u8WndSz;

    s32Ret = SC_IVS_MD_Init();
    SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, MD_INIT_FAIL,
        "Error(%#x),SC_IVS_MD_Init failed!\n", s32Ret);

MD_INIT_FAIL:

    if(SC_SUCCESS != s32Ret)
    {
        SAMPLE_IVE_Md_Uninit(pstMd);
    }
    return s32Ret;
}

static SC_VOID *SAMPLE_IVE_MdProc(SC_VOID *pArgs)
{
    SC_S32 s32Ret;
    SAMPLE_IVE_MD_S *pstMd;
    MD_ATTR_S *pstMdAttr;
    VIDEO_FRAME_INFO_S stBaseFrmInfo;
    VIDEO_FRAME_INFO_S stExtFrmInfo;
    SC_S32 s32VpssGrp = 0;
    SC_S32 as32VpssChn[] = {VPSS_CHN0, VPSS_CHN1};
    SC_S32 s32MilliSec = 20000;
    MD_CHN MdChn = 0;
    VO_LAYER voLayer = 0;
    VO_CHN voChn = 0;
    SC_BOOL bInstant = SC_TRUE;
    SC_S32 s32CurIdx = 0;
    SC_BOOL bFirstFrm = SC_TRUE;
    pstMd = (SAMPLE_IVE_MD_S *)(pArgs);
    pstMdAttr = &(pstMd->stMdAttr);
    //Create chn
    s32Ret = SC_IVS_MD_CreateChn(MdChn, &(pstMd->stMdAttr));
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_IVS_MD_CreateChn fail,Error(%#x)\n", s32Ret);
        return NULL;
    }

    IVE_SRC_IMAGE_S srcIveImg;
    struct timeval s_tv, e_tv;
    int framecount = 0;
    while (SC_FALSE == s_bStopSignal)
    {
        s32Ret = SC_MPI_VPSS_GetChnFrame(s32VpssGrp, as32VpssChn[1], &stExtFrmInfo, s32MilliSec);
        if(SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Error(%#x),SC_MPI_VPSS_GetChnFrame failed, VPSS_GRP(%d), VPSS_CHN(%d)!\n",
                s32Ret, s32VpssGrp, as32VpssChn[1]);
            continue;
        }

        s32Ret = SC_MPI_VPSS_GetChnFrame(s32VpssGrp, as32VpssChn[0], &stBaseFrmInfo, s32MilliSec);
        SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, EXT_RELEASE,
            "Error(%#x),SC_MPI_VPSS_GetChnFrame failed, VPSS_GRP(%d), VPSS_CHN(%d)!\n",
            s32Ret, s32VpssGrp, as32VpssChn[0]);

        if (SC_TRUE != bFirstFrm)
        {
            s32Ret = SAMPLE_COMM_IVE_DmaImage(&stExtFrmInfo, &pstMd->astImg[s32CurIdx], bInstant);
            SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, BASE_RELEASE,
                "SAMPLE_COMM_IVE_DmaImage fail,Error(%#x)\n", s32Ret);
        }
        else
        {
            s32Ret = SAMPLE_COMM_IVE_DmaImage(&stExtFrmInfo, &pstMd->astImg[s32CurIdx], bInstant);
            SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, BASE_RELEASE,
                "SAMPLE_COMM_IVE_DmaImage fail,Error(%#x)\n", s32Ret);
            bFirstFrm = SC_FALSE;
            goto CHANGE_IDX;//first frame just init reference frame
        }
        s32Ret = SC_IVS_MD_Process(MdChn, &pstMd->astImg[s32CurIdx], &pstMd->astImg[1 - s32CurIdx], NULL, &pstMd->stBlob);

        SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, BASE_RELEASE,
            "SC_IVS_MD_Process fail,Error(%#x)\n", s32Ret);
        SAMPLE_COMM_IVE_BlobToRect(SAMPLE_COMM_IVE_CONVERT_64BIT_ADDR(IVE_CCBLOB_S, pstMd->stBlob.u64VirAddr),
            &(pstMd->stRegion), IVE_RECT_NUM, 8,
            pstMdAttr->u32Width, pstMdAttr->u32Height, stBaseFrmInfo.stVFrame.u32Width, stBaseFrmInfo.stVFrame.u32Height);

        #if 1
        //Draw rect
        s32Ret = SAMPLE_COMM_VGS_FillRect(&stBaseFrmInfo, &pstMd->stRegion, 0x0000FF00);
        SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, BASE_RELEASE,
            "SAMPLE_COMM_VGS_FillRect fail,Error(%#x)\n", s32Ret);
        #endif

        s32Ret = SC_MPI_VO_SendFrame(voLayer, voChn, &stBaseFrmInfo, s32MilliSec);
        SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, BASE_RELEASE,
            "SC_MPI_VO_SendFrame fail,Error(%#x)\n", s32Ret);

CHANGE_IDX:
        //Change reference and current frame index
        s32CurIdx =    1 - s32CurIdx;

BASE_RELEASE:
        s32Ret = SC_MPI_VPSS_ReleaseChnFrame(s32VpssGrp, as32VpssChn[0], &stBaseFrmInfo);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Error(%#x),SC_MPI_VPSS_ReleaseChnFrame failed,Grp(%d) chn(%d)!\n",
                s32Ret, s32VpssGrp, as32VpssChn[0]);
        }

EXT_RELEASE:
        s32Ret = SC_MPI_VPSS_ReleaseChnFrame(s32VpssGrp, as32VpssChn[1], &stExtFrmInfo);
        if (SC_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Error(%#x),SC_MPI_VPSS_ReleaseChnFrame failed,Grp(%d) chn(%d)!\n",
                s32Ret, s32VpssGrp, as32VpssChn[1]);
        }
    }

    //destroy
    s32Ret = SC_IVS_MD_DestroyChn(MdChn);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_IVS_MD_DestroyChn fail,Error(%#x)\n", s32Ret);
    }

    return SC_NULL;
}

SC_VOID SAMPLE_IVE_Md(SC_VOID)
{
    SIZE_S stSize;
    PIC_SIZE_E enSize = PIC_D1_PAL;
    SC_S32 s32Ret = SC_SUCCESS;
    SC_CHAR acThreadName[16] = {0};

    memset(&s_stMd, 0, sizeof(s_stMd));
    /******************************************
     step 1: start vi vpss venc vo
     ******************************************/
    s32Ret = SAMPLE_COMM_IVE_StartViVpssVencVo(&s_stViConfig, &s_stMdSwitch, &enSize);
    SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, END_MD_0,
        "Error(%#x),SAMPLE_COMM_IVE_StartViVpssVencVo failed!\n", s32Ret);

    enSize = PIC_D1_PAL;
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSize, &stSize);
    SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, END_MD_0,
        "Error(%#x),SAMPLE_COMM_SYS_GetPicSize failed!\n", s32Ret);

    /******************************************
     step 2: Init Md
     ******************************************/
    s32Ret = SAMPLE_IVE_Md_Init(&s_stMd, stSize.u32Width, stSize.u32Height);
    SAMPLE_CHECK_EXPR_GOTO(SC_SUCCESS != s32Ret, END_MD_0,
        " Error(%#x),SAMPLE_IVE_Md_Init failed!\n", s32Ret);
    s_bStopSignal = SC_FALSE;
    /******************************************
      step 3: Create work thread
     ******************************************/
    snprintf(acThreadName, 16, "IVE_MdProc");
    prctl(PR_SET_NAME, (unsigned long)acThreadName, 0, 0, 0);
    pthread_create(&s_hMdThread, 0, SAMPLE_IVE_MdProc, (SC_VOID *)&s_stMd);

    SAMPLE_PAUSE();
    s_bStopSignal = SC_TRUE;
    pthread_join(s_hMdThread, SC_NULL);
    s_hMdThread = 0;

    SAMPLE_IVE_Md_Uninit(&(s_stMd));
    memset(&s_stMd, 0, sizeof(s_stMd));

END_MD_0:
    SAMPLE_COMM_IVE_StopViVpssVencVo(&s_stViConfig, &s_stMdSwitch);
    return ;
}

/******************************************************************************
* function : Md sample signal handle
******************************************************************************/
SC_VOID SAMPLE_IVE_Md_HandleSig(SC_VOID)
{
    s_bStopSignal = SC_TRUE;
    if (0 != s_hMdThread)
    {
        pthread_join(s_hMdThread, SC_NULL);
        s_hMdThread = 0;
    }
    SAMPLE_IVE_Md_Uninit(&(s_stMd));
    memset(&s_stMd, 0, sizeof(s_stMd));

    SAMPLE_COMM_IVE_StopViVpssVencVo(&s_stViConfig, &s_stMdSwitch);
}
