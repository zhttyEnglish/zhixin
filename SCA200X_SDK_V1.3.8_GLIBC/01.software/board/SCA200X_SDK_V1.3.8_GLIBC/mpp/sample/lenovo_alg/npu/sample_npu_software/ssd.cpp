#include <vector>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <algorithm>
#include <functional>
#include <math.h>
#include <iostream>
#include <fstream>

#include "ssd.h"
#include "sample_svp_postprocess.h"

using namespace std;

#define NET_WIDTH  300
#define NET_HEIGHT 300
#define CLASS_NUM  21
#define IOU_THRESHOLD         0.4f
#define CONFIDENCE_THRESHOLD  0.5f

SSD_detection::SSD_detection(NET_TYPE net_type, output_tensor *outputs_tensor, int tensorNum, int class_num = 21,
    float nms_threshold = 0.5f, float dect_threshold = 0.45f, int input_width = 300, int input_height = 300)
{
    this->net_type = net_type;
    this->SetDetectParam(class_num, input_width, input_height, nms_threshold, dect_threshold);
    decodeSort.values = new float[dect_param.windows_num];
    decodeSort.index = new int[dect_param.windows_num];
    decodeSort.sort = new int[dect_param.windows_num];
    softmaxScore = new float[dect_param.class_num];
    nms.keep = new int[dect_param.nms_overlap_top_num];
    nms.scores = new float[dect_param.nms_overlap_top_num];

    scores_flat = new float[dect_param.class_num * dect_param.windows_num];

    deltas_flat = new float[dect_param.per_box_num * dect_param.windows_num];
    this->tensorNum = tensorNum;
    this->pTensor = (output_tensor *)malloc(tensorNum * sizeof(output_tensor));
    memcpy(this->pTensor, outputs_tensor, tensorNum * sizeof(output_tensor));

    priorbox.resize(dect_param.windows_num, vector<float>(dect_param.per_box_num, 0.));
    this->GetPriorBoxesSSD();
}

SSD_detection::~SSD_detection()
{
    delete[]decodeSort.values;
    delete[]decodeSort.index;
    delete[]decodeSort.sort;
    delete[]softmaxScore;
    delete[]scores_flat;
    delete[]deltas_flat;
    delete[]nms.keep;
    delete[]nms.scores;
    free(this->pTensor);
}

void SSD_detection::fixedToFloatHWCSSD(float *dst, short *src, output_tensor &tensor, int index)
{
    int channel = tensor.u32OriChannels;
    int width = tensor.u32Width;
    int height = tensor.u32Height;
    int index_transform;

    for (int h = 0; h < height; h++)
    {
        for (int w = 0; w < width; w++)
        {
            for (int c = 0; c < channel; c++)
            {
                index_transform = entry_index(h, w, c, sizeof(short), tensor);
                dst[index + h * width * channel + w * channel + c] = (src[index_transform] - tensor.s32ZeroPoint) * tensor.dScaleFactor;
            }
        }
    }
}

void SSD_detection::fixedToFloatHWCSSD(float *dst, char *src, output_tensor &tensor, int index)
{
    int channel = tensor.u32OriChannels;
    int width = tensor.u32Width;
    int height = tensor.u32Height;
    int index_transform;

    for (int h = 0; h < height; h++)
    {
        for (int w = 0; w < width; w++)
        {
            for (int c = 0; c < channel; c++)
            {
                index_transform = entry_index(h, w, c, sizeof(char), tensor);
                dst[index + h * width * channel + w * channel + c] = (src[index_transform] - tensor.s32ZeroPoint) * tensor.dScaleFactor;
            }
        }
    }
}

void SSD_detection::fixedToFloatHWCSSD(float *dst, float *src, output_tensor &tensor, int index)
{
    int channel = tensor.u32OriChannels;
    int width = tensor.u32Width;
    int height = tensor.u32Height;
    int index_transform;

    for (int h = 0; h < height; h++)
    {
        for (int w = 0; w < width; w++)
        {
            for (int c = 0; c < channel; c++)
            {
                index_transform = entry_index(h, w, c, sizeof(float), tensor);
                dst[index + h * width * channel + w * channel + c] = src[index_transform];
            }
        }
    }
}

