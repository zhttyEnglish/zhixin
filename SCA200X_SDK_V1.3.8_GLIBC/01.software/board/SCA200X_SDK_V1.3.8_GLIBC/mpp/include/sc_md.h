#ifndef _SC_MD_H_
#define _SC_MD_H_

#include "sc_type.h"
#include "sc_common.h"
#include "sc_errno.h"
#include "sc_ive.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define MAX_MD_CHN  (64)

/*
* Definition md algorithm mode
*/
typedef enum scMD_ALG_MODE_E
{
    MD_ALG_MODE_BG = 0x0,  /* Base on background image */
    MD_ALG_MODE_REF = 0x1, /* Base on reference image */

    MD_ALG_MODE_BUTT
} MD_ALG_MODE_E;

typedef struct scMD_ATTR_S
{
    MD_ALG_MODE_E enAlgMode;         /* Md algorithm mode */
    IVE_SAD_MODE_E enSadMode;        /* Sad mode */
    IVE_SAD_OUT_CTRL_E enSadOutCtrl; /* Sad output ctrl */
    SC_U32              u32Width; /*Image width*/
    SC_U32              u32Height; /*Image height*/
    SC_U16 u16SadThr;                /* Sad thresh */
    IVE_CCL_CTRL_S stCclCtrl;        /* Ccl ctrl */
    IVE_ADD_CTRL_S stAddCtrl;        /* Add ctrl */
} MD_ATTR_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* _SC_MD_H_ */
