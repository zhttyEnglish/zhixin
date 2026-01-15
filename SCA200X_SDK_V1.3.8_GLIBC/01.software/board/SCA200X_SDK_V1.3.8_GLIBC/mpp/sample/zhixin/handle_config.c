#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <pthread.h>

#include "log.h"
#include "iniparser.h"
#include "dictionary.h"
#include "handle_config.h"

config_ini_t config;
camera_conf_t cam[4];
cam_status_t cam_status[4];
alarm_model_t * p_alarm_model = NULL;

int snapshot_type = -1;
int remote_permission = 1;   // 0 打开远程许可 不用报警   1 关闭远程许可 可以报警 
int timed_snapshot_switch = 0; // 定时抓拍开�?0 �?1 开
int snapshot_hour1 = -1;	// 定时抓拍时间 hour1 �?小时 hour2 晚小�?
int snapshot_hour2 = -1;
int snapshot_min1 = -1;
int snapshot_min2 = -1;
int remote_test_camera_id = -1;
char server_ip[16] = {0};
int port = 0;
int heartbeat_time = 300;
int alarm_level = 4;
int lv1_alarm_time = 300;
int lv2_alarm_time = 60;
int lv3_alarm_time = 10;
int device_num = 1;
char device_name[13] = {0};
char camera_name[4][13] = {0};
int model_count = 0;
char model[256] = {0};

//char alarm_list[4][32] = {0};
//int alarm_list_count[4] = {0};

int get_model_count(char * model, int model_str_len)
{
	int i = 0, count = 0;
	for(i = 0; i < model_str_len; i ++){
		if(model[i] == '='){
			count ++;
		}
	}
	return count;
}

