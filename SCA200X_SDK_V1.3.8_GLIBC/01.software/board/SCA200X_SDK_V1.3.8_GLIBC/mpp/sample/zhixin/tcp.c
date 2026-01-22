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
#include <pthread.h>

#include "log.h"
#include "handle_recv.h"
#include "upload_data.h"
#include "reply_type.h"
//#include "request_type.h"
#include "handle_config.h"
#include "upload_image.h"


#define PORT 6000
#define IP "192.168.31.211"
#define MAX_BUF_SIZE 512

int sockfd = -1;
//int common_ini_change = 1;

extern int heartbeat_time;
extern char server_ip[16];
extern int port;
extern config_ini_t config;
extern camera_conf_t cam[4];
extern int device_num;
extern char device_name[13];
extern char camera_name[4][13];
extern cam_status_t cam_status[4];

int tcp_connect()
{
	int ret = -1;
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0){
		log("socket error\r\n");
		return -1;
	}
	
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(server_ip);
	server_addr.sin_port = htons(port);

	log("server ip = %s port = %d\r\n", server_ip, port);
	ret = connect(fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(ret < 0){
		log("connect error\r\n");
		return -2;
	}
	return fd;
}

int tcp_nonblocking_recv(int conn_sockfd, void *rx_buf, int buf_len, int timeval_sec, int timeval_usec)
{
	fd_set readset;
	struct timeval timeout = {0, 0};
	int maxfd = 0;
	int fp0 = 0;
	int recv_bytes = 0;
	int ret = 0;

	timeout.tv_sec = timeval_sec;
	timeout.tv_usec = timeval_usec;
	FD_ZERO(&readset);           
	FD_SET(conn_sockfd, &readset);         

	maxfd = conn_sockfd > fp0 ? (conn_sockfd+1) : (fp0+1);    

	ret = select(maxfd, &readset, NULL, NULL, &timeout); 
	if (ret > 0){
		if (FD_ISSET(conn_sockfd, &readset)){
			if ((recv_bytes = recv(conn_sockfd, rx_buf, buf_len, MSG_DONTWAIT))<= 0){
				return recv_bytes;
			}
		}
	}
	else{
		return -1;
	}

	return recv_bytes;
}

int set_version(char * version)
{
	FILE *fp = fopen("/userdata/version", "r");
	if(fp == NULL){
		log("fopen error\r\n");
		return -1;
	}
	char buf[256] = {0}; 
	fread(buf, 1, 256, fp);
	fclose(fp);
//log("buf = %s\r\n", buf);
	int v1 = -1, v2 = -1, v3 = -1;
	char temp1[128] = {0};
	char temp2[128] = {0};
	char temp3[32] = {0};
	sscanf(buf, "%s\r\n%s\r\nFirmware_version=ZX_A1-v%d.%d.%d-%s", temp1, temp2, &v1, &v2, &v3, temp3);
//log("temp1 = %s temp2 = %s v1 = %d, v2 = %d v3 = %d temp3 = %s\r\n", temp1, temp2, v1, v2, v3, temp3);
	fp = fopen("/userdata/alg_version", "r");
	fread(buf, 1, 256, fp);
	fclose(fp);
//log("buf = %s\r\n", buf);

	int v4 = -1, v5 = -1, v6 = -1;
	sscanf(buf, "lenovo_algo_version:v%d.%d.%d", &v4, &v5, &v6);
//log("v4 = %d, v5 = %d v6 = %d\r\n", v4,v5,v6);
	sprintf(version, "[H1.1;V%d%d.%d-%d%d%d]", v1, v2, v3, v4, v5, v6);
	
	return 0;
}

int get_local_ip()
{
	char buf[1024] = {0};
	char local_ip[16] = {0};
	int ip1 = 0;
	int ip2 = 0;
	int ip3 = 0;
	int ip4 = 0;
	
	FILE * fp = fopen("/etc/network/interfaces", "r");
	if(fp == NULL){
		log("fp == NULL, open /etc/network/interfaces error\r\n");
		return -1;
	}

	fread(buf, 1, 1024, fp);

	fclose(fp);

	char *data_Start = NULL;
    char *data_ValueStart = NULL;
    char *data_ValueEnd = NULL;

	data_Start = strstr(buf, "address ");
    data_ValueStart = data_Start + strlen("address ");
    data_ValueEnd = strchr(data_ValueStart + 1, 'n');
    strncpy(local_ip, data_ValueStart, data_ValueEnd - data_ValueStart - 1);
	sscanf(local_ip, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);

	return ip4;
}


