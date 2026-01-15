#ifndef __SAMPLE_VO_H__
#define __SAMPLE_VO_H__

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

void SAMPLE_VO_HandleSig(SC_S32 signo);
SC_VOID SAMPLE_VOU_SYS_Exit(void);

SC_S32 SAMPLE_VO_MIPILCD_1024x600(SC_VOID);
SC_S32 SAMPLE_VO_HDMI_1920x1080(SC_VOID);
SC_S32 SAMPLE_VI_VO(void);
SC_S32 SAMPLE_VO_MIPILCD_800x1280(SC_VOID);
SC_S32 SAMPLE_VI_MIPILCD_800x1280(void);
SC_S32 SAMPLE_VI_MIPILCD_480x640(void);
SC_S32 SAMPLE_VO_MIPILCD_480x640(SC_VOID);
SC_S32 SAMPLE_VI_MIPILCD_720x1280(void);
SC_S32 SAMPLE_VO_MIPILCD_720x1280(SC_VOID);
SC_S32 SAMPLE_VO_MIPIRGBLCD_720x1280(SC_VOID);
SC_S32 SAMPLE_VI_MIPIRGBLCD_720x1280(void);
SC_S32 SAMPLE_VI_MIPIRGB_HDMI_LOOP(void);

SC_S32 nv3052c_spilcd_init(void);

SC_VOID SAMPLE_VO_StartThread(SC_VOID);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __SAMPLE_VO_H__*/