int parse_common_ini(char * path)
{
	dictionary * ini;
	int i = 0;

	memset(&cam_status, 0, sizeof(cam_status_t) * 4);

	if(p_alarm_model != NULL && model_count != 0)
	{
		free(p_alarm_model);
		model_count = 0;
	}

	ini = iniparser_load(path);
    if (ini==NULL) {
        log("cannot parse %s\r\n", path);
        return -1 ;
    }
    iniparser_dump(ini, stderr);
	
	strcpy(server_ip, iniparser_getstring(ini, "common:server_ip", "192.168.31.211"));
	port = iniparser_getint(ini, "common:port", 6000);
	strcpy(device_name, iniparser_getstring(ini, "common:device_name", "123456211000"));
	strcpy(camera_name[0], iniparser_getstring(ini, "common:camera_name1", "0000000000000"));
	strcpy(camera_name[1], iniparser_getstring(ini, "common:camera_name2", "0000000000000"));
	strcpy(camera_name[2], iniparser_getstring(ini, "common:camera_name3", "0000000000000"));
	strcpy(camera_name[3], iniparser_getstring(ini, "common:camera_name4", "0000000000000"));
	strcpy(model, iniparser_getstring(ini, "common:model", "{}"));
	snapshot_type = iniparser_getint(ini, "common:snapshot_type", -1);
	device_num = iniparser_getint(ini, "common:device_num", 1);
	heartbeat_time = iniparser_getint(ini, "common:heartbeat_time", 300);
	remote_permission = iniparser_getboolean(ini, "common:remote_permission", TRUE);
	timed_snapshot_switch = iniparser_getboolean(ini, "common:timed_snapshot_switch", FALSE);
	snapshot_hour1 = iniparser_getint(ini, "common:snapshot_hour1", -1);
	snapshot_hour2 = iniparser_getint(ini, "common:snapshot_hour2", -1);
	snapshot_min1 = iniparser_getint(ini, "common:snapshot_min1", -1);
	snapshot_min2 = iniparser_getint(ini, "common:snapshot_min2", -1);
	remote_test_camera_id = iniparser_getint(ini, "common:remote_test_camera_id", -1);
	alarm_level = iniparser_getint(ini, "common:alarm_level", 4);
	lv1_alarm_time = iniparser_getint(ini, "common:lv1_alarm_time", 300);
	lv2_alarm_time = iniparser_getint(ini, "common:lv2_alarm_time", 60);
	lv3_alarm_time = iniparser_getint(ini, "common:lv3_alarm_time", 10);

	for(i = 0; i < 4; i ++){
		cam_status[i].cam_id = i;
		//cam_status[i].device_name = camera_name[i];
		if(strncmp(camera_name[i], "000000", 6) != 0){
			memcpy(cam_status[i].device_name, camera_name[i], 13);
		}
	}

	model_count = get_model_count(model, strlen(model));
	int * model_alarm_level = (int *)malloc(sizeof(int) * model_count);
	char ** model_list = (char **)malloc(sizeof(char *) * model_count);

	for(i = 0; i < model_count; i ++)
	{
		model_list[i] = (char *)malloc(13);
		memset(model_list[i], 0, 13);
		model_alarm_level[i] = 0;
	}
	
	int len = strlen(model);
	int j = 0, k = 0;
	for(i = 1; i < len; i ++){
		if(model[i] == '}'){
			break;
		}
		if(model[i] == ';'){
			j ++;
			k = 0;
			continue;
		}
		if(model[i] >= '0' && model[i] <= '9'){
			model_alarm_level[j] = model[i] - 0x30;
			continue;
		}
		if(model[i] >= 'a' && model[i] <= 'z'){
			model_list[j][k] = model[i];
			k ++;
		}
	}

	p_alarm_model = (alarm_model_t *)malloc(sizeof(alarm_model_t) * model_count);

	for(i = 0; i < model_count; i ++)
	{
		log("model_type[%d] %s level %d\r\n", i, model_list[i], model_alarm_level[i]);

		if(strcmp(model_list[i], "person") == 0){
			p_alarm_model[i].alarm_type = 0;
		}else if(strcmp(model_list[i], "animal") == 0){
			p_alarm_model[i].alarm_type = 1;
		}else if(strcmp(model_list[i], "water") == 0){
			p_alarm_model[i].alarm_type = 2;
		}else if(strcmp(model_list[i], "fire") == 0){
			p_alarm_model[i].alarm_type = 3;
		}else if(strcmp(model_list[i], "smoke") == 0){
			p_alarm_model[i].alarm_type = 4;
		}else if(strcmp(model_list[i], "waterdrop") == 0){
			p_alarm_model[i].alarm_type = 5;
		}

		if(model_alarm_level[i] == 0){
			p_alarm_model[i].alarm_time = 0;
		}
		else if(model_alarm_level[i] == 1){
			p_alarm_model[i].alarm_time = lv1_alarm_time;
		}
		else if(model_alarm_level[i] == 2){
			p_alarm_model[i].alarm_time = lv2_alarm_time;
		}
		else if(model_alarm_level[i] == 3){
			p_alarm_model[i].alarm_time = lv3_alarm_time;
		}
		
		p_alarm_model[i].alarm_level = model_alarm_level[i];
		p_alarm_model[i].alarm_timestamp = 0;
		p_alarm_model[i].alarm_flag = 0;
		
		log("model_type[%d] %d level %d time %d\r\n", i, p_alarm_model[i].alarm_type, p_alarm_model[i].alarm_level, p_alarm_model[i].alarm_time);
	}

	for(i = 0; i < model_count; i ++){
		free(model_list[i]);
	}
	free(model_alarm_level);
	free(model_list);
	
	iniparser_freedict(ini);

	return 0;
}



int get_timestamp()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	//log("timestamp : %ld  %ld  %ld\r\n", tv.tv_sec, tv.tv_sec * 1000 + tv.tv_usec / 1000, tv.tv_sec * 1000000 + tv.tv_usec);
	return tv.tv_sec;
}

int get_file_timestamp(char *filename) 
{
    struct stat file_stat;

    if (stat(filename, &file_stat) == 0) {
        return file_stat.st_mtime;
    } else {
        log("stat error\r\n");
        return -1;
    }
}


