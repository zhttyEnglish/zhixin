#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <stdlib.h>
#include <numeric>
#include <cmath>
#include <sys/time.h>

#include "sample_svp_postprocess.h"

#include "yolov3.h"

using namespace std;
int g_yolovx = 0;

float IOU_THRESHOLD ;
float CONFIDENCE_THRESHOLD ;
int ANCHOR_SPECIES = 3;
int MAX_DETCTION_NUMBER;
int anchorSize[3][3 * 2];
int anchorSize_v3[3][3 * 2] =
{
    { 116, 90, 156, 198, 373, 326 },
    { 30, 61, 62, 45, 59, 119 },
    { 10, 13, 16, 30, 33, 23 }
};

int anchorSize_v5[3][3 * 2] =
{
    {10, 13, 16, 30, 33, 23 },
    { 30, 61, 62, 45, 59, 119 },
    { 116, 90, 156, 198, 373, 326 }
};

extern "C" int yolovx_postpross_and_getresult(void *pTensor, int tensorNum, uintptr_t virtAddr, void **pResult,
    int *ResultNum, int yolovx, int w, int h);

int yolovx_init(void )
{
    if(g_yolovx == 3)
    {
        IOU_THRESHOLD  =  0.5f;
        CONFIDENCE_THRESHOLD = 0.5f;
        MAX_DETCTION_NUMBER = (52 * 52 * 3 + 26 * 26 * 3 + 13 * 13 * 3);
        memcpy(anchorSize, anchorSize_v3, sizeof(anchorSize_v3));
    }
    else if(g_yolovx == 5)
    {
        IOU_THRESHOLD = 0.45f;
        CONFIDENCE_THRESHOLD = 0.2f;
        MAX_DETCTION_NUMBER = (80 * 80 * 3 + 40 * 40 * 3 + 20 * 20 * 3);
        memcpy(anchorSize, anchorSize_v5, sizeof(anchorSize_v5));
    }
    else
    {
        return -1;
    }

    return 0;
}

float sigmoid(float x)
{
    return (1 / (1 + exp(-x)));
}

int max_index(float *a, int n)
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

YoloDetection::YoloDetection(output_tensor *pTensorInfo, int tensorNum, int *inputTensorSize, int *anchorSize,
    float confThres, float iouThres)
{
    this->dets = (detection *)malloc(MAX_DETCTION_NUMBER * sizeof(detection));
    this->outputTensorNum = tensorNum;
    this->pTensor = (output_tensor *)malloc(this->outputTensorNum * sizeof(output_tensor));
    memcpy((void *)this->pTensor, pTensorInfo, this->outputTensorNum * sizeof(output_tensor));
    this->anchorSize = (int *)malloc(this->outputTensorNum * ANCHOR_SPECIES * 2 * sizeof(int));
    memcpy((void *)this->anchorSize, anchorSize, this->outputTensorNum * ANCHOR_SPECIES * 2 * sizeof(int));
    memcpy((void *)this->inputTensorSize, inputTensorSize, 2 * sizeof(int));
    this->confThres = confThres;
    this->iouThres = iouThres;

    this->totalClass = (this->pTensor[0].u32OriChannels - ANCHOR_SPECIES * 5) / ANCHOR_SPECIES;
    for (int i = 0; i < MAX_DETCTION_NUMBER; i++)
        (this->dets[i]).prob = (float *)malloc(sizeof(float) * totalClass);

}

YoloDetection::~YoloDetection()
{
    for (int i = 0; i < MAX_DETCTION_NUMBER; i++)
        free((this->dets[i]).prob);
    free(this->dets);
    free(this->pTensor);
    free(this->anchorSize);
}

