#include <stdint.h>

int set_alarmTypeFrame(char * buf, uint32_t alarm_id, int total_count, char * device_name, uint8_t * alarm_type_list);

int set_alarmImageFrame(char * buf, uint32_t alarm_id, uint32_t total_count, uint32_t image_num, uint8_t * data, int data_len);

int set_heartBeatFrame(char * buf, uint8_t status_num, uint8_t * device_status, char * heartbeat_time);

int set_modelTypeFrame(char * buf, uint8_t total_count, char model_type_list[][12]);

int set_deviceInfoFrame(char * buf, uint16_t heartbeat_time, uint8_t device_num, char * version, char device_name[][13]);

int set_getTTUInfoFrame(char * buf, uint16_t param);

int set_getUpdateFrame(char * buf, uint16_t param);

int set_alarmTypeRecoveryFrame(char * buf, int num, char * device_name, char * alarm_recover_list);

int set_uploadParamFrame(char * buf, uint16_t type);

int set_alarmParamFrame(char * buf, uint16_t type);

int set_uploadImageFrame(char * buf, uint16_t type, uint16_t camera_id, uint32_t total_count, uint32_t num, uint8_t * data, int data_len);




int set_ttu_reply(char * buf, uint16_t param, uint8_t len, char * data);

int set_updata_reply(char *buf, uint16_t param, uint8_t flag);

int set_snapshot_reply(char *buf, uint16_t snap_type, uint16_t status, uint16_t cameraid, uint8_t snap_switch, char * time);

int set_alarmparam_reply(char * buf, uint16_t alarm_param, uint8_t alarm_lv_cnt, uint16_t alarmtime1, uint16_t alarmtime2, 
								uint16_t alarmtime3, uint16_t alarm_model_len, char * model);