void show_parse()
{
	printf("========================================================================\r\n");
	printf("\rconfig:\r\n");
	printf("license : %s   sn : %s \r\n", config.license, config.sn);
	printf("deviceid : %s, camcount : %s, water : %02f, fire : %02f\r\n", config.deviceid, config.camcount, config.water, config.fire);
	printf("down : %02f, smog : %02f, animal : %02f, penson : %02f, pd : %d\r\n", config.down, config.smog, config.animal, config.person, config.pd);
	if(cam[0].in_use == 1)
	{
		printf("\rcma1:\r\n");
		printf("cameraid : %s , rect : %s\r\n", cam[0].cameraid, cam[0].rect);
		printf("camcode : %d , usewater : %d , usepenson : %d , usenaimal : %d\r\n", cam[0].camcode, cam[0].usewater, cam[0].useperson, cam[0].useanimal);
		printf("usrdown : %d , usesmog : %d , osd : %d , usefire : %d, framerate : %s\r\n", cam[0].usedown, cam[0].usesmog, cam[0].osd, cam[0].usefire, cam[0].framerate);
	}
	if(cam[1].in_use == 1)
	{
		printf("\rcma2:\r\n");
		printf("cameraid : %s , rect : %s\r\n", cam[1].cameraid, cam[1].rect);
		printf("camcode : %d , usewater : %d , usepenson : %d , usenaimal : %d\r\n", cam[1].camcode, cam[1].usewater, cam[1].useperson, cam[1].useanimal);
		printf("usrdown : %d , usesmog : %d , osd : %d , usefire : %d, framerate : %s\r\n", cam[1].usedown, cam[1].usesmog, cam[1].osd, cam[1].usefire, cam[1].framerate);
	}
	if(cam[2].in_use == 1)
	{
		printf("\rcma3:\r\n");
		printf("cameraid : %s , rect : %s\r\n", cam[2].cameraid, cam[2].rect);
		printf("camcode : %d , usewater : %d , usepenson : %d , usenaimal : %d\r\n", cam[2].camcode, cam[2].usewater, cam[2].useperson, cam[2].useanimal);
		printf("usrdown : %d , usesmog : %d , osd : %d , usefire : %d, framerate : %s\r\n", cam[2].usedown, cam[2].usesmog, cam[2].osd, cam[2].usefire, cam[2].framerate);
	}
	if(cam[3].in_use == 1)
	{
		printf("\rcma4:\r\n");
		printf("cameraid : %s , rect : %s\r\n", cam[3].cameraid, cam[3].rect);
		printf("camcode : %d , usewater : %d , usepenson : %d , usenaimal : %d\r\n", cam[3].camcode, cam[3].usewater, cam[3].useperson, cam[3].useanimal);
		printf("usrdown : %d , usesmog : %d , osd : %d , usefire : %d, framerate : %s\r\n", cam[3].usedown, cam[3].usesmog, cam[3].osd, cam[3].usefire, cam[3].framerate);
	}
	
	printf("========================================================================\r\n");

}