int YoloDetection::yoloBboxDecoding(short *yolovBuffer, int tensorIndex, detection *detsObj)
{
    //255*13*13  3*(80+5)*13*13
    float x, y, w, h;
    float pred_conf, clsVaue;
    int feature_w = pTensor[tensorIndex].u32Width;
    int feature_h = pTensor[tensorIndex].u32Height;
    //int feature_c = pTensor[tensorIndex].u32OriChannels;
    int inputTensorW = inputTensorSize[0];
    int inputTensorH = inputTensorSize[1];
    int count = 0;
    int index_transform;
    int tensor_zero_point = pTensor[tensorIndex].s32ZeroPoint;
    float tensor_scale_factor = pTensor[tensorIndex].dScaleFactor;

    for (int row = 0; row < feature_h; row++)
    {
        for (int col = 0; col < feature_w; col++)
        {
            for (int i = 0; i < ANCHOR_SPECIES; i++)
            {
                index_transform = entry_index(row, col, i * (totalClass + 5) + 4, sizeof(short), pTensor[tensorIndex]);
                //float x = (yolovBuffer[index_transform] - tensor_zero_point) * tensor_scale_factor;

                pred_conf = sigmoid((yolovBuffer[index_transform] - tensor_zero_point) * tensor_scale_factor);
                //pred_conf = yolov3Buffer[index_transform] * tensor_scale_factor;
                if (pred_conf <= confThres)
                    continue;

                index_transform = entry_index(row, col, i * (totalClass + 5) + 0, sizeof(short), pTensor[tensorIndex]);
                x = (yolovBuffer[index_transform] - tensor_zero_point) * tensor_scale_factor;

                index_transform = entry_index(row, col, i * (totalClass + 5) + 1, sizeof(short), pTensor[tensorIndex]);
                y = (yolovBuffer[index_transform] - tensor_zero_point) * tensor_scale_factor;

                index_transform = entry_index(row, col, i * (totalClass + 5) + 2, sizeof(short), pTensor[tensorIndex]);
                w = (yolovBuffer[index_transform] - tensor_zero_point) * tensor_scale_factor;

                index_transform = entry_index(row, col, i * (totalClass + 5) + 3, sizeof(short), pTensor[tensorIndex]);
                h = (yolovBuffer[index_transform] - tensor_zero_point) * tensor_scale_factor;
                if(3 == g_yolovx)
                {
                    detsObj[count].bbox.x = (sigmoid(x) + col) / feature_w;
                    detsObj[count].bbox.y = (sigmoid(y) + row) / feature_h;
                    detsObj[count].bbox.w = exp(w) * anchorSize[2 * i + 2 * tensorIndex * ANCHOR_SPECIES ] / inputTensorW;
                    detsObj[count].bbox.h = exp(h) * anchorSize[2 * i + 2 * tensorIndex * ANCHOR_SPECIES + 1] / inputTensorH;
                }
                else if(5 == g_yolovx)
                {
                    detsObj[count].bbox.x = (sigmoid(x) * 2.0f - 0.5f + col) / feature_w;
                    detsObj[count].bbox.y = (sigmoid(y) * 2.0f - 0.5f + row) / feature_h;
                    detsObj[count].bbox.w = pow((sigmoid(w) * 2.0f),
                            2) * anchorSize[2 * i + 2 * tensorIndex * ANCHOR_SPECIES ] / inputTensorW;
                    detsObj[count].bbox.h = pow((sigmoid(h) * 2.0f),
                            2) * anchorSize[2 * i + 2 * tensorIndex * ANCHOR_SPECIES + 1] / inputTensorH;
                }

                detsObj[count].objectness = pred_conf;
                detsObj[count].classes = totalClass;
                for (int j = 0; j < detsObj[count].classes; ++j)
                {
                    index_transform = entry_index(row, col, i * (totalClass + 5) + 5 + j, sizeof(short), pTensor[tensorIndex]);
                    clsVaue = sigmoid((yolovBuffer[index_transform] - tensor_zero_point) * tensor_scale_factor);
                    float prob = pred_conf * clsVaue;
                    detsObj[count].prob[j] = (prob > confThres) ? prob : 0;
                }
                ++count;
            }
        }
    }
    return count;
}

