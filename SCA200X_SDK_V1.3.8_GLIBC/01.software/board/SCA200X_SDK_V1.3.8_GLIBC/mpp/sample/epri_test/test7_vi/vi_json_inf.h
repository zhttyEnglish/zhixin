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
    char session[NG_STR_LEN];
    char status[NG_STR_LEN];
}Ng_test_res;

typedef struct
{
    char session[NG_STR_LEN];
    char command[NG_STR_LEN];
    char output_url[NG_STR_LEN];
    char response_url[NG_STR_LEN];
}Ng_vi_test;

cJSON *struct_to_json_Ng_vi_test(void* struct_obj);
void *json_to_struct_Ng_vi_test(cJSON* json_obj);
cJSON *struct_to_json_Ng_vi_test_res(void* struct_obj);
void *json_to_struct_Ng_vi_test_res(cJSON* json_obj);


#ifdef __cplusplus
}

#endif /* __cplusplus */

#endif

