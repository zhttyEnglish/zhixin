#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "reply_type.h"

#if 0
#include "request_type.h"
alarmTypeFrame_t 			alarm_type;				// 报警类型帧
alarmImageFrame_t 			alarm_image;			// 报警图片数据帧
heartBeatFrame_t			heart_beat;				// 心跳数据帧
modelTypeFrame_t 			model_type;				// 模型类型帧
deviceInfoFrame_t 			device_info;			// 设备信息类型帧
getTTUInfoFrame_t 			get_ttu_info;			// 获取TTU信息帧
getUpdateFrame_t 			get_update;				// 升级标志
alarmTypeRecoveryFrame_t 	alarm_recover;			// 报警类型恢复帧
uploadParamFrame_t 			upload_param;			// 抓拍查询
alarmParamFrame_t 			alarm_param;			// 报警参数查询
uploadImageFrame_t 			upload_image;			// 召测类型数据帧
#endif
#define HEAD_FLAG 0
#define VERSION 5
#define TYPE 4
#define PACKET_LEN 6
#define PAYLOAD_START 10


/*	alarm_id 0x0 ~ 0xffffffff
	total_count 报警总数, strlen(alarm_type_list)
	device_name  见 set_deviceInfoFrame
	alarm_type_list 报警类型 "012345" 		
	0 person 检测到人
	1 animal 检测到小动物
	2 water 检测到积水
	3 fire 检测到明火
	4 smoke 检测到烟
	5 waterdrop检测到水滴

	uint8_t list[1] = {0};
	int len = set_alarmTypeFrame(buf, 2, 1, "123456064001", list)
*/    
int set_alarmTypeFrame(char * buf, uint32_t alarm_id, int total_count, char * device_name, uint8_t * alarm_type_list)
{
/*  memset(&alarm_type, 0, sizeof(alarmTypeFrame_t));

	alarm_type.header.head_flag = 0xFCFCFCFC;
	alarm_type.header.version = 0x01;
	alarm_type.header.frame_type = 0;
	alarm_type.payload.alarm_id = alarm_id;
	alarm_type.payload.alarm_count = total_count;
	memcpy(alarm_type.payload.device_name, device_name, 12);
	alarm_type.payload.alarm_type = alarm_type_list;
	alarm_type.payload.finish_flag = 0xFDFD;
	alarm_type.header.package_size = sizeof(alarm_type.payload);
*/
	int i = 0;
	// header
	buf[HEAD_FLAG] = 0xFC; buf[HEAD_FLAG + 1] = 0xFC; buf[HEAD_FLAG + 2] = 0xFC; buf[HEAD_FLAG + 3] = 0xFC;
	buf[VERSION] = 0x01;
	buf[TYPE] = 0x00;
	// payload
	buf[PAYLOAD_START] = alarm_id & 0xFF; buf[PAYLOAD_START + 1] = (alarm_id >> 8) & 0xFF;
	buf[PAYLOAD_START + 2] = (alarm_id >> 16) & 0xFF; buf[PAYLOAD_START + 3] = (alarm_id >> 24) & 0xFF;
	buf[PAYLOAD_START + 4] = total_count;
	memcpy(buf + PAYLOAD_START + 5, device_name, 12);
	for(i = 0; i < total_count; i ++)
	{
		buf[PAYLOAD_START + 17 + i] = alarm_type_list[i] - 0x30;
	}
	buf[PAYLOAD_START + 17 + i] = 0xFD;
	buf[PAYLOAD_START + 18 + i] = 0xFD;
	// package size  角标为18 实际数量为19
	buf[PACKET_LEN] = (19 + total_count) % 256; buf[PACKET_LEN + 1] = (19 + total_count) / 256;
	buf[PACKET_LEN + 2] = (19 + total_count) / 65536; buf[PACKET_LEN + 3] = (19 + total_count) / 16777216;
	
	return total_count + 29;  //返回长度 
}

