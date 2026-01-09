
#ifndef _APP_RTSP_CLIENT_H
#define _APP_RTSP_CLIENT_H


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define RTSP_OK                     (0)
#define RTSP_FAIL                   (-1)
#define RTSP_TRUE                   (1)
#define RTSP_FALSE                  (0)

#define APP_OK (0)
#define APP_FAIL (-1)

typedef enum
{
    HAL_VIDEO_ENCODE_H264 = 0,
    HAL_VIDEO_ENCODE_MJPEG = 1,
    HAL_VIDEO_ENCODE_H265 = 2,
    HAL_VIDEO_ENCODE_RAW = 3,
}VENC_TYPE_E;

typedef struct
{
    void (*  recv_video_cb)(int chn, int recv_type, unsigned char* recv_buf, unsigned int recv_frame_size, 
            unsigned long long recv_pts, int encoder_type, int width, int height);
    void (*  stop_video_cb)(int chn);
    void *  recv_audio_cb;
}APP_RTSP_CLEINT_INIT_INFO_S;

typedef struct
{
    int     encode_type;  
    int     data_size;
    char    *data_ptr;
}APP_RTSP_CLEINT_AUDIO_S;

//VENC_TYPE_E encoder_type
typedef void(*app_rtsp_receive_handler)(int chn, int recv_type, unsigned char* recv_buf, unsigned int recv_frame_size, 
            unsigned long long recv_pts, int encoder_type, int width, int height);
typedef void(*app_rtsp_recv_audio_handler)(int chn, APP_RTSP_CLEINT_AUDIO_S *audio_info);
typedef void(*app_rtsp_stream_stop_handler)(int chn);

int app_rtsp_init(APP_RTSP_CLEINT_INIT_INFO_S input);
int app_rtsp_deinit();
int app_rtsp_client_start(int chn, const char *url);
int app_rtsp_client_stop(int chn);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif

