#ifndef __SAMPLE_VDEC_H__
#define __SAMPLE_VDEC_H__

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

SC_VOID SAMPLE_VDEC_HandleSig(SC_S32 signo);

SC_S32 SAMPLE_H265_VDEC_VPSS_VO(SC_VOID);
SC_S32 SAMPLE_H265_VDEC(SC_VOID);
SC_S32 SAMPLE_H264_VDEC(SC_VOID);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
