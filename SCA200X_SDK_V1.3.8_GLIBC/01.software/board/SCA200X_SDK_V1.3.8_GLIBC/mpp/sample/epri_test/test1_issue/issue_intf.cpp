
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "mpi_sys.h"
#include "epri_test.h"
#include "issue_intf.h"
#include "issue_json_inf.h"


#include "opencv2/opencv.hpp"
#include "opencv2/videoio.hpp"


#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "mpi_sys.h"
#include "sc_comm_ive.h"
#include "sc_ive.h"
#include "mpi_ive.h"
#include "sc_comm_svp.h"
#include "sc_npu.h"
#include "mpi_npu.h"

using namespace std;
using namespace cv;

#define CONFIDENCE_THRESHOLD 0.1
#define IOU_THRESHOLD  0.45f


int ISSUE_DoTest_Task(char *p_data_base_path, epri_issue_test *p_issue_info,epri_issue_test_res *p_issue_res);

float sigmoid(float x)
{
    return (1 / (1 + exp(-x)));
}

cv::Mat preprocess_img(cv::Mat &img, int INPUT_W, int INPUT_H) {
    int w, h, x, y;
    float r_w = INPUT_W / (img.cols * 1.0);
    float r_h = INPUT_H / (img.rows * 1.0);
    if (r_h > r_w) {
        w = INPUT_W;
        h = r_w * img.rows;
        x = 0;
        y = (INPUT_H - h) / 2;
    } else {
        w = r_h * img.cols;
        h = INPUT_H;
        x = (INPUT_W - w) / 2;
        y = 0;
    }
    cv::Mat re(h, w, CV_8UC3);
    cv::resize(img, re, re.size(), 0, 0, cv::INTER_LINEAR);
    cv::Mat out(INPUT_H, INPUT_W, CV_8UC3, cv::Scalar(128, 128, 128));
    re.copyTo(out(cv::Rect(x, y, re.cols, re.rows)));
    return out;
}

/////////////////////////////////////////////////////////////
static char g_base_path[256] = {0};

static int  issue_json_proc_cb(void *p_cJson)
{
    printf("issue_json_proc_cb\n");
    cJSON               *p_root_cJson = (cJSON*)p_cJson;
    cJSON               *p_response_cJson = NULL;
    epri_issue_test_res    test_json_res = {0};
    epri_issue_test       *p_test_json = NULL;
    int res = 0;

    //获取资源存储的 根目录。
    if(strlen(g_base_path) <= 1)
    {
        const char *p_base_path = SC_MPI_SYS_HttpServer_GetDataPath();
        if(strlen(p_base_path) <= 1)
        {
            SAMPLE_PRT("p_base_path[%s] is error!\n", p_base_path);
            return -1;
        }
        //save base path, for getting resource.
        strcpy(g_base_path, p_base_path);

        //printf("[Info] Get data base path is :%s\n", p_base_path);
    }

    printf("p_root_cJson %s\n",p_root_cJson);

    //将json 字符串转换为 结构体
    p_test_json = (epri_issue_test*)json_to_struct_epri_issue_test(p_root_cJson);
    if(NULL != p_test_json)
    {
        //获得资源的完整路径
        //get resource path: xxx->xxx_url
        //char res_full_path[256];
        //sprintf(res_full_path, "%s/%s", g_base_path, p_test_json->images.path);


        /*****************************************************************************/
        //调用用户 测试 主程序.
        //do somthing ...

        res = ISSUE_DoTest_Task(g_base_path, p_test_json, &test_json_res);
        /*****************************************************************************/


        //回复结果，post 到服务器
        //get response url: xxx->response_url
        if(!res)
        {
            strcpy(test_json_res.status, "success");
        }
        else
        {
            strcpy(test_json_res.status, "fail");
        }

        strcpy(test_json_res.session, p_test_json->session);

        //change struct to json.
        p_response_cJson = struct_to_json_epri_issue_test_res(&test_json_res);
        if(NULL == p_response_cJson)
        {
            SAMPLE_PRT("get response json fail!\n");
            return -1;
        }
        printf("response_url:%s\n", p_test_json->response_url);
        //post response to test web server.
        res = SC_MPI_SYS_HttpClient_PostData(p_test_json->response_url, MPI_SYS_POST_DATA_TYPE_CJSON, 0, p_response_cJson);
        if (res)
        {
            printf("res:%d response_url:%s\n", res, p_test_json->response_url);
        }

        //free response cjson obj.
        cJSON_Delete(p_response_cJson);

        //free input struct
        free(p_test_json);
        p_test_json = NULL;

        return res;
    }

    return -1;
}

typedef struct
{
	SVP_NPU_MODEL_S    	   	model;
	SVP_MEM_INFO_S		   	stIntMem;
	SVP_MEM_INFO_S 			stOutMem;
    SVP_MEM_INFO_S 			stTmpMem;
	NPU_IMAGE_S             s_stImgSet;
}MODEL_INFO;

#define ALIGNED_256B(x) ((x)%256 == 0 ? (x) : (((x)/256 + 1) * 256))
#define ALIGN16KB        (16384)
#define ALIGN_N(x, align)           (((x) + ((align) - 1)) & ~((align) - 1))


