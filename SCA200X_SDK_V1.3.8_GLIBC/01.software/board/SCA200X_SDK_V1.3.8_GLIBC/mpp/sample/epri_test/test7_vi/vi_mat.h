#ifndef __VI_MAT_H
#define __VI_MAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sample_comm.h"

void write_jpeg(int type, char *paddr, int len, SC_U64 pts, int id);

int vimat_getjpg(VIDEO_FRAME_INFO_S *pframe, char *poudata, int *pousize);


#ifdef __cplusplus
}

#endif /* __cplusplus */

#endif

