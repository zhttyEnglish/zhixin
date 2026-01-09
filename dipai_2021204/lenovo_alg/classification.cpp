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

#include "classification.h"
#include "sample_svp_postprocess.h"
using namespace std;

Classification::Classification(output_tensor *pTensorInfo)
{
    this->tensorNum = 1;
    this->pTensor = pTensorInfo;
    int malloc_size = sizeof(output_tensor);
    this->pTensorInfo = (output_tensor *)malloc(malloc_size);
    memcpy((void *)this->pTensorInfo, pTensorInfo, malloc_size);
}

Classification::~Classification()
{
    free(this->pTensorInfo);
}

static int cmp_pair(pair<int, double>a, pair<int, double>b)
{
    return a.second > b.second;
}

int Classification::classificationPostProcess(char *buff, vector<classification_output> &vOutput)
{
    std::vector<std::pair<int, double>> classfer;

    int oriChannels = pTensor->u32OriChannels;
    //double scale_factor = pTensor->dScaleFactor;

    vector<double> classiferVector;
    int point_number = 0;
    //double pointwise_mul_sum;
    //double quant_self_square_sum;
    //double float_self_square_sum;
    //double qf_diff_square_sum;
    char *buffQuantCurrentChar = NULL;
    short *buffQuantCurrentShort = NULL;

    __attribute__((__unused__)) int last_tensorBank = 0;
    __attribute__((__unused__)) int last_tensorBankOffset = 0;
    //int offsetFloat=0;
    int offsetQuant = 0;
    //int start_offset=this->pTensorInfo[0].u32Offset;
    for(int n = 0; n < 1; n++)
    {
        output_tensor tensor = this->pTensorInfo[n];
        int channel = tensor.u32OriChannels;
        int width = tensor.u32Width;
        int height = tensor.u32Height;
        int precision = tensor.u32Precision;
        int tensor_size = tensor.u32Size;
        int bits = precision / 8;
        // cout<<bits<<endl;

        if(precision == 8)
        {
            buffQuantCurrentChar = (char *)(buff + offsetQuant);
        }
        else
        {
            buffQuantCurrentShort = (short *)(buff + offsetQuant);
        }

        offsetQuant += tensor_size * bits;

        last_tensorBank = tensor.u32Bank;
        last_tensorBankOffset = tensor.u32Offset;

        point_number += width * height * channel;
        for (int h = 0; h < height; h++)
        {
            for (int w = 0; w < width; w++)
            {
                for (int c = 0; c < channel; c++)
                {
                    if(precision == 8)
                    {
                        int index_transform_quant = entry_index(h, w, c, sizeof(char), tensor);
                        double quanVal = (buffQuantCurrentChar[index_transform_quant] - tensor.s32ZeroPoint) * tensor.dScaleFactor;
                        classiferVector.push_back(quanVal);
                    }
                    else
                    {
                        int index_transform_quant = entry_index(h, w, c, sizeof(short), tensor);
                        double quanVal = (buffQuantCurrentShort[index_transform_quant] - tensor.s32ZeroPoint) * tensor.dScaleFactor;
                        classiferVector.push_back(quanVal);
                    }
                }
            }
        }

    }
    double softmaxSum = 0;
    for (int i = 0; i < oriChannels; i++)
    {
        softmaxSum += classiferVector[i];
    }

    for (int i = 0; i < oriChannels; i++)
    {
        classfer.emplace_back(i, classiferVector[i] / softmaxSum);
    }

    //find max top 5
    std::sort(std::begin(classfer), std::end(classfer), cmp_pair);

    classification_output obj;
    //1. If confidence is zero, ignore it
    //2. If the fifth and sixth are equal, both are used.
    for(unsigned int i = 0; i < classfer.size(); i++)
    {
        obj.classId = classfer[i].first;
        obj.confidence = classfer[i].second;

        if(classfer[i].second == 0)
            break;

        if(i >= 5)
        {
            if(classfer[i].second == classfer[4].second)
                vOutput.push_back(obj);
            else
                break;
        }
        else
        {
            vOutput.push_back(obj);
        }
    }

    return 0;
}

