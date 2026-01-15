int get_algo_result_thread_init();
int read_and_parse_json(char * path);
int get_camid(char * path);
int get_device_name(int cam_id, char * cam_name);
int if_exist(char * path);
int save_alarm_history(char * buf, int len, char * result_path, char * image_path);

