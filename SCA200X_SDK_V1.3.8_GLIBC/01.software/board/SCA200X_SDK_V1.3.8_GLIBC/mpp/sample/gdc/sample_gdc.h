#ifndef __SAMPLE_GDC_H__
#define __SAMPLE_GDC_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


    
typedef enum
{
    SC_IMG_YUV444P = 0,
    SC_IMG_YUV444SP = 1,
    SC_IMG_YUV422P = 2,
    SC_IMG_YUV422SP = 3,
    SC_IMG_YUV420P = 4,
    SC_IMG_YUV420SP = 5,
    SC_IMG_YUVI420 = 6, //YU12
    SC_IMG_YV12 = 7,
    SC_IMG_NV12 = 8,
    SC_IMG_NV21 = 9,
    SC_IMG_RGB = 10,
    SC_IMG_BGR = 11,
    SC_IMG_RGBD = 12,
    SC_IMG_RGB_INTLV = 13, //Not for src file.
    SC_IMG_BGR_INTLV = 14, //Not for src file.
    SC_IMG_GRAY = 15,
    SC_IMG_HWC_FLOAT_DATA = 16,
    SC_IMG_HWC_FIX_DATA = 17,
    SC_IMG_CHW_FLOAT_DATA = 18,
    SC_IMG_CHW_FIX_DATA = 19,
    SC_IMG_MAX
} SC_IMG_FORMAT_E;

typedef struct
{
    SC_U64 uptrAddrVirt;
    SC_U32 u32AddrPhy;
    SC_U32 u32Stride;
} SC_IMG_CHANNEL_S;

typedef struct
{
    SC_U32 u32FrameId;
    SC_IMG_FORMAT_E enFormat;
    SC_U32 u32Width;
    SC_U32 u32Height;
    SC_U32 u32ChannelNum;
    SC_U32 u32DataLen;
    SC_IMG_CHANNEL_S astChannels[4];
} SC_IMG_S;






#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __SAMPLE_VO_H__*/
