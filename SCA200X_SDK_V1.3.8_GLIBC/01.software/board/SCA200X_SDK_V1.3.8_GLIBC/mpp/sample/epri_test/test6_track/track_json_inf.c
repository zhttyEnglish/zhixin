// Copyright (c) 2021 Cesanta Software Limited
// All rights reserved
//
// Example HTTP client. Connect to `s_url`, send request, wait for a response,
// print the response and exit.
// You can change `s_url` from the command line by executing: ./example YOUR_URL
//
// To enable SSL/TLS, make SSL=OPENSSL or make SSL=MBEDTLS

#include <stdio.h>

#include "track_json_inf.h"
#include "s2j.h"

#if 0
typedef struct
{
	int x;
	int y;
	int w;
	int h;
}Ng_roi;
#endif
cJSON *struct_to_json_Ng_roi(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	Ng_roi *struct_obj_ = (Ng_roi *)struct_obj;
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, x);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, y);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, w);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, h);
	return json_obj_;
}

void *json_to_struct_Ng_roi(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, Ng_roi);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, x);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, y);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, w);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, h);
	return struct_obj_;
}

#if 0
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
#endif
cJSON *struct_to_json_Ng_track_test(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	Ng_track_test *struct_obj_ = (Ng_track_test *)struct_obj;
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, session);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, command);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, response_url);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, group_id);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, target_path);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, type);
	s2j_json_set_struct_element_by_func(json_obj_, struct_obj_, Ng_roi,roi);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, tracking_path);
	return json_obj_;
}

void *json_to_struct_Ng_track_test(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, Ng_track_test);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, session);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, command);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, response_url);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, group_id);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, target_path);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, type);
	s2j_struct_get_struct_element_by_func(struct_obj_, json_obj, Ng_roi,roi);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, tracking_path);
	return struct_obj_;
}

#if 0
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
#endif
cJSON *struct_to_json_Ng_track_test_res(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	Ng_track_test_res *struct_obj_ = (Ng_track_test_res *)struct_obj;
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, session);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, status);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, group_id);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, tracking_path);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, type);
	s2j_json_set_struct_element_by_func(json_obj_, struct_obj_, Ng_roi,roi);
	s2j_json_set_struct_element_by_func(json_obj_, struct_obj_, Ng_roi,offset_pos);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, angle);
	return json_obj_;
}

void *json_to_struct_Ng_track_test_res(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, Ng_track_test_res);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, session);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, status);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, group_id);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, tracking_path);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, type);
	s2j_struct_get_struct_element_by_func(struct_obj_, json_obj, Ng_roi,roi);
	s2j_struct_get_struct_element_by_func(struct_obj_, json_obj, Ng_roi,offset_pos);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, angle);
	return struct_obj_;
}










    
#if 0
typedef struct
{
	int group_id;
	char target_path[NG_STR_LEN];
	char type[NG_STR_LEN];
	Ng_roi roi;
	char tracking_path[NG_STR_LEN];
}track_file_target;
#endif
cJSON *struct_to_json_track_file_target(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	track_file_target *struct_obj_ = (track_file_target *)struct_obj;
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, group_id);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, target_path);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, type);
	s2j_json_set_struct_element_by_func(json_obj_, struct_obj_, Ng_roi,roi);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, tracking_path);
	return json_obj_;
}

void *json_to_struct_track_file_target(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, track_file_target);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, group_id);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, target_path);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, type);
	s2j_struct_get_struct_element_by_func(struct_obj_, json_obj, Ng_roi,roi);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, tracking_path);
	return struct_obj_;
}

#if 0
typedef struct
{
	int group_id;
	char tracking_path[NG_STR_LEN];
	char type[NG_STR_LEN];
	Ng_roi roi;
	Ng_roi offset_pos;
	int angle;
}track_file_result;
#endif
cJSON *struct_to_json_track_file_result(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	track_file_result *struct_obj_ = (track_file_result *)struct_obj;
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, group_id);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, tracking_path);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, type);
	s2j_json_set_struct_element_by_func(json_obj_, struct_obj_, Ng_roi,roi);
	s2j_json_set_struct_element_by_func(json_obj_, struct_obj_, Ng_roi,offset_pos);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, angle);
	return json_obj_;
}

void *json_to_struct_track_file_result(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, track_file_result);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, group_id);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, tracking_path);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, type);
	s2j_struct_get_struct_element_by_func(struct_obj_, json_obj, Ng_roi,roi);
	s2j_struct_get_struct_element_by_func(struct_obj_, json_obj, Ng_roi,offset_pos);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, angle);   
	return struct_obj_;
}

#if 0
typedef struct
{
	track_file_target target;
	track_file_result result;
}Ng_track_result_data_file;
#endif
cJSON *struct_to_json_Ng_track_result_data_file(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	Ng_track_result_data_file *struct_obj_ = (Ng_track_result_data_file *)struct_obj;
	s2j_json_set_struct_element_by_func(json_obj_, struct_obj_, track_file_target,target);
	s2j_json_set_struct_element_by_func(json_obj_, struct_obj_, track_file_result,result);
	return json_obj_;
}

void *json_to_struct_Ng_track_result_data_file(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, Ng_track_result_data_file);
	s2j_struct_get_struct_element_by_func(struct_obj_, json_obj, track_file_target,target);
	s2j_struct_get_struct_element_by_func(struct_obj_, json_obj, track_file_result,result);
	return struct_obj_;
}