int parse_config_ini_file(char * path)
{
	dictionary * ini;

	ini = iniparser_load(path);
    if (ini==NULL) {
        log("cannot parse %s\r\n", path);
        return -1 ;
    }
//    iniparser_dump(ini, stderr);

	memset(&config, 0, sizeof(config_ini_t));
	memset(&cam[0], 0, sizeof(camera_conf_t));
	memset(&cam[1], 0, sizeof(camera_conf_t));
	memset(&cam[2], 0, sizeof(camera_conf_t));
	memset(&cam[3], 0, sizeof(camera_conf_t));
	
	strcpy(config.license, iniparser_getstring(ini, "Config:license", NULL));
	strcpy(config.sn, iniparser_getstring(ini, "Config:sn", NULL));
	strcpy(config.camcount, iniparser_getstring(ini, "Config:camcount", NULL));
	strcpy(config.deviceid, iniparser_getstring(ini, "Config:deviceid", NULL));
	config.water = iniparser_getdouble(ini, "Config:water", 0.5);
	config.fire = iniparser_getdouble(ini, "Config:fire", 0.3);
	config.down = iniparser_getdouble(ini, "Config:down", 0.4);
	config.smog = iniparser_getdouble(ini, "Config:smog", 0.5);
	config.animal = iniparser_getdouble(ini, "Config:animal", 0.5);
	config.person = iniparser_getdouble(ini, "Config:person", 0.6);
	config.pd = iniparser_getboolean(ini, "Config:pd", FALSE);
	config.linesize = iniparser_getint(ini, "Config:linesize", 3);

	if(strchr(config.camcount, '1') != NULL)
	{
		cam[0].in_use = 1;
		strcpy(cam[0].cameraid, iniparser_getstring(ini, "Cam1:cameraid", "rtsp://username:password@ip/cam/realmonitor?channel=2&subtype=1"));
		strcpy(cam[0].rect, iniparser_getstring(ini, "Cam1:rect", "0,0,0,0,0,0"));
		strcpy(cam[0].framerate, iniparser_getstring(ini, "Cam1:frameRate", "m"));
		cam[0].camcode = iniparser_getint(ini, "Cam1:camcode", 0);
		cam[0].usewater = iniparser_getboolean(ini, "Cam1:usewater", TRUE);
		cam[0].useperson = iniparser_getboolean(ini, "Cam1:usepenson", TRUE);
		cam[0].useanimal = iniparser_getboolean(ini, "Cam1:useanimal", TRUE);
		cam[0].usedown = iniparser_getboolean(ini, "Cam1:usedown", TRUE);
		cam[0].usesmog = iniparser_getboolean(ini, "Cam1:usesmog", TRUE);
		cam[0].usefire = iniparser_getboolean(ini, "Cam1:usefire", TRUE);
		cam[0].osd = iniparser_getboolean(ini, "Cam1:osd", TRUE);
	}
	
	if(strchr(config.camcount, '2') != NULL)
	{
		cam[1].in_use = 1;
		strcpy(cam[1].cameraid, iniparser_getstring(ini, "Cam2:cameraid", "rtsp://username:password@ip/cam/realmonitor?channel=2&subtype=1"));
		strcpy(cam[1].rect, iniparser_getstring(ini, "Cam2:rect", "0,0,0,0,0,0"));
		strcpy(cam[1].framerate, iniparser_getstring(ini, "Cam2:frameRate", "m"));
		cam[1].camcode = iniparser_getint(ini, "Cam2:camcode", 0);
		cam[1].usewater = iniparser_getboolean(ini, "Cam2:usewater", TRUE);
		cam[1].useperson = iniparser_getboolean(ini, "Cam2:usepenson", TRUE);
		cam[1].useanimal = iniparser_getboolean(ini, "Cam2:useanimal", TRUE);
		cam[1].usedown = iniparser_getboolean(ini, "Cam2:usedown", TRUE);
		cam[1].usesmog = iniparser_getboolean(ini, "Cam2:usesmog", TRUE);
		cam[1].usefire = iniparser_getboolean(ini, "Cam2:usefire", TRUE);
		cam[1].osd = iniparser_getboolean(ini, "Cam2:osd", TRUE);
	}
	
	if(strchr(config.camcount, '3') != NULL)
	{
		cam[2].in_use = 1;
		strcpy(cam[2].cameraid, iniparser_getstring(ini, "Cam3:cameraid", "rtsp://username:password@ip/cam/realmonitor?channel=2&subtype=1"));
		strcpy(cam[2].rect, iniparser_getstring(ini, "Cam3:rect", "0,0,0,0,0,0"));
		strcpy(cam[2].framerate, iniparser_getstring(ini, "Cam3:frameRate", "m"));
		cam[2].camcode = iniparser_getint(ini, "Cam3:camcode", 0);
		cam[2].usewater = iniparser_getboolean(ini, "Cam3:usewater", TRUE);
		cam[2].useperson = iniparser_getboolean(ini, "Cam3:usepenson", TRUE);
		cam[2].useanimal = iniparser_getboolean(ini, "Cam3:useanimal", TRUE);
		cam[2].usedown = iniparser_getboolean(ini, "Cam3:usedown", TRUE);
		cam[2].usesmog = iniparser_getboolean(ini, "Cam3:usesmog", TRUE);
		cam[2].usefire = iniparser_getboolean(ini, "Cam3:usefire", TRUE);
		cam[2].osd = iniparser_getboolean(ini, "Cam3:osd", TRUE);
	}
	
	if(strchr(config.camcount, '4') != NULL)
	{
		cam[3].in_use = 1;
		strcpy(cam[3].cameraid, iniparser_getstring(ini, "Cam4:cameraid", "rtsp://username:password@ip/cam/realmonitor?channel=2&subtype=1"));
		strcpy(cam[3].rect, iniparser_getstring(ini, "Cam4:rect", "0,0,0,0,0,0"));
		strcpy(cam[3].framerate, iniparser_getstring(ini, "Cam4:frameRate", "m"));
		cam[3].camcode = iniparser_getint(ini, "Cam4:camcode", 0);
		cam[3].usewater = iniparser_getboolean(ini, "Cam4:usewater", TRUE);
		cam[3].useperson = iniparser_getboolean(ini, "Cam4:usepenson", TRUE);
		cam[3].useanimal = iniparser_getboolean(ini, "Cam4:useanimal", TRUE);
		cam[3].usedown = iniparser_getboolean(ini, "Cam4:usedown", TRUE);
		cam[3].usesmog = iniparser_getboolean(ini, "Cam4:usesmog", TRUE);
		cam[3].usefire = iniparser_getboolean(ini, "Cam4:usefire", TRUE);
		cam[3].osd = iniparser_getboolean(ini, "Cam4:osd", TRUE);
	}
#if 0
	for(int i = 0; i < 4; i ++){
		int pos = 0;
		if(cam[i].useperson == TRUE){		
			pos += sprintf(&alarm_list[i][pos], "0");
		}
		if(cam[i].useanimal == TRUE){		
			pos += sprintf(&alarm_list[i][pos], "1");
		}
		if(cam[i].usewater == TRUE){		
			pos += sprintf(&alarm_list[i][pos], "2");
		}
		if(cam[i].usefire == TRUE){		
			pos += sprintf(&alarm_list[i][pos], "3");
		}
		if(cam[i].usesmog == TRUE){		
			pos += sprintf(&alarm_list[i][pos], "4");
		}
		if(cam[i].usedown == TRUE){		
			pos += sprintf(&alarm_list[i][pos], "5");
		}
		alarm_list_count[i] = pos;
	}
#endif	
    iniparser_freedict(ini);

	show_parse();
	return 0;
}

