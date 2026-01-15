int tcp_pthread_init();
int handle_send_data_pre(char * buf, char * image_path, int cam_id);

int send_alarm_data_pre(char *buf, char * image_path, int alarm_id, 
			char * device_name, char *alarm_type_list, int alarm_type_count);
int sned_alarm_recover_pre(char * buf, uint8_t num, char * device_name, char * alarm_recover_list);
int send_getupdate(char * buf, int para);
int tcp_nonblocking_recv(int conn_sockfd, void *rx_buf, int buf_len, int timeval_sec, int timeval_usec);


