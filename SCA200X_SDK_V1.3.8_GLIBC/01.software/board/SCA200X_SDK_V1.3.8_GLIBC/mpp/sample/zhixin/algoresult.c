#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>

#include "log.h"
#include "tcp.h"
#include "handle_config.h"
#include "cJSON.h"
#include "queue.h"
#include "database.h"

#define SERVER_SOCK_FILE "/tmp/server.sock"

extern int model_count;
extern int remote_permission;
extern cam_status_t cam_status[4];
extern camera_conf_t cam[4];
extern alarm_model_t * p_alarm_model;
extern config_ini_t config;
//extern char alarm_list[4][32];
//extern int alarm_list_count[4]; 
	
int alarm_type_count = 0;
char alarm_type_list[16] = {0};
int alarm_recover_count = 0;
char alarm_recover_list[16] = {0};

uint32_t alarm_id = 1;

#define ALARM_CONFIDENCE 0.3

ImageQueue queue;


void set_error_led(int on_off)
{
	if(on_off == 0)
	{
		system("sudo /etc/init.d/led off 4");
	}
	else if(on_off == 1)
	{
		system("sudo /etc/init.d/led on 4");
	}
}

int if_exist(char * path)
{
	FILE * fp = NULL;
	fp = fopen(path, "r");
	if(fp == NULL){
		log("path %s open fail\r\n", path);
		return -1;
	}
	else{
		fclose(fp);
	}
	return 0;
}

/*
		-- 4字节 头标志 		0xFDFDFDFD
header	-- 1字节 协议版本 	0x01
		-- 4字节 包大小 		0x03 

payload -- 1字节 返回值		0x01  正常  
							0x02 算法结果 文件不存在或权限不足
							0x03 图像地址 文件不存在或权限不足	
							0x04 算法结果地址和图像地址 文件不存在或权限不足
							0x05 数据长度不对
		-- 2字节 结束标志 	0xFCFC

*/
int send_ack(int connect_fd, char *buf, int ret, char *result_path, char *image_path)
{
	int ret1 = if_exist(result_path); 
	int ret2 = if_exist(image_path); 
	int ret3 = 0;
	buf[0] = 0xFD; buf[1] = 0xFD; buf[2] = 0xFD; buf[3] = 0xFD;
	buf[4] = 0x01; buf[5] = 0x00; buf[6] = 0x00; buf[7] = 0x00;buf[8] = 0x03;
	buf[10] = 0xFC; buf[11] = 0xFC; 

	buf[9] = 0x01;
	
	if(ret == -1){
		buf[9] = 0x05;
		log("buffer size error\r\n");
	   ret3 = -1;
	}
	else if(ret1 == -1 && ret2 == -1)
	{
		buf[9] = 0x04;
		log("result_path %s image_path %s open error\r\n", result_path, image_path);
		ret3 = -2;
	}
	else if(ret2 == -1)
	{
		buf[9] = 0x03;
		log("image_path %s open error\r\n", image_path);
		ret3 = -3;
	}
	else if(ret1 == -1)
	{
		buf[9] = 0x02;
		log("result_path %s open error\r\n", result_path);
		ret3 = -4;
	}

//	int len = write(connect_fd, buf, 12);
//	if(len != 12){
//		log("send_ack write error\r\n");
//		ret3 = -5;
//	}
	return ret3;
}

/*
		-- 4字节 头标志 		0xFCFCFCFC
header	-- 1字节 协议版本 	0x01
		-- 4字节 包大小 		payload 长度 

		-- 2字节 算法结果路径长度  n
payload -- 2字节 图像路径长度 m
		-- n字节
		-- m字节
		-- 2字节 结束标志 	0xFDFD

*/
int get_result_image_path(char *buf, char *result_path, char *image_path)
{
	
	uint16_t result_path_len = 0;
	uint16_t image_path_len = 0;
	uint32_t payload_len = 0;
	
	result_path_len = (uint16_t )((buf[9] << 8) | buf[10]);
	image_path_len = (uint16_t )((buf[11] << 8) | buf[12]);
	
	payload_len = (uint16_t )((buf[5] << 24) | (buf[6] << 16) | (buf[7] << 8) | buf[8]);
	if(result_path_len + image_path_len + 6 != payload_len){
		return -1;
	}

	memcpy(result_path, buf + 13, result_path_len);
	memcpy(image_path, buf + 13 + result_path_len, image_path_len);

	return 0; 
}


