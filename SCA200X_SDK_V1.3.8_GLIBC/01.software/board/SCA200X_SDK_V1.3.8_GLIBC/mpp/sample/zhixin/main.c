#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "handle_config.h"
#include "tcp.h"
#include "algoresult.h"
#include "database.h"

extern int alarm_type_count;
extern char alarm_type_list[16];


//int read_config_timestamp = 0;
//int read_config_aux_timestamp = 0;
//int read_common_ini_timestamp = 0;
int main()
{
	parse_common_ini(COMMON_INI_PATH);
//	read_common_ini_timestamp = get_file_timestamp(COMMON_INI_PATH);
	parse_config_ini_file(CONFIG_INI_PATH);
//	read_config_timestamp = get_file_timestamp(CONFIG_INI_PATH);
//	parse_config_aux_ini("./config_aux.ini");
//	read_config_aux_timestamp = get_file_timestamp("./config_aux.ini");

	tcp_pthread_init();

//	check_file_pthread_init();

	get_algo_result_thread_init();

	database_init(ALARM_HISTORY_PATH);
	log("database_init done \r\n");

	while(1){
		sleep(1);
	}
	return 0;
}