int YoloDetection::yoloPostProcess(short *buff, vector<detection_output> &vOutput)
{
    int inputTensorW = inputTensorSize[0];
    int inputTensorH = inputTensorSize[1];
    short *yolovBuffer = NULL;
    int count = 0;
    int offset = 0;

    for (int tensor_index = 0; tensor_index < outputTensorNum; tensor_index++)
    {
        offset = (pTensor[tensor_index].u32Bank * 32 * 1024 * 1024 + pTensor[tensor_index].u32Offset);
        int bits = pTensor[tensor_index].u32Precision / 8;
        yolovBuffer = buff + offset / bits;
        count += this->yoloBboxDecoding(yolovBuffer, tensor_index, &dets[count]);
        // offset +=(pTensor[tensor_index].u32KNormNum+1)*pTensor[tensor_index].u32KStep;
        if (count > MAX_DETCTION_NUMBER)
            return -1;
    }

    do_nms_sort(dets, count, totalClass, this->iouThres);

    detection_output obj;
    for (int i = 0; i < count; i++)
    {
        int const obj_id = max_index(dets[i].prob, totalClass);
        float prob = dets[i].prob[obj_id];
        if (prob > this->confThres)
        {
            obj.x = std::max((float)0, (dets[i].bbox.x - dets[i].bbox.w / 2) * inputTensorW);
            obj.x = std::min(obj.x, (float)inputTensorW);
            obj.y = std::max((float)0, (dets[i].bbox.y - dets[i].bbox.h / 2) * inputTensorH);
            obj.y = std::min(obj.y, (float)inputTensorH);

            obj.w = dets[i].bbox.w * inputTensorW;
            obj.h = dets[i].bbox.h * inputTensorH;
            obj.confidence = prob;
            obj.classId = obj_id;
            vOutput.push_back(obj);
        }
    }

    return 0;
}

int YoloDetection::yoloBboxDecoding(signed char *yolovBuffer, int tensorIndex, detection *detsObj)
{
    //255*13*13  3*(80+5)*13*13
    float x, y, w, h;
    float pred_conf, clsVaue;
    int feature_w = pTensor[tensorIndex].u32Width;
    int feature_h = pTensor[tensorIndex].u32Height;

    //int feature_c = pTensor[tensorIndex].u32OriChannel;
    int inputTensorW = inputTensorSize[0];
    int inputTensorH = inputTensorSize[1];
    int count = 0;
    int index_transform;
    int tensor_zero_point = pTensor[tensorIndex].s32ZeroPoint;
    float tensor_scale_factor = pTensor[tensorIndex].dScaleFactor;

    //printf("tensor_scale_factor %f %d\n",tensor_scale_factor,tensor_zero_point);

    for (int row = 0; row < feature_h; row++)
    {
        for (int col = 0; col < feature_w; col++)
        {
            for (int i = 0; i < ANCHOR_SPECIES; i++)
            {
                index_transform = entry_index(row, col, i * (totalClass + 5) + 4, sizeof(char), pTensor[tensorIndex]);
                pred_conf = sigmoid((yolovBuffer[index_transform] - tensor_zero_point) * tensor_scale_factor);
                //pred_conf = yolov3Buffer[index_transform] * tensor_scale_factor;
                //printf("pred_conf %f yolovBuffer[index_transform] %x index_transform %d\n",pred_conf,yolovBuffer[index_transform],index_transform);
                if (pred_conf <= confThres)
                    continue;

                index_transform = entry_index(row, col, i * (totalClass + 5) + 0, sizeof(char), pTensor[tensorIndex]);
                x = (yolovBuffer[index_transform] - tensor_zero_point) * tensor_scale_factor;

                index_transform = entry_index(row, col, i * (totalClass + 5) + 1, sizeof(char), pTensor[tensorIndex]);
                y = (yolovBuffer[index_transform] - tensor_zero_point) * tensor_scale_factor;

                index_transform = entry_index(row, col, i * (totalClass + 5) + 2, sizeof(char), pTensor[tensorIndex]);
                w = (yolovBuffer[index_transform] - tensor_zero_point) * tensor_scale_factor;

                index_transform = entry_index(row, col, i * (totalClass + 5) + 3, sizeof(char), pTensor[tensorIndex]);
                h = (yolovBuffer[index_transform] - tensor_zero_point) * tensor_scale_factor;

                if(3 == g_yolovx)
                {
                    detsObj[count].bbox.x = (sigmoid(x) + col) / feature_w;
                    detsObj[count].bbox.y = (sigmoid(y) + row) / feature_h;
                    detsObj[count].bbox.w = exp(w) * anchorSize[2 * i + 2 * tensorIndex * ANCHOR_SPECIES] / inputTensorW;
                    detsObj[count].bbox.h = exp(h) * anchorSize[2 * i + 2 * tensorIndex * ANCHOR_SPECIES + 1] / inputTensorH;
                }
                else if(5 == g_yolovx)
                {
                    detsObj[count].bbox.x = (sigmoid(x) * 2.0f - 0.5f + col) / feature_w;
                    detsObj[count].bbox.y = (sigmoid(y) * 2.0f - 0.5f + row) / feature_h;
                    detsObj[count].bbox.w = pow((sigmoid(w) * 2.0f),
                            2) * anchorSize[2 * i + 2 * tensorIndex * ANCHOR_SPECIES ] / inputTensorW;
                    detsObj[count].bbox.h = pow((sigmoid(h) * 2.0f),
                            2) * anchorSize[2 * i + 2 * tensorIndex * ANCHOR_SPECIES + 1] / inputTensorH;
                }

                detsObj[count].objectness = pred_conf;
                detsObj[count].classes = totalClass;
                for (int j = 0; j < detsObj[count].classes; ++j)
                {
                    index_transform = entry_index(row, col, i * (totalClass + 5) + 5 + j, sizeof(char), pTensor[tensorIndex]);
                    clsVaue = sigmoid((yolovBuffer[index_transform] - tensor_zero_point) * tensor_scale_factor);
                    float prob = pred_conf * clsVaue;
                    detsObj[count].prob[j] = (prob > confThres) ? prob : 0;
                }
                ++count;
            }
        }
    }
    return count;
}

