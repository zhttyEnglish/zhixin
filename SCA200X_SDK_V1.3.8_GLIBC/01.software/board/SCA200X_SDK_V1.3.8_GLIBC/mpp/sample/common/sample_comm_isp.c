
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
#include <sys/prctl.h>

#include "sample_comm.h"

#define MAX_SENSOR_NUM      5
#define ISP_MAX_DEV_NUM     6

#ifdef SENSOR_IMX307
extern ISP_SNS_OBJ_S stSnsImx307Obj;
#endif

#ifdef SENSOR_SC2310
extern ISP_SNS_OBJ_S stSnsSc2310Obj;
#endif

#ifdef SENSOR_SC2210
extern ISP_SNS_OBJ_S stSnsSc2210Obj;
#endif

#ifdef SENSOR_SC4210
extern ISP_SNS_OBJ_S stSnsSc4210Obj;
#endif

#ifdef SENSOR_IMX415
extern ISP_SNS_OBJ_S stSnsImx415Obj;
#endif

#ifdef SENSOR_GC2053
extern ISP_SNS_OBJ_S stSnsGc2053Obj;
#endif

#ifdef SENSOR_GC2093
extern ISP_SNS_OBJ_S stSnsGc2093Obj;
#endif

#ifdef SENSOR_GC1603
extern ISP_SNS_OBJ_S stSnsGc1603Obj;
#endif

#ifdef SENSOR_IMX291
extern ISP_SNS_OBJ_S stSnsImx291Obj;
#endif

#ifdef SENSOR_OV13B10
extern ISP_SNS_OBJ_S stSnsOv13b10Obj;
#endif

static pthread_t    g_IspPid[ISP_MAX_DEV_NUM] = {0};
static SC_U32       g_au32IspSnsId[ISP_MAX_DEV_NUM] = {0, 1};

