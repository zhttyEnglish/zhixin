#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <vector>
#include "opencv2/opencv.hpp"


using namespace std;
using namespace cv;

#ifdef __cplusplus
extern "C" {
#endif

#include "mpi_sys.h"

void write_jpeg(int type, char *paddr, int len, SC_U64 pts, int id)
{
    char filename[128];
    memset(filename, 0, sizeof(filename));
    if (0 == type)
    {
        sprintf(filename, "/mnt/srcdata_write/vi_[%d]_%llu.yuv", id, pts);
    }
    else
    {
        sprintf(filename, "/mnt/srcdata_write/vi_[%d]_%llu.jpg", id, pts);
    }


    FILE *fp;
    fp = fopen(filename, "wb+");
    if (fp == NULL)
    {
    	printf("fp fopen error %d", errno);
        return;
    }
    int writetrycount = 1000;
    int writenum = 0;
    int ret = 0;
    while (writetrycount-- > 0)
    {
        ret = fwrite((char *)(paddr+writenum), 1, (len-writenum), fp);
        if (ret >= 0)
        {
            writenum += ret;
            if (writenum >= len)
            {
                break;
            }
        }
    }
    fclose(fp);

    printf("writejpeg->filename:%s, paddr:%p len:%d, ret:%d\n",
        filename, paddr, len, ret);

    return;
}

static void CopyYUVToMat(char *dst, char *pY, char *pU, char *pV,
    int width, int height, int *stride)
{
    int h, offset = 0;
    char *pviradr;

    for (h = 0; h < height; h++)
    {
        pviradr = pY + h * stride[0];
        memcpy(dst+offset, pviradr, width);
        offset += width;
    }

    for (h = 0; h < height/2; h++)
    {
        pviradr = pU + h * stride[1];
        memcpy(dst+offset, pviradr, width/2);
        offset += width/2;
    }

    for (h = 0; h < height/2; h++)
    {
        pviradr = pV + h * stride[2];
        memcpy(dst+offset, pviradr, width/2);
        offset += width/2;
    }
}

int vimat_getjpg(VIDEO_FRAME_INFO_S *pframe, char *poudata, int *pousize)
{
    int res;
    char *pVBufVirt_Y, *pVBufVirt_U, *pVBufVirt_V;
    SC_U32 u32YSize, u32CSize, u32UvHeight;
    int stride[3];

    Mat yuvimg;
    yuvimg.create( pframe->stVFrame.u32Height * 3 / 2, pframe->stVFrame.u32Width, CV_8UC1);

    printf("enPixelFormat:%d wh:%d-%d\n",
        pframe->stVFrame.enPixelFormat, pframe->stVFrame.u32Width,
        pframe->stVFrame.u32Height);

    if (PIXEL_FORMAT_YVU_PLANAR_420 != pframe->stVFrame.enPixelFormat)
    {
        fprintf(stderr, "no support video format(%d)\n", pframe->stVFrame.enPixelFormat);
        return SC_FAILURE;
    }

    u32YSize = (pframe->stVFrame.u32Stride[0]) * (pframe->stVFrame.u32Height);
    u32CSize = (pframe->stVFrame.u32Stride[1]) * (pframe->stVFrame.u32Height) / 2;
    u32UvHeight = pframe->stVFrame.u32Height / 2;

    pVBufVirt_Y = (SC_CHAR *) SC_MPI_SYS_Mmap(pframe->stVFrame.u64PhyAddr[0], u32YSize);
    if (NULL == pVBufVirt_Y)
    {
        fprintf(stderr, "vimat_getjpg y map err!\n");
        return SC_FAILURE;
    }

    pVBufVirt_U = (SC_CHAR *) SC_MPI_SYS_Mmap(pframe->stVFrame.u64PhyAddr[1], u32CSize);
    if (NULL == pVBufVirt_U)
    {
        fprintf(stderr, "vimat_getjpg u map err!\n");
        return SC_FAILURE;
    }

    pVBufVirt_V = (SC_CHAR *) SC_MPI_SYS_Mmap(pframe->stVFrame.u64PhyAddr[2], u32CSize);
    if (NULL == pVBufVirt_V)
    {
        fprintf(stderr, "vimat_getjpg v map err!\n");
        return SC_FAILURE;
    }
    stride[0] = pframe->stVFrame.u32Stride[0];
    stride[1] = pframe->stVFrame.u32Stride[1];
    stride[2] = pframe->stVFrame.u32Stride[2];

    CopyYUVToMat((char *)yuvimg.data, pVBufVirt_Y, pVBufVirt_U, pVBufVirt_V,
        pframe->stVFrame.u32Width, pframe->stVFrame.u32Height, stride);

    SC_MPI_SYS_Munmap(pVBufVirt_Y, u32YSize);
    SC_MPI_SYS_Munmap(pVBufVirt_U, u32CSize);
    SC_MPI_SYS_Munmap(pVBufVirt_V, u32CSize);


    cv::Mat rgbimg;
    cv::cvtColor(yuvimg, rgbimg, CV_YUV2BGR_I420);

    cv::Mat sample_resized;

#if 1
    cv::Size ousize = cv::Size(4000, 3000);
    cv::resize(rgbimg, sample_resized, ousize);
#else
    sample_resized = rgbimg;
#endif

    std::vector<unsigned char>buff;
    std::vector<int> param = std::vector<int>(2);
    param[0] = CV_IMWRITE_JPEG_QUALITY;
    param[1] = 95; // default(95) 0-100
    cv::imencode(".jpg", sample_resized, buff, param);

    *pousize = buff.size();
    memcpy(poudata, buff.data(), *pousize);

    printf("vimat_getjpg->*pousize(%d)\n", *pousize);

    return SC_SUCCESS;
}

#ifdef __cplusplus
}
#endif

