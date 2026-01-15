#ifndef __DESKTOP_VO_H__
#define __DESKTOP_VO_H__

#include "sc_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

void DeskTopVO_HandleSig(SC_S32 signo);

SC_S32 DeskTopVO_Hdmi_1080p(SC_VOID);
SC_S32 DeskTopVO_Mipi_800x600(SC_VOID);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __DESKTOP_VO_H__*/