int issud_svp_image_load(SVP_NPU_MODEL_CONF_S model_conf,MODEL_INFO &model_info,char *name)
{
    int ret;
    char mem_name[128];
    /*load model*/
    ret = SC_MPI_SVP_NPU_LoadModel(&model_conf,&model_info.model);
    if(ret)
    {
        printf("SC_MPI_SVP_NPU_LoadModel error 0x%x\n",ret);
        return ret;
    }

    memset(mem_name,0,sizeof(mem_name));
    sprintf(mem_name,"%s_din",name);
    model_info.stIntMem.u64Size = model_info.model.u32InMemSize;
    ret = SC_MPI_SYS_MmzAlloc_Align(&model_info.stIntMem.u64PhyAddr, (void **)&model_info.stIntMem.u64VirAddr, mem_name,NULL,
                                model_info.stIntMem.u64Size, ALIGN16KB);
    if(ret != 0)
    {
        printf("ret error\n");
        return -1;
    }

    memset((char *)model_info.stIntMem.u64VirAddr,0,model_info.stIntMem.u64Size);

    memset(mem_name,0,sizeof(mem_name));
    sprintf(mem_name,"%s_dout",name);
    model_info.stOutMem.u64Size = model_info.model.u32OutMemSize;

    ret = SC_MPI_SYS_MmzAlloc_Align(&model_info.stOutMem.u64PhyAddr, (void **)&model_info.stOutMem.u64VirAddr, mem_name,NULL,
                (SC_U64)model_info.stOutMem.u64Size, ALIGN16KB);

    if(ret != 0)
    {
        printf("ret error\n");
        return -1;
    }

    memset((char *)model_info.stOutMem.u64VirAddr,0,(SC_U64)model_info.stOutMem.u64Size);

    int stride_0 = ALIGNED_256B(model_info.model.astSrcTensor[0].u32Width);
    int stride_1 = ALIGNED_256B(model_info.model.astSrcTensor[0].u32Width);

    int h = model_info.model.astSrcTensor[0].u32Height;

    SVP_MEM_INFO_S st_frame;
    st_frame.u64Size = stride_0*h+stride_1*h*2;

    memset(mem_name,0,sizeof(mem_name));
    sprintf(mem_name,"%s_st",name);

    ret = SC_MPI_SYS_MmzAlloc(&st_frame.u64PhyAddr, (void **)&st_frame.u64VirAddr, mem_name,NULL,st_frame.u64Size);
    if(ret != 0)
    {
        printf("ret error\n");
        return -1;
    }

    memset((char*)st_frame.u64VirAddr,0,stride_0*h);
    memset((char*)st_frame.u64VirAddr+stride_0*h,0,stride_1*h*2);

    model_info.s_stImgSet.u32InputNum = 1;
    model_info.s_stImgSet.astInputImg[0].u32BatchNum = 1;

    model_info.s_stImgSet.astInputImg[0].stImages[0].au32Stride[0] = stride_0;
    model_info.s_stImgSet.astInputImg[0].stImages[0].au32Stride[1] = stride_1;
    model_info.s_stImgSet.astInputImg[0].stImages[0].au32Stride[2] = stride_1;

    model_info.s_stImgSet.astInputImg[0].stImages[0].au64PhyAddr[0] = st_frame.u64PhyAddr;
    model_info.s_stImgSet.astInputImg[0].stImages[0].au64PhyAddr[1] = st_frame.u64PhyAddr+stride_0*h;
    model_info.s_stImgSet.astInputImg[0].stImages[0].au64PhyAddr[2] = st_frame.u64PhyAddr+stride_0*h+stride_1*h;


    model_info.s_stImgSet.astInputImg[0].stImages[0].au64VirAddr[0] = st_frame.u64VirAddr;
    model_info.s_stImgSet.astInputImg[0].stImages[0].au64VirAddr[1] = st_frame.u64VirAddr+stride_0*h;
    model_info.s_stImgSet.astInputImg[0].stImages[0].au64VirAddr[2] = st_frame.u64VirAddr+stride_0*h+stride_1*h;

    model_info.s_stImgSet.astInputImg[0].stImages[0].u32Width = model_info.model.astSrcTensor[0].u32Width;
    model_info.s_stImgSet.astInputImg[0].stImages[0].u32Height = model_info.model.astSrcTensor[0].u32Height;

    model_info.s_stImgSet.astInputImg[0].stImages[0].enType = SVP_IMG_RGB;

    return 0;

}

int mat_to_svp(Mat &src_img,SVP_IMAGE_S& image,SVP_IMAGE_S& dst_image)
{
    unsigned short u16Stride = ALIGNED_256B(src_img.cols);
    char *pBuf = (char *)dst_image.au64VirAddr[0];

    cv::Mat channels[3];
    cv::split(src_img, channels);
    char *pchR = (char *)channels[2].data;
    char *pchG = (char *)channels[1].data;
    char *pchB = (char *)channels[0].data;

    for(int i = 0;i< src_img.rows;i++)
    {
        for(int j = 0;j< src_img.cols;j++)
        {
            pBuf[i*u16Stride + j]                 = pchR[i*src_img.cols + j];
            pBuf[(i+src_img.rows)*u16Stride + j]    = pchG[i*src_img.cols + j];
            pBuf[(i+src_img.rows*2)*u16Stride + j]  = pchB[i*src_img.cols + j];
        }
        for(int j = src_img.cols;j< u16Stride;j++)
        {
            pBuf[i*u16Stride + j]                 = 0;
            pBuf[(i+src_img.rows)*u16Stride + j]    = 0;
            pBuf[(i+src_img.rows*2)*u16Stride + j]  = 0;
        }
    }

    return 0;
}




