#include <random>
#include <execinfo.h>
#include<thread>
#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <opencv2/opencv.hpp>   // OpenCV 头文件

#include "mpi_ive.h"
#include "mpi_sys.h"
#include "sc_comm_svp.h"
#include "sc_npu.h"
#include "mpi_npu.h"

#include "yolov8.h"

#include "sample_svp_postprocess.h"

#define CONFIDENCE_THRESHOLD_V8 0.35f
// #define MAX_DETECT_NUM_V8 8400
#define MAX_DETECT_NUM_V8 2100
#define IOU_THRESHOLD 0.25f

static int m_totalClass = 8;
static int m_regmax = 16;

typedef struct {
    float x, y;
    float w, h;
    float score;
    int cls;
}Body_Box;


typedef struct {
  float x1, y1, x2, y2;
} Bbox;

typedef struct {
  Bbox bbox;
  int row;
  int col;
  int tensoridx;
  float score;
  int clsid;
} DetBox;

static inline int entry_index(int h, int w, int c, int byteUnit, NPU_TENSOR_S &pTensorInfo)
{
	int tensor_k_size_norm = pTensorInfo.u32KSizeNorm;
	int tensor_k_ddr_step = pTensorInfo.u32KStep;
	int tensor_row_ddr_step = pTensorInfo.u32RowStep;
	int tensor_k_norm_num = pTensorInfo.u32KNormNum;
	int tensor_last = pTensorInfo.u32KSizeLast;

	int AddressOffset = (c / tensor_k_size_norm)*tensor_k_ddr_step  + h*tensor_row_ddr_step;

	int memoryMigration = (c <tensor_k_norm_num*tensor_k_size_norm) ? \
		(AddressOffset + c%tensor_k_size_norm*byteUnit + w*tensor_k_size_norm*byteUnit) : (AddressOffset + (c - tensor_k_norm_num*tensor_k_size_norm)*byteUnit + w*tensor_last*byteUnit);
	return memoryMigration / byteUnit;
}

static bool nms_comparator(const DetBox& pa, const DetBox& pb) {
  if (pa.score > pb.score)
    return true;
  else
    return false;
}
static int vec_softmax(std::vector<float>& inputs) {
  float sum = 0;
  for (size_t i = 0; i < inputs.size(); ++i) {
    float v = inputs[i];
    inputs[i] = exp(v);
    sum += inputs[i];
  }
  for (size_t i = 0; i < inputs.size(); ++i) {
    inputs[i] = inputs[i] / sum;
  }
  return 0;
}

static float box_conv(std::vector<float>& inputs) {
  float sum = 0;
  for (size_t i = 0; i < inputs.size(); ++i) {
    sum += (i * inputs[i]);
  }
  return sum;
}
static float box_iou(Bbox a, Bbox b) {
  float inter_left = a.x1 > b.x1 ? a.x1 : b.x1;
  float inter_right = a.x2 < b.x2 ? a.x2 : b.x2;
  float inter_top = a.y1 > b.y1 ? a.y1 : b.y1;
  float inter_bottom = a.y2 < b.y2 ? a.y2 : b.y2;

  float area_a = (a.x2 - a.x1 + 1) * (a.y2 - a.y1 + 1);
  float area_b = (b.x2 - b.x1 + 1) * (b.y2 - b.y1 + 1);
  float inter_w = inter_right - inter_left + 1;
  float inter_h = inter_bottom - inter_top + 1;
  if (inter_w<=1)
    inter_w = 0;
  if (inter_h<=1)
    inter_h = 0;
  float area_inter = inter_w * inter_h;
  return area_inter / (area_a + area_b - area_inter + 1);
}


void do_nms_sort_v8(std::vector<DetBox>& dets, int classes, float thresh) {
  if(dets.size()<= 0){
    return;
  }
  sort(dets.begin(), dets.end(), nms_comparator);

  for (size_t i = 0; i < dets.size() - 1; ++i) {
    if (dets[i].score < 0) continue;
    Bbox a = dets[i].bbox;
    for (size_t j = i + 1; j < dets.size(); ++j) {
      if (dets[i].clsid != dets[j].clsid) continue;
      if (box_iou(a, dets[j].bbox) > thresh) {
        dets[j].score = -1;
      }
    }
  }
  auto iter = dets.begin();
  while (iter != dets.end()) {
    if (iter->score < 0)
      iter = dets.erase(iter);
    else
      iter++;
  }
}


static float sigmoid(float x) {
    return (1 / (1 + exp(-x)));
}


