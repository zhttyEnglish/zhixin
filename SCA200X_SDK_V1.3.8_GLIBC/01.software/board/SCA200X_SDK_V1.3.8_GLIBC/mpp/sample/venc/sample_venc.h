#ifndef __SAMPLE_VENC_H__
#define __SAMPLE_VENC_H__

#include "sc_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef SAMPLE_PRT
#define SAMPLE_PRT(fmt...)  \
    do {    \
        printf("[%s]-%d: ", __FUNCTION__, __LINE__);    \
        printf(fmt);    \
    } while( 0)
#endif

void SAMPLE_VENC_HandleSig(SC_S32 signo);
SC_VOID SAMPLE_VENC_SYS_Exit(void);

SC_S32 SAMPLE_VENC_FILE_H264(void);
SC_S32 SAMPLE_VENC_H265(void);
SC_S32 SAMPLE_VENC_Jpeg(void);
SC_S32 SAMPLE_VENC_2CHN(void);
SC_S32 SAMPLE_VENC_MODPARAM(void);
SC_S32 SAMPLE_VENC_SET_CHN_ATTR(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
