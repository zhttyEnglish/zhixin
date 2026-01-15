#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>        
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include "log.h"
#include "reply_type.h"
//#include "request_type.h"
#include "upload_data.h"
#include "handle_config.h"

extern int remote_permission;   // 0 打开远程许可 不用报警   1 关闭远程许可 可以报警 
extern int timed_snapshot_switch; // 定时抓拍开关 0 关 1 开
extern int snapshot_hour1;	// 定时抓拍时间 hour1 早 小时 hour2 晚小时
extern int snapshot_hour2;
extern int snapshot_min1;
extern int snapshot_min2;
extern int remote_test_camera_id;
extern int alarm_level;
extern int lv1_alarm_time;
extern int lv2_alarm_time;
extern int lv3_alarm_time;
extern char model[256];
extern int snapshot_type;
extern cam_status_t cam_status[4];

int get_file_size(FILE *stream)
{
	long file_size = -1;
	long cur_offset = ftell(stream);	// 获取当前偏移位置
	if (cur_offset == -1) {
		return -1;
	}
	if (fseek(stream, 0, SEEK_END) != 0) {	// 移动文件指针到文件末尾
		return -2;
	}
	file_size = ftell(stream);	// 获取此时偏移值，即文件大小
	if (file_size == -1) {
		return -3;
	}
	if (fseek(stream, cur_offset, SEEK_SET) != 0) {	// 将文件指针恢复初始位置
		return -4;
	}
	return file_size;
}

int alarm_image_data_upload(int sockfd, char *path, int alarm_id)
{
	FILE * fp = fopen(path, "r");
	int file_size = get_file_size(fp);
	log("file size = %d\r\n", file_size);

	char *buf = (char *)malloc(sizeof(char) * 10240 + 256);
	char *data = (char *)malloc(sizeof(char) * 10240);
	int len = -1, read_bytes = -1;
	int total_count = file_size / 10240 + 1;
	int count = 1;
	
	while((read_bytes = fread(data, 1, 10240, fp)) > 0)
	{
		//log("total count = %d count = %d read image data %d\r\n", total_count, count ,read_bytes);
		len = set_alarmImageFrame(buf, alarm_id, total_count, count, (uint8_t *)data, read_bytes);
		int ret = send(sockfd, buf, len, MSG_NOSIGNAL);
		while(ret != len){
			ret = send(sockfd, buf, len, MSG_NOSIGNAL);
		}
		//log("total count = %d count = %d read image data %d len = %d ret = %d \r\n", total_count, count ,read_bytes, len ,ret);
		count ++;
		usleep(5 * 1000);
	}
	log("total count = %d count = %d \r\n", total_count, count);
	log("send image done\r\n");

	if (ferror(fp)) {
        log("读取文件出错\r\n");
    }

	fclose(fp);
	free(data);
	free(buf);

	return 0;
}


int upload_image_data(int sockfd, char *path, int cmd_type, int cam_id)
{
	FILE * fp = fopen(path, "r");
	int file_size = get_file_size(fp);
	log("file size = %d\r\n", file_size);

	char *buf = (char *)malloc(sizeof(char) * 10240 + 256);
	char *data = (char *)malloc(sizeof(char) * 10240);
	int len = -1, read_bytes = -1;
	int total_count = file_size / 10240 + 1;
	int count = 1;
	
	while((read_bytes = fread(data, 1, 10240, fp)) > 0)
	{
		//log("read image data %d\r\n", read_bytes);
		len = set_uploadImageFrame(buf, cmd_type, cam_id, total_count, count, (uint8_t *)data, read_bytes);
		int ret = send(sockfd, buf, len, MSG_NOSIGNAL);
		while(ret != len){
			ret = send(sockfd, buf, len, MSG_NOSIGNAL);
		}
		count ++;
		usleep(5 * 1000);
	}
	log("total count = %d count = %d \r\n", total_count, count);
	log("===== send image done =====\r\n");
	if (ferror(fp)) {
        log("读取文件出错\r\n");
    }

	fclose(fp);
	free(data);
	free(buf);

	return 0;
}