static int  anchorSize[18] = {
 10, 13, 16, 30, 33, 23,
	 30, 61, 62, 45, 59, 119,
	116, 90, 156, 198, 373, 326
	};


typedef struct
{
    int left; 	// 矩形最左边的坐标
	int top; 	// 矩形最上边的坐标
	int right; 	// 矩形最右边的坐标
	int bottom; // 矩形最下边的坐标
}ISSUE_RECT_S;


typedef struct
{
    ISSUE_RECT_S     rect;
    float           confidence;
	unsigned int    classId;
}ISSUE_RESULT;

typedef struct {
	float x;
	float y;
	float w;
	float h;
	float confidence;
	unsigned int classId;
}DETECTION_RESULT;


typedef struct
{
    float x, y, w, h;
} box;


typedef struct
{
    box bbox;
    int classes;
    float *prob;
    float *mask;
    float objectness;
    int sort_class;
} detection_issue;


static int entry_index(int h, int w, int c, int byteUnit, NPU_TENSOR_S &pTensorInfo)
{
    /*
    int n = location / (layer_w*layer_h);
    int loc = location % (layer_w*layer_h);
    return (n*layer_w*layer_h*(4 + 80 + 1) + entry* layer_w*layer_h + loc);
    */
    int tensor_k_size_norm = pTensorInfo.u32KSizeNorm;
    int tensor_k_ddr_step = pTensorInfo.u32KStep;
    int tensor_row_ddr_step = pTensorInfo.u32RowStep;
    int tensor_k_norm_num = pTensorInfo.u32KNormNum;
    int tensor_last = pTensorInfo.u32KSizeLast;
    //int tensor_last_valid = pTensorInfo.u32OriChannel -tensor_k_norm_num*tensor_k_size_norm;

    int AddressOffset = (c / tensor_k_size_norm) * tensor_k_ddr_step + h * tensor_row_ddr_step;
    int memoryMigration = (c < tensor_k_norm_num * tensor_k_size_norm) ? \
        (AddressOffset + c % tensor_k_size_norm * byteUnit + w * tensor_k_size_norm * byteUnit) : (AddressOffset +
            (c - tensor_k_norm_num * tensor_k_size_norm) * byteUnit + w * tensor_last * byteUnit);
    return memoryMigration / byteUnit;
}

int BboxDecoding(MODEL_INFO model_info,auto Buffer, int tensorIndex, detection_issue *detsObj,int totalClass)
{
	float x, y, w, h;
	float pred_conf, clsVaue;

	int feature_w = model_info.model.astDstTensor[tensorIndex].u32Width;
	int feature_h = model_info.model.astDstTensor[tensorIndex].u32Height;
	//int feature_c = det_model.astDstTensor[tensorIndex].u32OriChannels;
	int inputTensorW = model_info.model.astSrcTensor[0].u32Width;
	int inputTensorH = model_info.model.astSrcTensor[0].u32Height;
	int count = 0;
	int index_transform;
	int tensor_zero_point = model_info.model.astDstTensor[tensorIndex].s32ZeroPoint;
	float tensor_scale_factor = model_info.model.astDstTensor[tensorIndex].dScaleFactor;

	for (int row = 0; row <feature_h; row++)
	{
		for (int col = 0; col <feature_w; col++)
		{
			for (int i = 0; i < 3; i++)
			{
				index_transform = entry_index(row, col, i * (totalClass + 5) + 4, sizeof(Buffer[0]), model_info.model.astDstTensor[tensorIndex]);
				if(sizeof(Buffer[0] != 4))
				    pred_conf = sigmoid((Buffer[index_transform] - tensor_zero_point) * tensor_scale_factor);
                else
                    pred_conf = sigmoid(Buffer[index_transform]);

				if (pred_conf<= CONFIDENCE_THRESHOLD)
					continue;

				index_transform = entry_index(row, col, i *(totalClass + 5) + 0, sizeof(Buffer[0]), model_info.model.astDstTensor[tensorIndex]);
                if(sizeof(Buffer[0] != 4))
				    x = (Buffer[index_transform] - tensor_zero_point) * tensor_scale_factor;
                else
                    x = (Buffer[index_transform]);

				index_transform = entry_index(row, col, i *(totalClass + 5) + 1, sizeof(Buffer[0]), model_info.model.astDstTensor[tensorIndex]);
                if(sizeof(Buffer[0] != 4))
				    y = (Buffer[index_transform] - tensor_zero_point) * tensor_scale_factor;
                else
                    y = Buffer[index_transform];

				index_transform = entry_index(row, col, i*(totalClass + 5) + 2, sizeof(Buffer[0]), model_info.model.astDstTensor[tensorIndex]);

                if(sizeof(Buffer[0] != 4))
				    w = (Buffer[index_transform] - tensor_zero_point) * tensor_scale_factor;
                else
                    w = Buffer[index_transform];

				index_transform = entry_index(row, col, i*(totalClass + 5) + 3, sizeof(Buffer[0]), model_info.model.astDstTensor[tensorIndex]);

                if(sizeof(Buffer[0] != 4))
				    h = (Buffer[index_transform] - tensor_zero_point) * tensor_scale_factor;
                else
                    h = Buffer[index_transform];

				detsObj[count].bbox.x = (sigmoid(x)*2.0f-0.5f + col) / feature_w;
				detsObj[count].bbox.y = (sigmoid(y)*2.0f-0.5f + row) / feature_h;

				detsObj[count].bbox.w = pow((sigmoid(w)*2.0f), 2)*anchorSize[2 * i + 2 * tensorIndex *3 ] / inputTensorW;
				detsObj[count].bbox.h = pow((sigmoid(h)*2.0f), 2)*anchorSize[2 * i + 2 * tensorIndex *3+ 1] / inputTensorH;

				detsObj[count].objectness = pred_conf;
				detsObj[count].classes = totalClass;
				for (int j = 0; j<detsObj[count].classes; ++j){
					index_transform = entry_index(row, col, i * (totalClass + 5) + 5 + j, sizeof(Buffer[0]), model_info.model.astDstTensor[tensorIndex]);
                    if(sizeof(Buffer[0] != 4))
                        clsVaue = sigmoid((Buffer[index_transform] - tensor_zero_point) * tensor_scale_factor);
                    else
                        clsVaue = sigmoid(Buffer[index_transform]);
					float prob = pred_conf*clsVaue;
					detsObj[count].prob[j] = (prob > CONFIDENCE_THRESHOLD) ? prob : 0;
				}
                //printf("count %d\n",count);
				++count;
			}
		}
	}
	return count;
}