/*	alarm_id 0x0 ~ 0xffffffff
	total_count 图片帧总数	图片大小按照图片数据体分成的总数
	image_num	图片帧数     从1开始
	data 	图片数据
	data_len 数据长度

	char *data = "123";
	int len = set_alarmImageFrame(buf, 1, 2, 1, data, 3)
*/
int set_alarmImageFrame(char * buf, uint32_t alarm_id, uint32_t total_count, uint32_t image_num, uint8_t * data, int data_len)
{
/*	memset(&alarm_image, 0, sizeof(alarmImageFrame_t));
	
	alarm_image.header.head_flag = 0xFCFCFCFC;
	alarm_image.header.version = 0x01;
	alarm_image.header.frame_type = 1;
	alarm_image.payload.alarm_id = alarm_id;
	alarm_image.payload.total_count = total_count;
	alarm_image.payload.image_num = image_num;
	alarm_image.payload.image_data = data;
	alarm_image.payload.finish_flag = 0xFDFD;
	alarm_image.header.package_size = sizeof(alarm_image.payload);
*/
	// header
	buf[HEAD_FLAG] = 0xFC; buf[HEAD_FLAG + 1] = 0xFC; buf[HEAD_FLAG + 2] = 0xFC; buf[HEAD_FLAG + 3] = 0xFC;
	buf[VERSION] = 0x01;
	buf[TYPE] = 0x01;	
	//payload
	buf[PAYLOAD_START] = alarm_id & 0xFF; buf[PAYLOAD_START + 1] = (alarm_id >> 8) & 0xFF;
	buf[PAYLOAD_START + 2] = (alarm_id >> 16) & 0xFF; buf[PAYLOAD_START + 3] = (alarm_id >> 24) & 0xFF;
	buf[PAYLOAD_START + 4] = total_count & 0xFF; buf[PAYLOAD_START + 5] = (total_count >> 8) & 0xFF;
	buf[PAYLOAD_START + 6] = (total_count >> 16) & 0xFF; buf[PAYLOAD_START + 7] = (total_count >> 24) & 0xFF;
	buf[PAYLOAD_START + 8] = image_num & 0xFF; buf[PAYLOAD_START + 9] = (image_num >> 8) & 0xFF;
	buf[PAYLOAD_START + 10] = (image_num >> 16) & 0xFF; buf[PAYLOAD_START + 11] = (image_num >> 24) & 0xFF;
	memcpy(buf + PAYLOAD_START + 12, (char *)data, data_len);
	buf[PAYLOAD_START + 12 + data_len] = 0xFD;
	buf[PAYLOAD_START + 13 + data_len] = 0xFD;
	//packet len
	buf[PACKET_LEN] = (14 + data_len) % 256; buf[PACKET_LEN + 1] = (14 + data_len) / 256;
	buf[PACKET_LEN + 2] = (14 + data_len) / 65536; buf[PACKET_LEN + 3] = (14 + data_len) / 16777216;
	
	return data_len + 24;
}