void SSD_detection::CHW_FloatToHWC_Float(float *dst, float *src, output_tensor &tensor, int index)
{
    int channel = tensor.u32OriChannels;
    int width = tensor.u32Width;
    int height = tensor.u32Height;

    for (int h = 0; h < height; h++)
    {
        for (int w = 0; w < width; w++)
        {
            for (int c = 0; c < channel; c++)
            {
                dst[index + h * width * channel + w * channel + c] = src[c * height * width + h * width + w];
            }
        }
    }
}

void SSD_detection::SetDetectParam(int class_num, int input_width, int input_height, float nms_threshold,
    float dect_threshold)
{
    switch (this->net_type)
    {
    case MOBILENETSSD_300:
        dect_param.class_num = class_num;
        dect_param.per_box_num = 4;
        dect_param.image_width = input_width;
        dect_param.image_height = input_height;
        dect_param.keep_box_max_num = 100;
        dect_param.nms_threshold = nms_threshold;
        dect_param.dect_threshold = dect_threshold;
        dect_param.nms_overlap_top_num = 200;
        dect_param.windows_num = 1917;//19*19*3+10*10*6+5*5*6+3*3*6+2*2*6+1*1*6
        dect_param.delta_stds = { 0.1f, 0.1f, 0.2f, 0.2f };
        dect_param.aspect_ratios = { { 2.0f, 1 / 2.0f, -1.0f, -1 / 1.0f },
            { 2.0f, 1 / 2.0f, 3.0f, 1 / 3.0f },
            { 2.0f, 1 / 2.0f, 3.0f, 1 / 3.0f },
            { 2.0f, 1 / 2.0f, 3.0f, 1 / 3.0f },
            { 2.0f, 1 / 2.0f, 3.0f, 1 / 3.0f },
            { 2.0f, 1 / 2.0f, 3.0f, 1 / 3.0f }
        };
        dect_param.min_size = { 60, 105, 150, 195, 240, 285 };
        dect_param.max_size = { -1, 150, 195, 240, 285, 300 };
        break;
    case MOBILENETSSD_PYTORCH_300:
        dect_param.class_num = class_num;
        dect_param.per_box_num = 4;
        dect_param.image_width = input_width;
        dect_param.image_height = input_height;
        dect_param.keep_box_max_num = 100;
        dect_param.nms_threshold = nms_threshold;
        dect_param.dect_threshold = dect_threshold;
        dect_param.nms_overlap_top_num = 200;
        dect_param.windows_num = 3000;//19*19*6+10*10*6+5*5*6+3*3*6+2*2*6+1*1*6
        dect_param.delta_stds = { 0.1f, 0.1f, 0.2f, 0.2f };
        dect_param.aspect_ratios = { { 2.0f, 1 / 2.0f, 3.0f, 1 / 3.0f },
            { 2.0f, 1 / 2.0f, 3.0f, 1 / 3.0f },
            { 2.0f, 1 / 2.0f, 3.0f, 1 / 3.0f },
            { 2.0f, 1 / 2.0f, 3.0f, 1 / 3.0f },
            { 2.0f, 1 / 2.0f, 3.0f, 1 / 3.0f },
            { 2.0f, 1 / 2.0f, 3.0f, 1 / 3.0f }
        };
        dect_param.min_size = { 60, 105, 150, 195, 240, 285 };
        dect_param.max_size = { 105, 150, 195, 240, 285, 300 };
        break;
    case SQUEEZENETSSD_300:
        dect_param.class_num = class_num;
        dect_param.per_box_num = 4;
        dect_param.image_width = input_width;
        dect_param.image_height = input_height;
        dect_param.keep_box_max_num = 100;
        dect_param.nms_threshold = nms_threshold;
        dect_param.dect_threshold = dect_threshold;

        dect_param.nms_overlap_top_num = 200;
        dect_param.windows_num = 8030;

        dect_param.conf_idex = { 1, 3, 5, 7, 9, 11 };
        dect_param.loc_idex = { 0, 2, 4, 6, 8, 10 };
        dect_param.delta_stds = { 0.1f, 0.1f, 0.2f, 0.2f };
        dect_param.aspect_ratios = { { 2.0f, 1 / 2.0f, -1.0f, -1 / 1.0f },
            { 2.0f, 1 / 2.0f, 3.0f, 1 / 3.0f },
            { 2.0f, 1 / 2.0f, 3.0f, 1 / 3.0f },
            { 2.0f, 1 / 2.0f, 3.0f, 1 / 3.0f },
            { 2.0f, 1 / 2.0f, 3.0f, 1 / 3.0f },
            { 2.0f, 1 / 2.0f, -1.0f, -1 / 1.0f }
        };
        dect_param.min_size = { 21, 45, 99, 153, 207, 261 };
        dect_param.max_size = { 45, 99, 153, 207, 261, 315 };
        dect_param.step = { { 8.0f, 16.0f, 32.0f, 64.0f, 100.0f, 300.0f },
            { 8.0f, 16.0f, 32.0f, 64.0f, 100.0f, 300.0f }
        };
        break;
    default:
        break;
    }
}