static int nms_comparator(const void *pa, const void *pb)
{
    detection_issue a = *(detection_issue *)pa;
    detection_issue b = *(detection_issue *)pb;
    float diff = 0;
    if(b.sort_class >= 0)
    {
        diff = a.prob[b.sort_class] - b.prob[b.sort_class];
    }
    else
    {
        diff = a.objectness - b.objectness;
    }
    if(diff < 0) return 1;
    else if(diff > 0) return -1;
    return 0;
}


static float overlap(float x1, float w1, float x2, float w2)
{
    float l1 = x1 - w1 / 2;
    float l2 = x2 - w2 / 2;
    float left = l1 > l2 ? l1 : l2;
    float r1 = x1 + w1 / 2;
    float r2 = x2 + w2 / 2;
    float right = r1 < r2 ? r1 : r2;
    return right - left;
}

static float box_intersection(box a, box b)
{
    float w = overlap(a.x, a.w, b.x, b.w);
    float h = overlap(a.y, a.h, b.y, b.h);
    if(w < 0 || h < 0) return 0;
    float area = w * h;
    return area;
}

static float box_union(box a, box b)
{
    float i = box_intersection(a, b);
    float u = a.w * a.h + b.w * b.h - i;
    return u;
}

static float box_iou(box a, box b)
{
    return box_intersection(a, b) / box_union(a, b);
}


void nms(detection_issue *dets, int total, int classes, float thresh)
{
    int i, j, k;
    k = total-1;
    for(i = 0; i <= k; ++i){
        if(dets[i].objectness == 0){
            detection_issue swap = dets[i];
            dets[i] = dets[k];
            dets[k] = swap;
            --k;
            --i;
        }
    }
    total = k+1;

    for(k = 0; k < classes; ++k){
        for(i = 0; i < total; ++i){
            dets[i].sort_class = k;
        }
        qsort(dets, total, sizeof(detection_issue), nms_comparator);
        for(i = 0; i < total; ++i){
            if(dets[i].prob[k] == 0) continue;
            box a = dets[i].bbox;
            for(j = i+1; j < total; ++j){
                box b = dets[j].bbox;
                if (box_iou(a, b) > thresh){
                    dets[j].prob[k] = 0;
                }
            }
        }
    }
}


int maxindex(float *a, int n)
{
    if (n <= 0) return -1;
    int i, max_i = 0;
    float max = a[0];
    for (i = 1; i < n; ++i)
    {
        if (a[i] > max)
        {
            max = a[i];
            max_i = i;
        }
    }
    return max_i;
}


int issuePostProcess(MODEL_INFO model_info,auto buff, std::vector<DETECTION_RESULT> & vOutput,detection_issue *dets,int totalClass)
{
	int inputTensorW = model_info.model.astSrcTensor[0].u32Width;
	int inputTensorH = model_info.model.astSrcTensor[0].u32Height;

	auto Buffer  = buff;

	int count = 0;
	int offset = 0;

	for (int tensor_index = 0; tensor_index < model_info.model.u16DstNum; tensor_index++)
	{
		offset = (model_info.model.astDstTensor[tensor_index].u32Bank * 32 * 1024 * 1024 + model_info.model.astDstTensor[tensor_index].u32Offset);
		int bits = model_info.model.astDstTensor[tensor_index].u32Precision/8;
        if(sizeof(Buffer[0]) == 4)
        {
            Buffer = buff+offset/4;
        }
        else
        {
            Buffer = buff + offset/bits;
        }

		count += BboxDecoding(model_info,Buffer, tensor_index,&dets[count],totalClass);

		if (count>(80 * 80 * 3 + 40 * 40 * 3 + 20 * 20 * 3))
			return -1;
	}


	nms(dets, count, totalClass, IOU_THRESHOLD);

	DETECTION_RESULT obj;
	for (int i = 0; i < count; i++)
	{
		int const obj_id = maxindex(dets[i].prob, totalClass);
		float prob = dets[i].prob[obj_id];

		if (prob > CONFIDENCE_THRESHOLD)
		{
			obj.confidence = prob;
            //printf("obj.classId %u obj.confidence %f\n",obj.classId,obj.confidence);


			obj.x = std::max((float)0, (dets[i].bbox.x - dets[i].bbox.w / 2) * inputTensorW);
			obj.x = std::min(obj.x, (float)inputTensorW);
			obj.y = std::max((float)0, (dets[i].bbox.y - dets[i].bbox.h / 2) * inputTensorH);
			obj.y = std::min(obj.y, (float)inputTensorH);

			obj.w = dets[i].bbox.w * inputTensorW;
			obj.h = dets[i].bbox.h * inputTensorH;

			obj.classId = obj_id;

			vOutput.push_back(obj);
		}
	}


	return 0;
}



