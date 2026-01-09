
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "app_rtsp_client.h"
#include "hal_vdec.h"
#include "sc_buffer.h"
#include "signal.h"
#include "mpi_venc.h"
#include "mpi_ive.h"

#include "scv_common.h"
#include "config_read.h"
#include "sc_comm_venc.h"
#include "mpi_sys.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "sample_npu.h"


#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp> 
using namespace std;
using namespace cv;


#define HDR_MAGIC   0xFCFCFCFC

typedef struct {
    unsigned int magic;      // 0xFCFCFCFC
    char  version;    // 0x01
    unsigned int body_len;   // payload 长度（高字节在前）
} __attribute__((packed)) Header;

typedef struct {
    unsigned short result_path_len;
    unsigned short image_path_len;
} __attribute__((packed)) PayloadHeader;

#ifndef min
#  define min(a, b) ((a) < (b) ? (a) : (b))
#endif

typedef struct  
{
	unsigned int	in;
	unsigned int	out;
	unsigned int	mask;
	unsigned int	esize;
	void		    *data;
}SC_FIFO;

typedef struct  
{
    int chn;
    int encoder_type;
    int frame_type;
    int width;
    int height;
    int size;
}VIDEO_INFO;

#define MAX_CHN_NUM (4)
#define MAX_BUF_SIZE (8*1024*1024)
SC_FIFO video_fifo[MAX_CHN_NUM];
int g_open_chn[MAX_CHN_NUM] = {0};
size_t g_frame_count[MAX_CHN_NUM] = {0};
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;
size_t f_id=1;
size_t num[4] = {0};

Config g_config;

SAMPLE_SVP_NPU_MODEL_S yolov8Model;
SAMPLE_SVP_NPU_PARAM_S yolov8Param;

const char *result_path_json = "/userdata/result";
const char *image_path_jpg = "/userdata/image";

VDEC_ATTRS_S attrs;

template <typename T> 
class SafeQueue {
    std::queue<T> q;
    std::mutex mtx;
    std::condition_variable cv;
    
public:
    void push(T value) {
        std::lock_guard<std::mutex> lock(mtx);
        q.push(std::move(value));
        cv.notify_one();
    }
    
    T wait_and_pop() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]{ return !q.empty(); });
        T value = std::move(q.front());
        q.pop();
        return value;
    }
};

SafeQueue<FRAME_INFO_S *> dishes;

int g_send_fd=0;
int re_connect_running=0;
int sc_kfifo_init(SC_FIFO *fifo, void *buffer,unsigned int size, size_t esize)
{
    if(size != 0 && (size & (size - 1)) == 0)
    {
        size /= esize;

        fifo->in = 0;
        fifo->out = 0;
        fifo->esize = esize;
        fifo->data = buffer;

        if (size < 2) {
            fifo->mask = 0;
            return -1;
        }
        fifo->mask = size - 1;

        return 0;
    }
    else
    {
        printf("size %u is not a power of 2",size);
        return -1;
    }
	
    return 0;
}

static inline unsigned int kfifo_unused(SC_FIFO *fifo)
{
	return (fifo->mask + 1) - (fifo->in - fifo->out);
}

static void kfifo_copy_in(SC_FIFO *fifo, const void *src,unsigned int len, unsigned int off)
{
	unsigned int size = fifo->mask + 1;
	unsigned int esize = fifo->esize;
	unsigned int l;

	off &= fifo->mask;
	if (esize != 1) 
    {
		off *= esize;
		size *= esize;
		len *= esize;
	}
	l = min(len, size - off);

	memcpy(fifo->data + off, src, l);
	memcpy(fifo->data, src + l, len - l);
}

unsigned int sc_kfifo_in(SC_FIFO *fifo,const void *buf, unsigned int len)
{
	unsigned int l;

	l = kfifo_unused(fifo);
	if (len > l)
    {
        printf("fifo is no space len %u left",len,l);
        return -1;
    }
		
	kfifo_copy_in(fifo, buf, len, fifo->in);
	fifo->in += len;
	return len;
}

static void kfifo_copy_out(SC_FIFO *fifo, void *dst,unsigned int len, unsigned int off)
{
	unsigned int size = fifo->mask + 1;
	unsigned int esize = fifo->esize;
	unsigned int l;

	off &= fifo->mask;
	if (esize != 1) 
    {
		off *= esize;
		size *= esize;
		len *= esize;
	}
	l = min(len, size - off);

	memcpy(dst, fifo->data + off, l);
	memcpy(dst + l, fifo->data, len - l);
}

unsigned int __kfifo_out_peek(SC_FIFO *fifo,void *buf, unsigned int len)
{
	unsigned int l;

	l = fifo->in - fifo->out;
	// if (len > l)
	// 	len = l;

    if (len > l)
    {
        return 0;
    }
    
	kfifo_copy_out(fifo, buf, len, fifo->out);
	return len;
}