/*  status_num  =  strlen(device_status) 
	device_status  0 在线 1 离线 按照 device_info 中设备的顺序
	sprintf(heartneat_time, "%4d%02d%02dT%02d02d%02dz", year, month, day, hour, min, sec);

	uint8_t status[2] = {0, 0};
	char time[30] = {0};
	sprintf(time, "%s", "2024-12-04T131528z");
	int len = set_heartBeatFrame(buf, 2, status, time);
*/
int set_heartBeatFrame(char * buf, uint8_t status_num, uint8_t * device_status, char * heartbeat_time)
{
/*	memset(&heart_beat, 0, sizeof(heartBeatFrame_t));
	
	heart_beat.header.head_flag = 0xFCFCFCFC;
	heart_beat.header.version = 0x01;
	heart_beat.header.frame_type = 2;
	heart_beat.payload.status_num = status_num;
	heart_beat.payload.device_status = device_status;
	memcpy(heart_beat.payload.heartbeat_time, heartbeat_time, 30);
	heart_beat.payload.finish_flag = 0xFDFD;
	heart_beat.header.package_size = sizeof(heart_beat.payload);
*/
	int i = 0;
	// header
	buf[HEAD_FLAG] = 0xFC; buf[HEAD_FLAG + 1] = 0xFC; buf[HEAD_FLAG + 2] = 0xFC; buf[HEAD_FLAG + 3] = 0xFC;
	buf[VERSION] = 0x01;
	buf[TYPE] = 0x02;	
	//payload
	buf[PAYLOAD_START] = status_num;
	for(i = 0; i < status_num; i ++){
		buf[PAYLOAD_START + i + 1] = device_status[i];
	}
	memcpy(buf + PAYLOAD_START + i + 1, heartbeat_time, 30);
	buf[PAYLOAD_START + 31 + i] = 0xFD;
	buf[PAYLOAD_START + 32 + i] = 0xFD;
	//packet len
	buf[PACKET_LEN] = (33 + i) % 256; buf[PACKET_LEN + 1] = (33 + i) / 256;
	buf[PACKET_LEN + 2] = (33 + i) / 65536; buf[PACKET_LEN + 3] = (33 + i) / 16777216;
	
	return status_num + 43;
}


/*  total_count = sizeof(model_type_list) / sizeof(model_type_list[0])
	model_type_list = {{"person"}, {"water"}, {"animal"}}

	char model[5][12] = {"person","animal","water", "fire", "smoke"};
	int len = set_modelTypeFrame(buf, 5, model);
*/
int set_modelTypeFrame(char * buf, uint8_t total_count, char model_type_list[][12])
{
/*	memset(&model_type, 0, sizeof(modelTypeFrame_t));
	
	model_type.header.head_flag = 0xFCFCFCFC;
	model_type.header.version = 0x01;
	model_type.header.frame_type = 3;
	model_type.payload.total_count = total_count;
	model_type.payload.model_type = model_type_list;
	model_type.payload.finish_flag = 0xFDFD;
	model_type.header.package_size = sizeof(model_type.payload);
*/
	int i = 0, pos = 0;
	// header
	buf[HEAD_FLAG] = 0xFC; buf[HEAD_FLAG + 1] = 0xFC; buf[HEAD_FLAG + 2] = 0xFC; buf[HEAD_FLAG + 3] = 0xFC;
	buf[VERSION] = 0x01;
	buf[TYPE] = 0x03;	
	//payload
	buf[PAYLOAD_START] = total_count;
	pos = PAYLOAD_START + 1;
	for(i = 0; i < total_count; i ++){
		memcpy(buf + pos, &model_type_list[i], strlen(model_type_list[i]));
		pos += strlen(model_type_list[i]);
		if(i + 1 < total_count){  // ';'
			buf[pos] = 0x3B;
			pos += 1;
		}
	}
	buf[pos] = 0xFD;
	buf[pos + 1] = 0xFD;
	pos += 2;
	//packet len
	buf[PACKET_LEN] = (pos - 10) % 256; buf[PACKET_LEN + 1] = (pos - 10) / 256;
	buf[PACKET_LEN + 2] = (pos - 10) / 65536; buf[PACKET_LEN + 3] = (pos - 10) / 16777216;
	
	return pos;
}

