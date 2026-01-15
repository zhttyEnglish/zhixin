
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
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include "sample_comm.h"

/*****************************************************************************
* function : start vpss grp.
*****************************************************************************/
SC_S32 SAMPLE_COMM_VPSS_Start(VPSS_GRP VpssGrp, SC_BOOL *pabChnEnable, VPSS_GRP_ATTR_S *pstVpssGrpAttr,
    VPSS_CHN_ATTR_S *pastVpssChnAttr)
{
    VPSS_CHN VpssChn;
    SC_S32 s32Ret;
    SC_S32 j;

    s32Ret = SC_MPI_VPSS_CreateGrp(VpssGrp, pstVpssGrpAttr);

    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VPSS_CreateGrp(grp:%d) failed with %#x!\n", VpssGrp, s32Ret);
        return SC_FAILURE;
    }

    s32Ret = SC_MPI_VPSS_StartGrp(VpssGrp);

    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("SC_MPI_VPSS_StartGrp failed with %#x\n", s32Ret);
        return SC_FAILURE;
    }

    for (j = 0; j < VPSS_MAX_PHY_CHN_NUM; j++)
    {
        if(SC_TRUE == pabChnEnable[j])
        {
            VpssChn = j;
            s32Ret = SC_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &pastVpssChnAttr[VpssChn]);

            if (s32Ret != SC_SUCCESS)
            {
                SAMPLE_PRT("SC_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
                return SC_FAILURE;
            }

            s32Ret = SC_MPI_VPSS_EnableChn(VpssGrp, VpssChn);

            if (s32Ret != SC_SUCCESS)
            {
                SAMPLE_PRT("SC_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
                return SC_FAILURE;
            }
        }
    }

    return SC_SUCCESS;
}

/*****************************************************************************
* function : stop vpss grp
*****************************************************************************/
SC_S32 SAMPLE_COMM_VPSS_Stop(VPSS_GRP VpssGrp, SC_BOOL *pabChnEnable)
{
    SC_S32 j;
    SC_S32 s32Ret = SC_SUCCESS;
    VPSS_CHN VpssChn;

    for (j = 0; j < VPSS_MAX_PHY_CHN_NUM; j++)
    {
        if(SC_TRUE == pabChnEnable[j])
        {
            VpssChn = j;
            s32Ret = SC_MPI_VPSS_DisableChn(VpssGrp, VpssChn);

            if (s32Ret != SC_SUCCESS)
            {
                SAMPLE_PRT("failed with %#x!\n", s32Ret);
                return SC_FAILURE;
            }
        }
    }

    s32Ret = SC_MPI_VPSS_StopGrp(VpssGrp);

    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    s32Ret = SC_MPI_VPSS_DestroyGrp(VpssGrp);

    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_VPSS_SetRes(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, SIZE_S *pSize)
{
    SC_S32 s32Ret = SC_SUCCESS;
    VPSS_CHN_ATTR_S stChnAttr = {};

    s32Ret = SC_MPI_VPSS_GetChnAttr(VpssGrp, VpssChn, &stChnAttr);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    s32Ret = SC_MPI_VPSS_DisableChn(VpssGrp, VpssChn);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    stChnAttr.enChnMode = VPSS_CHN_MODE_USER;
    stChnAttr.u32Width = pSize->u32Width;
    stChnAttr.u32Height = pSize->u32Height;
    s32Ret = SC_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &stChnAttr);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    s32Ret = SC_MPI_VPSS_EnableChn(VpssGrp, VpssChn);
    if (s32Ret != SC_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return SC_FAILURE;
    }

    return SC_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