unsigned int sc_kfifo_out(SC_FIFO *fifo,void *buf, unsigned int len)
{
	len = __kfifo_out_peek(fifo, buf, len);
	fifo->out += len;
	return len;
}

void app_stream_rtsp_recv_handler(int chn,int recv_type, unsigned char* recv_buf, 
					unsigned int recv_frame_size, unsigned long long recv_pts, int encoder_type, int width, int height)
{
    int ret;
 //   printf("chn %d encoder_type %d recv_type %d recv_frame_size %d width %d height %d recv_pts %llu\n",
//               chn,encoder_type,recv_type,recv_frame_size,width,height,recv_pts);

    VIDEO_INFO video_info;
    video_info.chn = chn;
    video_info.encoder_type = encoder_type;
    video_info.frame_type = recv_type;
    video_info.width = width;
    video_info.height = height;
    video_info.size = recv_frame_size;

    unsigned int l = kfifo_unused(&video_fifo[chn]);

    if(l < recv_frame_size+sizeof(VIDEO_INFO))
    {
        printf("fifo %d is no space left %u\n",chn,l);
        return ;
    }

    ret = sc_kfifo_in(&video_fifo[chn],&video_info,sizeof(VIDEO_INFO));
    if(ret == sizeof(VIDEO_INFO))
    {

    }
    else
    {
        printf("sc_kfifo_in error ret %d\n",ret);
    }

    ret = sc_kfifo_in(&video_fifo[chn],recv_buf,recv_frame_size);
    if(ret == recv_frame_size)
    {

    }
    else
    {
        printf("sc_kfifo_in error ret %d\n",ret);
    }
   
    g_frame_count[chn]++;

    return ;
}

void app_stream_rtsp_stop_handler(int chn)
{
     return ;
}

void *test_save(void *param)
{
    int *p = (int *)param;
    int chn = *p;
    int ret;

    printf("start save chn %d\n",chn);

    VIDEO_INFO *video_info=(VIDEO_INFO*)malloc(sizeof(VIDEO_INFO));
    
    while(1)
    {
        ret = sc_kfifo_out(&video_fifo[chn],video_info,sizeof(VIDEO_INFO));
        if(!ret)
        {
            usleep(10*1000);
            continue;
        }
        else 
        {
            //printf("get fifo %d %d %d %d %u\n",video_info->chn,video_info->encoder_type,video_info->width,video_info->height,video_info->size);
        }

        unsigned char *buf = (unsigned char *)malloc(video_info->size);
        while(1)
        {
            ret = sc_kfifo_out(&video_fifo[chn],buf,video_info->size);
            if(!ret)
            {
                usleep(10*1000);
            }
            else
            {
                break;
            }
        }

        if(video_info->encoder_type == HAL_VIDEO_ENCODE_H264)
        {
            if(attrs.vdec_type[chn] != PT_H264)
            {
                ret = mpp_vdec_set(chn,PT_H264);
                if(ret)
                {
                    printf("mpp_vdec_set error\n");
                    return NULL;
                }
                else
                {
                    printf("mpp_vdec_set OK PT_H264 chn %d\n",chn);
                    attrs.vdec_type[chn] = PT_H264;
                }
            }
        }
        else if(video_info->encoder_type == HAL_VIDEO_ENCODE_H265)
        {
            if(attrs.vdec_type[chn] != PT_H265)
            {
                ret = mpp_vdec_set(chn,PT_H265);
                if(ret)
                {
                    printf("mpp_vdec_set error\n");
                    return NULL;
                }
                else
                {
                    printf("mpp_vdec_set OK PT_H265 chn %d\n",chn);
                    attrs.vdec_type[chn] = PT_H265;
                }
            }
        }

        VDEC_DATA_S vdec_data;

        vdec_data.data = buf;
        vdec_data.len = video_info->size;
        vdec_data.pts = 0;
        static int stop_flag = 0;
    retry:
        if(!stop_flag)
        {
            ret = mpp_vdec_send_stream(chn,&vdec_data);
            if(ret)
            {
                printf("mpp_vdec_send_stream failed chn %d\n",chn);
            }

            free(buf);

            ret = mpp_vdec_error_check(chn);
            if(ret == 11)
            {
                printf("mpp_vdec_error_check error chn %d neet restart vdec\n",chn);
                mpp_vdec_stop(chn);
                mpp_vdec_close(chn);
                stop_flag = 1;
            }
        }
        else
        {
            if(video_info->frame_type != 1)
            {
                free(buf);
            }
            else if(video_info->frame_type == 1)
            {
                mpp_vdec_open(chn);
                mpp_vdec_start(chn);
                stop_flag = 0;
                goto retry;
            }
        }
    }

    free(video_info);
}

SC_S32 SAMPLE_COMM_VENC_SaveStream(FILE *pFd, VENC_STREAM_S *pstStream)
{
    SC_S32 i;
    int ret;
    
    for (i = 0; i < pstStream->u32PackCount; i++)
    {
        ret = fwrite(pstStream->pstPack[i].pu8Addr + pstStream->pstPack[i].u32Offset,
                1, pstStream->pstPack[i].u32Len - pstStream->pstPack[i].u32Offset, pFd);
        fflush(pFd);
        fdatasync(fileno(pFd));
    }

    return SC_SUCCESS;
}