int DFLBox(SVP_NPU_MODEL_S &model_info, auto feature, int tenidx, int h,
                       int w, std::vector<float>& box_coors) {
    int tensor_zero_point = model_info.astDstTensor[tenidx].s32ZeroPoint;
    float tensor_scale_factor =
        model_info.astDstTensor[tenidx].dScaleFactor;
    //int feature_w = model_info.astDstTensor[tenidx].u32Width;
    //int feature_h = model_info.astDstTensor[tenidx].u32Height;
    int index_transform = 0;
    // std::vector<float> box_coors;
    box_coors.clear();
    box_coors.resize(4);
    std::vector<float> box_pred;
    box_pred.resize(m_regmax);
    float bvalue;
    for (int d = 0; d < 4; d++) {
        for (int ri = 0; ri < m_regmax; ri++) {
            index_transform =
                entry_index(h, w, d * m_regmax + ri, sizeof(feature[0]),
                            model_info.astDstTensor[tenidx]);
            if (sizeof(feature[0]) != 4)
                bvalue = (feature[index_transform] - tensor_zero_point) *
                         tensor_scale_factor;
            else
                bvalue = feature[index_transform];
            box_pred[ri] = bvalue;
        }
        vec_softmax(box_pred);
        box_coors[d] = box_conv(box_pred);
    }
    float cx = w + 0.5;
    float cy = h + 0.5;
    box_coors[0] = cx - box_coors[0];
    box_coors[1] = cy - box_coors[1];
    box_coors[2] = cx + box_coors[2];
    box_coors[3] = cy + box_coors[3];
    return 0;
}

int YoloV8BoxDecoding(SVP_NPU_MODEL_S & model_info, auto feature,int tenidx, std::vector<DetBox> &m_dets)
{
    //float x, y, w, h;
    //float pred_conf, clsVaue;
     float clsVaue;

    int feature_w = model_info.astDstTensor[tenidx].u32Width;
    int feature_h = model_info.astDstTensor[tenidx].u32Height;
    int inputTensorW = model_info.astSrcTensor[0].u32Width;
    //int inputTensorH = model_info.astSrcTensor[0].u32Height;
    int count = 0;
    int index_transform;
    int tensor_zero_point = model_info.astDstTensor[tenidx].s32ZeroPoint;
    float tensor_scale_factor =
        model_info.astDstTensor[tenidx].dScaleFactor;
    int stride = inputTensorW / feature_w;
    for (int row = 0; row < feature_h; row++) {
        for (int col = 0; col < feature_w; col++) {
            bool find_box = false;
            float max_score = -1;
	          int max_cid = -1;
            for (int i = 0; i < m_totalClass; i++) {
              index_transform = entry_index(row, col, 4 * m_regmax + i, sizeof(feature[0]), model_info.astDstTensor[tenidx]);
              if (sizeof(feature[0]) != 4)
                clsVaue = sigmoid((feature[index_transform] - tensor_zero_point) * tensor_scale_factor);
              else
                clsVaue = sigmoid(feature[index_transform]);
              if (clsVaue > CONFIDENCE_THRESHOLD_V8) {
                max_score = clsVaue;
                max_cid = i;
                find_box = true;
              }
            }
               // 如果查找到得分高的box，开始解析box位置。
    	      if (find_box)
             {
    	        DetBox buf;
    	        std::vector<float> box;
    	        DFLBox(model_info,feature, tenidx, row, col, box);
    	        for (size_t cb = 0; cb < box.size(); cb++) {
    	          box[cb] *= stride;
    	        }
    	        buf.bbox.x1 = std::max(box[0], 0.0f);
    	        buf.bbox.y1 = std::max(box[1], 0.0f);
    	        buf.bbox.x2 = std::min(box[2], float(model_info.astSrcTensor[0].u32Width - 1));
    	        buf.bbox.y2 = std::min(box[3], float(model_info.astSrcTensor[0].u32Height - 1));

    	        buf.row = row;
    	        buf.col = col;
    	        buf.tensoridx = tenidx;
    	        buf.score = max_score > 1 ? 1 : max_score;
    	        buf.clsid = max_cid;
    	        m_dets.push_back(buf);
    	      }
        }
    }

    return count;
}