/*	heartbeat_time 默认 300s
	device_num  1(设备) + n-1(摄像头)
	version H1.1;V03.1-002   --- V03.1:APP的版本号 002：识别模型的版本号
	device_name 6(厂商id) + 3(ip后三位) + 1(值为n 表示接入的第n+1个盒子) + 2(设备id 00—AI盒子 0x x号摄像机. 1x--其他设备1) 
	例：厂商id 123456, 盒子1 IP:192.168.31.101, 盒子2 IP:192.168.31.201 盒子1的摄像头1 ip: 192.168.31.102 盒子2的摄像头2 ip: 192.168.31.202
	device_name 盒子1 123456 101 0 00 盒子2 123456 201 1 00
	camera_name 盒子1的摄像头1 123456 102 0 01  盒子2的摄像头2 123456 202 1 02

	char device[2][13] = {"123456111000", "123456064001"};
	int len = set_deviceInfoFrame(buf, 30, 2, "[H1.1;V01.03.01]", device);
*/
int set_deviceInfoFrame(char * buf, uint16_t heartbeat_time, uint8_t device_num, char * version, char device_name[][13])
{
/*	memset(&device_info, 0, sizeof(deviceInfoFrame_t));
	
	device_info.header.head_flag = 0xFCFCFCFC;
	device_info.header.version = 0x01;
	device_info.header.frame_type = 4;
	device_info.payload.heartbeat_frequency = heartbeat_time;
	device_info.payload.device_num = device_num;
	memcpy(device_info.payload.software_version, version, 16);
	memcpy(device_info.payload.device_name, device_name, 14);
	device_info.payload.camera_name = camera_name;
	device_info.payload.finish_flag = 0xFDFD;
	device_info.header.package_size = sizeof(device_info.payload);
*/
	int i = 0, pos = 0;
	// header
	buf[HEAD_FLAG] = 0xFC; buf[HEAD_FLAG + 1] = 0xFC; buf[HEAD_FLAG + 2] = 0xFC; buf[HEAD_FLAG + 3] = 0xFC;
	buf[VERSION] = 0x01;
	buf[TYPE] = 0x04;	
	// payload 
	buf[PAYLOAD_START] = heartbeat_time & 0xFF; buf[PAYLOAD_START + 1] = (heartbeat_time >> 8) & 0xFF;
	buf[PAYLOAD_START + 2] = device_num;
	memcpy(buf + PAYLOAD_START + 3, version, 16);
	buf[PAYLOAD_START + 19] = 0x7B;
	pos = PAYLOAD_START + 20;
	for(i = 0; i < device_num; i ++){
		memcpy(buf + pos, &device_name[i], strlen(device_name[i]));
		pos += strlen(device_name[i]);
		if(i + 1 < device_num){  // ';'
			buf[pos] = 0x3B;
			pos += 1;
		}
	}
	buf[pos] = 0x7D;
	buf[pos + 1] = 0xFD;
	buf[pos + 2] = 0xFD;
	pos += 3;
	//packet len
	buf[PACKET_LEN] = (pos - 10) % 256; buf[PACKET_LEN + 1] = (pos - 10) / 256;
	buf[PACKET_LEN + 2] = (pos - 10) / 65536; buf[PACKET_LEN + 3] = (pos - 10) / 16777216;
	
	return pos;
}

// param 1--gettime
//int len = set_getTTUInfoFrame(buf, 1);
int set_getTTUInfoFrame(char * buf, uint16_t param)
{
/*	memset(&get_ttu_info, 0, sizeof(getTTUInfoFrame_t));
	
	get_ttu_info.header.head_flag = 0xFCFCFCFC;
	get_ttu_info.header.version = 0x01;
	get_ttu_info.header.frame_type = 5;
	get_ttu_info.payload.parameter = param;
	get_ttu_info.payload.finish_flag = 0xFDFD;
	get_ttu_info.header.package_size = sizeof(get_ttu_info.payload);
*/
	// header
	buf[HEAD_FLAG] = 0xFC; buf[HEAD_FLAG + 1] = 0xFC; buf[HEAD_FLAG + 2] = 0xFC; buf[HEAD_FLAG + 3] = 0xFC;
	buf[VERSION] = 0x01;
	buf[TYPE] = 0x05;
	// payload
	buf[PAYLOAD_START] = param & 0xFF;  buf[PAYLOAD_START + 1] = (param >> 8) & 0xFF;
	buf[PAYLOAD_START + 2] = 0xFD;  buf[PAYLOAD_START + 3] = 0xFD;
	// packet len
	buf[PACKET_LEN] = 0x04; buf[PACKET_LEN + 1] = 0x00; buf[PACKET_LEN + 2] = 0x00; buf[PACKET_LEN + 3] = 0x00; 
	
	return 14;
}