int read_and_parse_json(char * path, int cam_id) //解析结果
{
	cJSON * root = NULL;
//	cJSON * num = NULL;
	cJSON * detections = NULL;
	char buf[1024] = {0};

//	char * list = "123456";  //配置文件中启用的算法类型

	FILE * fp = fopen(path, "r");
	if(fp == NULL){
		log("open %s fail\r\n", path);
		return -1;
	}
	fread(buf, 1, 1024, fp);
	fclose(fp);

	root = cJSON_Parse(buf);
	if(root == NULL){
		log("%s format error \r\n", path);
		return -2;
	}
//	num = cJSON_GetObjectItem(root, "num_detections");
//	log("num = %d\r\n", num->valueint);
	
	detections = cJSON_GetObjectItem(root, "detections");
	if (detections != NULL)
	{
		int i = 0;
		int array_size = cJSON_GetArraySize(detections);
//		log("array_size = %d\r\n", array_size);
		for(i = 0; i < array_size; i ++)
		{
			cJSON * array = cJSON_GetArrayItem(detections, i);
			cJSON * conf = cJSON_GetObjectItem(array, "conf");
			cJSON * cls_id = cJSON_GetObjectItem(array, "cls_id");
			log("conf = %f, cls_id = %d\r\n", conf->valuedouble, cls_id->valueint);
//			if(conf->valuedouble >= ALARM_CONFIDENCE)    // 报警置信度 大于阈值 
//			{
				int j = -1;
//				log("!!!!!!!!! alarm !!!!!!!\r\n");
				//char temp = '!';
				//sprintf(&temp, "%d", cls_id->valueint);
				
				if((cls_id->valueint == 0 || cls_id->valueint == 1 || cls_id->valueint == 2 || cls_id->valueint <= 3) && 
					conf->valuedouble > config.person && cam[cam_id].useperson == TRUE) // 有无安全帽 有无工装 都属于有人
				{
					log("remote_permission = %d cls_id->valueint = %d conf->valuedouble = %f\r\n", remote_permission, cls_id->valueint, conf->valuedouble);
					if(remote_permission == 0)//远程许可 不上报有人
					{
						continue;
					}
					for(j = 0; j < model_count; j ++){
						if(p_alarm_model[j].alarm_type == 0){ // 有人
							break;
						}
					}
					
				}
#if 0
				else if((cls_id->valueint == 4 || cls_id->valueint == 5) && 
					(conf->valuedouble >= config.smog || conf->valuedouble >= config.fire) &&
					 (cam[cam_id].usefire == TRUE || cam[cam_id].usesmog == TRUE)) // 吸烟
				{
					for(j = 0; j < model_count; j ++){
						if(p_alarm_model[j].alarm_type == 4 || p_alarm_model[j].alarm_type == 3){ // 烟 火
							break;
						}
					}
				}
#else
				else if(cls_id->valueint == 4 && conf->valuedouble > config.smog && cam[cam_id].usesmog == TRUE) // 吸烟
				{
					for(j = 0; j < model_count; j ++){
						if(p_alarm_model[j].alarm_type == 4){ // 烟
							break;
						}
					}
					log("cls_id->valueint = %d conf->valuedouble = %f\r\n", cls_id->valueint, conf->valuedouble);
				}
				else if(cls_id->valueint == 5 && conf->valuedouble > config.fire && cam[cam_id].usefire == TRUE) //烟火
				{
					for(j = 0; j < model_count; j ++){
						if(p_alarm_model[j].alarm_type == 3){ // 明火
							break;
						}
					}
					log("cls_id->valueint = %d conf->valuedouble = %f\r\n", cls_id->valueint, conf->valuedouble);
				}
#endif
				else if(cls_id->valueint == 6 && conf->valuedouble > config.water && cam[cam_id].usewater == TRUE) // 积水漏水
				{
					for(j = 0; j < model_count; j ++){
						if(p_alarm_model[j].alarm_type == 2){ // 积水、水滴
							break;
						}
					}
					log("cls_id->valueint = %d conf->valuedouble = %f\r\n", cls_id->valueint, conf->valuedouble);
				}
				else if(cls_id->valueint == 7 && conf->valuedouble > config.animal && cam[cam_id].useanimal == TRUE) // 小动物
				{
					for(j = 0; j < model_count; j ++){
						if(p_alarm_model[j].alarm_type == 1){ //动物
							break;
						}
					}
					log("cls_id->valueint = %d conf->valuedouble = %f\r\n", cls_id->valueint, conf->valuedouble);
				}

				if(j == model_count || j == -1){
					continue;
				}

				log("j = %d alarm_type = %d alarm_time = %d alarm_flag = %d\r\n", j, p_alarm_model[j].alarm_type, p_alarm_model[j].alarm_time, p_alarm_model[j].alarm_flag);
				if(p_alarm_model[j].alarm_time == 0) //报警时间为0 实时上报
				{
					p_alarm_model[j].alarm_flag = 0;
				}
				if(p_alarm_model[j].alarm_flag == 0) 
				{
					char temp[10] = "!!!!!!!!!!";
					sprintf(temp, "%d", p_alarm_model[j].alarm_type);
					if(strchr(alarm_type_list, temp[0]) == NULL) // 检测要返回的报警类型中是否有该种报警
					{
						alarm_type_count += sprintf(&alarm_type_list[alarm_type_count], "%d", p_alarm_model[j].alarm_type);
						//log("alarm_type_count %d alarm_type_list %s\r\n", alarm_type_count, alarm_type_list);
					}
					p_alarm_model[j].alarm_timestamp = get_timestamp();
					p_alarm_model[j].alarm_flag = 1;
				}
				log("alarm_type = %d alarm_time = %d alarm_flag = %d\r\n", p_alarm_model[j].alarm_type, p_alarm_model[j].alarm_time, p_alarm_model[j].alarm_flag);
//			}	
		}
	}

	return alarm_type_count;
}