int upload_image_for_permittion = 0;
int handle_send_data(int sockfd, char * buf, char * image_path, int cam_id)
{
//	log("+++++++++++++++++++++++++++++++++++++++++++\r\n");
	int i = 0; 

	for(i = 0; i < 4; i++){
		if(cam_status[i].cam_id == cam_id){
			break;
		}
	}
//	log("----- i = %d cam id = %d snapshot_type = %d remote_test_camera_id = %d\r\n", i, cam_id, snapshot_type, remote_test_camera_id);
	/* 召测 */
	if(snapshot_type == 2 && remote_test_camera_id != -1)
	{
		//上传照片
		if(cam_id == remote_test_camera_id) //判断相机id一致
		{
			upload_image_data(sockfd, image_path, 1, cam_id);
			log("========== remote test success ==========\r\n");
			//sleep(1);
			snapshot_type = -1;
			remote_test_camera_id = -1;
		}
	}
//	log("i = %d timed_snapshot_switch = %d, {%d:%d, %d:%d}\r\n", i, timed_snapshot_switch, snapshot_hour1, snapshot_min1, snapshot_hour2, snapshot_min2);
	/* 定时上报 */
	if(timed_snapshot_switch == 1)
	{
		
		time_t current_time;
	    struct tm *time_info;
		
	    time(&current_time);
	    time_info = localtime(&current_time);

//		log("====== time  %d : %d ======\r\n", time_info->tm_hour, time_info->tm_min);
		if((time_info->tm_hour == snapshot_hour1 && time_info->tm_min == snapshot_min1) ||
			(time_info->tm_hour == snapshot_hour2 && time_info->tm_min == snapshot_min2))
		{
			if(cam_status[i].timed_upload == 0)
			{
				int cmd_type = 0;
				if(time_info->tm_hour == snapshot_hour1 && time_info->tm_min == snapshot_min1){
					cmd_type = 2;
					//上传照片
					upload_image_data(sockfd, image_path, cmd_type, cam_id);
					log("========== morning timed upload success ==========\r\n");
					//sleep(1);
					cam_status[i].timed_upload = 1;
				}
				if(time_info->tm_hour == snapshot_hour2 && time_info->tm_min == snapshot_min2){
					cmd_type = 3;
					//上传照片
					upload_image_data(sockfd, image_path, cmd_type, cam_id);
					log("========== night timed upload success ==========\r\n");
					//sleep(1);
					cam_status[i].timed_upload = 1;
				}
			}
		}
		else{
			cam_status[i].timed_upload = 0;
		}
		
	}
	
	if(cam_status[i].remote_upload == 0)
	{
		if(upload_image_for_permittion == 1) //远程许可开启
		{
			upload_image_data(sockfd, image_path, 4, cam_id);
			log("========== remote permit on uplod success ==========\r\n");
			//sleep(1);
			upload_image_for_permittion = 0;
		}
		if(upload_image_for_permittion == 2) //远程许可关闭
		{
			upload_image_data(sockfd, image_path, 5, cam_id);
			log("========== morning timed off upload success ==========\r\n");
			//sleep(1);
			upload_image_for_permittion = 0;
		}
		cam_status[i].remote_upload = 1;
	}

	return 0;
}



// 报警 上传报警信息和图片 亮灯 led7
int alarm_test_count = 0;
int send_alarm_data(int sockfd, char * buf, char * image_path, int alarm_id, 
		char * device_name, char *alarm_type_list, int alarm_type_count)
{
#if 1
	//上报报警信息
	int len = set_alarmTypeFrame(buf, alarm_id, alarm_type_count, device_name, (uint8_t *)alarm_type_list);
	int ret = send(sockfd, buf, len, MSG_NOSIGNAL);
	if(ret != len){
		close(sockfd);
		log("send heartbeat fail\r\n");
		sockfd = -1;
		return -1;
	}
#else
	char alarm[2] = {0};
	if(alarm_test_count == 0){
		alarm[0] = 0x00;
	}
	if(alarm_test_count == 1){
		alarm[0] = 0x01;
	}
	if(alarm_test_count == 2){
		alarm[0] = 0x02;
	}
	if(alarm_test_count == 3){
		alarm[0] = 0x03;
	}
	if(alarm_test_count == 4){
		alarm[0] = 0x04;
	}
	if(alarm_test_count == 5){
		alarm[0] = 0x05;
	}
	if(alarm_test_count == 6){
		alarm[0] = 0x06;
	}
	alarm_test_count ++;
	if(alarm_test_count == 7){
		alarm_test_count = 0;
	}
	log("----- alarm = %s -----\r\n", alarm);
	int len = set_alarmTypeFrame(buf, alarm_id, 1, device_name, (uint8_t *)alarm);
	int ret = send(sockfd, buf, len, MSG_NOSIGNAL);
	if(ret != len){
		close(sockfd);
		log("send heartbeat fail\r\n");
		sockfd = -1;
		return -1;
	}
#endif
	
	for(int i = 0; i < len; i ++){
		printf("%02X ", buf[i]);
	}
	printf("\r\n");
	
	log("set_alarmTypeFrame done\r\n");
	usleep(500 * 1000);
	//上传图片
	alarm_image_data_upload(sockfd, image_path, alarm_id);
	return ret;
}