// param 1 -- 是否有升级文件 2 -- 设备终端等待文件
//int len = set_getUpdateFrame(buf, 1);
int set_getUpdateFrame(char * buf, uint16_t param)
{
/*	memset(&get_update, 0, sizeof(getUpdateFrame_t));
	
	get_update.header.head_flag = 0xFCFCFCFC;
	get_update.header.version = 0x01;
	get_update.header.frame_type = 6;
	get_update.payload.update_flag = update_flag;
	get_update.payload.finish_flag = 0xFDFD;
	get_update.header.package_size = sizeof(get_update.payload);
*/
	// header
	buf[HEAD_FLAG] = 0xFC; buf[HEAD_FLAG + 1] = 0xFC; buf[HEAD_FLAG + 2] = 0xFC; buf[HEAD_FLAG + 3] = 0xFC;
	buf[VERSION] = 0x01;
	buf[TYPE] = 0x06;	
	// payload
	buf[PAYLOAD_START] = param & 0xFF;  buf[PAYLOAD_START + 1] = (param >> 8) & 0xFF;
	buf[PAYLOAD_START + 2] = 0xFD;  buf[PAYLOAD_START + 3] = 0xFD;
	// packet len
	buf[PACKET_LEN] = 0x04; buf[PACKET_LEN + 1] = 0x00; buf[PACKET_LEN + 2] = 0x00; buf[PACKET_LEN + 3] = 0x00;
	
	return 14;
}

/*	num 报警类型恢复总数
	device_name alarm_recover_list 见 set_alarmTypeFrame
	char list[1] = {0};
	int len = set_alarmTypeRecoveryFrame(buf, 1, "123456064001", list);
*/
int set_alarmTypeRecoveryFrame(char * buf, uint8_t num, char * device_name, char * alarm_recover_list)
{
/*	memset(&alarm_recover, 0, sizeof(alarmTypeRecoveryFrame_t));
	
	alarm_recover.header.head_flag = 0xFCFCFCFC;
	alarm_recover.header.version = 0x01;
	alarm_recover.header.frame_type = 7;
	alarm_recover.payload.alarm_recovery = alarm_recover_list;
	memcpy(alarm_recover.payload.device_name, device_name, 12);
	alarm_recover.payload.recover_num = num;
	alarm_recover.payload.finish_flag = 0xFDFD;
	alarm_recover.header.package_size = sizeof(alarm_recover.payload);
*/
	int i = 0;
	// header
	buf[HEAD_FLAG] = 0xFC; buf[HEAD_FLAG + 1] = 0xFC; buf[HEAD_FLAG + 2] = 0xFC; buf[HEAD_FLAG + 3] = 0xFC;
	buf[VERSION] = 0x01;
	buf[TYPE] = 0x07;	
	// payload
	buf[PAYLOAD_START] = num;
	memcpy(buf + PAYLOAD_START + 1, device_name, 12);
	for(i = 0; i < num; i ++){
		buf[PAYLOAD_START + 13 + i] = alarm_recover_list[i];
	}
	buf[PAYLOAD_START + 13 + i] = 0xFD;
	buf[PAYLOAD_START + 14 + i] = 0xFD;
	// package size  角标为14 实际数量为15
	buf[PACKET_LEN] = (15 + i) % 256; buf[PACKET_LEN + 1] = (15 + i) / 256;
	buf[PACKET_LEN + 2] = (15 + i) / 65536; buf[PACKET_LEN + 3] = (15 + i) / 16777216;
	
	return i + 25;
}