SAMPLE_SNS_TYPE_E g_enSnsType[MAX_SENSOR_NUM] =
{
    SENSOR0_TYPE,
    SENSOR1_TYPE,
    SENSOR2_TYPE,
    SENSOR3_TYPE,
    SENSOR4_TYPE
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_IMX307_2M_30FPS =
{
    {0, 0, 1920, 1080},
    {1920, 1080},
    30,
    SC_BAYER_RGGB,
    WDR_MODE_NONE,
    0,
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_IMX307_MIPI_2M_30FPS_WDR2TO1_LINE =
{
    {4, 0, 1920, 1080},
    {1952, 2678},
    25,
    SC_BAYER_RGGB,
    WDR_MODE_2To1_LINE,
    0,
    {4, 0, 1924, 1080},
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_IMX291_2M_60FPS =
{
    {0, 0, 1920, 1080},
    {1920, 1080},
    60,
    SC_BAYER_RGGB,
    WDR_MODE_NONE,
    0,
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_IMX291_2M_120FPS =
{
    {0, 0, 1920, 1080},
    {1920, 1080},
    120,
    SC_BAYER_RGGB,
    WDR_MODE_NONE,
    0,
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_SC2310_2M_30FPS =
{
    {0, 0, 1920, 1080},
    {1920, 1080},
    30,
    SC_BAYER_BGGR,
    WDR_MODE_NONE,
    0,
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_SC2310_MIPI_2M_30FPS_WDR2TO1_LINE =
{
    {4, 4, 1920, 1080},
    {1920, 1080},
    25,
    SC_BAYER_BGGR,
    WDR_MODE_2To1_LINE,
    0,
    {4, 4, 1924, 1084},
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_SC2210_2M_30FPS =
{
    {0, 0, 1920, 1080},
    {1920, 1080},
    30,
    SC_BAYER_BGGR,
    WDR_MODE_NONE,
    0,
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_SC4210_4M_30FPS =
{
    {0, 0, 2560, 1440},
    {2560, 1440},
    30,
    SC_BAYER_BGGR,
    WDR_MODE_NONE,
    0,
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_IMX415_4K_30FPS =
{
    {12, 22, 3840, 2160},
    {3864, 2192},
    30,
    SC_BAYER_GBRG,
    WDR_MODE_NONE,
    0,
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_IMX415_4K_60FPS =
{
    {12, 22, 3840, 2160},
    {3864, 2192},
    60,
    SC_BAYER_GBRG,
    WDR_MODE_NONE,
    0,
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_IMX415_4K_30FPS_WDR2TO1_LINE =
{
    {12, 22, 3840, 2160},
    {3864, 2192},
    30,
    SC_BAYER_GBRG,
    WDR_MODE_2To1_LINE,
    0,
    {0, 0, 3840, 2160},
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_GC2053_MIPI_2M_30FPS =
{
    {0, 0, 1920, 1080},
    {1920, 1080},
    30,
    SC_BAYER_RGGB,
    WDR_MODE_NONE,
    0,
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_GC2093_MIPI_2M_30FPS =
{
    {0, 0, 1920, 1080},
    {1920, 1080},
    30,
    SC_BAYER_RGGB,
    WDR_MODE_NONE,
    0,
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_GC2093_MIPI_2M_30FPS_WDR2TO1_LINE =
{
    {0, 0, 1920, 1080},
    {1922, 1080},
    30,
    SC_BAYER_RGGB,
    WDR_MODE_2To1_LINE,
    1,
    {0, 0, 1920, 1080}
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_GC1603_MIPI_1M_60FPS =
{
    {0, 0, 1280, 720},
    {1280, 720},
    60,
    SC_BAYER_RGGB,
    WDR_MODE_NONE,
    0,
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_GC1603_MIPI_1M_120FPS =
{
    {0, 0, 1280, 720},
    {1280, 720},
    120,
    SC_BAYER_GBRG,
    WDR_MODE_NONE,
    0,
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_OV13B10_MIPI_13M_1FPS =
{
    {0, 0, 4096, 3120},
    {4096, 3120},
    1,
    SC_BAYER_BGGR,
    WDR_MODE_NONE,
    0,
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_OV13B10_MIPI_2M_30FPS =
{
    {0, 0, 1920, 1080},
    {1920, 1080},
    30,
    SC_BAYER_BGGR,
    WDR_MODE_NONE,
    0,
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_ISP_VIN_0_2M_30FPS =
{
    {0, 0, 1920, 1080},
    {1920, 1080},
    30,
    SC_BAYER_RGGB,
    WDR_MODE_NONE,
    0,
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_TP9930_1080P =
{
    .stWndRect = {0, 0, 1920, 1080},
    .stSnsSize = {1920, 1080},
    .f32FrameRate = 25,
    .enBayer = SC_BAYER_RGGB,
    .enWDRMode = WDR_MODE_NONE,
    .u8SnsMode = 0,
};

SC_S32 SAMPLE_COMM_ISP_GetIspAttrBySns(SAMPLE_SNS_TYPE_E enSnsType, ISP_PUB_ATTR_S *pstPubAttr)
{
    switch (enSnsType)
    {
    case SONY_IMX307_MIPI_2M_30FPS_12BIT:
    case SONY_IMX291_MIPI_2M_30FPS_12BIT:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_IMX307_2M_30FPS, sizeof(ISP_PUB_ATTR_S));
        break;

    case SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_IMX307_MIPI_2M_30FPS_WDR2TO1_LINE, sizeof(ISP_PUB_ATTR_S));
        break;

    case SONY_IMX291_MIPI_2M_60FPS_10BIT:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_IMX291_2M_60FPS, sizeof(ISP_PUB_ATTR_S));
        break;

    case SONY_IMX291_MIPI_2M_120FPS_10BIT:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_IMX291_2M_120FPS, sizeof(ISP_PUB_ATTR_S));
        break;

    case SMART_SC2310_MIPI_2M_30FPS_12BIT:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_SC2310_2M_30FPS, sizeof(ISP_PUB_ATTR_S));
        break;

    case SMART_SC2310_MIPI_2M_30FPS_12BIT_WDR2TO1:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_SC2310_MIPI_2M_30FPS_WDR2TO1_LINE, sizeof(ISP_PUB_ATTR_S));
        break;

    case SMART_SC2210_MIPI_2M_30FPS_12BIT:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_SC2210_2M_30FPS, sizeof(ISP_PUB_ATTR_S));
        break;

    case SMART_SC4210_MIPI_4M_30FPS_12BIT:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_SC4210_4M_30FPS, sizeof(ISP_PUB_ATTR_S));
        break;

    case SONY_IMX415_MIPI_8M_30FPS_12BIT:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_IMX415_4K_30FPS, sizeof(ISP_PUB_ATTR_S));
        break;

    case SONY_IMX415_MIPI_8M_60FPS_12BIT:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_IMX415_4K_60FPS, sizeof(ISP_PUB_ATTR_S));
        break;

    case SONY_IMX415_MIPI_8M_30FPS_12BIT_WDR2TO1:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_IMX415_4K_30FPS_WDR2TO1_LINE, sizeof(ISP_PUB_ATTR_S));
        break;

    case GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_GC2053_MIPI_2M_30FPS, sizeof(ISP_PUB_ATTR_S));
        break;

    case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_GC2093_MIPI_2M_30FPS, sizeof(ISP_PUB_ATTR_S));
        break;

    case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT_WDR2TO1:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_GC2093_MIPI_2M_30FPS_WDR2TO1_LINE, sizeof(ISP_PUB_ATTR_S));
        break;

    case GALAXYCORE_GC1603_MIPI_1M_60FPS_10BIT:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_GC1603_MIPI_1M_60FPS, sizeof(ISP_PUB_ATTR_S));
        break;

    case GALAXYCORE_GC1603_MIPI_1M_120FPS_10BIT:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_GC1603_MIPI_1M_120FPS, sizeof(ISP_PUB_ATTR_S));
        break;

    case OMNIVISION_OV13B10_MIPI_13M_1FPS_10BIT:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_OV13B10_MIPI_13M_1FPS, sizeof(ISP_PUB_ATTR_S));
        break;

    case OMNIVISION_OV13B10_MIPI_2M_30FPS_10BIT:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_OV13B10_MIPI_2M_30FPS, sizeof(ISP_PUB_ATTR_S));
        break;

    case ISP_VIN_0_MEM_2M_30FPS_12BIT:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_ISP_VIN_0_2M_30FPS, sizeof(ISP_PUB_ATTR_S));
        break;

    case TP9930_DVP_4VC_1080P_25FPS:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_TP9930_1080P, sizeof(ISP_PUB_ATTR_S));
        break;

    default:
        memcpy(pstPubAttr, &ISP_PUB_ATTR_IMX307_2M_30FPS, sizeof(ISP_PUB_ATTR_S));
        break;
    }

    return SC_SUCCESS;
}