int Classification::classificationPostProcess(short *buff, vector<classification_output> &vOutput)
{
    std::vector<std::pair<int, double>> classfer;

    int oriChannels = pTensor->u32OriChannels;
    //double scale_factor = pTensor->dScaleFactor;

    vector<double> classiferVector;

    int point_number = 0;
    //double pointwise_mul_sum;
    //double quant_self_square_sum;
    //double float_self_square_sum;
    //double qf_diff_square_sum;
    char *buffQuantCurrentChar = NULL;
    short *buffQuantCurrentShort = NULL;

    __attribute__((__unused__)) int last_tensorBank = 0;
    __attribute__((__unused__)) int last_tensorBankOffset = 0;
    //int offsetFloat=0;
    int offsetQuant = 0;
    //int start_offset=this->pTensorInfo[0].u32Offset;
    for(int n = 0; n < 1; n++)
    {
        output_tensor tensor = this->pTensorInfo[n];
        int channel = tensor.u32OriChannels;
        int width = tensor.u32Width;
        int height = tensor.u32Height;
        int precision = tensor.u32Precision;

        int bits = precision / 8;

        offsetQuant = tensor.u32Offset / bits;

        if(precision == 8)
        {
            buffQuantCurrentChar = (char *)(buff + offsetQuant);
        }
        else
        {
            buffQuantCurrentShort = (short *)(buff + offsetQuant);
        }

        last_tensorBank = tensor.u32Bank;
        last_tensorBankOffset = tensor.u32Offset;

        point_number += width * height * channel;
        for (int h = 0; h < height; h++)
        {
            for (int w = 0; w < width; w++)
            {
                for (int c = 0; c < channel; c++)
                {
                    if(precision == 8)
                    {
                        int index_transform_quant = entry_index(h, w, c, sizeof(char), tensor);
                        double quanVal = (buffQuantCurrentChar[index_transform_quant] - tensor.s32ZeroPoint) * tensor.dScaleFactor;
                        classiferVector.push_back(quanVal);
                    }
                    else
                    {
                        int index_transform_quant = entry_index(h, w, c, sizeof(short), tensor);
                        double quanVal = (buffQuantCurrentShort[index_transform_quant] - tensor.s32ZeroPoint) * tensor.dScaleFactor;
                        classiferVector.push_back(quanVal);
                    }
                }
            }
        }

    }

    double softmaxSum = 0;
    for (int i = 0; i < oriChannels; i++)
    {
        softmaxSum += classiferVector[i];
    }

    for (int i = 0; i < oriChannels; i++)
    {
        classfer.emplace_back(i, classiferVector[i] / softmaxSum);
    }

    //find max top 5
    std::sort(std::begin(classfer), std::end(classfer), cmp_pair);

    classification_output obj;
    //1. If confidence is zero, ignore it
    //2. If the fifth and sixth are equal, both are used.
    for(unsigned int i = 0; i < classfer.size(); i++)
    {
        obj.classId = classfer[i].first;
        obj.confidence = classfer[i].second;

        if(classfer[i].second == 0)
            break;

        if(i >= 5)
        {
            if(classfer[i].second == classfer[4].second)
                vOutput.push_back(obj);
            else
                break;
        }
        else
        {
            vOutput.push_back(obj);
        }
    }

    return 0;
}

int Classification::classificationPostProcess(float *buff, vector<classification_output> &vOutput)
{
    std::vector<std::pair<int, double>> classfer;

    int oriChannels = pTensor->u32OriChannels;

    float *classiferBuffer = (float *)buff;
    vector<float> classiferVector(classiferBuffer, classiferBuffer + oriChannels );

    double softmaxSum = 0;
    for (int i = 0; i < oriChannels; i++)
    {
        softmaxSum += (classiferVector[i]) ;
    }

    for (int i = 0; i < oriChannels; i++)
    {
        classfer.emplace_back(i, (classiferVector[i]) / softmaxSum);
    }

    //find max top 5
    std::sort(std::begin(classfer), std::end(classfer), cmp_pair);

    classification_output obj;
    //1. If confidence is zero, ignore it
    //2. If the fifth and sixth are equal, both are used.
    for(unsigned int i = 0; i < classfer.size(); i++)
    {
        obj.classId = classfer[i].first;
        obj.confidence = classfer[i].second;

        if(classfer[i].second == 0)
            break;

        if(i >= 5)
        {
            if(classfer[i].second == classfer[4].second)
                vOutput.push_back(obj);
            else
                break;
        }
        else
        {
            vOutput.push_back(obj);
        }
    }

    return 0;
}

int Classification::classificationChwPostProcess(float *buff, vector<classification_output> &vOutput)
{
    std::vector<std::pair<int, double>> classfer;

    int oriChannels = pTensor->u32OriChannels;

    float *classiferBuffer = (float *)buff;
    vector<float> classiferVector(classiferBuffer, classiferBuffer + oriChannels );
    /*
    double softmaxSum = 0;
    for (int i = 0; i < oriChannels; i++) {
        softmaxSum += (classiferVector[i]) ;
    }

    for (int i = 0; i < oriChannels; i++) {
        classfer.emplace_back(i, (classiferVector[i])/softmaxSum);
    }
    */

    //find max top 5
    std::sort(std::begin(classfer), std::end(classfer), cmp_pair);

    classification_output obj;
    //1. If confidence is zero, ignore it
    //2. If the fifth and sixth are equal, both are used.
    for(unsigned int i = 0; i < classfer.size(); i++)
    {
        obj.classId = classfer[i].first;
        obj.confidence = classfer[i].second;

        if(classfer[i].second == 0)
            break;

        if(i >= 5)
        {
            if(classfer[i].second == classfer[4].second)
                vOutput.push_back(obj);
            else
                break;
        }
        else
        {
            vOutput.push_back(obj);
        }
    }

    return 0;
}

extern "C" int dcnn_postpross_and_getresult(void *pTensor, int tensorNum, uintptr_t virtAddr, void **pResult,
    int *ResultNum);
int dcnn_postpross_and_getresult(void *pTensor, int tensorNum, uintptr_t virtAddr, void **pResult, int *ResultNum)
{
    output_tensor *ptensor = (output_tensor *)pTensor;

    Classification classfer(&ptensor[0]);
    vector<classification_output> output;

    if(ptensor[0].u32Precision == 8)
    {
        classfer.classificationPostProcess((char *)virtAddr, output);
    }
    else if(ptensor[0].u32Precision == 16)
    {
        classfer.classificationPostProcess((short *)virtAddr, output);
    }

    if(output.empty())
    {
        *pResult = NULL;
        *ResultNum = 0;
    }
    else
    {
        classification_output *p;

        p = (classification_output *)malloc(sizeof(classification_output) * output.size());
        if(NULL == p)
        {
            printf("get_ssd_result malloc mem ERROR\n");
            return -1;
        }

        *pResult = p;

        memset(*pResult, 0x00, sizeof(sizeof(classification_output)*output.size()));

        *ResultNum = output.size();
        int i = 0;

        for (auto iter = output.begin(); iter != output.end(); iter++)
        {
            p[i].classId = iter->classId;
            p[i].confidence = iter->confidence;
            i++;
        }
    }

    return 0;
}
