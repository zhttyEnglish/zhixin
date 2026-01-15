// Copyright (c) 2021 Cesanta Software Limited
// All rights reserved
//
// Example HTTP client. Connect to `s_url`, send request, wait for a response,
// print the response and exit.
// You can change `s_url` from the command line by executing: ./example YOUR_URL
//
// To enable SSL/TLS, make SSL=OPENSSL or make SSL=MBEDTLS

#include <stdio.h>

#include "issue_json_inf.h"
#include "s2j.h"

cJSON *struct_to_json_epri_rois(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	epri_rois *struct_obj_ = (epri_rois *)struct_obj;
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, x);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, y);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, w);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, h);
	return json_obj_;
}

void *json_to_struct_epri_rois(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, epri_rois);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, x);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, y);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, w);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, h);
	return struct_obj_;
}

cJSON *struct_to_json_epri_images(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	epri_images *struct_obj_ = (epri_images *)struct_obj;
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, path);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, location);
    s2j_json_set_array_element(json_obj_, struct_obj_, string, issue_types, NG_ARRAY_CNT);
	s2j_json_set_struct_array_element_by_func(json_obj_, struct_obj_, epri_rois,rois,NG_ARRAY_CNT);
	return json_obj_;
}

void *json_to_struct_epri_images(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, epri_images);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, path);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, location);
	s2j_struct_get_array_element(struct_obj_,json_obj, string, issue_types);
	s2j_struct_get_struct_array_element_by_func(struct_obj_, json_obj, epri_rois,rois);
	return struct_obj_;
}


cJSON *struct_to_json_epri_issues(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	epri_issues *struct_obj_ = (epri_issues *)struct_obj;
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, issue_type);
	s2j_json_set_basic_element(json_obj_, struct_obj_, double, score);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, x);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, y);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, w);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, h);
	return json_obj_;
}

void *json_to_struct_epri_issues(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, epri_issues);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, issue_type);
	s2j_struct_get_basic_element(struct_obj_,json_obj, double, score);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, x);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, y);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, w);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, h);
	return struct_obj_;
}


cJSON *struct_to_json_epri_issue_test(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	epri_issue_test *struct_obj_ = (epri_issue_test *)struct_obj;
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, session);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, command);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, response_url);
	s2j_json_set_struct_array_element_by_func(json_obj_, struct_obj_, epri_images,images,NG_ARRAY_CNT);
	return json_obj_;
}

void *json_to_struct_epri_issue_test(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, epri_issue_test);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, session);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, command);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, response_url);
	s2j_struct_get_struct_array_element_by_func(struct_obj_, json_obj, epri_images,images);
	return struct_obj_;
}

cJSON *struct_to_json_epri_issue_test_res(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	epri_issue_test_res *struct_obj_ = (epri_issue_test_res *)struct_obj;
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, session);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, status);
	s2j_json_set_struct_array_element_by_func(json_obj_, struct_obj_, epri_res_images,images,struct_obj_->images_cnt);
	return json_obj_;
}

void *json_to_struct_epri_issue_test_res(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, epri_issue_test_res);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, session);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, status);
	s2j_struct_get_struct_array_element_by_func(struct_obj_, json_obj, epri_res_images,images);
	return struct_obj_;
}

cJSON *struct_to_json_epri_res_images(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	epri_res_images *struct_obj_ = (epri_res_images *)struct_obj;
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, path);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, inference_time);
    printf("struct_obj_->path:%s inference_time:%d issues_cnt:%d issue_type:%s\n",
        struct_obj_->path, struct_obj_->inference_time, struct_obj_->issues_cnt,
        struct_obj_->issues[0].issue_type);
	s2j_json_set_struct_array_element_by_func(json_obj_, struct_obj_, epri_issues,issues,struct_obj_->issues_cnt);
	return json_obj_;
}

void *json_to_struct_epri_res_images(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, epri_res_images);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, path);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, inference_time);
	s2j_struct_get_struct_array_element_by_func(struct_obj_, json_obj, epri_issues,issues);
	return struct_obj_;
}


