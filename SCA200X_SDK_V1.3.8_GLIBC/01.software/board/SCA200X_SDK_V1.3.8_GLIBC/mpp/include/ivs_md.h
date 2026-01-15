#ifndef _IVS_MD_H_
#define _IVS_MD_H_

#include "sc_md.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

SC_S32 SC_IVS_MD_Init(SC_VOID);
SC_S32 SC_IVS_MD_Exit(SC_VOID);

SC_S32 SC_IVS_MD_CreateChn(MD_CHN MdChn, MD_ATTR_S *pstMdAttr);
SC_S32 SC_IVS_MD_DestroyChn(MD_CHN MdChn);

SC_S32 SC_IVS_MD_SetChnAttr(MD_CHN MdChn, MD_ATTR_S *pstMdAttr);
SC_S32 SC_IVS_MD_GetChnAttr(MD_CHN MdChn, MD_ATTR_S *pstMdAttr);
SC_S32 SC_IVS_MD_GetBg(MD_CHN MdChn, IVE_DST_IMAGE_S *pstBg);

SC_S32 SC_IVS_MD_Process(MD_CHN MdChn, IVE_SRC_IMAGE_S *pstCur,
    IVE_SRC_IMAGE_S *pstRef,
    IVE_DST_IMAGE_S *pstSad,
    IVE_DST_MEM_INFO_S *pstBlob);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* _IVS_MD_H_ */