void SSD_detection::PermuteSSD(short *ssdBuff)
{
    int dst_offset = 0;
    // int offset=0;
    for (int i = 0; i < tensorNum / 2; i++)
    {
        int channel = pTensor[2 * i + 1].u32OriChannels;
        int width = pTensor[2 * i + 1].u32Width;
        int height = pTensor[2 * i + 1].u32Height;
        int bits = pTensor[2 * i + 1].u32Precision / 8;
        int offset = (pTensor[2 * i + 1].u32Bank * 32 * 1024 * 1024 + pTensor[2 * i + 1].u32Offset);
        // offset +=(pTensor[2 * i].u32KNormNum+1)*pTensor[2 * i].u32KStep;
        short *src = ssdBuff + offset / bits;
        fixedToFloatHWCSSD(scores_flat, src, pTensor[2 * i + 1], dst_offset);
        // offset +=(pTensor[2 * i+1].u32KNormNum+1)*pTensor[2 * i+1].u32KStep;
        dst_offset += width * height * channel;
    }
    dst_offset = 0;
    // offset=0;
    for (int i = 0; i < tensorNum / 2; i++)
    {
        int channel = pTensor[2 * i].u32OriChannels;
        int width = pTensor[2 * i].u32Width;
        int height = pTensor[2 * i].u32Height;
        int offset = (pTensor[2 * i].u32Bank * 32 * 1024 * 1024 + pTensor[2 * i].u32Offset);
        int bits = pTensor[2 * i].u32Precision / 8;
        short *src = ssdBuff + offset / bits;
        fixedToFloatHWCSSD(deltas_flat, src, pTensor[2 * i], dst_offset);
        // offset +=(pTensor[2 * i].u32KNormNum+1)*pTensor[2 * i].u32KStep;
        // offset +=(pTensor[2 * i+1].u32KNormNum+1)*pTensor[2 * i+1].u32KStep;
        dst_offset += width * height * channel;
    }
}

