int alarm_image_data_upload(int sockfd, char *path, int alarm_id);
int upload_image_data(int sockfd, char *path, int cmd_type, int cam_id);
int handle_send_data(int sockfd, char * buf, char * image_path, int cam_id);
int send_alarm_data(int sockfd, char * buf, char * image_path, int alarm_id, 
		char * device_name, char *alarm_type_list, int alarm_type_count);


