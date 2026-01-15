#ifndef __DEVICE_TEST_H
#define __DEVICE_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cJSON.h"

#define NG_STR_LEN (256)
#define NG_ARRAY_CNT (64)

typedef struct
{
    char path[NG_STR_LEN];
    char location[NG_STR_LEN];
}epri_images_filter;

typedef struct
{
    char session[NG_STR_LEN];
    char command[NG_STR_LEN];
    char response_url[NG_STR_LEN];
    int  group_id;
    epri_images_filter images[NG_ARRAY_CNT];
}epri_filter_test;




typedef struct
{
    char path[NG_STR_LEN];
    char issue_type[NG_STR_LEN];
    float score;
    int x;
    int y;
    int w;
    int h;
    int flag;
}epri_issues_filter;
typedef struct
{
    char session[NG_STR_LEN];
    char status[NG_STR_LEN];
    int  group_id;
    int  inference_time;
    int  issues_cnt;
    epri_issues_filter issues[NG_ARRAY_CNT];
}epri_filter_test_res;


cJSON *struct_to_json_epri_images_filter(void* struct_obj);

void *json_to_struct_epri_images_filter(cJSON* json_obj);

cJSON *struct_to_json_epri_filter_test(void* struct_obj);

void *json_to_struct_epri_filter_test(cJSON* json_obj);

cJSON *struct_to_json_epri_issues_filter(void* struct_obj);

void *json_to_struct_epri_issues_filter(cJSON* json_obj);

cJSON *struct_to_json_epri_filter_test_res(void* struct_obj);

void *json_to_struct_epri_filter_test_res(cJSON* json_obj);







#ifdef __cplusplus
}

#endif /* __cplusplus */

#endif