int YoloV8PostProcess(SVP_NPU_MODEL_S &model_info, auto buff,std::vector<DetBox> &m_dets)
{
   // int inputTensorW = model_info.astSrcTensor[0].u32Width;
   // int inputTensorH = model_info.astSrcTensor[0].u32Height;

    auto feature_buffer = buff;
    int offset = 0;

    m_dets.clear();
    for (int tensor_index = 0; tensor_index < model_info.u16DstNum; tensor_index++)
    {
        offset = (model_info.astDstTensor[tensor_index].u32Bank * 32 *
                      1024 * 1024 + model_info.astDstTensor[tensor_index].u32Offset);
        int bits = model_info.astDstTensor[tensor_index].u32Precision / 8;
        if (sizeof(buff[0]) == 4) {
            feature_buffer = buff + offset / 4;
        } else {
            feature_buffer = buff + offset / bits;
        }

        YoloV8BoxDecoding(model_info, feature_buffer, tensor_index,m_dets);
    }
    // TODO: NMS
    do_nms_sort_v8(m_dets, m_totalClass, IOU_THRESHOLD);

    return 0;
}

// typedef struct scSAMPLE_SVP_NPU_PARAM_S
// {
//     SVP_MEM_INFO_S          stIntMem;
//     SVP_MEM_INFO_S          stOutMem;
//     SVP_MEM_INFO_S          PictMem;
// } SAMPLE_SVP_NPU_PARAM_S;
// #define ALIGNED_256B(x) ((x)%256 == 0 ? (x) : (((x)/256 + 1) * 256))
// #define ALIGN16KB        (16384)

// extern "C" int get_yolov8_result(void *u64VirAddr,SVP_NPU_MODEL_S model_info,void **pResult, int *ResultNum);

extern "C" int get_yolov8_result(void *u64VirAddr,
                                  SVP_NPU_MODEL_S model_info,
                                  void **pResult, 
                                  int *ResultNum,
                                  float iou_th,
                                  float conf_th,
                                  int class_num);

extern "C" int read_img_by_opencv(const char *pcSrcFile,
                       NPU_IMAGE_S *pstImg,
                       SAMPLE_SVP_NPU_PARAM_S *pstNpuParam,
                       int oriImgWidth,
                       int oriImgHeight,
                       int resizeImgWidth,
                       int resizeImgHeight,
                       int saveFlag,
                       const char* saveFolder);

