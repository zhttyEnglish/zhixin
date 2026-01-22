#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#define _XOPEN_SOURCE
#define __USE_XOPEN
#include <time.h>

#include "log.h"
#include "reply_type.h"
//#include "request_type.h"
#include "iniparser.h"
#include "dictionary.h"
#include "handle_config.h"
#include "tcp.h"

getTTUReplyFrame_t ttu_reply;
getUpdateReplyFrame_t update_reply;
hostIssueFrame_t	host_reply;
alarmLevelSetFrame_t	alarm_lv_reply;
#if 0
extern alarmTypeFrame_t 			alarm_type;				// 报警类型�?
extern alarmImageFrame_t 			alarm_image;			// 报警图片数据�?
extern heartBeatFrame_t				heart_beat;				// 心跳数据�?
extern modelTypeFrame_t 			model_type;				// 模型类型�?
extern deviceInfoFrame_t 			device_info;			// 设备信息类型�?
extern getTTUInfoFrame_t 			get_ttu_info;			// 获取TTU信息�?
extern getUpdateFrame_t 			get_update;				// 升级标志
extern alarmTypeRecoveryFrame_t 	alarm_recover;			// 报警类型恢复�?
extern uploadParamFrame_t 			upload_param;			// 抓拍查询
extern alarmParamFrame_t 			alarm_param;			// 报警参数查询
extern uploadImageFrame_t 			upload_image;			// 召测类型数据�?
#endif
extern int remote_permission;   // 0 打开远程许可 不用报警   1 关闭远程许可 可以报警 
extern int timed_snapshot_switch; // 定时抓拍开�?0 �?1 开
extern int snapshot_hour1;	// 定时抓拍时间 hour1 �?小时 hour2 晚小�?
extern int snapshot_hour2;
extern int snapshot_min1;
extern int snapshot_min2;
extern int remote_test_camera_id;
extern int alarm_level;
extern int lv1_alarm_time;
extern int lv2_alarm_time;
extern int lv3_alarm_time;
extern char model[256];
extern cam_status_t cam_status[4];
extern int snapshot_type;

