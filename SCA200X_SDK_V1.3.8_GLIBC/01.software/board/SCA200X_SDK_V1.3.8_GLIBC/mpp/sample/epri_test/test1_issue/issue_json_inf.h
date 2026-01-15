#ifndef __DEVICE_TEST_H
#define __DEVICE_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cJSON.h"

#define NG_STR_LEN (256)
#define NG_ARRAY_CNT (100)


typedef struct
{
    int x;
    int y;
    int w;
    int h;
}epri_rois;

typedef struct
{
    char path[NG_STR_LEN];
    char location[NG_STR_LEN];
    char issue_types[NG_ARRAY_CNT][NG_STR_LEN];
    epri_rois rois[NG_ARRAY_CNT];
}epri_images;

typedef struct
{
    char issue_type[NG_STR_LEN];
    float score;
    int x;
    int y;
    int w;
    int h;
}epri_issues;

typedef struct
{
    char session[NG_STR_LEN];
    char command[NG_STR_LEN];
    char response_url[NG_STR_LEN];
    epri_images images[NG_ARRAY_CNT];
}epri_issue_test;


typedef struct
{
    char path[NG_STR_LEN];
    int inference_time;
    int  issues_cnt;
    epri_issues issues[NG_ARRAY_CNT];
}epri_res_images;

typedef struct
{
    char session[NG_STR_LEN];
    char status[NG_STR_LEN];
    int  images_cnt;
    epri_res_images images[NG_ARRAY_CNT];
}epri_issue_test_res;



cJSON *struct_to_json_epri_rois(void* struct_obj);
cJSON *struct_to_json_epri_issue_types(void* struct_obj);

void *json_to_struct_epri_rois(cJSON* json_obj);
void *json_to_struct_epri_issue_types(cJSON* json_obj);

cJSON *struct_to_json_epri_images(void* struct_obj);
void *json_to_struct_epri_images(cJSON* json_obj);

cJSON *struct_to_json_epri_issues(void* struct_obj);
void *json_to_struct_epri_issues(cJSON* json_obj);

cJSON *struct_to_json_epri_issue_test(void* struct_obj);
void *json_to_struct_epri_issue_test(cJSON* json_obj);

cJSON *struct_to_json_epri_issue_test_res(void* struct_obj);
void *json_to_struct_epri_issue_test_res(cJSON* json_obj);

cJSON *struct_to_json_epri_res_images(void* struct_obj);
void *json_to_struct_epri_res_images(cJSON* json_obj);


#ifdef __cplusplus
}

#endif /* __cplusplus */

#endif

