#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_STR_LEN 256
#define MAX_CAM 10

typedef struct {
    float fire;
    float down;
    float animal;
    float person;
    float smog;
    float water;
} AlgoThreshold;

typedef struct {
    char broker[MAX_STR_LEN];
    int port;
} Mqtt;

typedef struct {
    int left;
    int top;
    int right;
    int bottom;
} obj_rect;

typedef struct {
    obj_rect rect;
    float score; // 置信度
    float prob;  // 概率值
} obj_info;

typedef enum {
    FRAME_RATE_HIGH,
    FRAME_RATE_MEDIUM,
    FRAME_RATE_LOW,
    FRAME_RATE_UNKNOWN
} FrameRate;

typedef struct {
    char cameraid[MAX_STR_LEN];
    bool useperson;
    bool useanimal;
    bool usedown;
    bool usefire;
    bool usesmog;
    bool usewater;
    obj_info objs;
    bool osd;
    FrameRate frameRate;
    char camcode[MAX_STR_LEN];
} Cam;

typedef struct {
    char license[MAX_STR_LEN];
    char deviceid[MAX_STR_LEN];
    char camcount[MAX_STR_LEN];
    char sn[MAX_STR_LEN];
    bool pd;
    int linesize;
    AlgoThreshold algo;
    Mqtt mqtt;
    Cam cams[MAX_CAM];
    int cam_num;
} Config;


void parse_config(const char *filename, Config *cfg);