int YoloDetection::yoloBboxDecoding(float *yolovBuffer, int tensorIndex, detection *detsObj)
{
    //255*13*13  3*(80+5)*13*13
    float x, y, w, h;
    float pred_conf, clsVaue;
    int feature_w = pTensor[tensorIndex].u32Width;
    int feature_h = pTensor[tensorIndex].u32Height;
    //int feature_c = pTensor[tensorIndex].u32OriChannel;
    int inputTensorW = inputTensorSize[0];
    int inputTensorH = inputTensorSize[1];
    int count = 0;
    int index_transform;
    //int tensor_zero_point = pTensor[tensorIndex].s32ZeroPoint;
    //float tensor_scale_factor = pTensor[tensorIndex].dScaleFactor;
    for (int row = 0; row < feature_h; row++)
    {
        for (int col = 0; col < feature_w; col++)
        {
            for (int i = 0; i < ANCHOR_SPECIES; i++)
            {
                index_transform = entry_index(row, col, i * (totalClass + 5) + 4, sizeof(float), pTensor[tensorIndex]);
                pred_conf = sigmoid(yolovBuffer[index_transform]);
                //printf("pred_conf %f\n",pred_conf);
                //pred_conf = yolov3Buffer[index_transform] * tensor_scale_factor;
                if (pred_conf <= confThres)
                    continue;

                index_transform = entry_index(row, col, i * (totalClass + 5) + 0, sizeof(float), pTensor[tensorIndex]);
                x = (yolovBuffer[index_transform]);

                index_transform = entry_index(row, col, i * (totalClass + 5) + 1, sizeof(float), pTensor[tensorIndex]);
                y = (yolovBuffer[index_transform]);

                index_transform = entry_index(row, col, i * (totalClass + 5) + 2, sizeof(float), pTensor[tensorIndex]);
                w = (yolovBuffer[index_transform]);

                index_transform = entry_index(row, col, i * (totalClass + 5) + 3, sizeof(float), pTensor[tensorIndex]);
                h = (yolovBuffer[index_transform]);

                if(3 == g_yolovx)
                {
                    detsObj[count].bbox.x = (sigmoid(x) + col) / feature_w;
                    detsObj[count].bbox.y = (sigmoid(y) + row) / feature_h;
                    detsObj[count].bbox.w = exp(w) * anchorSize[2 * i + 2 * tensorIndex * ANCHOR_SPECIES] / inputTensorW;
                    detsObj[count].bbox.h = exp(h) * anchorSize[2 * i + 2 * tensorIndex * ANCHOR_SPECIES + 1] / inputTensorH;
                }
                else if(5 == g_yolovx)
                {
                    detsObj[count].bbox.x = (sigmoid(x) * 2.0f - 0.5f + col) / feature_w;
                    detsObj[count].bbox.y = (sigmoid(y) * 2.0f - 0.5f + row) / feature_h;
                    detsObj[count].bbox.w = pow((sigmoid(w) * 2.0f),
                            2) * anchorSize[2 * i + 2 * tensorIndex * ANCHOR_SPECIES ] / inputTensorW;
                    detsObj[count].bbox.h = pow((sigmoid(h) * 2.0f),
                            2) * anchorSize[2 * i + 2 * tensorIndex * ANCHOR_SPECIES + 1] / inputTensorH;
                }

                detsObj[count].objectness = pred_conf;
                detsObj[count].classes = totalClass;
                for (int j = 0; j < detsObj[count].classes; ++j)
                {
                    index_transform = entry_index(row, col, i * (totalClass + 5) + 5 + j, sizeof(float), pTensor[tensorIndex]);
                    clsVaue = sigmoid(yolovBuffer[index_transform]);
                    float prob = pred_conf * clsVaue;
                    detsObj[count].prob[j] = (prob > confThres) ? prob : 0;
                }
                ++count;
                //printf("count %d\n",count);
            }
        }
    }
    return count;
}