int get_camid(char * path)
{
	int id = -1;
	long long int timestamp = 0;
	sscanf(path, "/userdata/result/cam%d_%lld_result.json", &id, &timestamp);
//	log("id = %d, timestamp = %lld\r\n", id, timestamp);
	return id;
}

long long int get_alarm_timestamp(char * path)
{
	int id = -1;
	long long int timestamp = 0;
	sscanf(path, "/userdata/result/cam%d_%lld_result.json", &id, &timestamp);
//	log("id = %d, timestamp = %lld\r\n", id, timestamp);
	return timestamp;
}


int get_device_name(int cam_id, char * cam_name)
{
//	log("---\r\n");
	for(int i = 0; i < 4; i ++)
	{
		if(cam_id == cam_status[i].cam_id && 
			strncmp(cam_status[i].device_name, "000000", 6) != 0)
		{
			log("cam_status[%d].cam_id = %d device name = %s\r\n", i, cam_status[i].cam_id, cam_status[i].device_name);
			cam_status[i].online = 1;
			memcpy(cam_name, cam_status[i].device_name, 13);
			break;
		}
	}
	return 0;
}

int change_cls_id(char * buf)
{
	cJSON * root = NULL;
	cJSON * num = NULL;
	cJSON * detections = NULL;
	cJSON * new_root = NULL;
	cJSON * new_detections = NULL;
	
	root = cJSON_Parse(buf);
	num = cJSON_GetObjectItem(root, "num_detections");
	detections = cJSON_GetObjectItem(root, "detections");

	new_root = cJSON_CreateObject();
	cJSON_AddNumberToObject(new_root, "num_detections", num->valueint);

	new_detections = cJSON_CreateArray();
	
	for(int i = 0; i < num->valueint; i ++)
	{
		cJSON * array = cJSON_GetArrayItem(detections, i);
		cJSON * conf = cJSON_GetObjectItem(array, "conf");
		cJSON * cls_id = cJSON_GetObjectItem(array, "cls_id");
		cJSON * x1 = cJSON_GetObjectItem(array, "x1");
		cJSON * y1 = cJSON_GetObjectItem(array, "y1");
		cJSON * x2 = cJSON_GetObjectItem(array, "x2");
		cJSON * y2 = cJSON_GetObjectItem(array, "y2");

		cJSON* new_detections_item = cJSON_CreateObject();
		cJSON_AddNumberToObject(new_detections_item, "x1", x1->valuedouble);
		cJSON_AddNumberToObject(new_detections_item, "y1", y1->valuedouble);
		cJSON_AddNumberToObject(new_detections_item, "x2", x2->valuedouble);
		cJSON_AddNumberToObject(new_detections_item, "y2", y2->valuedouble);

		if(cls_id->valueint == 0 || cls_id->valueint == 1 || cls_id->valueint == 2 || cls_id->valueint == 3){
			cJSON_AddNumberToObject(new_detections_item, "cls_id", 0);
		}
		else if(cls_id->valueint == 4){
			cJSON_AddNumberToObject(new_detections_item, "cls_id", 4);
		}
		else if(cls_id->valueint == 5){
			cJSON_AddNumberToObject(new_detections_item, "cls_id", 3);
		}
		else if(cls_id->valueint == 6){
			cJSON_AddNumberToObject(new_detections_item, "cls_id", 2);
		}
		else if(cls_id->valueint == 7){
			cJSON_AddNumberToObject(new_detections_item, "cls_id", 1);
		}

		cJSON_AddNumberToObject(new_detections_item, "conf", conf->valuedouble);

		cJSON_AddItemToArray(new_detections, new_detections_item);
	}	
	cJSON_AddItemToObject(new_root, "detections", new_detections);

	char * temp = cJSON_Print(new_root);
//	log("change json = %s\r\n", temp);
	memcpy(buf, temp, 1024);
	return 0;
}