int get_issue_result(MODEL_INFO model_info,std::vector<ISSUE_RESULT>& det_rects,detection_issue *dets,int totalClass)
{
    det_rects.clear();

    std::vector<DETECTION_RESULT>  d_rect;
    d_rect.clear();

    if(!strcmp("float",model_info.model.astDstTensor[0].achType))
	{
		issuePostProcess(model_info,(float *)(model_info.stOutMem.u64VirAddr), d_rect,dets,totalClass);
	}
	else
	{
		if((model_info.model.astDstTensor[0].u32Precision)==8){
			issuePostProcess(model_info,(signed char *)(model_info.stOutMem.u64VirAddr), d_rect,dets,totalClass);
		}
		else if((model_info.model.astDstTensor[0].u32Precision)==16){
		    issuePostProcess(model_info,(signed short *)(model_info.stOutMem.u64VirAddr), d_rect,dets,totalClass);
		}
	}

    det_rects.resize(d_rect.size());

    for(size_t i=0;i<d_rect.size();i++)
    {
        det_rects[i].rect.left = d_rect[i].x;
        det_rects[i].rect.top = d_rect[i].y;
        det_rects[i].rect.right = det_rects[i].rect.left + d_rect[i].w;
        det_rects[i].rect.bottom = det_rects[i].rect.top + d_rect[i].h;
        det_rects[i].confidence = d_rect[i].confidence;
        det_rects[i].classId = d_rect[i].classId;
    }

    return 0;
}


void transform(const int &ih, const int &iw, const int &oh, const int &ow, float& xmin, float& ymin, float& xmax, float& ymax)
{
	float scale = std::min(static_cast<float>(ow) / static_cast<float>(iw), static_cast<float>(oh) / static_cast<float>(ih));
	int nh = static_cast<int>(scale * static_cast<float>(ih));
	int nw = static_cast<int>(scale * static_cast<float>(iw));
	int dh = (oh - nh) / 2;
	int dw = (ow - nw) / 2;
	xmin = (xmin - dw) / scale;
	ymin = (ymin - dh) / scale;
	xmax = (xmax - dw) / scale;
	ymax = (ymax - dh) / scale;
}

#if 0
typedef enum
{
    CRANE=0,// - 吊车,
    TOWER_CRANE=1,// - 塔吊,
    CEMENT_PUMP_TRUCK=2,// - 水泥泵车,
    DOZER=3,// - 推土机,
    DIGGER=4,// - 挖掘机,
    FIRE,// - 山火,
    SMOKE,// - 烟雾,
    MUCK_TRUCK,// - 渣土机,
    SHOVEL,// - 铲车,
    KITE,// - 风筝,
    BALLOON,// - 气球,
    FILM,// - 塑料薄膜,
    DUSTPROOF_NET,// - 防尘网,
    REFLECTIVE_FILM,// - 反光膜,
    TREE_BARRIER,// - 树障
}CLASS;
#endif
char cl_name[15][64] = {
    "crane",
    "tower_crane",
    "cement_pump_truck",
    "dozer_shovel",
    "truck",
    "digger",
    "fire_smoke",
    "fire_smoke",
    "dozer_shovel",
    "kite_balloon",
    "kite_balloon",
    "film_dustproofnet_reflectivefilm",
    "film_dustproofnet_reflectivefilm",
    "film_dustproofnet_reflectivefilm",
    "tree_barrier",
    };


typedef struct
{
    int id;
    char class_name[15][64];
}CLASS;

CLASS g_class;

int issue_svp_image_unload(MODEL_INFO& model_info)
{
    if(model_info.stIntMem.u64Size != 0)
    {
        SC_MPI_SYS_MmzFree(model_info.stIntMem.u64PhyAddr, (void*)(model_info.stIntMem.u64PhyAddr));
        model_info.stIntMem.u64Size = 0;
        model_info.stIntMem.u64PhyAddr = 0;
        model_info.stIntMem.u64PhyAddr = 0;
    }

    if(model_info.stOutMem.u64Size != 0)
    {
        SC_MPI_SYS_MmzFree(model_info.stOutMem.u64PhyAddr, (void*)(model_info.stOutMem.u64PhyAddr));
        model_info.stOutMem.u64Size = 0;
        model_info.stOutMem.u64PhyAddr = 0;
        model_info.stOutMem.u64PhyAddr = 0;
    }

    if(model_info.stTmpMem.u64Size != 0)
    {
        SC_MPI_SYS_MmzFree(model_info.stTmpMem.u64PhyAddr, (void*)(model_info.stTmpMem.u64PhyAddr));
        model_info.stTmpMem.u64Size = 0;
        model_info.stTmpMem.u64PhyAddr = 0;
        model_info.stTmpMem.u64PhyAddr = 0;
    }

    SC_MPI_SYS_MmzFree(model_info.s_stImgSet.astInputImg[0].stImages[0].au64PhyAddr[0],
            (void*)(model_info.s_stImgSet.astInputImg[0].stImages[0].au64PhyAddr[0]));

    SC_MPI_SVP_NPU_UnloadModel(&model_info.model);

    return 0;
}