int read_img_by_opencv(const char *pcSrcFile,
                       NPU_IMAGE_S *pstImg,
                       SAMPLE_SVP_NPU_PARAM_S *pstNpuParam,
                       int oriImgWidth,
                       int oriImgHeight,
                       int resizeImgWidth,
                       int resizeImgHeight,
                       int saveFlag,
                       const char* saveFolder)
{
  
   // printf("----SAMPLE_COMM_SVP_NPU_ReadImgByOpencv-----\n");

    //==============================
    // 1. 从 .rgb 读取原始数据 (HWC, RGB)
    //==============================
    FILE *fp = fopen(pcSrcFile, "rb");
    if (fp == NULL)
    {
        //  SAMPLE_SVP_TRACE_ERR("fopen %s error \n", pcSrcFile);
        return SC_FAILURE;
    }

    size_t srcSize = (size_t)oriImgWidth * oriImgHeight * 3;
    SC_CHAR *srcBuf = (SC_CHAR *)malloc(srcSize);
    if (srcBuf == NULL)
    {
        //  SAMPLE_SVP_TRACE_ERR("malloc error\n");
        fclose(fp);
        return SC_FAILURE;
    }

    size_t readCnt = fread(srcBuf, 1, srcSize, fp);
    fclose(fp);
    if (readCnt != srcSize)
    {
        //  SAMPLE_SVP_TRACE_ERR("fread error, size mismatch\n");
        free(srcBuf);
        return SC_FAILURE;
    }

    // 用原始 RGB 数据构造 Mat（注意：这里的通道顺序就是 RGB）
    cv::Mat rgbImg(oriImgHeight, oriImgWidth, CV_8UC3, srcBuf);

    //=================================================================
    // 【新增功能】saveFlag != 0 时，把原始图片保存到 saveFolder 下
    //=================================================================
    if (saveFlag != 0 && saveFolder != NULL && saveFolder[0] != '\0')
    {
        // (1) 判断目录是否存在，不存在则创建
        struct stat st;
        if (stat(saveFolder, &st) != 0)
        {
            // 目录不存在，尝试创建（只创建一层目录）
            if (mkdir(saveFolder, 0755) != 0)
            {
                // mkdir 失败，可以根据需要打印日志，但不影响后续推理流程
                // fprintf(stderr, "mkdir %s failed\n", saveFolder);
            }
        }
        else
        {
            // 存在但不是目录的情况，也可以根据需要做额外处理
            if (!S_ISDIR(st.st_mode))
            {
                // fprintf(stderr, "%s exists but is not a directory\n", saveFolder);
            }
        }

        // (2) 生成时间格式的文件名 Year_Month_day_hour_min_sec.jpg
        time_t now = time(NULL);
        struct tm tm_now;
        localtime_r(&now, &tm_now);  // Linux/Unix 下的线程安全版本

        char filename[64];
        snprintf(filename, sizeof(filename),
                 "%04d_%02d_%02d_%02d_%02d_%02d.jpg",
                 tm_now.tm_year + 1900,
                 tm_now.tm_mon + 1,
                 tm_now.tm_mday,
                 tm_now.tm_hour,
                 tm_now.tm_min,
                 tm_now.tm_sec);

        // 拼接完整路径 saveFolder + '/' + filename
        char filepath[512];
        size_t len = strlen(saveFolder);
        if (len > 0 && (saveFolder[len - 1] == '/' || saveFolder[len - 1] == '\\'))
        {
            snprintf(filepath, sizeof(filepath), "%s%s", saveFolder, filename);
        }
        else
        {
            snprintf(filepath, sizeof(filepath), "%s/%s", saveFolder, filename);
        }

        // (3) OpenCV 使用 BGR 存图，这里从 RGB 转为 BGR 再存为 jpg
        cv::Mat bgrImg;
        cv::cvtColor(rgbImg, bgrImg, cv::COLOR_RGB2BGR);
        cv::imwrite(filepath, bgrImg);
    }
    //=================================================================
    
    //==============================
    // 2. OpenCV 做 letterbox resize 到 resizeImgWidth x resizeImgHeight
    //==============================
    int dstW = resizeImgWidth;
    int dstH = resizeImgHeight;

    float r = std::min(dstW * 1.0f / oriImgWidth,
                       dstH * 1.0f / oriImgHeight);
    int newUnpadW = (int)std::round(oriImgWidth  * r);
    int newUnpadH = (int)std::round(oriImgHeight * r);

    cv::Mat resizedRGB;
    cv::resize(rgbImg, resizedRGB, cv::Size(newUnpadW, newUnpadH));

    int dw = dstW - newUnpadW;
    int dh = dstH - newUnpadH;

    int top    = dh / 2;
    int bottom = dh - top;
    int left   = dw / 2;
    int right  = dw - left;

    // 填充颜色用 114（YOLO 常用），根据需要可以改
    cv::Mat letterboxRGB;
    cv::copyMakeBorder(resizedRGB, letterboxRGB,
                       top, bottom, left, right,
                       cv::BORDER_CONSTANT,
                       cv::Scalar(114, 114, 114));

#if 0
    cv::Mat letterboxBGR;
    cv::cvtColor(letterboxRGB, letterboxBGR, cv::COLOR_RGB2BGR);
    cv::imwrite("debug_letterbox.jpg", letterboxBGR);
#endif

    free(srcBuf);  // 原始 buf 可以释放了

    //==============================
    // 3. 检查 pstImg 的宽高是否和目标一致
    //==============================
    SC_U32 u32Width  = pstImg->astInputImg[0].stImages[0].u32Width;
    SC_U32 u32Height = pstImg->astInputImg[0].stImages[0].u32Height;
    if (u32Width != (SC_U32)dstW || u32Height != (SC_U32)dstH)
    {
        // SAMPLE_SVP_TRACE_ERR("pstImg size(%u,%u) != resize(%d,%d)\n",u32Width, u32Height, dstW, dstH);
        return SC_FAILURE;
    }

    //==============================
    // 4. HWC(RGB) -> CHW(RGB)
    //==============================
    int width    = dstW;
    int height   = dstH;
    int channels = 3;
    size_t chwSize = (size_t)width * height * channels;

    SC_CHAR *chw_buf = (SC_CHAR *)malloc(chwSize);

    if (chw_buf == NULL)
    {
        //  SAMPLE_SVP_TRACE_ERR("malloc chw_buf error\n");
        return SC_FAILURE;
    }

    unsigned char *srcData = letterboxRGB.data;

    for (int c = 0; c < channels; ++c)
    {
        for (int h = 0; h < height; ++h)
        {
            for (int w = 0; w < width; ++w)
            {
                // HWC 索引
                size_t hwc_idx = (size_t)h * width * channels + w * channels + c;
                // CHW 索引
                size_t chw_idx = (size_t)c * height * width + h * width + w;
                chw_buf[chw_idx] = srcData[hwc_idx];
            }
        }
    }

    //==============================
    // 5. 拷贝到 NPU 连续内存（三平面 + stride 对齐）
    //==============================
    unsigned short u16Stride = ALIGNED_256B(u32Width);

    SC_CHAR *pBuf = (SC_CHAR *)((size_t)pstNpuParam->PictMem.u64VirAddr);

    SC_CHAR *pchR = chw_buf;
    SC_CHAR *pchG = chw_buf + u32Height * u32Width;
    SC_CHAR *pchB = chw_buf + 2 * u32Height * u32Width;

    for (SC_U32 i = 0; i < u32Height; i++)
    {
        // 有效区域拷贝
        for (SC_U32 j = 0; j < u32Width; j++)
        {
            pBuf[i * u16Stride + j]                      = pchR[i * u32Width + j];
            pBuf[(i + u32Height) * u16Stride + j]        = pchG[i * u32Width + j];
            pBuf[(i + 2 * u32Height) * u16Stride + j]    = pchB[i * u32Width + j];
        }
        // stride 右边补 0
        for (SC_U32 j = u32Width; j < u16Stride; j++)
        {
            pBuf[i * u16Stride + j]                      = 0;
            pBuf[(i + u32Height) * u16Stride + j]        = 0;
            pBuf[(i + 2 * u32Height) * u16Stride + j]    = 0;
        }
    }

    //==============================
    // 6. 填充 pstImg 的地址信息
    //==============================
    pstImg->astInputImg[0].stImages[0].au64VirAddr[0] =
        (uintptr_t)(pstNpuParam->PictMem.u64VirAddr);
    pstImg->astInputImg[0].stImages[0].au64VirAddr[1] =
        (uintptr_t)(pstNpuParam->PictMem.u64VirAddr + (SC_U64)u16Stride * u32Height);
    pstImg->astInputImg[0].stImages[0].au64VirAddr[2] =
        (uintptr_t)(pstNpuParam->PictMem.u64VirAddr + (SC_U64)u16Stride * u32Height * 2);
    pstImg->astInputImg[0].stImages[0].au64VirAddr[3] = 0;

    pstImg->astInputImg[0].stImages[0].au64PhyAddr[0] =
        (uintptr_t)(pstNpuParam->PictMem.u64PhyAddr);
    pstImg->astInputImg[0].stImages[0].au64PhyAddr[1] =
        (uintptr_t)(pstNpuParam->PictMem.u64PhyAddr + (SC_U64)u16Stride * u32Height);
    pstImg->astInputImg[0].stImages[0].au64PhyAddr[2] =
        (uintptr_t)(pstNpuParam->PictMem.u64PhyAddr + (SC_U64)u16Stride * u32Height * 2);
    pstImg->astInputImg[0].stImages[0].au64PhyAddr[3] = 0;

    free(chw_buf);

    return SC_SUCCESS;
}