int save_alarm_history(char * result_path, char * image_path, char * alarm_list, int alarm_len)
{
//	sql = "INSERT INTO alarm_history (alarm_id, alarm_time, camera_name, alarm_image, person_alarm, animal_alarm, fire_alarm, smoke_alarm, water_alarm) "
//	"VALUES ('1', '1677800119829', 'cam0', '/userdata/image/cam0_1677800119829_image.jpg', 'TRUE', 'TRUE', 'TRUE', 'TRUE', 'TRUE');";

	char alarm_sql[2048] = {0};
	int pos = 0;
	int id = get_camid(result_path);
	long long int timestamp = get_alarm_timestamp(result_path);
	char camera_name[16] = {0};
	get_device_name(id, camera_name);
	int alarm_type[6] = {0};

	char buf[1024] = {0};
	FILE * fp = fopen(result_path, "r");
	fread(buf, 1, 1024, fp);
	fclose(fp);

//	log("before change json = %s\r\n", buf);
	change_cls_id(buf);

	for(int i = 0; i < alarm_len; i ++){
		if(alarm_list[i] == '0'){ //人
			alarm_type[0] = 1;
		}
		if(alarm_list[i] == '1'){ //动物
			alarm_type[1] = 1;
		}
		if(alarm_list[i] == '2'){ //积水
			alarm_type[2] = 1;
		}
		if(alarm_list[i] == '3'){ //明火
			alarm_type[3] = 1;
		}
		if(alarm_list[i] == '4'){ //烟雾
			alarm_type[4] = 1;
		}
//		if(alarm_list[i] == '5'){ //漏水
//			alarm_type[5] = 1;
//		}
	}
	
	pos += sprintf(&alarm_sql[pos], "INSERT INTO alarm_history (alarm_id, alarm_time, camera_name, alarm_image, person_alarm, animal_alarm, fire_alarm, smoke_alarm, water_alarm, detail) ");
	pos += sprintf(&alarm_sql[pos], "VALUES ('%d', '%lld', '%s', '%s', '%d', '%d', '%d', '%d', '%d', '%s');", 
								alarm_id, timestamp, camera_name, image_path, 
								alarm_type[0] == 1 ? TRUE : FALSE,
								alarm_type[1] == 1 ? TRUE : FALSE,
								alarm_type[3] == 1 ? TRUE : FALSE,
								alarm_type[4] == 1 ? TRUE : FALSE,
								(alarm_type[2] == 1 || alarm_type[5] == 1) ? TRUE : FALSE,
								buf);

//	log("databse alarm_sql = %s\r\n", alarm_sql);
	pos = database_insert_alarm(alarm_sql);
	if(pos != 0){
		log("----- database insert error ----- \r\n");
		return -1;
	}
#if 0
	cJSON * root = NULL;
	cJSON * num = NULL;
	cJSON * detections = NULL;

	char detail_sql[1024] = {0};
	char buf[1024] = {0};

	memset(alarm_sql, 0, 1024);
	FILE * fp = fopen(result_path, "r");
	fread(alarm_sql, 1, 1024, fp);
	fclose(fp);

	pos = 0;
	root = cJSON_Parse(alarm_sql);
	num = cJSON_GetObjectItem(root, "num_detections");
	detections = cJSON_GetObjectItem(root, "detections");
	
	for(int i = 0; i < num->valueint; i ++)
	{
		cJSON * array = cJSON_GetArrayItem(detections, i);
		cJSON * conf = cJSON_GetObjectItem(array, "conf");
		cJSON * cls_id = cJSON_GetObjectItem(array, "cls_id");
		cJSON * x1 = cJSON_GetObjectItem(array, "x1");
		cJSON * y1 = cJSON_GetObjectItem(array, "y1");
		cJSON * x2 = cJSON_GetObjectItem(array, "x2");
		cJSON * y2 = cJSON_GetObjectItem(array, "y2");
		memset(buf, 0, 1024);
		sprintf(buf, "{(x1:%02f, y1:%02f), (x2:%02f, y2:%02f)}", x1->valuedouble, y1->valuedouble, x2->valuedouble, y2->valuedouble);

		char type[12] = {0};
		if(cls_id->valueint >= 0 && cls_id->valueint <= 3){
			sprintf(type, "%s", "person");
		}else if(cls_id->valueint == 4){
			sprintf(type, "%s", "smoke");
		}else if(cls_id->valueint == 5){
			sprintf(type, "%s", "fire");
		}else if(cls_id->valueint == 6){
			sprintf(type, "%s", "water");
		}else if(cls_id->valueint == 7){
			sprintf(type, "%s", "animal");
		}
		
		pos += sprintf(&detail_sql[pos], "INSERT INTO alarm_history_detail (alarm_id, alarm_num, alarm_type, alarm_conf, alarm_pos) ");
		pos += sprintf(&detail_sql[pos], "VALUES ('%d', '%d', '%s', '%02f', '%s');", alarm_id, i + 1, type, conf->valuedouble, buf);

		log("databse detail_sql = %s\r\n", detail_sql);
		pos = database_insert_detail(detail_sql);
		if(pos != 0){
			log("----- database insert error ----- \r\n");
			return -2;
		}
	}	
#endif
	return 0;
}

