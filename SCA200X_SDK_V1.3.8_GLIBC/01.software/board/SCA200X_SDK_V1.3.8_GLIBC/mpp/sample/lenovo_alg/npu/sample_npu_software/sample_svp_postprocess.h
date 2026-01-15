#ifndef __POST_PROCESS_H__
#define __POST_PROCESS_H__

#define MAX_NAME_LEN 256

typedef struct
{
    unsigned int u32ID;
    unsigned int u32Bank;
    unsigned int u32Offset;
    unsigned int u32Height;
    unsigned int u32KStep;
    unsigned int u32KNormNum;
    unsigned int u32KSizeLast;
    unsigned int u32KSizeNorm;
    char achName[MAX_NAME_LEN];
    char achType[MAX_NAME_LEN];
    unsigned int u32Num;
    unsigned int u32OriChannels;
    unsigned int u32OriFrameSize;
    unsigned int u32Precision;
    unsigned int u32RowStep;
    unsigned int u32TensorStep; //used for batch mode
    double dScaleFactor;
    unsigned int u32Size;
    unsigned int u32Width;
    int s32ZeroPoint;
    char achLayoutType[MAX_NAME_LEN];
} output_tensor;

typedef struct
{
    unsigned int classId;
    float confidence;
} classification_output;

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
} detection;

typedef struct
{
    float x;
    float y;
    float w;
    float h;
    unsigned int classId;
    float confidence;
} detection_output;

int entry_index(int h, int w, int c, int byteUnit, output_tensor &pTensorInfo);

int chw_entry_index(int h, int w, int c, output_tensor &pTensorInfo);

int getImgID(char *name);

void do_nms_sort(detection *dets, int total, int classes, float thresh);

#endif /* __POST_PROCESS_H__ */
