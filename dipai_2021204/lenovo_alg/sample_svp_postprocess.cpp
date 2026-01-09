#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "sample_svp_postprocess.h"

static int nms_comparator(const void *pa, const void *pb)
{
    detection a = *(detection *)pa;
    detection b = *(detection *)pb;
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

void do_nms_sort(detection *dets, int total, int classes, float thresh)
{
    int i, j, k;
    k = total - 1;
    for(i = 0; i <= k; ++i)
    {
        if(dets[i].objectness == 0)
        {
            detection swap = dets[i];
            dets[i] = dets[k];
            dets[k] = swap;
            --k;
            --i;
        }
    }
    total = k + 1;

    for(k = 0; k < classes; ++k)
    {
        for(i = 0; i < total; ++i)
        {
            dets[i].sort_class = k;
        }
        qsort(dets, total, sizeof(detection), nms_comparator);
        for(i = 0; i < total; ++i)
        {
            if(dets[i].prob[k] == 0) continue;
            box a = dets[i].bbox;
            for(j = i + 1; j < total; ++j)
            {
                box b = dets[j].bbox;
                if (box_iou(a, b) > thresh)
                {
                    dets[j].prob[k] = 0;
                }
            }
        }
    }
}

int entry_index(int h, int w, int c, int byteUnit, output_tensor &pTensorInfo)
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

int chw_entry_index(int h, int w, int c, output_tensor &pTensorInfo)
{
    /*
    int n = location / (layer_w*layer_h);
    int loc = location % (layer_w*layer_h);
    return (n*layer_w*layer_h*(4 + 80 + 1) + entry* layer_w*layer_h + loc);
    */
    int tensorWidth = pTensorInfo.u32Width;
    int tensorHeight = pTensorInfo.u32Height;

    int memoryMigration = c * tensorWidth * tensorHeight + h * tensorWidth + w;
    return memoryMigration;
}

int getImgID(char *name)
{
    if(name == NULL)
        return 0;

    int len = strlen(name);

    char *pStart = name;
    char *pEnd = name + len - 1;

    char *pNumStart = NULL;
    char *pNumEnd = NULL;

    while(pEnd > pStart)
    {
        if(*pEnd >= '0' && *pEnd <= '9')
        {
            pNumEnd = pEnd;
            break;
        }

        pEnd--;
    }

    if(pNumEnd == NULL)
    {
        return 0;
    }

    while(pEnd >= pStart)
    {
        if(*pEnd < '0' || *pEnd > '9')
            break;
        pEnd--;
    }

    pNumStart = pEnd + 1;

    char buf[64];
    if((unsigned int)(pNumEnd - pNumStart + 1) >= sizeof(buf))
        return 0;

    memset(buf, 0, sizeof(buf));
    memcpy(buf, pNumStart, pNumEnd - pNumStart + 1);

    int val = atoi(buf);

    return val;
}