int YoloDetection::yoloPostProcess(signed char *buff, vector<detection_output> &vOutput)
{
    int inputTensorW = inputTensorSize[0];
    int inputTensorH = inputTensorSize[1];
    signed char *yolovBuffer = NULL;
    int count = 0;
    int offset = 0;
    //float iouThres = 0.5f;
    //printf("confThres %f\n",this->confThres);
    for (int tensor_index = 0; tensor_index < outputTensorNum; tensor_index++)
    {
        int bits = pTensor[tensor_index].u32Precision / 8;
        offset = (pTensor[tensor_index].u32Bank * 32 * 1024 * 1024 + pTensor[tensor_index].u32Offset);
        yolovBuffer = buff + offset / bits;

        count += this->yoloBboxDecoding(yolovBuffer, tensor_index, &dets[count]);
        // offset +=(pTensor[tensor_index].u32KNormNum+1)*pTensor[tensor_index].u32KStep;
        if (count > MAX_DETCTION_NUMBER)
            return -1;
        //anchorSize += ANCHOR_SPECIES * 2;
    }

    do_nms_sort(dets, count, totalClass, this->iouThres);
    detection_output obj;
    for (int i = 0; i < count; i++)
    {
        int const obj_id = max_index(dets[i].prob, totalClass);
        float prob = dets[i].prob[obj_id];
        if (prob > this->confThres)
        {
            obj.x = std::max((float)0, (dets[i].bbox.x - dets[i].bbox.w / 2) * inputTensorW);
            obj.x = std::min(obj.x, (float)inputTensorW);
            obj.y = std::max((float)0, (dets[i].bbox.y - dets[i].bbox.h / 2) * inputTensorH);
            obj.y = std::min(obj.y, (float)inputTensorH);

            obj.w = dets[i].bbox.w * inputTensorW;
            obj.h = dets[i].bbox.h * inputTensorH;
            obj.confidence = prob;
            obj.classId = obj_id;
            vOutput.push_back(obj);
        }
    }

    return 0;
}

