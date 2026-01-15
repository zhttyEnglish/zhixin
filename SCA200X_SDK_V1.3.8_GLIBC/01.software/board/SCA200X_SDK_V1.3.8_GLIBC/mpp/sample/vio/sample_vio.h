#ifndef __SAMPLE_VIO_H__
#define __SAMPLE_VIO_H__

#include "sc_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef SAMPLE_PRT
#define SAMPLE_PRT(fmt...)   \
    do {\
        printf("[%s]-%d: ", __FUNCTION__, __LINE__);\
        printf(fmt);\
    }while(0)
#endif

#ifndef PAUSE
#define PAUSE()  do {\
        printf("---------------press Enter key to exit!---------------\n");\
        getchar();\
    } while (0)
#endif

SC_VOID SAMPLE_VIO_MsgInit(SC_VOID);
SC_VOID SAMPLE_VIO_MsgExit(SC_VOID);

void SAMPLE_VIO_HandleSig(SC_S32 signo);

SC_S32 SAMPLE_VIO_ViOnlineVpssOfflineRoute(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_ViOfflineVpssOfflineRoute(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_ViDoubleChnRoute(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_ViRotate(SC_U32 u32VoIntfType);

SC_S32 SAMPLE_VIO_ViResoSwitch(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_ViDoubleWdrPipe(SC_U32 u32VoIntfType);
sc_s32 SAMPLE_VIO_ViDeMuxYuv(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_ViSetUsrPic(SC_U32 u32VoIntfType);

SC_S32 SAMPLE_VIO_VioViChn1ResoSwitch(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_ViCrop(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_ViRaw(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_ViOsd(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_ViCover(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_ViMosaic(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_Vi2Sensor(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_ViHdrSwitch(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_Vi2Chn(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_ViOpenCloseRaw(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_ViGetBgr(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_ViAlgRect(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_Vi4Sensor(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_ViSensorSwitch(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_ViOnlineVencSnap(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_Vi3Sensor_415d307(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_Vi2SensorVenc(SC_U32 u32VoIntfType);
SC_S32 SAMPLE_VIO_MIPIRGB_HDMI_LOOP(void);
SC_S32 SAMPLE_VIO_IMX415(SC_U32 u32VoIntfType);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __SAMPLE_VIO_H__*/
