#pragma once 

#include <unistd.h>
#include "sc_comm_video.h"

#ifdef __cplusplus
    extern "C" {
#endif  //@__cplusplus

#define VDECCHN_MAX 4

typedef int (*vdec_frame_handler)(int chn, void*packet);

typedef enum
{
    VIDEO_720P,      /* 1280 * 720  */
    VIDEO_1080P,     /* 1920 * 1080 */
}RESOLUTION_E;


typedef struct
{
	vdec_frame_handler    frame_cb;
    size_t                max_channels;

    RESOLUTION_E          max_chn_reso[VDECCHN_MAX];
    PAYLOAD_TYPE_E         vdec_type[VDECCHN_MAX];
} VDEC_ATTRS_S;

typedef enum 
{
	VDEC_STATE_CLOSE = 0,
	VDEC_STATE_OPEN,
	VDEC_STATE_START,
	VDEC_STATE_STOP,
	VDEC_STATE_ERROR, //@when this state, need reset
}VDEC_STATE_E;

typedef struct
{
    unsigned long long  pts;
	unsigned char *data;
	int      len;
}VDEC_DATA_S;

typedef struct 
{
    int            vpssGrp;
    int            vpssChn;
    VIDEO_FRAME_INFO_S  frame;
}FRAME_INFO_S;


//最大1080P h264/5
int mpp_vdec_init(VDEC_ATTRS_S *attrs);
int mpp_vdec_deinit(void);
int mpp_vdec_open(int chn);
int mpp_vdec_close(int chn);
int mpp_vdec_start(int chn);
int mpp_vdec_stop(int chn);
int mpp_vdec_set(int chn, PAYLOAD_TYPE_E type);
int mpp_vdec_send_stream(int chn, VDEC_DATA_S *vdec_data);
int mpp_vdec_release_frame(FRAME_INFO_S *frame_info);
int mpp_vdec_error_check(int chn);
#ifdef __cplusplus
    }
#endif //@__cplusplus
    