void SSD_detection::PermuteSSD(char *ssdBuff)
{
    int dst_offset = 0;
    //int offset=0;
    for (int i = 0; i < tensorNum / 2; i++)
    {
        int channel = pTensor[2 * i + 1].u32OriChannels;
        int width = pTensor[2 * i + 1].u32Width;
        int height = pTensor[2 * i + 1].u32Height;
        // int bits=pTensor[2 * i + 1].u32Precision/8;
        int offset = (pTensor[2 * i + 1].u32Bank * 32 * 1024 * 1024 + pTensor[2 * i + 1].u32Offset);
        int bits = pTensor[2 * i].u32Precision / 8;
        // offset +=(pTensor[2 * i].u32KNormNum+1)*pTensor[2 * i].u32KStep;
        char *src = ssdBuff + offset / bits;
        fixedToFloatHWCSSD(scores_flat, src, pTensor[2 * i + 1], dst_offset);
        // offset +=(pTensor[2 * i+1].u32KNormNum+1)*pTensor[2 * i+1].u32KStep;
        dst_offset += width * height * channel;
    }
    dst_offset = 0;
    // offset=0;
    for (int i = 0; i < tensorNum / 2; i++)
    {
        int channel = pTensor[2 * i].u32OriChannels;
        int width = pTensor[2 * i].u32Width;
        int height = pTensor[2 * i].u32Height;
        int offset = (pTensor[2 * i].u32Bank * 32 * 1024 * 1024 + pTensor[2 * i].u32Offset);
        int bits = pTensor[2 * i].u32Precision / 8;
        char *src = ssdBuff + offset / bits;
        fixedToFloatHWCSSD(deltas_flat, src, pTensor[2 * i], dst_offset);
        // offset +=(pTensor[2 * i].u32KNormNum+1)*pTensor[2 * i].u32KStep;
        // offset +=(pTensor[2 * i+1].u32KNormNum+1)*pTensor[2 * i+1].u32KStep;
        dst_offset += width * height * channel;
    }
}

void SSD_detection::PermuteSSD(float *ssdBuff)
{
    int dst_offset = 0;

    for (int i = 0; i < tensorNum / 2; i++)
    {
        int channel = pTensor[2 * i + 1].u32OriChannels;
        int width = pTensor[2 * i + 1].u32Width;
        int height = pTensor[2 * i + 1].u32Height;
        int offset = (pTensor[2 * i + 1].u32Bank * 32 * 1024 * 1024 + pTensor[2 * i + 1].u32Offset) / sizeof(float);
        float *src = ssdBuff + offset;
        /*
        sprintf(filename, "tensor_%d.txt", 2*i+1);
        write_bin(src, outputs_tensor->tensor[2 * i + 1], filename);
        */
        fixedToFloatHWCSSD(scores_flat, src, pTensor[2 * i + 1], dst_offset);
        dst_offset += width * height * channel;
    }
    dst_offset = 0;
    for (int i = 0; i < tensorNum / 2; i++)
    {
        int channel = pTensor[2 * i].u32OriChannels;
        int width = pTensor[2 * i].u32Width;
        int height = pTensor[2 * i].u32Height;
        int offset = (pTensor[2 * i].u32Bank * 32 * 1024 * 1024 + pTensor[2 * i].u32Offset) / sizeof(float);
        float *src = ssdBuff + offset;
        fixedToFloatHWCSSD(deltas_flat, src, pTensor[2 * i], dst_offset);
        dst_offset += width * height * channel;
    }
}

void SSD_detection::CHW_PermuteSSD(float *ssdBuff)
{
    int scores_flat_dst_offset = 0;
    int deltas_flat_dst_offset = 0;
    int offset = 0;

    for (int i = 0; i < tensorNum; i++)
    {
        int channel = pTensor[i].u32OriChannels;
        int width = pTensor[i].u32Width;
        int height = pTensor[i].u32Height;
        float *src = ssdBuff + offset;
        if (i % 2 != 0)
        {
            CHW_FloatToHWC_Float(scores_flat, src, pTensor[i], scores_flat_dst_offset);
            scores_flat_dst_offset += width * height * channel;
        }
        else
        {
            CHW_FloatToHWC_Float(deltas_flat, src, pTensor[i], deltas_flat_dst_offset);
            deltas_flat_dst_offset += width * height * channel;
        }
        offset += width * height * channel;
    }
}