int popen_exe(char * buf)
{
	FILE * fp;
	int redo = 0;
	do{		
		redo = 0;	
		fp = popen(buf, "r");
		char log[128] = {0};
		while(fgets(log, 128, fp) != NULL){
			if(strstr(log, "failed") != NULL){
				redo = 1;
			}	
		}
	}while(redo);
	pclose(fp);
	return 0;
}


static void * get_algo_result_process(void * args)
{
//	log("----------\r\n");
	while(1)
	{
		int local_sock = socket(AF_UNIX, SOCK_STREAM, 0);
		if(local_sock < 0){
			log("socket fail\r\n");
			sleep(3);
			continue;
		}

		// 设置发送和接收超时
	    struct timeval timeout;
	    timeout.tv_sec = 3;
	    timeout.tv_usec = 0;
	    if (setsockopt(local_sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
	        log("setsockopt: send timeout");
	        close(local_sock);
			sleep(3);
			continue;
	    }
		timeout.tv_sec = 5;
		if (setsockopt(local_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
			log("setsockopt: receive timeout");
			close(local_sock);
			sleep(3);
			continue;
		}

		unlink(SERVER_SOCK_FILE);

		struct sockaddr_un server;
		bzero(&server, sizeof(server));
		server.sun_family = AF_UNIX;
		strcpy(server.sun_path, SERVER_SOCK_FILE);

		int ret = bind(local_sock, (struct sockaddr *)&server, sizeof(server));
		if(ret < 0){
			log("bind fail\r\n");
			close(local_sock);
			sleep(3);
			continue;
		}

		listen(local_sock, 10);

		struct sockaddr_un client;
		bzero(&client, sizeof(client));
		socklen_t len = sizeof(client);

		int connect_fd = accept(local_sock, (struct sockaddr *)&client, &len);
		if(connect_fd < 0){
			log("accept fail\r\n");
			close(local_sock);
			sleep(3);
			continue;
		}
		set_error_led(0);

		char buf[1024] = {0};
		int recv_len = 0;
//		int i = 0, cam_id = -1;
//		char device_name[16] = {0};
		char result_path[128] = {0};
		char image_path[128] = {0};
		while(1)
		{	
//			memset(device_name, 0, 16);
			memset(result_path, 0, 128);
			memset(image_path, 0, 128);
			memset(buf, 0, 1024);
			recv_len = read(connect_fd, buf, 1024);
			if(recv_len <= 0){
				log("======== recv_len <= 0 ========== \r\n");
				break;
			}
//			log("======= get_algo_result_process recv:\r\n");
//			for(int i = 0; i < recv_len; i ++){
//				printf("%02X ", (uint8_t)buf[i]);
//				if(15 == i % 16){
//					printf("\r\n");
//				}
//			}
//			printf("\r\n");
//			log("---------- recv_len = %d connect_fd = %d ----------\r\n", recv_len, connect_fd);

			ret = get_result_image_path(buf, result_path, image_path);
			int cam_id = get_camid(result_path);
			log("cam_id = %d result_path %s image_path %s\r\n", cam_id, result_path, image_path);
//			ret = send_ack(connect_fd, buf, ret, result_path, image_path);
//			if(ret == -5){
//				log("send ack or open file error ret = %d\r\n", ret);
//				set_error_led(1);  //故障灯
//				break;
//			}
//			else if(ret < 0 && ret > -5){
//				continue;
//			}
			if(cam[cam_id].in_use == 1){
				queue_allocate(&queue, image_path, result_path);
			}else{
				remove(image_path);
				remove(result_path);
			}
			usleep(50 * 1000);
		}
		set_error_led(1);
	    close(local_sock);
		usleep(500 * 1000);
	}
	queue_destroy(&queue);
	return 0;
}

extern int TTU_init_flag;
static void * send_alarm_process(void * args)
{
	char buf[1024] = {0};
	char device_name[16] = {0};
	int cam_id = -1;
	while(1)
	{	
		if(queue.elements[queue.tail].image_path != NULL && queue.elements[queue.tail].image_path != NULL && queue.elements[queue.tail].in_use == 1)
		{
			//if(TTU_init_flag)
			//{
			// 读取结果
			cam_id = get_camid(queue.elements[queue.tail].result_path);
			get_device_name(cam_id, device_name);
			alarm_type_count = read_and_parse_json(queue.elements[queue.tail].result_path, cam_id);
			log("alarm_type_count %d alarm_type_list %s\r\n", alarm_type_count, alarm_type_list);

			memset(buf, 0, 1024);
			sprintf(buf, "/userdata/draw_text %s %s", queue.elements[queue.tail].image_path, queue.elements[queue.tail].result_path);
			popen_exe(buf);
			usleep(200 * 1000);
			
			memset(buf, 0, 1024);
			handle_send_data_pre(buf, queue.elements[queue.tail].image_path, cam_id + 1); //cam_id 需要设置
			//log("---------- after handle_send_data_pre ----------\r\n");
			
			if(alarm_type_count != 0)
			{
				log("+++++++++++++++++++++++ alarm +++++++++++++++++++++\r\n");
				// 报警 上传报警信息和图片 
				memset(buf, 0, 1024);
				send_alarm_data_pre(buf, queue.elements[queue.tail].image_path, alarm_id, device_name, alarm_type_list, alarm_type_count);
				// 报警灯
				system("sudo /etc/init.d/led on 8");
				memset(buf, 0, 1024);
				save_alarm_history(queue.elements[queue.tail].result_path, queue.elements[queue.tail].image_path, alarm_type_list, alarm_type_count);

				database_select();

				alarm_id ++;
				memset(alarm_type_list, '!', 16);
				alarm_type_count = 0;	

				if(if_exist(queue.elements[queue.tail].result_path) == 0){
					if(remove(queue.elements[queue.tail].result_path) != 0){
						log("========== delete %s error ==========\r\n", queue.elements[queue.tail].result_path);
					}
				}
				
			}else{
				//log("--- remove %s %s ---\r\n", queue.elements[queue.tail].result_path, queue.elements[queue.tail].image_path);
				if(if_exist(queue.elements[queue.tail].result_path) == 0){
					if(remove(queue.elements[queue.tail].result_path) != 0){
						log("========== delete %s error ==========\r\n", queue.elements[queue.tail].result_path);
					}
				}
				if(if_exist(queue.elements[queue.tail].image_path) == 0){
					if(remove(queue.elements[queue.tail].image_path) != 0){
						log("========== delete %s error ==========\r\n", queue.elements[queue.tail].image_path);
					}
				}
			}
			//}
			//else{
			//	remove_image();
			//	remove_json();
			//}
			
			queue_release(&queue, queue.elements[queue.tail].image_path, queue.elements[queue.tail].result_path);
			
			int time_stamp = get_timestamp();
			for(int i = 0; i < model_count; i ++)
			{
				if(p_alarm_model[i].alarm_flag == 1)
				{
					if(p_alarm_model[i].alarm_time == 0)
					{
						system("sudo /etc/init.d/led off 8");
					}
					else if(p_alarm_model[i].alarm_time != 0)
					{
						if(time_stamp - p_alarm_model[i].alarm_timestamp >= p_alarm_model[i].alarm_time * 3)
						{
							alarm_recover_count += sprintf(&alarm_recover_list[alarm_recover_count], "%d", p_alarm_model[i].alarm_type);
							p_alarm_model[i].alarm_flag = 0;
							p_alarm_model[i].alarm_timestamp = 0;
						}
					}
				}
			}
			
			if(alarm_recover_count != 0)
			{
				log("-------------------- cancel alarm --------------------\r\n");
				// 报警 上传报警信息和图片 
				memset(buf, 0, 1024);
				sned_alarm_recover_pre(buf, alarm_recover_count, device_name, alarm_recover_list);
				// 报警灯
				system("sudo /etc/init.d/led off 8");
				
				memset(alarm_recover_list, '!', 16);
				alarm_recover_count = 0;
			}
		}
		usleep(50 * 1000);
	}
	database_exit();
	return 0;
}


int get_algo_result_thread_init()
{
	queue_init(&queue);

	pthread_t algo_result_pid;
	pthread_t send_alarm_pid;
//	pthread_create(&algo_result_pid, NULL, tcp_server_task, NULL);
	pthread_create(&algo_result_pid, NULL, get_algo_result_process, NULL);
	pthread_create(&send_alarm_pid, NULL, send_alarm_process, NULL);
	//pthread_join(algo_result_pid, NULL);
	
	return 0;
}