int send_device_info(char * buf)
{
	memset(buf, 0, MAX_BUF_SIZE);
	// 盒子id
	char device[5][13] = {0};
	char version[16] = {0};
	log("device_name = %s\r\n", device_name);
	int ip4 = get_local_ip();
	//memcpy(device[0], device_name, 13);
	sprintf(device[0], "1202SC%03d000", ip4);
	
	for(int i = 0; i < 4; i ++){
		if(strncmp(camera_name[i], "000000", 6) != 0){
			log("camera_name = %s\r\n", camera_name[i]);
			memcpy(device[i + 1], camera_name[i], 13);
			memcpy(cam_status[i].device_name, camera_name[i], 13);
		}
	}
	set_version(version);
	log("version = %s\r\n", version);
	int len = set_deviceInfoFrame(buf, heartbeat_time, device_num, version, device);
	int ret = send(sockfd, buf, len, 0);
	if(ret != len){
		close(sockfd);
		log("send set_deviceInfoFrame fail\r\n");
		sockfd = -1;
	}
	return ret;
}

int send_heartbeat(char * buf)
{
	time_t current_time;
    struct tm *t;
	char heartbeat_timestamp[30] = {0};
	uint8_t device_status[5] = {0};

	device_status[0] = 0;
	for(int i = 1; i < 5; i ++)
	{
		if(strncmp(cam_status[i - 1].device_name, "000000", 6) != 0)
		{
			if(cam_status[i - 1].online == 0){
				device_status[i] = 1;
			}
			if(cam_status[i - 1].online == 1){
				device_status[i] = 0;
			}
		}
	}
	
    time(&current_time);
    t = localtime(&current_time);
	
	memset(buf, 0, MAX_BUF_SIZE);
	
	sprintf(heartbeat_timestamp, "%04d%02d%02dT%02d%02d%02dz", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	log("heartbeat_timestamp = %s sockfd = %d device_num = %d \r\n", heartbeat_timestamp, sockfd, device_num);
	
	int len = set_heartBeatFrame(buf, device_num, device_status, heartbeat_timestamp);	
	int ret = send(sockfd, buf, len, MSG_NOSIGNAL);
	if(ret != len){
		close(sockfd);
		log("send heartbeat fail\r\n");
		sockfd = -1;
	}

	for(int i = 0; i < 4; i ++){
		cam_status[i].online = 0;
	}
	return ret;
}

int send_getTTU(char * buf)
{
	memset(buf, 0, MAX_BUF_SIZE);
	int len = set_getTTUInfoFrame(buf, 1);
	int ret = send(sockfd, buf, len, MSG_NOSIGNAL);
	if(ret != len){
		close(sockfd);
		log("send heartbeat fail\r\n");
		sockfd = -1;
	}
	return ret;
}

int send_getupdate(char * buf, int para)
{
	int len = set_getUpdateFrame(buf, para);
	int ret = send(sockfd, buf, len, MSG_NOSIGNAL);
	if(ret != len){
		close(sockfd);
		log("send heartbeat fail\r\n");
		sockfd = -1;
	}
	return ret;
}

int send_query_upload_param(char * buf)
{
	memset(buf, 0, MAX_BUF_SIZE);
	int len = set_uploadParamFrame(buf, 1);
	int ret = send(sockfd, buf, len, MSG_NOSIGNAL);
	if(ret != len){
		close(sockfd);
		log("send heartbeat fail\r\n");
		sockfd = -1;
	}
	return ret;
}

int send_query_alarm_param(char * buf, int type)
{
	memset(buf, 0, MAX_BUF_SIZE);
	int len = set_alarmParamFrame(buf, type);
	int ret = send(sockfd, buf, len, MSG_NOSIGNAL);
	if(ret != len){
		close(sockfd);
		log("send heartbeat fail\r\n");
		sockfd = -1;
	}
	return ret;
}

int query_flag = 3;
void query_param(char * buf)
{
	if(query_flag == 0)
	{
		send_query_upload_param(buf);
		//sleep(1);
	}
	else if(query_flag == 1)
	{
		send_query_alarm_param(buf, 1);
		//sleep(1);
	}
	else if(query_flag == 2)
	{
		send_query_alarm_param(buf, 2);
		//sleep(1);
	}
	else{
		return;
	}
	query_flag ++;
}

int handle_send_data_pre(char * buf, char * image_path, int cam_id)
{
	handle_send_data(sockfd, buf, image_path, cam_id);
	return 0;
}

int send_alarm_data_pre(char *buf, char * image_path, int alarm_id, 
			char * device_name, char *alarm_type_list, int alarm_type_count)
{
	send_alarm_data(sockfd, buf, image_path, alarm_id, device_name, alarm_type_list, alarm_type_count);
	return 0;
}

int sned_alarm_recover_pre(char * buf, uint8_t num, char * device_name, char * alarm_recover_list)
{
	int len = set_alarmTypeRecoveryFrame(buf, num, device_name, alarm_recover_list);
	int ret = send(sockfd, buf, len, MSG_NOSIGNAL);
	if(ret != len){
		close(sockfd);
		log("sned_alarm_recover_pre fail\r\n");
		sockfd = -1;
	}
	return 0;
}


static void * tcp_process(void * args)
{
	char buf[MAX_BUF_SIZE] = {0};
	int len = -1;
	while(1)
	{
		sockfd = tcp_connect();
		if(sockfd < 0){
			log("tcp  connect error\r\n");
			sleep(3);
			close(sockfd);
			sockfd = -1;
			continue;
		}
		log("tcp_connect success sockfd = %d\r\n", sockfd);

		//system("sudo /etc/init.d/led on 5");

		int oldSocketFlag = fcntl(sockfd, F_GETFL, 0);
		int newSocketFlag = oldSocketFlag | O_NONBLOCK;
		if (fcntl(sockfd, F_SETFL,  newSocketFlag) == -1)
		{
		  	close(sockfd);
			log("set sockfd nonblock error\r\n ");
			sockfd = -1;
		}

		send_device_info(buf);
		log("send set_deviceInfoFrame done\r\n");
		sleep(1);
		long int heartbeat_timestamp1 = get_timestamp() - heartbeat_time -1;
		long int getTTU_timestamp1 = get_timestamp() - heartbeat_time - 2;
		//long int getupdate_timestamp1 = get_timestamp() - 1;
		while (-1 != sockfd) 
		{	
			/* 注册 */
//			if(common_ini_change == 1){
//				query_flag = 0;  
//				send_device_info(buf);
//				log("send set_deviceInfoFrame done\r\n");
//				common_ini_change = 0;
//			}
			
			/* 查询参数 */ 
			query_param(buf);
			
			/* 时间同步 */
			long int getTTU_timestamp2 = get_timestamp();
			if(getTTU_timestamp2 - getTTU_timestamp1 >= heartbeat_time)
			{
				log("------- set get ttu time-----\r\n");
				send_getTTU(buf);
				getTTU_timestamp1 = get_timestamp();
				//sleep(1);
				query_flag = 0;
			}
			
			//long int getupdate_timestamp2 = get_timestamp();
			//if(getupdate_timestamp2 - getupdate_timestamp1 >= heartbeat_time)
			//{
			//	send_getupdate(buf, 1);
			//	getupdate_timestamp1 = get_timestamp();
			//}
			
			/* 心跳 */
			long int heartbeat_timestamp2 = get_timestamp();
			if(heartbeat_timestamp2 - heartbeat_timestamp1 >= heartbeat_time)
			{
				send_heartbeat(buf);
				heartbeat_timestamp1 = get_timestamp();
				
				//sleep(1);
			}

			/* receive and parse test */
			memset(buf, 0, MAX_BUF_SIZE);
			len = tcp_nonblocking_recv(sockfd, buf, MAX_BUF_SIZE, 1, 0);
			if(len > 0)
			{
//				for(int i = 0; i < len; i ++){
//					printf("%02X ", buf[i]);
//					if(i % 16 == 15){
//						printf("\r\n");
//					}
//				}
				handle_recv(sockfd, (uint8_t *)buf, len);
			}
			else if(len == 0){
				close(sockfd);
				sockfd = -1;
//				common_ini_change = 1;
			}
			usleep(100 * 1000);
		}
		//system("sudo /etc/init.d/led off 5");
		sleep(1);
	}
	return 0;

}

int tcp_pthread_init()
{
	pthread_t tcp_pid;
	
	pthread_create(&tcp_pid, NULL, tcp_process, NULL);
	
	//pthread_join(tcp_pid, NULL);
	
	return 0;
}

				