void SSD_detection::GetPriorBoxesSSD()
{
    int idx = 0;
    float img_height = (float)dect_param.image_height;
    float img_width = (float)dect_param.image_width;
    for (int i = 0; i < tensorNum / 2; i++)
    {
        int layer_height = pTensor[2 * i].u32Height;
        int layer_width = pTensor[2 * i].u32Width;

        float step_h;
        float step_w;
        if (dect_param.step.size() == 0)
        {
            step_h = img_height / layer_height;
            step_w = img_width / layer_width;
        }
        else
        {
            step_h = dect_param.step[0][i];
            step_w = dect_param.step[1][i];
        }
        for (int h = 0; h < layer_height; h++)
        {
            for (int w = 0; w < layer_width; w++)
            {
                float center_x = (w + 0.5) * step_w;
                float center_y = (h + 0.5) * step_h;
                float box_width, box_height;
                box_width = box_height = dect_param.min_size[i];
                assert(priorbox[idx].size() == 4);
                priorbox[idx][0] = (center_x - box_width / 2.0) / img_width;
                priorbox[idx][1] = (center_y - box_height / 2.0) / img_height;
                priorbox[idx][2] = (center_x + box_width / 2.0) / img_width;
                priorbox[idx][3] = (center_y + box_height / 2.0) / img_height;
                idx++;
                if (dect_param.max_size[i] > 0)
                {
                    box_width = box_height = sqrt(dect_param.min_size[i] * dect_param.max_size[i]);

                    priorbox[idx][0] = (center_x - box_width / 2.0) / img_width;
                    priorbox[idx][1] = (center_y - box_height / 2.0) / img_height;
                    priorbox[idx][2] = (center_x + box_width / 2.0) / img_width;
                    priorbox[idx][3] = (center_y + box_height / 2.0) / img_height;
                    idx++;
                }
                for (int r = 0; r < dect_param.per_box_num; r++)
                {
                    float ar = dect_param.aspect_ratios[i][r];
                    if (fabs(ar - 1.) < 1e-6 || ar < 0.)
                    {
                        continue;
                    }
                    box_width = dect_param.min_size[i] * sqrt(ar);
                    box_height = dect_param.min_size[i] / sqrt(ar);

                    // xmin
                    priorbox[idx][0] = (center_x - box_width / 2.0) / img_width;
                    // ymin
                    priorbox[idx][1] = (center_y - box_height / 2.0) / img_height;
                    // xmax
                    priorbox[idx][2] = (center_x + box_width / 2.0) / img_width;
                    // ymax
                    priorbox[idx][3] = (center_y + box_height / 2.0) / img_height;
                    idx++;
                }
            }
        }
    }
}

void SSD_detection::SoftmaxSSD()
{
    int windouws_num = dect_param.windows_num;
    int class_num = dect_param.class_num;
    float *prob = nullptr;
    for (int i = 0; i < windouws_num; ++i)
    {
        prob = this->scores_flat + i * class_num;
        for (int j = 0; j < class_num; ++j)
            softmaxScore[j] = prob[j] - prob[0];
        float e_sum = 0.;
        for (int j = 0; j < class_num; ++j)
            e_sum += (softmaxScore[j] = exp(softmaxScore[j]));
        float inv_e_sum = 1.f / e_sum;
        for (int j = 0; j < class_num; ++j)
            prob[j] = softmaxScore[j] * inv_e_sum;
    }
}

