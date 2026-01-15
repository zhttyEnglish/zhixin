#ifndef __DEVICE_TEST_H
#define __DEVICE_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cJSON.h"

#define NG_STR_LEN (256)
typedef struct
{
    int x;
    int y;
    int w;
    int h;
}Ng_roi;

typedef struct
{
    char session[NG_STR_LEN];
    char command[NG_STR_LEN];
    char response_url[NG_STR_LEN];
    int group_id;
    char target_path[NG_STR_LEN];
    char type[NG_STR_LEN];
    Ng_roi roi;
    char tracking_path[NG_STR_LEN];      
}Ng_track_test;

typedef struct
{
    char session[NG_STR_LEN];
    char status[NG_STR_LEN];
    int group_id;
    char tracking_path[NG_STR_LEN];
    char type[NG_STR_LEN];
    Ng_roi roi;
    Ng_roi offset_pos;
    int angle;
}Ng_track_test_res;



typedef struct
{
    int group_id;
    char target_path[NG_STR_LEN];
    char type[NG_STR_LEN];
    Ng_roi roi;
    char tracking_path[NG_STR_LEN];
}track_file_target;
typedef struct
{
    int group_id;
    char tracking_path[NG_STR_LEN];
    char type[NG_STR_LEN];
    Ng_roi roi;
    Ng_roi offset_pos;
    int angle;
}track_file_result;
typedef struct
{
    track_file_target target;
    track_file_result result;
}Ng_track_result_data_file;


cJSON *struct_to_json_Ng_roi(void* struct_obj);

void *json_to_struct_Ng_roi(cJSON* json_obj);

cJSON *struct_to_json_Ng_track_test(void* struct_obj);

void *json_to_struct_Ng_track_test(cJSON* json_obj);

cJSON *struct_to_json_Ng_track_test_res(void* struct_obj);

void *json_to_struct_Ng_track_test_res(cJSON* json_obj);
    
void *json_to_struct_Ng_track_result_data_file(cJSON* json_obj);




#ifdef __cplusplus
}

#endif /* __cplusplus */

#endif

