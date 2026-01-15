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
    int x;
    int y;
}Ng_point;

typedef struct
{
    char path[NG_STR_LEN];
    char location[NG_STR_LEN];
    Ng_point point1;
    Ng_point point2;
}Ng_images_cmd;

typedef struct
{
    char session[NG_STR_LEN];
    char command[NG_STR_LEN];
    char response_url[NG_STR_LEN];
    Ng_images_cmd images[NG_ARRAY_CNT];
}Ng_distance_test;

typedef struct
{
    char path[NG_STR_LEN];
    float distance;
}Ng_images_res;

typedef struct
{
    char session[NG_STR_LEN];
    char status[NG_STR_LEN];
    int images_cnt;
    Ng_images_res images[NG_ARRAY_CNT];
}Ng_distance_test_res;




typedef struct
{
    Ng_point point1;
    Ng_point point2;
}Ng_file_target;
typedef struct
{
    float distance;
}Ng_file_result;

typedef struct
{
    Ng_file_target target;
    Ng_file_result result;
}Ng_result_data_file;



cJSON *struct_to_json_Ng_distance_test(void* struct_obj);
void *json_to_struct_Ng_distance_test(cJSON* json_obj);

cJSON *struct_to_json_Ng_distance_test_res(void* struct_obj);
void *json_to_struct_Ng_distance_test_res(cJSON* json_obj);


#ifdef __cplusplus
}

#endif /* __cplusplus */

#endif

