#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void*       SCV_HANDLE;
#define SCV_MAX_CHAR_LENGTH (256)

#define SCV_OK 0 ///< 正常运行

typedef int scv_errcode;
#define SCV_E_INVALIDARG -11 //< 参数错误
#define SCV_E_HANDLE -12     //< 句柄错误

typedef struct {
    unsigned int 	 stride[3];
    unsigned long    phy_addr[3];
    unsigned long    vir_addr[3];
} SCV_FRAME_ADDR_S;

typedef enum {
    SCV_IMAGE_SOURCE_ADDR = 0x00,  //有物理地址
    SCV_IMAGE_SOURCE_DATA = 0x01,  //只有虚拟地址
    SCV_IMAGE_SOURCE_MAX,
} SCV_IMAGE_SOURCE_TYPE_E;

typedef struct {
    SCV_IMAGE_SOURCE_TYPE_E addr_type; 
    union
    {
        SCV_FRAME_ADDR_S addr;
        unsigned char *data;
    };
} SCV_IMAGE_DATA_S;

typedef enum  {
    SCV_PIX_FMT_YUV420P,
    SCV_PIX_FMT_RGB,                           
    SCV_PIX_FMT_BGR, 
    SCV_PIX_FMT_GRAY,
    SCV_PIX_FMT_NONE,
} SCV_IMAGE_FORMAT_E;

typedef struct {
    int                             width;      // 宽度(以像素为单位)
    int                             height;	    // 高度(以像素为单位)
    SCV_IMAGE_FORMAT_E      	    type;	    // 像素格式
    SCV_IMAGE_DATA_S                image_data; // 图像格式
    size_t                          frame_id;   // 送入帧id,透传不使用
    struct timeval                  time_stamp; // 时间戳
    void 							*reserved;  // 内部保留, 请勿使用
    void 							*user_data; // 帧对应的用户输入数据
} SCV_FRAME_S;

typedef struct  
{
    int x;                                  
    int y;                                  
} SCV_POINT2i_S;

typedef struct  
{
    float x;                                  
    float y;                                  
} SCV_POINT2f_S;

typedef struct
{
    int left; 	// 矩形最左边的坐标
	int top; 	// 矩形最上边的坐标
	int right; 	// 矩形最右边的坐标
	int bottom; // 矩形最下边的坐标
}SCV_RECT_S;

#ifdef __cplusplus
}
#endif