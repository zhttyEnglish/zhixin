// Copyright (c) 2021 Cesanta Software Limited
// All rights reserved
//
// Example HTTP client. Connect to `s_url`, send request, wait for a response,
// print the response and exit.
// You can change `s_url` from the command line by executing: ./example YOUR_URL
//
// To enable SSL/TLS, make SSL=OPENSSL or make SSL=MBEDTLS

#include <stdio.h>

#include "audio_alarm_json_inf.h"
#include "s2j.h"


cJSON *struct_to_json_Ng_audio_alarm_test(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	Ng_audio_alarm_test *struct_obj_ = (Ng_audio_alarm_test *)struct_obj;
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, session);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, command);
	s2j_json_set_basic_element(json_obj_, struct_obj_, int, duration);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, response_url);
	return json_obj_;
}

void *json_to_struct_Ng_audio_alarm_test(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, Ng_audio_alarm_test);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, session);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, command);
	s2j_struct_get_basic_element(struct_obj_,json_obj, int, duration);  
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, response_url);
	return struct_obj_;
}


cJSON *struct_to_json_Ng_audio_alarm_test_res(void* struct_obj)
{
	s2j_create_json_obj(json_obj_);
	Ng_test_res *struct_obj_ = (Ng_test_res *)struct_obj;
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, session);
	s2j_json_set_basic_element(json_obj_, struct_obj_, string, status);
	return json_obj_;
}

void *json_to_struct_Ng_audio_alarm_test_res(cJSON* json_obj)
{
	s2j_create_struct_obj(struct_obj_, Ng_test_res);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, session);
	s2j_struct_get_basic_element(struct_obj_,json_obj, string, status);
	return struct_obj_;
}