void SSD_detection::AscendSortIndexSSD()
{
    int *sort = decodeSort.sort;
    float *data = decodeSort.values;
    int length = decodeSort.num;
    for (int m = 0; m < length; m++)
    {
        sort[m] = m;
    }
    const int bubble_level = 8;
    struct
    {
        int lb, ub;
    } stack[48];

    int sp = 0;

    int   temp;
    int   lb_val;

    stack[0].lb = 0;
    stack[0].ub = length - 1;

    while (sp >= 0)
    {
        int lb = stack[sp].lb;
        int ub = stack[sp--].ub;

        for (;;)
        {
            int diff = ub - lb;
            if (diff < bubble_level)
            {
                int i, j;
                int *arr = sort + lb;

                for (i = diff; i > 0; i--)
                {
                    int f = 0;
                    for (j = 0; j < i; j++)
                    {
                        if (data[arr[j + 1]] < data[arr[j]])
                        {
                            temp = arr[j];
                            arr[j] = arr[j + 1];
                            arr[j + 1] = temp;
                            f = 1;
                        }
                    }
                    if (!f)
                        break;
                }
                break;
            }
            else
            {
                /* select pivot and exchange with 1st element */
                int  m = lb + (diff >> 1);
                int  i = lb + 1, j = ub;

                lb_val = sort[m];

                sort[m] = sort[lb];
                sort[lb] = lb_val;

                /* partition into two segments */
                for (;;)
                {
                    for (; i < j && data[sort[i]] < data[lb_val]; i++);
                    for (; j >= i && data[lb_val] < data[sort[j]]; j--);

                    if (i >= j) break;
                    temp = sort[i];
                    sort[i++] = sort[j];
                    sort[j--] = temp;
                }

                /* pivot belongs in A[j] */
                sort[lb] = sort[j];
                sort[j] = lb_val;

                /* keep processing smallest segment, and stack largest*/
                if (j - lb <= ub - j)
                {
                    if (j + 1 < ub)
                    {
                        stack[++sp].lb = j + 1;
                        stack[sp].ub = ub;
                    }
                    ub = j - 1;
                }
                else
                {
                    if (j - 1 > lb)
                    {
                        stack[++sp].lb = lb;
                        stack[sp].ub = j - 1;
                    }
                    lb = j + 1;
                }
            }
        }
    }
}

void SSD_detection::BoxTransformSSD(float *tpbox, vector<float> &priorbox)
{
    float box0, box1, box2, box3;

    float *delta = tpbox;

    float tpw0 = priorbox[0];
    float tpw1 = priorbox[1];
    float tpw2 = priorbox[2];
    float tpw3 = priorbox[3];

    float b_x = (tpw0 + tpw2) * 0.5;
    float b_y = (tpw1 + tpw3) * 0.5;
    float b_w = (tpw2 - tpw0);
    float b_h = (tpw3 - tpw1);

    float dx = delta[0] * dect_param.delta_stds[0];
    float dy = delta[1] * dect_param.delta_stds[1];
    float dw = delta[2] * dect_param.delta_stds[2];
    float dh = delta[3] * dect_param.delta_stds[3];

    float x = dx * b_w + b_x;
    float y = dy * b_h + b_y;

    float w = exp(dw) * b_w;
    float h = exp(dh) * b_h;

    float half_w = w * 0.5;
    float half_h = h * 0.5;

    box0 = x - half_w;
    box1 = y - half_h;
    box2 = x + half_w;
    box3 = y + half_h;

    #if 1
    box0 = std::min(box0, 1.f);
    delta[0] = std::max(box0, 0.f);
    box1 = std::min(box1, 1.f);
    delta[1] = std::max(box1, 0.f);
    box2 = std::min(box2, 1.f);
    delta[2] = std::max(box2, 0.f);
    box3 = std::min(box3, 1.f);
    delta[3] = std::max(box3, 0.f);
    #else
    delta[0] = box0;
    delta[1] = box1;
    delta[2] = box2;
    delta[3] = box3;
    #endif
}

int SSD_detection::NonMaxSuppressSSD(vector<vector<float>> &boxes, int num_boxes)
{
    int i;
    //compute areas
    float *areas = new float[num_boxes];
    float *pool = new float[num_boxes];
    for (i = 0; i < num_boxes; i++)
    {
        vector<float> &box = boxes[i];
        areas[i] = (box[2] - box[0]) * (box[3] - box[1]);
        //pool[i] = i;
    }

    for (i = 0; i < num_boxes; i++) pool[i] = i;
    int num_pools = num_boxes;

    int num_keeps = 0;
    while (num_pools > 0)
    {
        int max = pool[0];
        nms.keep[num_keeps] = max;
        num_keeps++;
        if (num_pools == 1) break;
        if (num_keeps >= dect_param.keep_box_max_num) break;

        // remove overlapping boxes
        vector<float> &box = boxes[max];
        float x0 = box[0];
        float y0 = box[1];
        float x1 = box[2];
        float y1 = box[3];
        float a = areas[max];

        int num_pools1 = 0;
        for (i = 1; i < num_pools; i++)
        {
            //compute overlap
            int j = pool[i];
            vector<float> &box = boxes[j];
            float xx0 = box[0];
            float yy0 = box[1];
            float xx1 = box[2];
            float yy1 = box[3];
            float aa = areas[j];
            float w = std::min(xx1, x1) - std::max(xx0, x0);
            float h = std::min(yy1, y1) - std::max(yy0, y0);
            float intersect = std::max(0.f, w) * std::max(0.f, h);
            float overlap = intersect / (a + aa - intersect);

            if (overlap < dect_param.nms_threshold)
            {
                pool[num_pools1] = j;
                num_pools1++;
            }
        }
        num_pools = num_pools1;
    }
    delete[] areas;
    delete[] pool;
    return num_keeps;
}

