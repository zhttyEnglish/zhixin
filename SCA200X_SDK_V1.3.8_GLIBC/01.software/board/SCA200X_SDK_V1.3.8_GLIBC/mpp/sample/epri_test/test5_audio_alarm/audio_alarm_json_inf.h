#ifndef __DEVICE_TEST_H
#define __DEVICE_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cJSON.h"

#define NG_STR_LEN (256)

typedef struct
{
    char session[NG_STR_LEN];
    char command[NG_STR_LEN];
    int  duration;
    char response_url[NG_STR_LEN];
}Ng_audio_alarm_test;

typedef struct
{
    char session[NG_STR_LEN];
    char status[NG_STR_LEN];
}Ng_test_res;


cJSON *struct_to_json_Ng_audio_alarm_test(void* struct_obj);
void *json_to_struct_Ng_audio_alarm_test(cJSON* json_obj);

cJSON *struct_to_json_Ng_audio_alarm_test_res(void* struct_obj);
void *json_to_struct_Ng_audio_alarm_test_res(cJSON* json_obj);


#ifdef __cplusplus
}

#endif /* __cplusplus */

#endif