int get_yolov8_result(void *u64VirAddr,
                      SVP_NPU_MODEL_S model_info,
                      void **pResult, 
                      int *ResultNum,
                      float iou_th,
                      float conf_th,
                      int class_num)
{
    std::vector<DetBox> m_dets;

    if (!strcmp("float", model_info.astDstTensor[0].achType))
    {
        YoloV8PostProcess(model_info, (float*)(u64VirAddr),m_dets);
    }
    else
    {
        if ((model_info.astDstTensor[0].u32Precision) == 8)
        {
            YoloV8PostProcess(model_info,(signed char*)(u64VirAddr),m_dets);
        }
        else if ((model_info.astDstTensor[0].u32Precision) == 16)
        {
            YoloV8PostProcess(model_info,(signed short*)(u64VirAddr),m_dets);
        }
    }

    detection_output *p;
    p = (detection_output *)malloc(sizeof(detection_output) * m_dets.size());
    if(NULL == p)
    {
        printf("get_ssd_result malloc mem ERROR\n");
        return -1;
    }
    *pResult = p;
    memset(*pResult, 0x00, sizeof(sizeof(detection_output)*m_dets.size()));

    *ResultNum = m_dets.size();

    for(size_t i=0;i<m_dets.size();i++)
    {
        p[i].classId = m_dets[i].clsid;
        p[i].x = m_dets[i].bbox.x1;
        p[i].y = m_dets[i].bbox.y1;
        p[i].w = m_dets[i].bbox.x2-m_dets[i].bbox.x1;
        p[i].h = m_dets[i].bbox.y2-m_dets[i].bbox.y1;
        p[i].confidence = m_dets[i].score;
    }

    return 0;
}