int net_send_result(char *jpg_name_path,char *json_name_path)
{
	unsigned short end_falg = 0xFDFD;
	
    char result_path[512]; //json
    memset(result_path,0,512);
    strcpy(result_path,json_name_path);

    char image_path[512];
    memset(image_path,0,512);
    strcpy(image_path,jpg_name_path);

    PayloadHeader ph;
	ph.result_path_len = ntohs(strlen(result_path));
	ph.image_path_len = ntohs(strlen(image_path));
	
	Header he;
	
	he.magic = 0xFCFCFCFC;
	he.version = 0x01;
	he.body_len = htonl(sizeof(PayloadHeader)+strlen(result_path)+strlen(image_path)+2);

	int ph_len = strlen(result_path)+strlen(image_path)+2;

	char *buf = (char *)malloc(sizeof(Header)+sizeof(PayloadHeader)+ph_len);
	
	memcpy(buf,&he,sizeof(he));
	
	memcpy(buf+sizeof(he),&ph,sizeof(ph));
	char *p = buf+sizeof(he)+sizeof(ph);
	memcpy(p,result_path,strlen(result_path));
	p+=strlen(result_path);
	memcpy(p,image_path,strlen(image_path));
	p+=strlen(image_path);
	memcpy(p,&end_falg,2);
	
	int ret = write(g_send_fd,buf,sizeof(Header)+sizeof(PayloadHeader)+ph_len);
    if(ret != sizeof(Header)+sizeof(PayloadHeader)+ph_len)
    {
        perror("write");
        printf("net_send_result write error\n");
        return -1;
    }
	printf("net_send_result success g_send_fd = %d !\n");
//	for(int i = 0; i < sizeof(Header)+sizeof(PayloadHeader)+ph_len;i ++)
//	{
//		printf("%02x ", buf[i]);
//		if(15 == i % 16){
//			printf("\r\n");
//		}
//	}
//	printf("\r\n");
    return 0;
}

int draw_text(char *image_path)
{
	char text[256] = {0};
	cv::Mat img = cv::imread(image_path);
	
	// 文本内容
    time_t timep;
    struct tm *p;
    time (&timep);
    p = gmtime(&timep);
    sprintf(text,"Alarm Time: %04d-%02d-%02d %02d:%02d:%02d",1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, 8 + p->tm_hour, p->tm_min, p->tm_sec);

    // 文本位置
    cv::Point org(50, 150);

    // 字体类型
    int fontFace = FONT_HERSHEY_SIMPLEX;

    // 字体缩放
    double fontScale = 1.5;

    // 颜色 (红色) 
    cv::Scalar color(0, 0, 255);

    // 线条粗细
    int thickness = 2;

    // 绘制文本 putText(canvas,"(50,300)",Point(50,300),1,1,Scalar(0));
    cv::putText(img, text, org, fontFace, fontScale, color, thickness);

	cv::imwrite(image_path, img);
    return 0;
}