int SSD_detection::SSD_BboxDecoding(vector<detection_output> &dets)
{
    this->SoftmaxSSD();
    int num_dets = 0;
    int c, n, i;
    for (c = 1; c < dect_param.class_num; c++)
    {

        decodeSort.num = 0;
        for (n = 0; n < dect_param.windows_num; n++)
        {
            int scores_flat_idex = n * dect_param.class_num + c;
            if (scores_flat[scores_flat_idex] > dect_param.dect_threshold)
            {
                decodeSort.values[decodeSort.num] = scores_flat[scores_flat_idex];
                decodeSort.index[decodeSort.num] = n;
                decodeSort.num++;
            }
        }
        this->AscendSortIndexSSD();
        vector<vector<float>>boxes;
        int num_boxes = std::min(decodeSort.num, dect_param.nms_overlap_top_num);
        for (i = 0; i < num_boxes; i++)
        {
            int j = decodeSort.index[decodeSort.sort[num_boxes - 1 - i]];
            int idex_sf = j * dect_param.class_num + c;
            nms.scores[i] = scores_flat[idex_sf];
            float tpbox[4];
            int idex_df = j * dect_param.per_box_num;
            tpbox[0] = deltas_flat[idex_df];
            tpbox[1] = deltas_flat[idex_df + 1];
            tpbox[2] = deltas_flat[idex_df + 2];
            tpbox[3] = deltas_flat[idex_df + 3];
            BoxTransformSSD(tpbox, priorbox[j]);
            vector<float>box_temp(4, 0.);
            box_temp[0] = tpbox[0];
            box_temp[1] = tpbox[1];
            box_temp[2] = tpbox[2];
            box_temp[3] = tpbox[3];
            boxes.push_back(box_temp);
        }
        detection_output d;
        int num_keeps = NonMaxSuppressSSD(boxes, num_boxes);
        for (i = 0; i < num_keeps; i++)
        {
            int j = nms.keep[i];

            vector<float> &box = boxes[j];
            d.x = box[0] * dect_param.image_width;
            d.y = box[1] * dect_param.image_height;
            d.w = box[2] * dect_param.image_width - d.x;
            d.h = box[3] * dect_param.image_height - d.y;
            d.classId = c;
            d.confidence = nms.scores[j];
            dets.push_back(d);
            num_dets++;
        }
        boxes.clear();
    }
    return num_dets;
}

void SSD_detection::SSD_PostProcess(short *ssdBuff, vector<detection_output> &dets)
{
    assert(4 == dect_param.per_box_num);
    vector<detection_output>result_temp;

    PermuteSSD(ssdBuff);
    int num_dets = SSD_BboxDecoding(result_temp);
    vector<std::pair<float, int> > idex_vector;
    for (int k = 0; k < num_dets; ++k)
    {
        detection_output &d = result_temp[k];
        idex_vector.push_back(std::make_pair(d.confidence, k));
    }
    std::partial_sort(idex_vector.begin(), idex_vector.begin() + num_dets, idex_vector.end(),
        std::greater<std::pair<float, int>>());
    for (int result_i = 0; result_i < num_dets; result_i++)
    {
        detection_output &d = result_temp[idex_vector[result_i].second];
        dets.push_back(d);
    }
}