int parse_TTU_reply(uint8_t * buf, int buf_size)
{
	memset(&ttu_reply, 0, sizeof(getTTUReplyFrame_t));
	
 	ttu_reply.payload.param_type = (uint16_t )((buf[10] << 8) | buf[9]);
	ttu_reply.payload.len = buf[11];
	ttu_reply.payload.data = (char *)malloc(buf[12]);
	memcpy(ttu_reply.payload.data, buf + 12, buf[11]);

	log("param_type = %d len = %d data: %s\r\n", ttu_reply.payload.param_type, ttu_reply.payload.len, ttu_reply.payload.data);

	struct tm timeinfo;
	//2024/12/04 13:02:44
    strptime(ttu_reply.payload.data, "%Y/%m/%d %H:%M:%S",  &timeinfo);
    int timeStamp = mktime(&timeinfo);
    log("timeStamp = %d\r\n", timeStamp);

	char date_cmd[64] = {0};
	char hwclock_cmd[64] = {0};
	sprintf(date_cmd, "sudo timedatectl set-time \"%04d-%02d-%02d %02d:%02d:%02d\"", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, 
			timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
	sprintf(hwclock_cmd, "sudo hwclock --set --date=\"%04d-%02d-%02d %02d:%02d:%02d\"", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, 
			timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
	log("date_cmd = %s\r\n", date_cmd);
	log("hwclock_cmd = %s\r\n", hwclock_cmd);
	system("timedatectl set-ntp no");
	system(date_cmd);
	system(hwclock_cmd);

	free(ttu_reply.payload.data);
	
	return 0;
}

int parse_update_reply(uint8_t * buf, int buf_size)
{
	memset(&update_reply, 0, sizeof(getUpdateReplyFrame_t));
	
	update_reply.payload.para_type = (uint16_t )((buf[10] << 8) | buf[9]);
	update_reply.payload.update_flag = buf [11];

	log("param_type = %d update_flag = %d \r\n", update_reply.payload.para_type, update_reply.payload.update_flag);

	// todo :
	if(update_reply.payload.update_flag == 1)
	{
		char buf[32] = {0};
		send_getupdate(buf, 2);
	}
	
	return 0;
}

extern int upload_image_for_permittion;
int parse_snapshot(uint8_t * buf, int buf_size)
{
	void *dictionary = iniparser_load(PARAM_INI_PATH);
    if (!dictionary) {
        log("cannot parse\r\n");
        return -2;
    }
	
	memset(&host_reply, 0, sizeof(hostIssueFrame_t));

	host_reply.payload.snapshot_type = (uint16_t )((buf[10] << 8) | buf[9]);
	if(host_reply.payload.snapshot_type == 1)
	{
		host_reply.payload.snapshot_switch = buf[11];
		memcpy(host_reply.payload.snapshot_time, buf + 12, 13);
		log("snapshot_type = %d snapshot_switch = %d snapshot_time %s \r\n", host_reply.payload.snapshot_type, host_reply.payload.snapshot_switch, host_reply.payload.snapshot_time);

		timed_snapshot_switch = host_reply.payload.snapshot_switch;
		sscanf(host_reply.payload.snapshot_time, "{%d:%d;%d:%d}", &snapshot_hour1, &snapshot_min1, &snapshot_hour2, &snapshot_min2);

		char hour1[3] = {0};char hour2[3] ={0};char min1[3] = {0};char min2[3] = {0};
		sprintf(hour1, "%d", snapshot_hour1);sprintf(hour2, "%d", snapshot_hour2);
		sprintf(min1, "%d", snapshot_min1);sprintf(min2, "%d", snapshot_min2);
		log("====== %d:%d %d:%d ======\r\n", snapshot_hour1, snapshot_min1, snapshot_hour2, snapshot_min2);
		iniparser_set(dictionary, "param:timed_snapshot_switch", timed_snapshot_switch == 1 ? "TRUE" : "FALSE");
		iniparser_set(dictionary, "param:snapshot_hour1", hour1);
		iniparser_set(dictionary, "param:snapshot_min1", min1);
		iniparser_set(dictionary, "param:snapshot_hour2", hour2);
		iniparser_set(dictionary, "param:snapshot_min2", min2);
		
	}
	else if(host_reply.payload.snapshot_type == 2 )
	{
		host_reply.payload.data = (uint16_t )((buf[12] << 8) | buf[11]);
		log("snapshot_type = %d camera_id = %d \r\n", host_reply.payload.snapshot_type, host_reply.payload.data);

		remote_test_camera_id = host_reply.payload.data;
		snapshot_type = 2;
		//char id[3] = {0};
		//sprintf(id, "%d", remote_test_camera_id);
		//iniparser_set(dictionary, "common:remote_test_camera_id", id);
	}
	else if(host_reply.payload.snapshot_type == 3)
	{
		host_reply.payload.data = (uint16_t )((buf[12] << 8) | buf[11]);
		log("snapshot_type = %d permit = %d \r\n", host_reply.payload.snapshot_type, host_reply.payload.data);
		
		remote_permission = host_reply.payload.data;
		if(remote_permission == 1){
			upload_image_for_permittion = 1;
		}else{
			upload_image_for_permittion = 2;
		}
		for(int i = 0; i < 4; i ++){
			if(strncmp(cam_status[i].device_name, "000000", 6) != 0){
				cam_status[i].remote_upload = 0;
			}		
		}
		iniparser_set(dictionary, "param:remote_permission", remote_permission == 1 ? "TRUE" : "FALSE");
	}

	FILE *fp = fopen(PARAM_INI_PATH, "w+");
    if (!fp) {
        log("iniparser: cannot open\r\n");
        iniparser_freedict(dictionary);
    	return -1;
    }
    iniparser_dump_ini(dictionary, fp);
    fclose(fp);
	
	return 0; 
}

int parse_alarm_level_reply(uint8_t * buf, int buf_size)
{
//	log("-----\r\n");
	void *dictionary = iniparser_load(PARAM_INI_PATH);
    if (!dictionary) {
        log("cannot parse\r\n");
        return -2;
    }
	
	memset(&alarm_lv_reply, 0, sizeof(alarmLevelSetFrame_t));

	alarm_lv_reply.payload.finish_flag = 0xFCFC;
	alarm_lv_reply.payload.alarm_param = (uint16_t )((buf[10] << 8) | buf[9]);
	if(alarm_lv_reply.payload.alarm_param == 1)
	{
		alarm_lv_reply.payload.alarm_level_count = buf[11];
		alarm_lv_reply.payload.lv1_alarm_time = (uint16_t )((buf[13] << 8) | buf[12]);
		alarm_lv_reply.payload.lv2_alarm_time = (uint16_t )((buf[15] << 8) | buf[14]);
		alarm_lv_reply.payload.lv3_alarm_time = (uint16_t )((buf[17] << 8) | buf[16]);
		log("alarm_param = %d alarm_level_count = %d \r\n", alarm_lv_reply.payload.alarm_param, alarm_lv_reply.payload.alarm_level_count);
		log("lv1_alarm_time = %d lv2_alarm_time = %d lv3_alarm_time %d\r\n", alarm_lv_reply.payload.lv1_alarm_time, alarm_lv_reply.payload.lv2_alarm_time, alarm_lv_reply.payload.lv3_alarm_time);

		alarm_level = alarm_lv_reply.payload.alarm_level_count;
		lv1_alarm_time = alarm_lv_reply.payload.lv1_alarm_time;
		lv2_alarm_time = alarm_lv_reply.payload.lv2_alarm_time;
		lv3_alarm_time = alarm_lv_reply.payload.lv3_alarm_time;
		
		char lv[5] = {0}; char lv1[5] = {0}; char lv2[5] = {0}; char lv3[5] = {0};
		sprintf(lv, "%d", alarm_level); sprintf(lv1, "%d", lv1_alarm_time);
		sprintf(lv2, "%d", lv2_alarm_time); sprintf(lv3, "%d", lv3_alarm_time);
		log("lv %s lv1 %s lv2 %s lv3 %s\r\n", lv, lv1, lv2, lv3);
		iniparser_set(dictionary, "param:alarm_level", lv);
		iniparser_set(dictionary, "param:lv1_alarm_time", lv1);
		iniparser_set(dictionary, "param:lv2_alarm_time", lv2);
		iniparser_set(dictionary, "param:lv3_alarm_time", lv3);
	}
	else if(alarm_lv_reply.payload.alarm_param == 2)
	{
		log("alarm_lv_reply.payload.alarm_param\r\n");
		alarm_lv_reply.payload.alarm_model_len = (uint16_t )((buf[12] << 8) | buf[11]);
		alarm_lv_reply.payload.model = (char *)malloc(alarm_lv_reply.payload.alarm_model_len);
		memcpy(alarm_lv_reply.payload.model, buf + 13, alarm_lv_reply.payload.alarm_model_len);
		log("alarm_param = %d alarm_model_len = %d model %s\r\n", alarm_lv_reply.payload.alarm_param, alarm_lv_reply.payload.alarm_model_len, alarm_lv_reply.payload.model);
		memcpy(model, alarm_lv_reply.payload.model, alarm_lv_reply.payload.alarm_model_len);
		
		iniparser_set(dictionary, "param:model", model);
		free(alarm_lv_reply.payload.model);
	}
	
	FILE *fp = fopen(PARAM_INI_PATH, "w+");
    if (!fp) {
        log("iniparser: cannot open\r\n");
        iniparser_freedict(dictionary);
    	return -1;
    }

    iniparser_dump_ini(dictionary, fp);
    fclose(fp);
	
	return 0; 
}


int TTU_init_flag = 0;
int handle_recv(int sockfd, uint8_t * buf, int buf_size)
{
	if(buf[0] != 0xFD || buf[1] != 0xFD || buf[2] != 0xFD || buf[3] != 0xFD) //检查包�?
	{
		log("packet header error\r\n");
		return -1;
	}

	if(buf[5] + buf[6] * 256 + buf[7] * 65536 + buf[8] * 256 * 65536 != buf_size - 9 && buf_size > 9) //检查长�?
	{
		log("recv Multiple packages\r\n");
		int i = 0, j = 0, k = 0;
		for(i = 0; i < buf_size - 1; i ++){
			if(buf[i] == 0xFC && buf[i + 1] == 0xFC)
			{
				j ++;
			}
		}
		char ** str = (char **)malloc(sizeof(char *) * j);
		int * len = (int *)malloc(sizeof(int) * j);
		for(i = 0; i < j; i ++){
			str[i] = (char *)malloc(sizeof(char) * 256);
			
		}
		j = 0;
		for(i = 0; i < buf_size; i ++){
			if(buf[i] != 0xFC){
				str[j][k] = buf[i];
				k ++;
			}
			else{
				str[j][k] = buf[i];
				str[j][k + 1] = buf[i];
				len[j] = k + 2;
				j ++;
				k = 0;
				i ++;
			}
		}
		for(i = 0; i < j; i ++){
			handle_recv(sockfd, (uint8_t*)str[i], len[i]);
		}
		TTU_init_flag = 1;
	}
	
	switch(buf[4])  //根据包类�?分开解析
	{
		case 5 :
			parse_TTU_reply(buf, buf_size);
		break;
		
		case 6 :
			parse_update_reply(buf, buf_size);
		break;
				
		case 8 :
			parse_snapshot(buf, buf_size);
		break;

		case 9 :
			parse_alarm_level_reply(buf, buf_size);
		break;

		default : 
			log("packet type error\r\n");
			return -3;
	}
	
	return 0;
}

#if 0
int pasre_alarmTypeFrame(char * buf, int buf_size)
{
	
	alarm_type.header.package_size = ((buf[9] << 24) | (buf[8] << 16) | (buf[7] << 8) | buf[6]) & 0xFFFFFFF;
	alarm_type.payload.alarm_id = ((buf[13] << 24) | (buf[12] << 16) | (buf[11] << 8) | buf[10]) & 0xFFFFFFF;
	alarm_type.payload.alarm_count = buf[14];
	memcpy(alarm_type.payload.device_name, buf + 15, 12);

	alarm_type.payload.alarm_type = (uint8_t *)malloc(sizeof(uint8_t) * alarm_type.payload.alarm_count);
	for(int i = 0 ; i < buf[14]; i ++){
		alarm_type.payload.alarm_type[i] = buf[27 + i];
	}
	log("pasre_alarmTypeFrame done\r\n");
	return 0;
}


int pasre_alarmImageFrame(char * buf, int buf_size)
{
	alarm_image.header.package_size = ((buf[9] << 24) | (buf[8] << 16) | (buf[7] << 8) | buf[6]) & 0xFFFFFFF;
	alarm_image.payload.alarm_id = ((buf[13] << 24) | (buf[12] << 16) | (buf[11] << 8) | buf[10]) & 0xFFFFFFF;
	alarm_image.payload.total_count = ((buf[17] << 24) | (buf[16] << 16) | (buf[15] << 8) | buf[14]) & 0xFFFFFFF;
	alarm_image.payload.image_num = ((buf[21] << 24) | (buf[20] << 16) | (buf[19] << 8) | buf[18]) & 0xFFFFFFF;
	alarm_image.payload.image_data = (unsigned char *)malloc(sizeof(unsigned char) * 10240);
	memcpy(alarm_image.payload.image_data, buf + 22, buf_size - 24);
	log("pasre_alarmImageFrame done\r\n");
	return 0;
}


int pasre_heartBeatFrame(char * buf, int buf_size)
{
	int i = 0;
	heart_beat.header.package_size = ((buf[9] << 24) | (buf[8] << 16) | (buf[7] << 8) | buf[6]) & 0xFFFFFFF;
	heart_beat.payload.status_num = buf[10];
	heart_beat.payload.device_status = (uint8_t *)malloc(sizeof(uint8_t) * heart_beat.payload.status_num);
	for(i = 0; i < heart_beat.payload.status_num; i ++){
		heart_beat.payload.device_status[i] = buf[11 + i];
	}
	memcpy(heart_beat.payload.heartbeat_time, buf + 11 + i, 30);

	log("pasre_heartBeatFrame done\r\n");
	return 0;
}


int pasre_modelTypeFrame(char * buf, int buf_size)
{
	model_type.header.package_size = ((buf[9] << 24) | (buf[8] << 16) | (buf[7] << 8) | buf[6]) & 0xFFFFFFF;
	model_type.payload.total_count = buf[10];

	model_type.payload.model_type = (char **)malloc(sizeof(char *) * buf[10]);
	int i = 0, j = 0, k = 0;
	for(i = 0; i < buf[10]; i ++){
		model_type.payload.model_type[i] = (char *)malloc(sizeof(char) * 12);
	}
	i = 0;
	for(j = 11; j < buf_size; j ++){
		if(buf[j] == 0x3B){
			i ++;
			k = 0;
			continue;
		}
		if(buf[j] == 0xFD){
			break;
		}
		model_type.payload.model_type[i][k] = buf[j];
		k++;
	}
	for(i = 0; i < buf[10]; i ++){
		log("camera_name[%d] = %s\r\n", i ,model_type.payload.model_type[i]);
	}
	log("pasre_modelTypeFrame done\r\n");
	return 0;
}


int pasre_deviceInfoFrame(char * buf, int buf_size)
{
	device_info.header.package_size = ((buf[9] << 24) | (buf[8] << 16) | (buf[7] << 8) | buf[6]) & 0xFFFFFFF;
	device_info.payload.heartbeat_frequency = ((buf[11] << 8) | buf[10]) & 0xFFFF;
	device_info.payload.device_num = buf[12];
	memcpy(device_info.payload.software_version, buf + 13, 16);
	memcpy(device_info.payload.device_name, buf + 30, 12);

	device_info.payload.camera_name = (char **)malloc(sizeof(char *) * (buf[12] - 1));
	int i = 0, j = 0, k = 0;
	for(i = 0; i < buf[10]; i ++){
		device_info.payload.camera_name[i] = (char *)malloc(sizeof(char) * 13);
	}
	i = 0;
	for(j = 43; j < buf_size; j ++){
		if(buf[j] == 0x3B){
			i ++;
			k = 0;
			continue;
		}
		if(buf[j] == 0x7D){
			break;
		}
		device_info.payload.camera_name[i][k] = buf[j];
		k++;
	}
	for(i = 0; i < buf[12] - 1; i ++){
		log("camera_name[%d] = %s\r\n", i ,device_info.payload.camera_name[i]);
	}
	log("pasre_modelTypeFrame done\r\n");
	return 0;
}


int pasre_getTTUInfoFrame(char * buf, int buf_size)
{
	get_ttu_info.header.package_size = ((buf[9] << 24) | (buf[8] << 16) | (buf[7] << 8) | buf[6]) & 0xFFFFFFF;
	get_ttu_info.payload.parameter = (buf[11] << 8 | buf[10]) & 0xFFFF;
	
	log("pasre_getTTUInfoFrame done %d \r\n", get_ttu_info.payload.parameter);
	return 0;
}


int pasre_getUpdateFrame(char * buf, int buf_size)
{
	get_update.header.package_size = ((buf[9] << 24) | (buf[8] << 16) | (buf[7] << 8) | buf[6]) & 0xFFFFFFF;
	get_update.payload.update_flag = (buf[11] << 8 | buf[10]) & 0xFFFF;
	
	log("pasre_getUpdateFrame done %d \r\n", get_update.payload.update_flag);
	return 0;
}


int pasre_alarmTypeRecoveryFrame(char * buf, int buf_size)
{
	alarm_recover.header.package_size = ((buf[9] << 24) | (buf[8] << 16) | (buf[7] << 8) | buf[6]) & 0xFFFFFFF;

	alarm_recover.payload.recover_num = buf[10];
	memcpy(alarm_recover.payload.device_name, buf + 11, 12);
	alarm_recover.payload.alarm_recovery = (uint8_t *)malloc(sizeof(uint8_t) * buf[10]);
	for(int i = 0; i < buf[10]; i ++){
		alarm_recover.payload.alarm_recovery[i] = (uint8_t)buf[23 + i];
	}
	
	log("pasre_alarmTypeRecoveryFrame done\r\n");
	return 0;
}


int pasre_uploadParamFrame(char * buf, int buf_size)
{
	upload_param.header.package_size = ((buf[9] << 24) | (buf[8] << 16) | (buf[7] << 8) | buf[6]) & 0xFFFFFFF;
	upload_param.payload.snapshot_type = (buf[11] << 8 | buf[10]) & 0xFFFF;

	log("pasre_uploadParamFrame done %d\r\n", upload_param.payload.snapshot_type);
	return 0;
}


int pasre_alarmParamFrame(char * buf, int buf_size)
{
	alarm_param.header.package_size = ((buf[9] << 24) | (buf[8] << 16) | (buf[7] << 8) | buf[6]) & 0xFFFFFFF;
	alarm_param.payload.alarm_param_type = (buf[11] << 8 | buf[10]) & 0xFFFF;

	log("pasre_alarmParamFrame done %d\r\n", alarm_param.payload.alarm_param_type);
	return 0;
}


int pasre_uploadImageFrame(char * buf, int buf_size)
{
	upload_image.header.package_size = ((buf[9] << 24) | (buf[8] << 16) | (buf[7] << 8) | buf[6]) & 0xFFFFFFF;
	upload_image.payload.type = (buf[11] << 8 | buf[10]) & 0xFFFF;
	upload_image.payload.camera_id = (buf[13] << 8 | buf[12]) & 0xFFFF;
	upload_image.payload.image_count = ((buf[17] << 24) | (buf[16] << 16) | (buf[15] << 8) | buf[14]) & 0xFFFFFFF;
	upload_image.payload.image_num = ((buf[21] << 24) | (buf[20] << 16) | (buf[19] << 8) | buf[18]) & 0xFFFFFFF;

	upload_image.payload.image_data = (uint8_t *)malloc(sizeof(uint8_t) * 10240);
	memcpy(upload_image.payload.image_data, buf + 22, buf_size - 24);


	log("pasre_uploadImageFrame done \r\n");
	return 0;
}



int handle_recv_else(int sockfd, uint8_t * buf, int buf_size)
{
	switch(buf[4])  //根据包类�?分开解析
	{
		case 0  : pasre_alarmTypeFrame((char *)buf, buf_size); 			break;
		case 1  : pasre_alarmImageFrame((char *)buf, buf_size); 		break;
		case 2  : pasre_heartBeatFrame((char *)buf, buf_size); 			break;
		case 3  : pasre_modelTypeFrame((char *)buf, buf_size); 			break;
		case 4  : pasre_deviceInfoFrame((char *)buf, buf_size); 		break;
		case 5  : pasre_getTTUInfoFrame((char *)buf, buf_size); 		break;
		case 6  : pasre_getUpdateFrame((char *)buf, buf_size); 			break;
		case 7  : pasre_alarmTypeRecoveryFrame((char *)buf, buf_size); 	break;
		case 8  : pasre_uploadParamFrame((char *)buf, buf_size); 		break;
		case 9  : pasre_alarmParamFrame((char *)buf, buf_size); 		break;
		case 10 : pasre_uploadImageFrame((char *)buf, buf_size); 		break;
		default : 
			log("packet type error\r\n");
			return -3;
	}
	return 0;
}

int handle_recv_pre(int sockfd, uint8_t * buf, int buf_size)
{
	log(" buf size = %d \r\n", buf_size);
	if(buf[0] == 0xFD && buf[1] == 0xFD && buf[2] == 0xFD && buf[3] == 0xFD) //检查包�?
	{
		if(buf[5] + buf[6] * 256 + buf[7] * 65536 + buf[8] * 256 * 65536 == buf_size - 9) //检查长�?
		{
			handle_recv(sockfd, buf, buf_size);
		}
		else{
			log("buf size error\r\n");
			return -2;
		}
	}
#if 0
	else if(buf[0] == 0xFC && buf[1] == 0xFC && buf[2] == 0xFC && buf[3] == 0xFC)
	{
		if(buf[6] + buf[7] * 256 + buf[8] * 65536 + buf[9] * 256 * 65536 == buf_size - 10) //检查长�?
		{
			handle_recv_else(sockfd, buf, buf_size);
		}
		else{
			return -3;
		}
	}
#endif
	else{
		log("buf header error\r\n");
		return -1;
	}
	
	return 0;
}

#endif

