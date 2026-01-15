// Copyright (c) 2021 Cesanta Software Limited
// All rights reserved
//
// Example HTTP client. Connect to `s_url`, send request, wait for a response,
// print the response and exit.
// You can change `s_url` from the command line by executing: ./example YOUR_URL
//
// To enable SSL/TLS, make SSL=OPENSSL or make SSL=MBEDTLS

#include <stdio.h>

#include "filter_json_inf.h"
#include "s2j.h"
#if 0
typedef struct
{
	char path[NG_STR_LEN];
	char location[NG_STR_LEN];
}epri_images_filter;
#endif
cJSON *struct_to_json_epri_images_filter(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	epri_images_filter *struct_obj_ = (epri_images_filter *)struct_obj;
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, path);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, location);
	return json_obj_;
}

void *json_to_struct_epri_images_filter(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, epri_images_filter);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, path);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, location);
	return struct_obj_;
}

#if 0
typedef struct
{
	char session[NG_STR_LEN];
	char command[NG_STR_LEN];
	char response_url[NG_STR_LEN];
	int group_id;
	epri_images_filter images[NG_ARRAY_CNT];
}epri_filter_test;
#endif
cJSON *struct_to_json_epri_filter_test(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	epri_filter_test *struct_obj_ = (epri_filter_test *)struct_obj;
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, session);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, command);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, response_url);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, group_id);
	s2j_json_set_struct_array_element_by_func(json_obj_, struct_obj_, epri_images_filter,images,NG_ARRAY_CNT);
	return json_obj_;
}

void *json_to_struct_epri_filter_test(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, epri_filter_test);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, session);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, command);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, response_url);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, group_id);
	s2j_struct_get_struct_array_element_by_func(struct_obj_, json_obj, epri_images_filter,images);
	return struct_obj_;
}

#if 0
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
#endif
cJSON *struct_to_json_epri_issues_filter(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	epri_issues_filter *struct_obj_ = (epri_issues_filter *)struct_obj;
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, path);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, issue_type);
	s2j_json_set_basic_element(json_obj_, struct_obj_, double, score);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, x);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, y);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, w);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, h);
    s2j_json_set_basic_element(json_obj_, struct_obj_, int, flag);
	return json_obj_;
}

void *json_to_struct_epri_issues_filter(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, epri_issues_filter);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, path);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, issue_type);
	s2j_struct_get_basic_element(struct_obj_,json_obj, double, score);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, x);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, y);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, w);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, h);
    s2j_struct_get_basic_element(struct_obj_,json_obj, int, flag);
	return struct_obj_;
}

#if 0
typedef struct
{
	char session[NG_STR_LEN];
	char status[NG_STR_LEN];
	int group_id;
	int inference_time;
	int issues_cnt;
	epri_issues_filter issues[NG_ARRAY_CNT];
}epri_filter_test_res;
#endif
cJSON *struct_to_json_epri_filter_test_res(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	epri_filter_test_res *struct_obj_ = (epri_filter_test_res *)struct_obj;
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, session);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, status);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, group_id);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, inference_time);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, issues_cnt);
	s2j_json_set_struct_array_element_by_func(json_obj_, struct_obj_, epri_issues_filter,issues,struct_obj_->issues_cnt);
	return json_obj_;
}

void *json_to_struct_epri_filter_test_res(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, epri_filter_test_res);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, session);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, status);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, group_id);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, inference_time);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, issues_cnt);
	s2j_struct_get_struct_array_element_by_func(struct_obj_, json_obj, epri_issues_filter,issues);
	return struct_obj_;
}