//	param 1 — 查询定时上报参数
// set_uploadParamFrame(buf, 1);
int set_uploadParamFrame(char * buf, uint16_t type)
{
/*	memset(&upload_param, 0, sizeof(uploadParamFrame_t));
	
	upload_param.header.head_flag = 0xFCFCFCFC;
	upload_param.header.version = 0x01;
	upload_param.header.frame_type = 8;
	upload_param.payload.snapshot_type = type;
	upload_param.payload.finish_flag = 0xFDFD;
	upload_param.header.package_size = sizeof(upload_param.payload);
*/
	// header
	buf[HEAD_FLAG] = 0xFC; buf[HEAD_FLAG + 1] = 0xFC; buf[HEAD_FLAG + 2] = 0xFC; buf[HEAD_FLAG + 3] = 0xFC;
	buf[VERSION] = 0x01;
	buf[TYPE] = 0x08;	
	// payload
	buf[PAYLOAD_START] = type & 0xFF;  buf[PAYLOAD_START + 1] = (type >> 8) & 0xFF;
	buf[PAYLOAD_START + 2] = 0xFD;  buf[PAYLOAD_START + 3] = 0xFD;
	// packet len
	buf[PACKET_LEN] = 0x04; buf[PACKET_LEN + 1] = 0x00; buf[PACKET_LEN + 2] = 0x00; buf[PACKET_LEN + 3] = 0x00; 
	
	return 14;
}


//	type 1---报警参数  2---报警分类
// set_alarmParamFrame(buf, 1);
int set_alarmParamFrame(char * buf, uint16_t type)
{
/*	memset(&alarm_param, 0, sizeof(alarmParamFrame_t));
	
	alarm_param.header.head_flag = 0xFCFCFCFC;
	alarm_param.header.version = 0x01;
	alarm_param.header.frame_type = 9;
	alarm_param.payload.alarm_param_type = type;
	alarm_param.payload.finish_flag = 0xFDFD;
	alarm_param.header.package_size = sizeof(alarm_param.payload);
*/
	// header
	buf[HEAD_FLAG] = 0xFC; buf[HEAD_FLAG + 1] = 0xFC; buf[HEAD_FLAG + 2] = 0xFC; buf[HEAD_FLAG + 3] = 0xFC;
	buf[VERSION] = 0x01;
	buf[TYPE] = 0x09;
	// payload
	buf[PAYLOAD_START] = type & 0xFF;  buf[PAYLOAD_START + 1] = (type >> 8) & 0xFF;
	buf[PAYLOAD_START + 2] = 0xFD;  buf[PAYLOAD_START + 3] = 0xFD;
	// packet len
	buf[PACKET_LEN] = 0x04; buf[PACKET_LEN + 1] = 0x00; buf[PACKET_LEN + 2] = 0x00; buf[PACKET_LEN + 3] = 0x00; 
	
	return 14;
}


/*	type 召测类型 1—召测 2—早上定时  3—晚上定时 4—远程许可打开  5--远程许可关闭
	camera_id 摄像头ID	1---N
	total_count 图片帧总数	图片大小按照图片数据体分成的总数
	num 图片帧序列号	从序号1开始计数…
	data 图片数据体	必须满足N<1Mbyte

	uint8_t * data = "123";
	int len = set_uploadImageFrame(buf, 2, 1, 2, 1, data, 3);
*/
int set_uploadImageFrame(char * buf, uint16_t type, uint16_t camera_id, uint32_t total_count, uint32_t num, uint8_t * data, int data_len)
{
/*	memset(&upload_image, 0, sizeof(uploadImageFrame_t));
	
	upload_image.header.head_flag = 0xFCFCFCFC;
	upload_image.header.version = 0x01;
	upload_image.header.frame_type = 10;
	upload_image.payload.type = type;
	upload_image.payload.camera_id = camera_id;
	upload_image.payload.image_count = total_count;
	upload_image.payload.image_num = num;
	upload_image.payload.image_data = data;
	upload_image.payload.finish_flag = 0xFDFD;
	upload_image.header.package_size = sizeof(upload_image.payload);
*/
	// header
	buf[HEAD_FLAG] = 0xFC; buf[HEAD_FLAG + 1] = 0xFC; buf[HEAD_FLAG + 2] = 0xFC; buf[HEAD_FLAG + 3] = 0xFC;
	buf[VERSION] = 0x01;
	buf[TYPE] = 0x0A;	
	// payload
	buf[PAYLOAD_START] = type & 0xFF;  buf[PAYLOAD_START + 1] = (type >> 8) & 0xFF;
	buf[PAYLOAD_START + 2] = camera_id & 0xFF;  buf[PAYLOAD_START + 3] = (camera_id >> 8) & 0xFF;
	buf[PAYLOAD_START + 4] = total_count & 0xFF; buf[PAYLOAD_START + 5] = (total_count >> 8) & 0xFF;
	buf[PAYLOAD_START + 6] = (total_count >> 16) & 0xFF; buf[PAYLOAD_START + 7] = (total_count >> 24) & 0xFF;
	buf[PAYLOAD_START + 8] = num & 0xFF; buf[PAYLOAD_START + 9] = (num >> 8) & 0xFF;
	buf[PAYLOAD_START + 10] = (num >> 16) & 0xFF; buf[PAYLOAD_START + 11] = (num >> 24) & 0xFF;
	memcpy(buf + PAYLOAD_START + 12, (char *)data, data_len);
	buf[PAYLOAD_START + 12 + data_len] = 0xFD;
	buf[PAYLOAD_START + 13 + data_len] = 0xFD;
	//packet len
	buf[PACKET_LEN] = (14 + data_len) % 256; buf[PACKET_LEN + 1] = (14 + data_len) / 256;
	buf[PACKET_LEN + 2] = (14 + data_len) / 65536; buf[PACKET_LEN + 3] = (14 + data_len) / 16777216;
	
	return data_len + 24;
}