int ISSUE_DoTest_Task(char *p_data_base_path, epri_issue_test *p_issue_info,epri_issue_test_res *p_issue_res)
{
    int i, j, ret = -1;
    char cmdname[1024] = {0};
    char command_ex[] = "detect_issue_model";

    int s32Ret = -1;
    //用户实际测试 函数
    SAMPLE_PRT("********** do issue test **********\n");

    struct timeval stStart;
    struct timeval stEnd;

    float all_time;

    //load model
	MODEL_INFO model_info;
	SVP_NPU_MODEL_CONF_S model_conf;
	memset(&model_conf,0x00,sizeof(model_conf));

	char *pszModelFile = (char *)"./model/shudianxiantondao_int16.npubin";

	FILE *fp = fopen(pszModelFile, "rb");
	if(fp == NULL)
	{
		printf("fopen pszModelFile %s error\n",pszModelFile);
		return -1;
	}

	s32Ret = fseek(fp, 0L, SEEK_END);
	SC_SL slFileSize = 0;
	slFileSize = ftell(fp);
	s32Ret = fseek(fp, 0L, SEEK_SET);

	SC_U8 *pu8VirAddr = (SC_U8 *)malloc(slFileSize);
	if(pu8VirAddr == NULL)
	{
		printf("malloc error\n");
		fclose(fp);
		return -1;
	}

	s32Ret = fread(pu8VirAddr, slFileSize, 1, fp);
	if(s32Ret != 1)
	{
		printf("fread error\n");
		free(pu8VirAddr);
		fclose(fp);
		return -1;
	}

	model_conf.stModelBuf.u64VirAddr = (unsigned long long)pu8VirAddr;
    model_conf.stModelBuf.u64Size = slFileSize;

	model_conf.enPriority = SVP_NPU_PRIORITY_NORMAL;
	model_conf.enCbMode = SVP_NPU_NO_CB;

	s32Ret = issud_svp_image_load(model_conf, model_info,(char *)"issu");
	if(s32Ret)
	{
		printf("SC_MPI_SVP_NPU_LoadModel error 0x%x\n",s32Ret);
		free(pu8VirAddr);
		fclose(fp);
		return -1;
	}

	free(pu8VirAddr);
	fclose(fp);

    int WW = model_info.model.astSrcTensor[0].u32Width;
    int HH = model_info.model.astSrcTensor[0].u32Height;

    printf("WW %d HH %d\n",WW,HH);

    //execute cpu test.
    if (!memcmp(p_issue_info->command, command_ex, strlen(command_ex)))
    {
        //printf("ISSUE_DoTest_Task->%s start!\n", p_data_base_path);
        //printf("response_url:%s\n", p_issue_info->response_url);

        for (i = 0; i < NG_ARRAY_CNT; i++)
        {
            gettimeofday(&stStart, NULL);
            if (strlen(p_issue_info->images[i].path) < 1)
            {
                continue;;
            }

            p_issue_res->images_cnt++;
            strcpy(p_issue_res->images[i].path,p_issue_info->images[i].path);

            sprintf(cmdname, "%s/%s", p_data_base_path, p_issue_info->images[i].path);
            if (access(cmdname, F_OK) != 0)
            {
                printf("cmdname:%s no exist!\n", cmdname);
                continue;
            }

            Mat cvimg = cv::imread(cmdname, cv::IMREAD_ANYCOLOR);
			if(cvimg.empty())
			{
				printf("Imread file %s failed!\r\n", cmdname);
				goto unload;
			}

            int roi_flag = 0;
            printf("image[%d]->cmdname:%s\n", i, cmdname);
            printf("location:%s\n", p_issue_info->images[i].location);
            int class_max[15];
            memset(class_max,0,sizeof(class_max));
            for (j = 0; j < NG_ARRAY_CNT; j++)
            {
                if (strlen(p_issue_info->images[i].issue_types[j]) < 1)
                {
                    continue;
                }

                for(int num=0;num<15;num++)
                {
                    if(!strcmp(g_class.class_name[num],p_issue_info->images[i].issue_types[j]))
                    {
                        class_max[num]=1;
                    }
                }

                printf("issue_types[%d]:%s\n",j, p_issue_info->images[i].issue_types[j]);
            }

            if(!strcmp("all",p_issue_info->images[i].issue_types[0]))
            {
               for(int m=0;m<15;m++)
               {
                    class_max[m]=1;
               }
            }

            for (j = 0; j < NG_ARRAY_CNT; j++)
            {
                if((p_issue_info->images[i].rois[j].x ==0) &&
					(p_issue_info->images[i].rois[j].y==0) &&
					(p_issue_info->images[i].rois[j].w == 0) &&
					(p_issue_info->images[i].rois[j].h==0))
				{
					continue;
				}
                roi_flag = 1;
                printf("rois[%d]:%d-%d-%d-%d\n",j, p_issue_info->images[i].rois[j].x,p_issue_info->images[i].rois[j].y,
                        p_issue_info->images[i].rois[j].w,p_issue_info->images[i].rois[j].h);


                int xx = p_issue_info->images[i].rois[j].x;
                int yy = p_issue_info->images[i].rois[j].y;
                int ww = p_issue_info->images[i].rois[j].w;
                int hh = p_issue_info->images[i].rois[j].h;

                Mat crop = cvimg(Rect(xx,yy,ww,hh));
                Mat tmm_img = preprocess_img(crop, WW, HH);
            	if(tmm_img.empty())
            	{
            	   printf("tmm_img.empty()\r\n");
            	   goto unload;
            	}
                cv::Mat sample_resized;
                cv::Size input_geometry = cv::Size(WW, HH);
            	if(tmm_img.size() != input_geometry)
            		cv::resize(tmm_img, sample_resized, input_geometry);
            	else
            		sample_resized = tmm_img;

                Mat src_img;
                src_img = sample_resized;

                SVP_IMAGE_S image;
                mat_to_svp(src_img,image,model_info.s_stImgSet.astInputImg[0].stImages[0]);

                SVP_NPU_HANDLE hSvpNpuHandle = 0;

                gettimeofday(&stStart, NULL);
                ret = SC_MPI_SVP_NPU_Forward(&hSvpNpuHandle, &model_info.s_stImgSet, &model_info.stIntMem,&model_info.model,&model_info.stOutMem,SC_TRUE);
                if(ret != 0)
                {
                    printf("SC_MPI_SVP_NPU_Forward error  ret 0x%x\n",ret);
                    goto unload;
                }
                gettimeofday(&stEnd, NULL);
                all_time = (float)((float)(stEnd.tv_sec - stStart.tv_sec) * 1000000 + (float)(stEnd.tv_usec - stStart.tv_usec));
                detection_issue *dets;
	            int totalClass;

                totalClass = (model_info.model.astDstTensor[0].u32OriChannels - 3 * 5) / 3;
            	dets = (detection_issue *)malloc((80*80*3 + 40*40*3 + 160*160*3)*sizeof(detection_issue));
            	for (int i = 0; i < (80*80*3 + 40*40*3 + 160*160*3); i++)
                {
            		(dets[i]).prob = (float*)malloc(sizeof(float)*totalClass);
                }
                std::vector<ISSUE_RESULT> issue_rects;
                issue_rects.clear();
                get_issue_result(model_info,issue_rects,dets,totalClass);

                //free dets.
                for (int i = 0; i < (80*80*3 + 40*40*3 + 160*160*3); i++)
                {
                    free((dets[i]).prob);
                }
                free(dets);

                for (size_t i = 0; i < issue_rects.size(); i++)
                {
            		float xmin = issue_rects[i].rect.left;
            		float ymin = issue_rects[i].rect.top;
            		float xmax = issue_rects[i].rect.right;
            		float ymax = issue_rects[i].rect.bottom;

            		transform(cvimg.rows, cvimg.cols, HH, WW, xmin, ymin, xmax, ymax);

            		issue_rects[i].rect.left = xmin+p_issue_info->images[i].rois[j].x;
            		issue_rects[i].rect.top = ymin+p_issue_info->images[i].rois[j].y;
            		issue_rects[i].rect.right = xmax+p_issue_info->images[i].rois[j].w;
            		issue_rects[i].rect.bottom = ymax+p_issue_info->images[i].rois[j].h;
                }

                for(size_t ii=0;ii<issue_rects.size();ii++)
                {
                    for(int k=0;k<15;k++)
                    {
                        if(class_max[k] == 1)
                        {
                             if((int)issue_rects[ii].classId == k)
                             {
                                p_issue_res->images[i].inference_time = all_time;
                                p_issue_res->images[i].issues[ii].score = issue_rects[ii].confidence;
                                strcpy(p_issue_res->images[i].issues[ii].issue_type,g_class.class_name[k]);
                                p_issue_res->images[i].issues[ii].x = issue_rects[ii].rect.left;
                                p_issue_res->images[i].issues[ii].y = issue_rects[ii].rect.right;
                                p_issue_res->images[i].issues[ii].w = issue_rects[ii].rect.right-issue_rects[ii].rect.left;
                                p_issue_res->images[i].issues[ii].h = issue_rects[ii].rect.bottom-issue_rects[ii].rect.top;
                                p_issue_res->images[i].issues_cnt++;
                             }
                        }
                    }

                    printf("ID:%d score %f (l,r,t,b)%d %d %d %d\n",issue_rects[ii].classId,issue_rects[ii].confidence,
                        issue_rects[ii].rect.left,issue_rects[ii].rect.right,
                            issue_rects[ii].rect.top,issue_rects[ii].rect.bottom);
                }



            }
            //全图检测
            if(0 == roi_flag)
            {
                Mat tmm_img = preprocess_img(cvimg, WW, HH);
            	if(tmm_img.empty())
            	{
            	   printf("tmm_img.empty()\r\n");
            	   goto unload;
            	}

                imwrite("tmm_img.jpg",tmm_img);

                cv::Mat sample_resized;
                cv::Size input_geometry = cv::Size(WW, HH);
            	if(tmm_img.size() != input_geometry)
            		cv::resize(tmm_img, sample_resized, input_geometry);
            	else
            		sample_resized = tmm_img;

                Mat src_img;
                src_img = sample_resized;

                SVP_IMAGE_S image;
                mat_to_svp(src_img,image,model_info.s_stImgSet.astInputImg[0].stImages[0]);

                SVP_NPU_HANDLE hSvpNpuHandle = 0;

                gettimeofday(&stEnd, NULL);
                all_time = (float)((float)(stEnd.tv_sec - stStart.tv_sec) * 1000000 + (float)(stEnd.tv_usec - stStart.tv_usec));
                printf("pre_timer:%f\n", all_time);

                gettimeofday(&stStart, NULL);
                ret = SC_MPI_SVP_NPU_Forward(&hSvpNpuHandle, &model_info.s_stImgSet, &model_info.stIntMem,&model_info.model,&model_info.stOutMem,SC_TRUE);
                if(ret != 0)
                {
                    printf("SC_MPI_SVP_NPU_Forward error  ret 0x%x\n",ret);
                    goto unload;
                }
                gettimeofday(&stEnd, NULL);
                all_time = (float)((float)(stEnd.tv_sec - stStart.tv_sec) * 1000000 + (float)(stEnd.tv_usec - stStart.tv_usec));
                detection_issue *dets;
	            int totalClass;
                printf("forward_timer:%f\n", all_time);

                gettimeofday(&stStart, NULL);
                totalClass = (model_info.model.astDstTensor[0].u32OriChannels - 3 * 5) / 3;
            	dets = (detection_issue *)malloc((80*80*3 + 40*40*3 + 160*160*3)*sizeof(detection_issue));
            	for (int i = 0; i < (80*80*3 + 40*40*3 + 160*160*3); i++)
                {
            		(dets[i]).prob = (float*)malloc(sizeof(float)*totalClass);
                }
                std::vector<ISSUE_RESULT> issue_rects;
                issue_rects.clear();
                get_issue_result(model_info,issue_rects,dets,totalClass);

                //free dets.
                for (int i = 0; i < (80*80*3 + 40*40*3 + 160*160*3); i++)
                {
                    free((dets[i]).prob);
                }
                free(dets);

            	for (size_t i = 0; i < issue_rects.size(); i++)
                {
            		float xmin = issue_rects[i].rect.left;
            		float ymin = issue_rects[i].rect.top;
            		float xmax = issue_rects[i].rect.right;
            		float ymax = issue_rects[i].rect.bottom;

            		transform(cvimg.rows, cvimg.cols, HH, WW, xmin, ymin, xmax, ymax);

            		issue_rects[i].rect.left = xmin;
            		issue_rects[i].rect.top = ymin;
            		issue_rects[i].rect.right = xmax;
            		issue_rects[i].rect.bottom = ymax;
                }
                int mm=0;
                for(size_t ii=0;ii<issue_rects.size();ii++)
                {
                    for(int k=0;k<15;k++)
                    {
                        if(class_max[k] == 1)
                        {
                             if(((int)issue_rects[ii].classId == k) && k < 8 )
                             {
                                printf("k %d\n",k);
                                p_issue_res->images[i].inference_time = all_time;
                                p_issue_res->images[i].issues[mm].score = issue_rects[ii].confidence;
                                strcpy(p_issue_res->images[i].issues[mm].issue_type,g_class.class_name[k]);
                                p_issue_res->images[i].issues[mm].x = issue_rects[ii].rect.left;
                                p_issue_res->images[i].issues[mm].y = issue_rects[ii].rect.top;
                                p_issue_res->images[i].issues[mm].w = issue_rects[ii].rect.right-issue_rects[ii].rect.left;
                                p_issue_res->images[i].issues[mm].h = issue_rects[ii].rect.bottom-issue_rects[ii].rect.top;
                                mm++;
                                p_issue_res->images[i].issues_cnt++;
                                printf("ID:%d score %f (l,r,t,b)%d %d %d %d\n",issue_rects[ii].classId,issue_rects[ii].confidence,
                                issue_rects[ii].rect.left,issue_rects[ii].rect.right,
                                    issue_rects[ii].rect.top,issue_rects[ii].rect.bottom);
                             }
                        }
                    }


                }
                gettimeofday(&stEnd, NULL);
                all_time = (float)((float)(stEnd.tv_sec - stStart.tv_sec) * 1000000 + (float)(stEnd.tv_usec - stStart.tv_usec));
                printf("back_timer:%f\n", all_time);
            }

        }

        printf("ISSUE_DoTest_Task->%s end!\n", p_data_base_path);
    }
    else
    {
        printf("command:%s err\n", p_issue_info->command);
    }

    unload:
    int rrr = issue_svp_image_unload(model_info);
    if(rrr != 0)
    {
        printf("issue_svp_image_unload  error\n");
        return rrr;
    }
    if(ret)
    {
        return -1;
    }

    return ret;
}

int issue_http_intf_init()
{
    int res = 0;

    memcpy(g_class.class_name,cl_name,sizeof(cl_name));

    // 用户注册    Json 命令回调函数        command :"detect_issue_model"
    res = SC_MPI_SYS_HttpServer_RegProc("detect_issue_model", (SC_MPI_SYS_HttpSVR_ProcJson_Cb)issue_json_proc_cb);

    return res;
}