#if 0
extern int read_config_aux_timestamp;

int parse_config_aux_ini(char * path)
{
	//char * path = "/data/app/auxctrlapp/configFile/config_aux.ini";
	//char * path = "./config_aux.ini";
	
	dictionary * ini;
	ini = iniparser_load(path);
    if (ini==NULL) {
        log("cannot parse %s\r\n", path);
        return -1 ;
    }

	int model_count = iniparser_getint(ini, "MODEL:model_cnt", 0);

	// 动态申请内�?保存 model 配置
	int * model_level = (int *)malloc(sizeof(int) * model_count);

	char ** model = (char **)malloc(sizeof(char *) * model_count);
	if(model == NULL){
		log("model malloc fail\e\n");
		return -2;
	}
	for(int i = 0; i < model_count; i ++){
		model[i] = (char *)malloc(16);
		if(model[i] == NULL){
			log("model[%d] malloc fail\r\n", i);
			for(int j = 0; j < i; j ++){
				free(model[j]);
			}
			free(model);
			return -3;
		}
	}

	char model_name[32] = {0};
	char level_name[32] = {0};
	for(int i = 0; i < model_count; i ++){
		sprintf(model_name, "MODEL:model%d", (short)i);
		sprintf(level_name, "MODEL:modelLevel%d", (short)i);
		strcpy(model[i], iniparser_getstring(ini, model_name, NULL));
		model_level[i] = iniparser_getint(ini, level_name, 0);
	}
	
	iniparser_freedict(ini);

#if 1
	printf("==========================================================\r\n");
	printf("model_count = %d\r\n", model_count);
	for(int i = 0; i < model_count; i ++){
		printf("model[%d] = %s\tmodel_level[%d] = %d\r\n", i, model[i], i, model_level[i]);
	}
	printf("==========================================================\r\n");
#endif	
	
	for(int i = 0; i < model_count; i ++){
		free(model[i]);
	}
	free(model);
	free(model_level);
	return 0;
}
#endif
#if 0
int change_ini_file(char * path)
{
	void *dictionary = NULL;
    FILE *fp = NULL;
    int ret = 0;

    if (!path) {
        log("invalid path\r\n");
        return -1;
    }

    dictionary = iniparser_load(path);
    if (!dictionary) {
        log("cannot parse %s\r\n", path);
        return -2;
    }

    /* set section */
    ret = iniparser_set(dictionary, "Config", NULL);
    if (ret < 0) {
        log("cannot set section Config in: %s\r\n", path);
        ret = -3;
        goto free_dict;
    }

    /* set key/value pair */
    ret = iniparser_set(dictionary, "Config:pd", "FALSE");
    if (ret < 0) {
        log("cannot set key/value Config:pd in: %s\r\n", path);
        ret = -4;
        goto free_dict;
    }

    fp = fopen(path, "w+");
    if (!fp) {
        log("iniparser: cannot open %s\r\n", path);
        ret = -5;
        goto free_dict;
    }

    iniparser_dump_ini(dictionary, fp);
    fclose(fp);
	
	return 0;
	
free_dict:
    iniparser_freedict(dictionary);
    return ret;
}
#endif
#if 0
extern int read_config_timestamp;
extern int read_common_ini_timestamp;
extern int common_ini_change;