ISP_SNS_OBJ_S *SAMPLE_COMM_ISP_GetSnsObj(SC_U32 u32SnsId)
{
    SAMPLE_SNS_TYPE_E enSnsType;

    enSnsType = g_enSnsType[u32SnsId];
    SAMPLE_PRT("Get sensor object, ID=%d\n", u32SnsId);

    switch (enSnsType)
    {
        #ifdef SENSOR_IMX307
    case SONY_IMX307_MIPI_2M_30FPS_12BIT:
    case SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1:
        return &stSnsImx307Obj;
        #endif

        #ifdef SENSOR_IMX291
    case SONY_IMX291_MIPI_2M_30FPS_12BIT:
    case SONY_IMX291_MIPI_2M_60FPS_10BIT:
    case SONY_IMX291_MIPI_2M_120FPS_10BIT:
        return &stSnsImx291Obj;
        #endif

        #ifdef SENSOR_SC2310
    case SMART_SC2310_MIPI_2M_30FPS_12BIT:
    case SMART_SC2310_MIPI_2M_30FPS_12BIT_WDR2TO1:
        return &stSnsSc2310Obj;
        #endif

        #ifdef SENSOR_SC2210
    case SMART_SC2210_MIPI_2M_30FPS_12BIT:
        return &stSnsSc2210Obj;
        #endif

        #ifdef SENSOR_SC4210
    case SMART_SC4210_MIPI_4M_30FPS_12BIT:
        return &stSnsSc4210Obj;
        #endif

        #ifdef SENSOR_IMX415
    case SONY_IMX415_MIPI_8M_30FPS_12BIT:
    case SONY_IMX415_MIPI_8M_60FPS_12BIT:
    case SONY_IMX415_MIPI_8M_30FPS_12BIT_WDR2TO1:
        return &stSnsImx415Obj;
        #endif

        #ifdef SENSOR_GC2053
    case GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT:
        return &stSnsGc2053Obj;
        #endif

        #ifdef SENSOR_GC2093
    case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT:
    case GALAXYCORE_GC2093_MIPI_2M_30FPS_10BIT_WDR2TO1:
        return &stSnsGc2093Obj;
        #endif

        #ifdef SENSOR_GC1603
    case GALAXYCORE_GC1603_MIPI_1M_60FPS_10BIT:
    case GALAXYCORE_GC1603_MIPI_1M_120FPS_10BIT:
        return &stSnsGc1603Obj;
        #endif

        #ifdef SENSOR_OV13B10
    case OMNIVISION_OV13B10_MIPI_13M_1FPS_10BIT:
    case OMNIVISION_OV13B10_MIPI_2M_30FPS_10BIT:
        return &stSnsOv13b10Obj;
        #endif

        #ifdef SENSOR_ISPIN
    case ISP_VIN_0_MEM_2M_30FPS_12BIT:
        return &stSnsIspIn0Obj;
        #endif

        #ifdef SENSOR_DVP
    case TP9930_DVP_4VC_1080P_25FPS:
        return &stSnsTp9930Obj;
        #endif

    default:
        return SC_NULL;
    }
}