void SSD_detection::SSD_PostProcess(char *ssdBuff, vector<detection_output> &dets)
{
    assert(4 == dect_param.per_box_num);
    vector<detection_output>result_temp;
    PermuteSSD(ssdBuff);
    int num_dets = SSD_BboxDecoding(result_temp);
    vector<std::pair<float, int> > idex_vector;
    for (int k = 0; k < num_dets; ++k)
    {
        detection_output &d = result_temp[k];
        idex_vector.push_back(std::make_pair(d.confidence, k));
    }
    std::partial_sort(idex_vector.begin(), idex_vector.begin() + num_dets, idex_vector.end(),
        std::greater<std::pair<float, int>>());
    for (int result_i = 0; result_i < num_dets; result_i++)
    {
        detection_output &d = result_temp[idex_vector[result_i].second];
        dets.push_back(d);
    }
}

void SSD_detection::SSD_PostProcess(float *ssdBuff, vector<detection_output> &dets)
{
    assert(4 == dect_param.per_box_num);
    vector<detection_output>result_temp;
    PermuteSSD(ssdBuff);
    int num_dets = SSD_BboxDecoding(result_temp);
    vector<std::pair<float, int> > idex_vector;
    for (int k = 0; k < num_dets; ++k)
    {
        detection_output &d = result_temp[k];
        idex_vector.push_back(std::make_pair(d.confidence, k));
    }
    std::partial_sort(idex_vector.begin(), idex_vector.begin() + num_dets, idex_vector.end(),
        std::greater<std::pair<float, int>>());
    for (int result_i = 0; result_i < num_dets; result_i++)
    {
        detection_output &d = result_temp[idex_vector[result_i].second];
        dets.push_back(d);
    }
}

void SSD_detection::SSD_CHW_PostProcess(float *ssdBuff, vector<detection_output> &dets)
{
    assert(4 == dect_param.per_box_num);
    vector<detection_output>result_temp;
    CHW_PermuteSSD(ssdBuff);
    int num_dets = SSD_BboxDecoding(result_temp);
    vector<std::pair<float, int> > idex_vector;
    for (int k = 0; k < num_dets; ++k)
    {
        detection_output &d = result_temp[k];
        idex_vector.push_back(std::make_pair(d.confidence, k));
    }
    std::partial_sort(idex_vector.begin(), idex_vector.begin() + num_dets, idex_vector.end(),
        std::greater<std::pair<float, int>>());
    for (int result_i = 0; result_i < num_dets; result_i++)
    {
        detection_output &d = result_temp[idex_vector[result_i].second];
        dets.push_back(d);
    }
}

extern "C" int ssd_postpross_and_getresult(void *pTensor, int tensorNum, uintptr_t virtAddr, void **pResult,
    int *ResultNum);
int ssd_postpross_and_getresult(void *pTensor, int tensorNum, uintptr_t virtAddr, void **pResult, int *ResultNum)
{
    output_tensor *ptensor = (output_tensor *)pTensor;
    SSD_detection ssd = SSD_detection(MOBILENETSSD_300, (output_tensor *)ptensor, tensorNum, CLASS_NUM, IOU_THRESHOLD,
            CONFIDENCE_THRESHOLD, NET_WIDTH, NET_HEIGHT);
    vector<detection_output> dets;

    if((ptensor[0].u32Precision) == 8)
    {
        ssd.SSD_PostProcess((char *)virtAddr, dets);
    }
    else if((ptensor[0].u32Precision) == 16)
    {
        ssd.SSD_PostProcess((short *)virtAddr, dets);
    }

    if(dets.empty())
    {
        *pResult = NULL;
        *ResultNum = 0;
    }
    else
    {
        detection_output *p;

        p = (detection_output *)malloc(sizeof(detection_output) * dets.size());
        if(NULL == p)
        {
            printf("get_ssd_result malloc mem ERROR\n");
            return -1;
        }

        *pResult = p;
        memset(*pResult, 0x00, sizeof(sizeof(detection_output)*dets.size()));

        *ResultNum = dets.size();
        int i = 0;
        for (auto iter = dets.begin(); iter != dets.end(); iter++)
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