int YoloDetection::yoloPostProcess(float *buff, vector<detection_output> &vOutput)
{
    int inputTensorW = inputTensorSize[0];
    int inputTensorH = inputTensorSize[1];
    float *yolovBuffer = NULL;
    int count = 0;
    int offset = 0;
    //float iouThres = 0.5f;
    for (int tensor_index = 0; tensor_index < outputTensorNum; tensor_index++)
    {
        offset = (pTensor[tensor_index].u32Bank * 32 * 1024 * 1024 + pTensor[tensor_index].u32Offset) / sizeof(float);
        yolovBuffer = buff + offset;
        count += this->yoloBboxDecoding(yolovBuffer, tensor_index, &dets[count]);
        if (count > MAX_DETCTION_NUMBER)
            return -1;
        //anchorSize += ANCHOR_SPECIES * 2;
    }

    do_nms_sort(dets, count, totalClass, this->iouThres);

    detection_output obj;
    for (int i = 0; i < count; i++)
    {
        int const obj_id = max_index(dets[i].prob, totalClass);
        float prob = dets[i].prob[obj_id];
        if (prob > this->confThres)
        {
            obj.x = std::max((float)0, (dets[i].bbox.x - dets[i].bbox.w / 2) * inputTensorW);
            obj.x = std::min(obj.x, (float)inputTensorW);
            obj.y = std::max((float)0, (dets[i].bbox.y - dets[i].bbox.h / 2) * inputTensorH);
            obj.y = std::min(obj.y, (float)inputTensorH);

            obj.w = dets[i].bbox.w * inputTensorW;
            obj.h = dets[i].bbox.h * inputTensorH;
            obj.confidence = prob;
            obj.classId = obj_id;
            vOutput.push_back(obj);
        }
    }

    return 0;
}

int YoloDetection::yoloCHWBboxDecoding(float *yolovBuffer, int tensorIndex, detection *detsObj)
{
    //255*13*13  3*(80+5)*13*13
    float x, y, w, h;
    float pred_conf, clsVaue;
    int feature_w = pTensor[tensorIndex].u32Width;
    int feature_h = pTensor[tensorIndex].u32Height;
    //int feature_c = pTensor[tensorIndex].u32OriChannel;
    int inputTensorW = inputTensorSize[0];
    int inputTensorH = inputTensorSize[1];
    int count = 0;
    int index_transform;
    for (int row = 0; row < feature_h; row++)
    {
        for (int col = 0; col < feature_w; col++)
        {
            for (int i = 0; i < ANCHOR_SPECIES; i++)
            {
                index_transform = chw_entry_index(row, col, i * (totalClass + 5) + 4, pTensor[tensorIndex]);
                pred_conf = sigmoid(yolovBuffer[index_transform]);
                //pred_conf = yolov3Buffer[index_transform] * tensor_scale_factor;
                if (pred_conf <= confThres)
                    continue;
                index_transform = chw_entry_index(row, col, i * (totalClass + 5) + 0, pTensor[tensorIndex]);
                x = (yolovBuffer[index_transform]);

                index_transform = chw_entry_index(row, col, i * (totalClass + 5) + 1, pTensor[tensorIndex]);
                y = (yolovBuffer[index_transform]);

                index_transform = chw_entry_index(row, col, i * (totalClass + 5) + 2, pTensor[tensorIndex]);
                w = (yolovBuffer[index_transform]);

                index_transform = chw_entry_index(row, col, i * (totalClass + 5) + 3, pTensor[tensorIndex]);
                h = (yolovBuffer[index_transform]) ;

                detsObj[count].bbox.x = (sigmoid(x) + col) / feature_w;
                detsObj[count].bbox.y = (sigmoid(y) + row) / feature_h;
                detsObj[count].bbox.w = exp(w) * anchorSize[2 * i + 2 * tensorIndex * ANCHOR_SPECIES] / inputTensorW;
                detsObj[count].bbox.h = exp(h) * anchorSize[2 * i + 2 * tensorIndex * ANCHOR_SPECIES + 1] / inputTensorH;

                detsObj[count].objectness = pred_conf;
                detsObj[count].classes = totalClass;
                for (int j = 0; j < detsObj[count].classes; ++j)
                {
                    index_transform = chw_entry_index(row, col, i * (totalClass + 5) + 5 + j, pTensor[tensorIndex]);
                    clsVaue = sigmoid(yolovBuffer[index_transform]);
                    float prob = pred_conf * clsVaue;
                    detsObj[count].prob[j] = (prob > confThres) ? prob : 0;
                }
                ++count;
            }
        }
    }
    return count;
}