int save_jpg(FRAME_INFO_S *frame_info,cJSON *root)
{
    pthread_mutex_lock(&result_mutex);

    int vecn_chn = 0;

    int VencFd =0;

    SC_S32 s32Ret = SC_FAILURE;
    s32Ret = SC_MPI_VENC_SendFrame(vecn_chn,&frame_info->frame,100);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SC_MPI_VENC_SendFrame failed vecn_chn %d\n",vecn_chn);
        goto EXIT;
    }

    VencFd = SC_MPI_VENC_GetFd(vecn_chn);
    if(VencFd < 0)
    {
        printf("SC_MPI_VENC_GetFd failed vecn_chn %d\n",vecn_chn);
        goto EXIT;
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(VencFd, &read_fds);
    
    struct timeval TimeoutVal;
    TimeoutVal.tv_sec  = 2;
    TimeoutVal.tv_usec = 0;//1000 * 200;
    s32Ret = select(VencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
    if (s32Ret < 0)
    {
        printf("select failed! vecn_chn %d\n",vecn_chn);
        goto EXIT;
    }
    else if (s32Ret == 0)
    {
        printf("get venc stream time out, exit vecn_chn %d\n",vecn_chn);
        goto EXIT;
    }
    else
    {
        VENC_STREAM_S stStream;
        VENC_CHN_STATUS_S stStat;
        if (FD_ISSET(VencFd, &read_fds))
        {
            /*******************************************************
             step 2.1 : query how many packs in one-frame stream.
            *******************************************************/
            memset(&stStream, 0, sizeof(stStream));

            s32Ret = SC_MPI_VENC_QueryStatus(0, &stStat);
            if (SC_SUCCESS != s32Ret)
            {
                printf("SC_MPI_VENC_QueryStatus chn[%d] failed with %#x!\n", vecn_chn, s32Ret);
                goto EXIT;
            }

            /*******************************************************
            step 2.2 :suggest to check both u32CurPacks and u32LeftStreamFrames at the same time,for example:
                if(0 == stStat.u32CurPacks || 0 == stStat.u32LeftStreamFrames)
                {
                SAMPLE_PRT("NOTE: Current  frame is NULL!\n");
                continue;
                }
            *******************************************************/
            if(0 == stStat.u32CurPacks)
            {
                printf("NOTE: vecn_chn %d Current  frame is NULL!\n", vecn_chn);
                goto EXIT;
            }

            /*******************************************************
             step 2.3 : malloc corresponding number of pack nodes.
            *******************************************************/
            stStream.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
            if (NULL == stStream.pstPack)
            {
                printf("malloc stream pack failed! vecn_chn %d \n",vecn_chn);
                goto EXIT;
            }

            /*******************************************************
             step 2.4 : call mpi to get one-frame stream
            *******************************************************/
            stStream.u32PackCount = stStat.u32CurPacks;
            s32Ret = SC_MPI_VENC_GetStream(vecn_chn, &stStream, SC_TRUE);
            if (SC_SUCCESS != s32Ret)
            {
                free(stStream.pstPack);
                stStream.pstPack = NULL;
                printf("SC_MPI_VENC_GetStream failed with %#x! vecn_chn %d\n", s32Ret,vecn_chn);
                goto EXIT;
            }

            struct timeval tv;
            gettimeofday(&tv, NULL);
            long long ms = (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
            
            char file_name[256];
            memset(file_name, 0, sizeof(file_name));
            sprintf(file_name, "%s/cam%d_%lld_image.jpg", image_path_jpg,frame_info->vpssGrp, ms);

            char json_name[256];
            memset(json_name, 0, sizeof(json_name));
            sprintf(json_name, "%s/cam%d_%lld_image.json", result_path_json,frame_info->vpssGrp, ms);

            FILE *fp = fopen(file_name, "w+");

            /*******************************************************
             step 2.5 : save frame to file
            *******************************************************/
            s32Ret = SAMPLE_COMM_VENC_SaveStream(fp, &stStream);
            if (SC_SUCCESS != s32Ret)
            {
                free(stStream.pstPack);
                stStream.pstPack = NULL;
                printf("vecn_chn %d save stream failed!\n",vecn_chn);
                fclose(fp);
                goto EXIT;;
            }

            fclose(fp);

            FILE *file = fopen(json_name, "w");
            char *jsonString = cJSON_Print(root);
            fprintf(file, "%s\n", jsonString);
            fclose(file);
            free(jsonString);
			
			// to do add osd to image
			draw_text(file_name);
			//printf("=========== draw_text done ================\r\n");

            s32Ret = net_send_result(file_name,json_name);
            if(s32Ret)
            {
                printf("send result failed!\n");
            }

            /*******************************************************
             step 2.6 : release stream
                *******************************************************/
            s32Ret = SC_MPI_VENC_ReleaseStream(vecn_chn, &stStream);
            if (SC_SUCCESS != s32Ret)
            {
                printf("SC_MPI_VENC_ReleaseStream failed! vecn_chn %d\n",vecn_chn);
                free(stStream.pstPack);
                stStream.pstPack = NULL;
                goto EXIT;;
            }

            /*******************************************************
             step 2.7 : free pack nodes
            *******************************************************/
            free(stStream.pstPack);
            stStream.pstPack = NULL;
        }
    }

EXIT:
    pthread_mutex_unlock(&result_mutex);
    return s32Ret;
}

static SC_U16 SAMPLE_COMM_IVE_CalcStride(SC_U32 u32Width, SC_U8 u8Align)
{
    return (u32Width + (u8Align - u32Width % u8Align) % u8Align);
}
int yuv2csc(FRAME_INFO_S *frame_info)
{
    IVE_SRC_IMAGE_S stCSCIn = {0};
    stCSCIn.u32Width = frame_info->frame.stVFrame.u32Width;
    stCSCIn.u32Height = frame_info->frame.stVFrame.u32Height;
    stCSCIn.enType = IVE_IMAGE_TYPE_YUV420P;

    stCSCIn.au32Stride[0] = frame_info->frame.stVFrame.u32Stride[0];
    stCSCIn.au32Stride[1] = frame_info->frame.stVFrame.u32Stride[1];
    stCSCIn.au32Stride[2] = frame_info->frame.stVFrame.u32Stride[2];

    stCSCIn.au64PhyAddr[0] = frame_info->frame.stVFrame.u64PhyAddr[0];
    stCSCIn.au64PhyAddr[1] = frame_info->frame.stVFrame.u64PhyAddr[1];
    stCSCIn.au64PhyAddr[2] = frame_info->frame.stVFrame.u64PhyAddr[2];

    stCSCIn.au64VirAddr[0] = frame_info->frame.stVFrame.u64VirAddr[0];
    stCSCIn.au64VirAddr[1] = frame_info->frame.stVFrame.u64VirAddr[1];
    stCSCIn.au64VirAddr[2] = frame_info->frame.stVFrame.u64VirAddr[2];

    IVE_SRC_IMAGE_S stCSCOut = {0};
    stCSCOut.u32Width = frame_info->frame.stVFrame.u32Width;
    stCSCOut.u32Height = frame_info->frame.stVFrame.u32Height;
    stCSCOut.enType = IVE_IMAGE_TYPE_U8C3_PACKAGE;
    
    stCSCOut.au32Stride[0] = SAMPLE_COMM_IVE_CalcStride(stCSCOut.u32Width, 16) * 3;

    SC_U32 u32Size = stCSCOut.au32Stride[0] * stCSCOut.u32Height;
    int s32Ret = SC_MPI_SYS_MmzAlloc(&stCSCOut.au64PhyAddr[0], (SC_VOID **)&stCSCOut.au64VirAddr[0], NULL, SC_NULL, u32Size);
    if (s32Ret != SC_SUCCESS)
    {
        printf("Mmz Alloc fail,Error(%#x)\n", s32Ret);
        return s32Ret;
    }
    stCSCOut.au64VirAddr[1] = 0;
    stCSCOut.au64VirAddr[2] = 0;
    stCSCOut.au64PhyAddr[1] = 0;
    stCSCOut.au64PhyAddr[2] = 0;
    stCSCOut.au32Stride[1] = 0;
    stCSCOut.au32Stride[2] = 0;

    SC_BOOL bBlock = SC_TRUE;
    SC_BOOL bFinish;
    FILE *fp = NULL;
    SC_U8 *pU8 = NULL;
    SC_U16 height;
    SC_U16 width;

    IVE_HANDLE hIve;
    IVE_CSC_CTRL_S stIveCscCtrl;
    stIveCscCtrl.enMode = IVE_CSC_MODE_VIDEO_BT601_YUV2RGB;
    SC_BOOL bInstance = SC_TRUE;
    s32Ret = SC_MPI_IVE_CSC(&hIve, &stCSCIn, &stCSCOut, &stIveCscCtrl, bInstance);
    if(s32Ret != SC_SUCCESS)
    {
        printf("SC_MPI_IVE_csc err %d!\n", s32Ret);
        goto exit;
    }

    s32Ret = SC_MPI_IVE_Query(hIve, &bFinish, bBlock);
    while (SC_ERR_IVE_QUERY_TIMEOUT == s32Ret)
    {
        usleep(100000);
        goto exit;
    }

    fp = fopen("/tmp/1.rgb","w+");

    height = frame_info->frame.stVFrame.u32Height;
    width = frame_info->frame.stVFrame.u32Width;

    pU8 = (SC_U8 *)(SC_UL)stCSCOut.au64VirAddr[0];
    for (int y = 0; y < height; y++)
    {
        if ( width * 3 != fwrite(pU8, 1, width * 3, fp))
        {
            printf("write file fail\n");
            s32Ret = -1;
            fclose(fp);
            goto exit;
        }

        pU8 += stCSCOut.au32Stride[0];
    }

    fclose(fp);

    s32Ret = 0;

exit:
    SC_MPI_SYS_MmzFree((stCSCOut.au64PhyAddr[0]),(SC_VOID *)(SC_UL)(stCSCOut.au64VirAddr[0]));

    return s32Ret;

}

void *lenovo_alg(void *param)
{
    while(1)
    {
        FRAME_INFO_S *frame_info = dishes.wait_and_pop();

        struct timeval stStart;
        struct timeval stEnd;

        gettimeofday(&stStart, NULL);
        yuv2csc(frame_info);

        int oriImgWidth = 1920;
        int oriImgHeight = 1080;
        int resizeImgWidth = 640;
        int resizeImgHeight = 640;
        float iou_th = 0.3;
        //float conf_th = 0.6;
	float conf_th = 0.1;
        int class_num = 8;
        int saveFlag = 0;  //如果开启存图,此项置为1
        const char* saveFolder = "tmp/result";
        /* JSON 根对象 */
        cJSON *root = cJSON_CreateObject();
        SVP_NPU_YOLOv8_Predict(&yolov8Model,&yolov8Param,"/tmp/1.rgb",oriImgWidth,oriImgHeight,resizeImgWidth,resizeImgHeight,
                           iou_th,conf_th,class_num,saveFlag,saveFolder,root);

        int ret = save_jpg(frame_info,root);
        if(ret)
        {
            printf("save jpg error\n");
        }

        cJSON_Delete(root);

        // gettimeofday(&stEnd, NULL);
        // float all_time = (float)((float)(stEnd.tv_sec - stStart.tv_sec) * 1000 + (float)(stEnd.tv_usec - stStart.tv_usec)/1000);
        // printf("chn %d save time %f ms \n",frame_info->vpssGrp,all_time);

        ret = mpp_vdec_release_frame((FRAME_INFO_S*)frame_info);
        if(ret)
        {
            printf("chn %d release frame error\n",frame_info->vpssGrp);
        }
		usleep(10 * 1000);
    }
}

//解码数据
int vdec_frame(int chn, void*packet)
{
    int ret;
    FRAME_INFO_S *frame_info = (FRAME_INFO_S*)packet;
 
//    printf("vdec chn %d u32Width %d u32Height %d\n",chn,frame_info->frame.stVFrame.u32Width,frame_info->frame.stVFrame.u32Height);
    
    //测试存图功能和网络发送功能
    num[chn]++;
    //if(num[chn] % 1000 == 0)
	if(num[chn] % 50 == 0)
    {
        dishes.push(frame_info);
		//printf("vdec chn %d u32Width %d u32Height %d\n",chn,frame_info->frame.stVFrame.u32Width,frame_info->frame.stVFrame.u32Height);
        return 0;
    }

    ret = mpp_vdec_release_frame((FRAME_INFO_S*)frame_info);
    if(ret)
    {
        printf("chn %d release frame error\n",frame_info->vpssGrp);
    }

    return 0;
}

SC_S32 SAMPLE_COMM_SYS_Init(VB_CONFIG_S *pstVbConfig)
{
    SC_S32 s32Ret = SC_FAILURE;

    SC_MPI_SYS_Exit();
    SC_MPI_VB_Exit();

    if (NULL == pstVbConfig)
    {
        printf("input parameter is null, it is invaild!\n");
        return SC_FAILURE;
    }

    s32Ret = SC_MPI_VB_SetConfig(pstVbConfig);

    if (SC_SUCCESS != s32Ret)
    {
        printf("SC_MPI_VB_SetConf failed!\n");
        return SC_FAILURE;
    }

    s32Ret = SC_MPI_VB_Init();

    if (SC_SUCCESS != s32Ret)
    {
        printf("SC_MPI_VB_Init failed!\n");
        return SC_FAILURE;
    }

    s32Ret = SC_MPI_SYS_Init();

    if (SC_SUCCESS != s32Ret)
    {
        printf("SC_MPI_SYS_Init failed!\n");
        SC_MPI_VB_Exit();
        return SC_FAILURE;
    }

    return SC_SUCCESS;
}

int sc_venc_init(VENC_CHN VencChn)
{
    SC_S32 s32Ret;
    VENC_CHN_ATTR_S        stVencChnAttr;
    stVencChnAttr.stVencAttr.enType          = PT_JPEG;
    stVencChnAttr.stVencAttr.u32MaxPicWidth  = 1920;
    stVencChnAttr.stVencAttr.u32MaxPicHeight = 1080;
    stVencChnAttr.stVencAttr.u32PicWidth     = 1920;/*the picture width*/
    stVencChnAttr.stVencAttr.u32PicHeight    = 1080;/*the picture height*/
    stVencChnAttr.stVencAttr.u32BufSize      = 1920 * 1080 * 2;/*stream buffer size*/
    stVencChnAttr.stVencAttr.u32Profile      = 0;
    stVencChnAttr.stVencAttr.bByFrame        = SC_TRUE;/*get stream mode is slice mode or frame mode?*/

    VENC_ATTR_JPEG_S       stJpegAttr;
    stJpegAttr.bSupportDCF     = SC_FALSE;
    stJpegAttr.stMPFCfg.u8LargeThumbNailNum = 0;
    stJpegAttr.enReceiveMode = VENC_PIC_RECEIVE_SINGLE;
    memcpy(&stVencChnAttr.stVencAttr.stAttrJpege, &stJpegAttr, sizeof(VENC_ATTR_JPEG_S));

    stVencChnAttr.stGopAttr.enGopMode  = VENC_GOPMODE_NORMALP;
    stVencChnAttr.stGopAttr.stNormalP.s32IPQpDelta = 0;

    s32Ret = SC_MPI_VENC_CreateChn(VencChn, &stVencChnAttr);
    if (SC_SUCCESS != s32Ret)
    {
        printf("SC_MPI_VENC_CreateChn [%d] faild with %#x! ===\n", VencChn, s32Ret);
        return s32Ret;
    }

    VENC_RECV_PIC_PARAM_S  stRecvParam;
    stRecvParam.s32RecvPicNum = -1;
    s32Ret = SC_MPI_VENC_StartRecvFrame(VencChn, &stRecvParam);
    if (SC_SUCCESS != s32Ret)
    {
        printf("SC_MPI_VENC_StartRecvPic faild with%#x! \n", s32Ret);
        return SC_FAILURE;
    }

    VENC_JPEG_PARAM_S stJpegParam;
	SC_MPI_VENC_GetJpegParam(VencChn, &stJpegParam);
	stJpegParam.u32Qfactor = 9;
	SC_MPI_VENC_SetJpegParam(VencChn, &stJpegParam);

	VENC_CHN_PARAM_S stChnParam;
	SC_MPI_VENC_GetChnParam(VencChn, &stChnParam);
	stChnParam.stFrameRate.s32SrcFrmRate = -1;
	stChnParam.stFrameRate.s32DstFrmRate = -1;
	SC_MPI_VENC_SetChnParam(VencChn, &stChnParam);

    return 0;
}

void *re_connect(void *param)
{
    size_t tmp_count[VDECCHN_MAX] = {0};
    size_t connect_flag[VDECCHN_MAX] = {0};
    sleep(10);
    while(re_connect_running)
    {
        for(int i=0;i<VDECCHN_MAX;i++)
        {
            if(g_open_chn[i] == 1)
            {
                if(tmp_count[i]!=g_frame_count[i])
                {
                    tmp_count[i] = g_frame_count[i];
                    connect_flag[i] = 0;
                }
                else
                {
                    connect_flag[i]++;
                }
            }
        }
        if(re_connect_running == 0)
        {
            break;
        }
        sleep(2);
        printf("connect_flag: %d %d %d %d\n",connect_flag[0],connect_flag[1],connect_flag[2],connect_flag[3]);
        if(re_connect_running == 0)
        {
            break;
        }
        for(int i=0;i<VDECCHN_MAX;i++)
        {
            if(connect_flag[i]>5)
            {
                printf("need to connect %d\n",i);
                app_rtsp_client_stop(i);
                app_rtsp_client_start(i, g_config.cams[i].cameraid);
            }
        }
		usleep(10 * 1000);
    }
}

int sc_vdec_init()
{
    int ret = -1;
    
    VB_CONFIG_S stVbConfig;

    memset(&stVbConfig, 0, sizeof(VB_CONFIG_S));
    stVbConfig.u32MaxPoolCnt             = 2;
    stVbConfig.astCommPool[0].u32BlkCnt  = 16 * 4;
    stVbConfig.astCommPool[0].u64BlkSize = COMMON_GetPicBufferSize(1920, 1080,
            PIXEL_FORMAT_YVU_PLANAR_420, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    
    stVbConfig.astCommPool[1].u32BlkCnt  = 10;
    stVbConfig.astCommPool[1].u64BlkSize = COMMON_GetPicBufferSize(1920, 1080,
            PIXEL_FORMAT_YVU_PLANAR_420, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);


    int s32Ret = SAMPLE_COMM_SYS_Init(&stVbConfig);
    if (s32Ret != SC_SUCCESS)
    {
        printf("init sys fail for %#x!\n", s32Ret);
        return -1;
    }

    attrs.max_channels = g_config.cam_num;
    for(int i=0;i<attrs.max_channels;i++)
    {
        attrs.max_chn_reso[i] = VIDEO_1080P;
        attrs.vdec_type[i] = PT_H264;
    }
    
    attrs.frame_cb = vdec_frame;

    ret = mpp_vdec_init(&attrs);
    if(ret)
    {
        printf("mpp_vdec_init failed ret %d\n",ret);
        return -1;
    }

    for(int i=0;i<attrs.max_channels;i++)
    {
        ret = mpp_vdec_open(i);
        if(ret)
        {
            printf("mpp_vdec_open failed ret %d\n",ret);
            return -1;
        }

        ret = mpp_vdec_start(i);
        if(ret)
        {
            printf("mpp_vdec_start failed ret %d\n",ret);
            return -1;
        }
    } 
   
    return ret;
}

void SAMPLE_VDEC_HandleSig(SC_S32 signo)
{
    re_connect_running = 0;
    sleep(1);
    int ret;
    for(int i=0;i<VDECCHN_MAX;i++)
    {
        if(g_open_chn[i] == 1)
        {
            ret = app_rtsp_client_stop(i);
            if(ret)
            {
                printf("app_rtsp_client_stop failed ret %d\n",ret);
            }
            else
            {
                printf("app_rtsp_client_stop success\n");
            }
        }
    }

    ret = app_rtsp_deinit();
    if(ret)
    {
        printf("app_rtsp_deinit failed ret %d\n",ret);
    }
    else
    {
        printf("app_rtsp_deinit success\n");
    }

    sleep(1);

    for(int i=0;i<VDECCHN_MAX;i++)
    {
        if(g_open_chn[i] == 1)
        {
             ret = mpp_vdec_stop(i);
            if(ret)
            {
                printf("mpp_vdec_stop failed ret %d\n",ret);
            }
            else
            {
                printf("mpp_vdec_stop success\n");
            }

            ret = mpp_vdec_close(i);
            if(ret)
            {
                printf("file %s line %d mpp_vdec_close failed ret %d\n",__FILE__,__LINE__,ret);
            }
            else
            {
                printf("file %s line %d mpp_vdec_close OK ret %d\n",__FILE__,__LINE__,ret);
            }   
        }
    }

    ret = mpp_vdec_deinit();
    if(ret)
    {
        printf("mpp_vdec_deinit failed ret %d\n",ret);
    }

    int vdnc_chn = 0;
    ret = SC_MPI_VENC_StopRecvFrame(vdnc_chn);
    if (ret)
    {
        printf("SC_MPI_VENC_StopRecvPic vechn[%d] failed with %#x!\n", vdnc_chn, ret);
    }

    ret = SC_MPI_VENC_DestroyChn(vdnc_chn);
    if (ret)
    {
        printf("SC_MPI_VENC_DestroyChn vechn[%d] failed with %#x!\n", vdnc_chn, ret);
    }

    SVP_NPU_YOLOv8_Deinit(&yolov8Model, &yolov8Param);

    SC_MPI_SYS_Exit();
    SC_MPI_VB_ExitModCommPool(VB_UID_VDEC);
    SC_MPI_VB_Exit();

    printf("in SAMPLE_VDEC_HandleSig signo %d\n",signo);

    exit(0);
    return ;
}

int net_init()
{
    g_send_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (g_send_fd < 0) 
	{ 
		perror("socket"); 
		return -1; 
	}

    int yes = 1; // 用于setsockopt的标志
    // 设置SO_REUSEADDR选项
    if (setsockopt(g_send_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes)) < 0) 
    {
        perror("setsockopt failed");
        return -1; 
    }

    struct sockaddr_un serv = {0};
    serv.sun_family = AF_UNIX;
    strncpy(serv.sun_path, "/tmp/server.sock", sizeof(serv.sun_path) - 1);

    if (connect(g_send_fd, (struct sockaddr *)&serv, sizeof(serv)) < 0) 
	{
        perror("connect"); 
        close(g_send_fd);
        return -1; 
    }

    return 0;
}

int main()
{
    SC_U32 u32ChipId;
    SC_MPI_SYS_GetChipId(&u32ChipId);

    SC_U64 u64CustomCode;
    SC_MPI_SYS_GetCustomCode(&u64CustomCode);

    printf("u32ChipId: %x, u64CustomCode: %llx\n",u32ChipId,u64CustomCode);

    //return 0;

    signal(SIGINT, SAMPLE_VDEC_HandleSig);
    signal(SIGTERM, SAMPLE_VDEC_HandleSig);

    int ret = -1;

    parse_config("/userdata/config.ini", &g_config);

    for(int i=0;i<MAX_CHN_NUM;i++)
    {
        void *buf = malloc(MAX_BUF_SIZE);
        memset(buf,0,MAX_BUF_SIZE);

        sc_kfifo_init(&video_fifo[i],buf,MAX_BUF_SIZE,1);
    }

    APP_RTSP_CLEINT_INIT_INFO_S client_info;

    memset(&client_info, 0, sizeof(APP_RTSP_CLEINT_INIT_INFO_S));

    client_info.recv_video_cb = app_stream_rtsp_recv_handler;
    client_info.stop_video_cb = app_stream_rtsp_stop_handler;
    ret = app_rtsp_init(client_info);
    if(ret)
    {
        printf("app_rtsp_init failed\n");
        return -1;
    }

    // 生成Req文件
    SVP_NPU_GenerateLicense();

    //lenovo alg init 
    const char *lic_file_path = "/userdata/lenovo_alg/0c0f4da4.lic"; //"/userdata/lenovo_alg/f1232d9f.lic";
    // 公钥文件
    const char *public_key_path = "/userdata/lenovo_alg/public_key.pem";

    const char *pcModel = "/userdata/lenovo_alg/model_v1.npubin";

    int s32Ret = SVP_NPU_YOLOv8_LoadModel(pcModel, &yolov8Model, &yolov8Param,lic_file_path,public_key_path);
    if (s32Ret != SC_SUCCESS)
    {
        printf("Load model failed!\n");
        return SC_FAILURE;
    }

    ret = sc_vdec_init();
    if(ret)
    {
        printf("vdec_init failed\n");
        return -1;
    }

    VENC_CHN VencChn = 0;
    ret = sc_venc_init(VencChn);
    if(ret)
    {
        printf("vdec_init failed\n");
        return -1;
    }

    ret = net_init();
    if(ret)
    {
        printf("net_init failed\n");
        return -1;
    }

    pthread_t tid[MAX_CHN_NUM];

    for(int chn=0;chn<g_config.cam_num;chn++)
    {
        if(strlen(g_config.cams[chn].cameraid)!=0)
        {
            ret = app_rtsp_client_start(chn, g_config.cams[chn].cameraid);
            if(ret)
            {
                printf("rtsp chn %d start failed\n",chn);
            }
            else
            {
                pthread_create(&tid[chn], 0, test_save, (void *)&chn);
                g_open_chn[chn] = 1;
                usleep(300*1000);
            }
        }
        else
        {
            printf("chn %d cameraid is null\n");
        }
    }

    pthread_t tidd;
    re_connect_running = 1;
    ret = pthread_create(&tidd, 0, re_connect, NULL);

    ret = pthread_create(&tidd, 0, lenovo_alg, NULL);

    while(re_connect_running)
    {
        sleep(1);
    }
}
