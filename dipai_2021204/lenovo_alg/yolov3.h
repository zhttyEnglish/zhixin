#include<string>
#include<vector>

#include "sample_svp_postprocess.h"

using namespace std;
#ifndef __YOLO_LEYER_H_
#define __YOLO_LEYER_H_

class YoloDetection
{
public:
    YoloDetection(output_tensor *pTensorInfo, int tensorNum,     int *inputTensorSize, int *anchorSize, float confThres,
        float confThres1);
    ~YoloDetection();
    int yoloBboxDecoding(short *yolovBuffer, int tensorIndex, detection *detsObj);
    int yoloBboxDecoding(signed char *yolovBuffer, int tensorIndex, detection *detsObj);
    int yoloBboxDecoding(float *yolovBuffer, int tensorIndex, detection *detsObj);
    int yoloPostProcess(short *buff, vector<detection_output> &vOutput);
    int yoloPostProcess(signed char *buff, vector<detection_output> &vOutput);
    int yoloPostProcess(float *buff, vector<detection_output> &vOutput);
    int yoloCHWBboxDecoding(float *yolovBuffer, int tensorIndex, detection *detsObj);
    int yoloCHWPostProcess(float *buff, vector<detection_output> &vOutput);
private:
    float confThres;
    float iouThres;
    int outputTensorNum;
    output_tensor *pTensor;
    int *anchorSize;
    detection *dets;
    int inputTensorSize[2];
    int totalClass;
};

#endif