static void * check_file_process(void * args)
{
	long int timestamp1 = get_timestamp();
	while(1)
	{
		long int timestamp2 = get_timestamp();
		if(timestamp2 - timestamp1 >= 10)
		{
			int temp = get_file_timestamp(CONFIG_INI_PATH);
			if(temp != read_config_timestamp){
				parse_config_ini_file(CONFIG_INI_PATH);
				read_config_timestamp = temp;
				temp = 0;
				log("%s changed \r\n", CONFIG_INI_PATH);
			}
#if 0
			temp = get_file_timestamp("./config_aux.ini");
			if(temp != read_config_aux_timestamp){
				parse_config_aux_ini("./config_aux.ini");
				read_config_aux_timestamp = temp;
				temp = 0;
				log("config_aux.ini changed \r\n");
			}
#endif
			temp = get_file_timestamp(COMMON_INI_PATH);
			if(temp != read_common_ini_timestamp){
				parse_common_ini(COMMON_INI_PATH);
				read_common_ini_timestamp = temp;
				common_ini_change = 1;
				temp = 0;
				log("%s changed \r\n", COMMON_INI_PATH);
			}
			
			timestamp1 = get_timestamp();
		}

		usleep(500 * 1000);
	}
	return 0;
}

int check_file_pthread_init()
{
	log("=====\r\n");
	memset(&cam_status, 0, sizeof(cam_status_t) * 4);
//	for(int i = 0; i < 4; i ++){
//		memset(alarm_list[i], '!', 32);
//	}
	pthread_t check_file_pid;
	
	pthread_create(&check_file_pid, NULL, check_file_process, NULL);
	
	//pthread_join(check_file_pid, NULL);

	return 0;
}
#endif