#if 0
/*	param 参数返回类型	获取时间类型：1---获取时间
	len	返回参数长度N	时间----默认30bytes
	data 参数内容	格式：20190301T093009z 备注：多余字节为空字节

	set_ttu_reply(buf, 1, 19, "2024/12/04 13:02:44");
*/
int set_ttu_reply(char * buf, uint16_t param, uint8_t len, char * data)
{
	// header
	buf[0] = 0xFD;buf[1] = 0xFD;buf[2] = 0xFD;buf[3] = 0xFD;
	buf[4] = 0x05;
	// payload
	buf[9] = param & 0xFF; buf[10] = (param >> 8) & 0xFF;
	buf[11] = len;
	memcpy(buf + 12, data, len);
	buf[len + 12] = 0xFC; buf[len + 13] = 0xFC;
	// packet len
	buf[5] = (len + 5) % 256; buf[6] = (len + 5) / 256;
	buf[7] = (len + 5) / 65536; buf[8] = (len + 5) / 16777216;

	return len + 14;
}

/*	param 获取类型：1---获取升级标志 
	flag	返回升级标志	0—无升级包  1---有升级包（升级包文件名: Update-Vxx.xx.bin）

	set_updata_reply(buf, 1, 0);
*/
int set_updata_reply(char *buf, uint16_t param, uint8_t flag)
{
	// header
	buf[0] = 0xFD;buf[1] = 0xFD;buf[2] = 0xFD;buf[3] = 0xFD;
	buf[4] = 0x06;
	// payload
	buf[9] = param & 0xFF; buf[10] = (param >> 8) & 0xFF;
	buf[11] = flag;
	buf[12] = 0xFC; buf[13] = 0xFC;
	// packet len
	buf[5] = 0x05; buf[6] = 0; buf[7] = 0; buf[8] = 0;
	
	return 14;
}


