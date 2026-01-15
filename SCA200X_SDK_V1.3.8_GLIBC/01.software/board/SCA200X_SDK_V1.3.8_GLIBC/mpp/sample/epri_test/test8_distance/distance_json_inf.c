// Copyright (c) 2021 Cesanta Software Limited
// All rights reserved
//
// Example HTTP client. Connect to `s_url`, send request, wait for a response,
// print the response and exit.
// You can change `s_url` from the command line by executing: ./example YOUR_URL
//
// To enable SSL/TLS, make SSL=OPENSSL or make SSL=MBEDTLS

#include <stdio.h>

#include "distance_json_inf.h"
#include "s2j.h"

cJSON *struct_to_json_Ng_point(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	Ng_point *struct_obj_ = (Ng_point *)struct_obj;	
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, x);
    s2j_json_set_basic_element(json_obj_, struct_obj_, int, y); 
	return json_obj_;
}

void *json_to_struct_Ng_point(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, Ng_point);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, x);  
    s2j_struct_get_basic_element(struct_obj_,json_obj, int, y); 
	return struct_obj_;
}

cJSON *struct_to_json_Ng_images_cmd(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	Ng_images_cmd *struct_obj_ = (Ng_images_cmd *)struct_obj;	
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, path);
    s2j_json_set_basic_element(json_obj_, struct_obj_, string, location); 
    s2j_json_set_struct_element_by_func(json_obj_, struct_obj_, Ng_point,point1);
    s2j_json_set_struct_element_by_func(json_obj_, struct_obj_, Ng_point,point2);
	return json_obj_;
}

void *json_to_struct_Ng_images_cmd(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, Ng_images_cmd);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, path);  
    s2j_struct_get_basic_element(struct_obj_,json_obj, string, location); 
    s2j_struct_get_struct_element_by_func(struct_obj_, json_obj, Ng_point,point1);
    s2j_struct_get_struct_element_by_func(struct_obj_, json_obj, Ng_point,point2);
	return struct_obj_;
}

cJSON *struct_to_json_Ng_distance_test(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	Ng_distance_test *struct_obj_ = (Ng_distance_test *)struct_obj;	
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, session);
    s2j_json_set_basic_element(json_obj_, struct_obj_, string, command); 
    s2j_json_set_basic_element(json_obj_, struct_obj_, string, response_url); 
    s2j_json_set_struct_array_element_by_func(json_obj_, struct_obj_, Ng_images_cmd,images,NG_ARRAY_CNT);
	return json_obj_;
}

void *json_to_struct_Ng_distance_test(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, Ng_distance_test);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, session);  
    s2j_struct_get_basic_element(struct_obj_,json_obj, string, command); 
    s2j_struct_get_basic_element(struct_obj_,json_obj, string, response_url); 
    s2j_struct_get_struct_array_element_by_func(struct_obj_, json_obj, Ng_images_cmd,images);
	return struct_obj_;
}



cJSON *struct_to_json_Ng_images_res(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	Ng_images_res *struct_obj_ = (Ng_images_res *)struct_obj;	
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, path);
    s2j_json_set_basic_element(json_obj_, struct_obj_, double, distance); 
	return json_obj_;
}

void *json_to_struct_Ng_images_res(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, Ng_images_res);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, path);  
    s2j_struct_get_basic_element(struct_obj_,json_obj, double, distance); 
	return struct_obj_;
}


cJSON *struct_to_json_Ng_distance_test_res(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	Ng_distance_test_res *struct_obj_ = (Ng_distance_test_res *)struct_obj;	
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, session);
    s2j_json_set_basic_element(json_obj_, struct_obj_, string, status); 
    s2j_json_set_struct_array_element_by_func(json_obj_, struct_obj_, Ng_images_res,images,struct_obj_->images_cnt);
	return json_obj_;
}

void *json_to_struct_Ng_distance_test_res(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, Ng_distance_test_res);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, session);  
    s2j_struct_get_basic_element(struct_obj_,json_obj, string, status); 
    s2j_struct_get_struct_array_element_by_func(struct_obj_, json_obj, Ng_images_res,images);
	return struct_obj_;
}





cJSON *struct_to_json_Ng_file_target(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	Ng_file_target *struct_obj_ = (Ng_file_target *)struct_obj;
    s2j_json_set_struct_element_by_func(json_obj_, struct_obj_, Ng_point,point1);
    s2j_json_set_struct_element_by_func(json_obj_, struct_obj_, Ng_point,point2);
	return json_obj_;
}

void *json_to_struct_Ng_file_target(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, Ng_file_target);
    s2j_struct_get_struct_element_by_func(struct_obj_, json_obj, Ng_point,point1);
    s2j_struct_get_struct_element_by_func(struct_obj_, json_obj, Ng_point,point2);
	return struct_obj_;
}

cJSON *struct_to_json_Ng_file_result(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	Ng_file_result *struct_obj_ = (Ng_file_result *)struct_obj;
    s2j_json_set_basic_element(json_obj_, struct_obj_, double, distance);    
	return json_obj_;
}

void *json_to_struct_Ng_file_result(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, Ng_file_result);
     s2j_struct_get_basic_element(struct_obj_,json_obj, double, distance); 
	return struct_obj_;
}

cJSON *struct_to_json_Ng_result_data_file(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	Ng_result_data_file *struct_obj_ = (Ng_result_data_file *)struct_obj;
    s2j_json_set_struct_element_by_func(json_obj_, struct_obj_, Ng_file_target,target);  
    s2j_json_set_struct_element_by_func(json_obj_, struct_obj_, Ng_file_result,result);  
	return json_obj_;
}

void *json_to_struct_Ng_result_data_file(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, Ng_result_data_file);
    s2j_struct_get_struct_element_by_func(struct_obj_,json_obj, Ng_file_target,target); 
    s2j_struct_get_struct_element_by_func(struct_obj_,json_obj, Ng_file_result,result); 
	return struct_obj_;
}



