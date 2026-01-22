#include <stdint.h>
#include <stdbool.h>

typedef struct config_ini
{
	char license[32];
	char deviceid[12];
	char camcount[8];
	char sn[32];
	double smog;
	double fire;
	double down;
	double water;
	double animal;
	double person;
	bool pd;
	int linesize;
}config_ini_t;


typedef struct camera_conf
{
	char cameraid[128];
	char rect[32];
	bool useperson;
	bool useanimal;
	bool usedown;
	bool usewater;
	bool usesmog;
	bool usefire;
	int camcode;
	int osd;
	char framerate[2];//h / m / l ¸ß/ÖÐ/µÍ
	int in_use;
}camera_conf_t;

typedef struct cam_status
{
	char device_name[16];
	int online;
	int cam_id;
	int remote_upload;
	int timed_upload;
	int remote_test_upload;
}cam_status_t;


typedef struct alarm_model
{
	int alarm_type;
	int alarm_level;
	int alarm_time;
	long long int alarm_timestamp;
	int alarm_flag;
}alarm_model_t;

int parse_common_ini(char * path);
int parse_param_ini(char * path);
int parse_config_ini_file(char * path);
//int create_config_ini(char * path);
//int change_ini_file(char * path);
//int parse_config_aux_ini(char * path);

int get_timestamp();
int get_file_timestamp(char *filename);

//int check_file_pthread_init();