static void *SAMPLE_COMM_ISP_Thread(void *param)
{
    SC_S32 s32Ret;
    ISP_DEV IspDev;
    SC_CHAR szThreadName[20];

    IspDev = (ISP_DEV)param;

    snprintf(szThreadName, 20, "ISP%d_RUN", IspDev);
    prctl(PR_SET_NAME, szThreadName, 0, 0, 0);

    SAMPLE_PRT("ISP Dev[%d] is running!\n", IspDev);
    s32Ret = SC_MPI_ISP_Run(IspDev);
    if (SC_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SC_MPI_ISP_Run failed with %#x!\n", s32Ret);
    }

    return NULL;
}

/******************************************************************************
* funciton : ISP init
******************************************************************************/
SC_S32 SAMPLE_COMM_ISP_Aelib_Callback(ISP_DEV IspDev)
{
    ALG_LIB_S stAeLib;

    stAeLib.s32Id = IspDev;
    strncpy(stAeLib.acLibName, SC_AE_LIB_NAME, sizeof(SC_AE_LIB_NAME));
    CHECK_RET(SC_MPI_AE_Register(IspDev, &stAeLib), "aelib register call back");
    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_ISP_Aelib_UnCallback(ISP_DEV IspDev)
{
    ALG_LIB_S stAeLib;

    stAeLib.s32Id = IspDev;
    strncpy(stAeLib.acLibName, SC_AE_LIB_NAME, sizeof(SC_AE_LIB_NAME));
    CHECK_RET(SC_MPI_AE_UnRegister(IspDev, &stAeLib), "aelib unregister call back");
    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_ISP_Awblib_Callback(ISP_DEV IspDev)
{
    ALG_LIB_S stAwbLib;

    stAwbLib.s32Id = IspDev;
    strncpy(stAwbLib.acLibName, SC_AWB_LIB_NAME, sizeof(SC_AWB_LIB_NAME));
    CHECK_RET(SC_MPI_AWB_Register(IspDev, &stAwbLib), "awblib register call back");
    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_ISP_Awblib_UnCallback(ISP_DEV IspDev)
{
    ALG_LIB_S stAwbLib;

    stAwbLib.s32Id = IspDev;
    strncpy(stAwbLib.acLibName, SC_AWB_LIB_NAME, sizeof(SC_AWB_LIB_NAME));
    CHECK_RET(SC_MPI_AWB_UnRegister(IspDev, &stAwbLib), "awblib unregister call back");
    return SC_SUCCESS;
}

/******************************************************************************
* funciton : ISP Run
******************************************************************************/
SC_S32 SAMPLE_COMM_ISP_Run(ISP_DEV IspDev)
{
    SC_S32 s32Ret = 0;

    pthread_attr_t *pstAttr = NULL;

    s32Ret = pthread_create(&g_IspPid[IspDev], pstAttr, SAMPLE_COMM_ISP_Thread, (SC_VOID *)IspDev);
    if (0 != s32Ret)
    {
        SAMPLE_PRT("Create isp running thread failed! error: %d, %s\r\n", s32Ret, strerror(s32Ret));
        goto out;
    }

out:
    if (NULL != pstAttr)
    {
        pthread_attr_destroy(pstAttr);
    }

    return s32Ret;
}

SC_S32 SAMPLE_COMM_ISP_Sensor_Regiter_callback(ISP_DEV IspDev, SC_U32 u32SnsId)
{
    ALG_LIB_S stAeLib;
    ALG_LIB_S stAwbLib;
    const ISP_SNS_OBJ_S *pstSnsObj;
    SC_S32    s32Ret = -1;

    if (MAX_SENSOR_NUM <= u32SnsId)
    {
        SAMPLE_PRT("invalid sensor id: %d\n", u32SnsId);
        return SC_FAILURE;
    }

    pstSnsObj = SAMPLE_COMM_ISP_GetSnsObj(u32SnsId);
    if (SC_NULL == pstSnsObj)
    {
        SAMPLE_PRT("sensor %d not exist!\n", u32SnsId);
        return SC_FAILURE;
    }

    stAeLib.s32Id  = IspDev;
    stAwbLib.s32Id = IspDev;
    strncpy(stAeLib.acLibName,  SC_AE_LIB_NAME,  sizeof(SC_AE_LIB_NAME));
    strncpy(stAwbLib.acLibName, SC_AWB_LIB_NAME, sizeof(SC_AWB_LIB_NAME));
    //strncpy(stAfLib.acLibName, SC_AF_LIB_NAME, sizeof(SC_AF_LIB_NAME));
    if (pstSnsObj->pfnRegisterCallback != SC_NULL)
    {
        s32Ret = pstSnsObj->pfnRegisterCallback(IspDev, &stAeLib, &stAwbLib);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("sensor_register_callback failed with %#x!\n", s32Ret);
            return s32Ret;
        }
    }
    else
    {
        SAMPLE_PRT("sensor_register_callback failed with SC_NULL!\n");
    }

    g_au32IspSnsId[IspDev] = u32SnsId;

    return SC_SUCCESS;
}

SC_S32 SAMPLE_COMM_ISP_Sensor_UnRegiter_callback(ISP_DEV IspDev)
{
    ALG_LIB_S stAeLib;
    ALG_LIB_S stAwbLib;
    SC_U32 u32SnsId;
    const ISP_SNS_OBJ_S *pstSnsObj;
    SC_S32    s32Ret = -1;

    u32SnsId = g_au32IspSnsId[IspDev];
    if (MAX_SENSOR_NUM <= u32SnsId)
    {
        SAMPLE_PRT("%s: invalid sensor id: %d\n", __FUNCTION__, u32SnsId);
        return SC_FAILURE;
    }

    pstSnsObj = SAMPLE_COMM_ISP_GetSnsObj(u32SnsId);
    if (SC_NULL == pstSnsObj)
    {
        return SC_FAILURE;
    }

    stAeLib.s32Id  = IspDev;
    stAwbLib.s32Id = IspDev;
    strncpy(stAeLib.acLibName, SC_AE_LIB_NAME, sizeof(SC_AE_LIB_NAME));
    strncpy(stAwbLib.acLibName, SC_AWB_LIB_NAME, sizeof(SC_AWB_LIB_NAME));
    //strncpy(stAfLib.acLibName, SC_AF_LIB_NAME, sizeof(SC_AF_LIB_NAME));

    if (pstSnsObj->pfnUnRegisterCallback != SC_NULL)
    {
        s32Ret = pstSnsObj->pfnUnRegisterCallback(IspDev, &stAeLib, &stAwbLib);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("sensor_unregister_callback failed with %#x!\n", s32Ret);
            return s32Ret;
        }
    }
    else
    {
        SAMPLE_PRT("sensor_unregister_callback failed with SC_NULL!\n");
    }

    return SC_SUCCESS;
}

/******************************************************************************
* funciton : stop ISP, and stop isp thread
******************************************************************************/
SC_VOID SAMPLE_COMM_ISP_Stop(ISP_DEV IspDev)
{
    if (g_IspPid[IspDev])
    {
        SC_MPI_ISP_Exit(IspDev);
        pthread_join(g_IspPid[IspDev], NULL);
        SAMPLE_COMM_ISP_Awblib_UnCallback(IspDev);
        SAMPLE_COMM_ISP_Aelib_UnCallback(IspDev);
        SAMPLE_COMM_ISP_Sensor_UnRegiter_callback(IspDev);
        g_IspPid[IspDev] = 0;
    }

    return;
}

SC_VOID SAMPLE_COMM_All_ISP_Stop(SC_VOID)
{
    ISP_DEV IspDev;

    for (IspDev = 0; IspDev < ISP_MAX_DEV_NUM; IspDev++)
    {
        SAMPLE_COMM_ISP_Stop(IspDev);
    }
}

static ISP_SNS_TYPE_E SAMPLE_COMM_GetSnsBusType(SAMPLE_SNS_TYPE_E enSnsType)
{
    ISP_SNS_TYPE_E enBusType;

    switch (enSnsType)
    {
    default:
        enBusType = ISP_SNS_I2C_TYPE;
        break;
    }

    return enBusType;
}

SC_S32 SAMPLE_COMM_ISP_BindSns(ISP_DEV IspDev, SC_U32 u32SnsId, SAMPLE_SNS_TYPE_E enSnsType, SC_S8 s8SnsDev)
{
    ISP_SNS_COMMBUS_U uSnsBusInfo;
    ISP_SNS_TYPE_E enBusType;
    const ISP_SNS_OBJ_S *pstSnsObj;
    SC_S32 s32Ret;

    if (MAX_SENSOR_NUM <= u32SnsId)
    {
        SAMPLE_PRT("invalid sensor id: %d\n", u32SnsId);
        return SC_FAILURE;
    }

    pstSnsObj = SAMPLE_COMM_ISP_GetSnsObj(u32SnsId);
    if (SC_NULL == pstSnsObj)
    {
        SAMPLE_PRT("sensor %d not exist!\n", u32SnsId);
        return SC_FAILURE;
    }

    enBusType = SAMPLE_COMM_GetSnsBusType(enSnsType);
    if (ISP_SNS_I2C_TYPE == enBusType)
    {
        uSnsBusInfo.s8I2cDev = s8SnsDev;
    }
    else
    {
        uSnsBusInfo.s8SspDev.bit4SspDev = s8SnsDev;
        uSnsBusInfo.s8SspDev.bit4SspCs  = 0;
    }

    if (SC_NULL != pstSnsObj->pfnSetBusInfo)
    {
        s32Ret = pstSnsObj->pfnSetBusInfo(IspDev, uSnsBusInfo);
        if (s32Ret != SC_SUCCESS)
        {
            SAMPLE_PRT("set sensor bus info failed with %#x!\n", s32Ret);
            return s32Ret;
        }
    }
    else
    {
        SAMPLE_PRT("not support set sensor bus info!\n");
        return SC_FAILURE;
    }

    return s32Ret;
}

SC_VOID SAMPLE_COMM_ISP_SetSnsType(SC_U32 u32SnsId, SAMPLE_SNS_TYPE_E enSnsType)
{
    g_enSnsType[u32SnsId] = enSnsType;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