/*	snap_type 抓拍类型	1—定时上报参数 2—召测 3—远程许可
	status	远程许可状态	0—打开远程许可   1—关闭远程许可
	cameraid	召测相机ID	和下面参数选择。(定时抓拍2选一)
	snap_switch 定时抓拍开关	0—-关   1---开 (召测抓拍2选一)
	time 早上加晚上定时时间	格式：{08:00;17:30}
	set_snapshot_reply(buf, 1, 0, 0, 1, "{16:56;16:59}");
*/
int set_snapshot_reply(char *buf, uint16_t snap_type, uint16_t status, uint16_t cameraid, uint8_t snap_switch, char * time)
{
	// header
	buf[0] = 0xFD;buf[1] = 0xFD;buf[2] = 0xFD;buf[3] = 0xFD;
	buf[4] = 0x08;
	// payload
	buf[9] = snap_type & 0xFF; buf[10] = (snap_type >> 8) & 0xFF;
	
	if(snap_type == 0x01)
	{
		buf[11] = snap_switch;
		memcpy(buf + 12, time, 13);
		buf[25] = 0xFC; buf[26] = 0xFC;
		buf[5] = 0x12; buf[6] = 0; buf[7] = 0; buf[8] = 0;
		
		return 27;
	}
	else if(snap_type == 0x02){
		buf[11] = cameraid & 0xFF;
		buf[12] = (cameraid >> 8) & 0xFF;
		buf[13] = 0xFC; 
		buf[14] = 0xFC;
		buf[5] = 0x06; buf[6] = 0; buf[7] = 0; buf[8] = 0;
		
		return 15;
	}
	else if(snap_type == 0x03){
		buf[11] = status & 0xFF;
		buf[12] = (status >> 8) & 0xFF;
		buf[13] = 0xFC; 
		buf[14] = 0xFC;
		buf[5] = 0x06; buf[6] = 0; buf[7] = 0; buf[8] = 0;
		
		return 15;
	}
	return 0;
}


/*	alarm_param 报警参数类别	1---报警参数  2---报警分类
	alarm_lv_cnt 报警等级数量	默认4,最大9
	alarmtime1 一级报警时间	单位（秒），默认300秒发送一次
	alarmtime2 二级报警时间	单位（秒），默认60秒
	alarmtime3 三级报警时间	单位（秒），默认10秒， 0表示实时发送
	alarm_model_len 模型报警长度N	0xFFFF
	model Model等级格式	{person=1;fire=2;water=3}

	set_alarmparam_reply(buf, 1, 4, 300, 60, 15, 0, NULL);
	set_alarmparam_reply(buf, 2, 0, 0, 0, 0, 54, "{person=0;animal=0;water=0;fire=1;smoke=0;waterdrop=0}");
*/
int set_alarmparam_reply(char * buf, uint16_t alarm_param, uint8_t alarm_lv_cnt, uint16_t alarmtime1, uint16_t alarmtime2, 
								uint16_t alarmtime3, uint16_t alarm_model_len, char * model)
{
	// header
	buf[0] = 0xFD;buf[1] = 0xFD;buf[2] = 0xFD;buf[3] = 0xFD;
	buf[4] = 0x09;
	// payload
	buf[9] = alarm_param & 0xFF; buf[10] = (alarm_param >> 8) & 0xFF;

	if(alarm_param == 1){
		buf[11] = alarm_lv_cnt;
		buf[12] = alarmtime1 & 0xFF; buf[13] = (alarmtime1 >> 8) & 0xFF;
		buf[14] = alarmtime2 & 0xFF; buf[15] = (alarmtime2 >> 8) & 0xFF;
		buf[16] = alarmtime3 & 0xFF; buf[17] = (alarmtime3 >> 8) & 0xFF;
		buf[18] = 0xFC; buf[19] = 0xFC;
		buf[5] = 0x0B; buf[6] = 0; buf[7] = 0; buf[8] = 0;
		
		return 20;
	}
	else if(alarm_param == 2){
		buf[11] = alarm_model_len & 0xFF; buf[12] = (alarm_model_len >> 8) & 0xFF;
		memcpy(buf + 13, model, alarm_model_len);
		buf[alarm_model_len + 13] = 0xFC; buf[alarm_model_len + 14] = 0xFC;
		buf[5] = (alarm_model_len + 6) % 256; buf[6] = (alarm_model_len + 6) / 256;
		buf[7] = (alarm_model_len + 6) / 65536; buf[8] = (alarm_model_len + 6) / 16777216;
		
		return alarm_model_len + 15;
	}
	return 0;
}
#endif
								