int YoloDetection::yoloCHWPostProcess(float *buff, vector<detection_output> &vOutput)
{
    int inputTensorW = inputTensorSize[0];
    int inputTensorH = inputTensorSize[1];
    float *yolovBuffer = NULL;
    int count = 0;
    int offset = 0;
    for (int tensor_index = 0; tensor_index < outputTensorNum; tensor_index++)
    {
        yolovBuffer = buff + offset;
        count += this->yoloCHWBboxDecoding(yolovBuffer, tensor_index, &dets[count]);
        offset += pTensor[tensor_index].u32Height * pTensor[tensor_index].u32Width * pTensor[tensor_index].u32OriChannels;
        if (count > MAX_DETCTION_NUMBER)
            return -1;
        //anchorSize += ANCHOR_SPECIES * 2;
    }
    do_nms_sort(dets, count, totalClass, 0.5f);
    detection_output obj;
    for (int i = 0; i < count; i++)
    {
        int const obj_id = max_index(dets[i].prob, totalClass);
        float prob = dets[i].prob[obj_id];
        if (prob > 0)
        {
            obj.x = max(round((dets[i].bbox.x - dets[i].bbox.w / 2) * inputTensorW), 0.0f);
            obj.y = max(round((dets[i].bbox.y - dets[i].bbox.h / 2) * inputTensorH), 0.0f);
            obj.w = dets[i].bbox.w * inputTensorW;
            obj.h = dets[i].bbox.h * inputTensorH;
            obj.confidence = prob;
            obj.classId = obj_id;
            vOutput.push_back(obj);
        }
    }
    return 0;
}

int yolovx_postpross_and_getresult(void *pTensor, int tensorNum, uintptr_t virtAddr, void **pResult, int *ResultNum,
    int yolovx, int w, int h)
{
    g_yolovx = yolovx;
    int ret = yolovx_init();
    if(0 != ret)
    {
        return -1;
    }

    output_tensor *ptensor = (output_tensor *)pTensor;
    vector<detection_output> detectionOutputVector;
    int inputTensorSize[2];

    inputTensorSize[0] = w;
    inputTensorSize[1] = h;

    YoloDetection yolo = YoloDetection(ptensor, tensorNum, inputTensorSize, &anchorSize[0][0], CONFIDENCE_THRESHOLD,
            IOU_THRESHOLD);

    if(!strcmp("float", ptensor[0].achType))
    {
        yolo.yoloPostProcess((float *)virtAddr, detectionOutputVector);
    }
    else
    {
        if((ptensor[0].u32Precision) == 8)
        {
            yolo.yoloPostProcess((signed char *)virtAddr, detectionOutputVector);
        }
        else if((ptensor[0].u32Precision) == 16)
        {
            yolo.yoloPostProcess((short *)virtAddr, detectionOutputVector);
        }
    }

    if(detectionOutputVector.empty())
    {
        *pResult = NULL;
        *ResultNum = 0;
    }
    else
    {
        detection_output *p;
        p = (detection_output *)malloc(sizeof(detection_output) * detectionOutputVector.size());
        if(NULL == p)
        {
            printf("get_ssd_result malloc mem ERROR\n");
            return -1;
        }
        *pResult = p;
        memset(*pResult, 0x00, sizeof(sizeof(detection_output)*detectionOutputVector.size()));

        *ResultNum = detectionOutputVector.size();
        int i = 0;
        for (auto iter = detectionOutputVector.begin(); iter != detectionOutputVector.end(); iter++)
        {
            p[i].classId = iter->classId;
            p[i].x = iter->x;
            p[i].y = iter->y;
            p[i].w = iter->w;
            p[i].h = iter->h;
            p[i].confidence = iter->confidence;
            i++;
        }
    }

    return 0;
}

