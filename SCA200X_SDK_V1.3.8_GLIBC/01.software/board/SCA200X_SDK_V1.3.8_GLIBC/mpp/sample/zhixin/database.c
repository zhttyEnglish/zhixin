#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <stdint.h>
#include "log.h"

extern uint32_t alarm_id;
sqlite3 * db = NULL;

int set_alarm_id(void *NotUsed, int argc, char **argv, char **azColName)
{
	NotUsed = NULL;
	sscanf(argv[0], "%d", &alarm_id);
	log("argv[0] = %s, alarm_id = %d\r\n", argv[0], alarm_id);
	alarm_id += 1;
	
    return 0;
}

int database_init(char * path)
{
    char *err_msg = 0; 
	int rc;  
  
	// 打开数据库（如果它不存在，则创建一个新的）	
	rc = sqlite3_open(path, &db);  
	if (rc) {  
		log("open database error : %s\n", sqlite3_errmsg(db));  
		return -1;  
	} else {  
		log("open database success\r\n");  
	}  

//	char * camera = "CREATE TABLE IF NOT EXISTS camera ("
//						"camera_name VARCHAR(4) PRIMARY KEY, "
//						"rtsp_addr VARCHAR(128) NOT NULL UNIQUE, "
//						"person_enable BOOLEAN DEFAULT (TRUE), "
//						"animal_enable BOOLEAN DEFAULT (TRUE), "
//						"down_enable BOOLEAN DEFAULT (TRUE), "
//						"fire_enable BOOLEAN DEFAULT (TRUE), "
//						"smoke_enable BOOLEAN DEFAULT (TRUE), "
//						"water_enable BOOLEAN DEFAULT (TRUE), "
//						"rect TEXT NOT NULL, "
//						"osd_enable BOOLEAN DEFAULT (TRUE), "
//						"frame_rate VARCHAR(1) NOT NULL, "
//						"cam_code VARCHAR(128) DEFAULT NULL "
//					");";
#if 1
	char * camera = "CREATE TABLE IF NOT EXISTS camera ("
			        "id INTEGER PRIMARY KEY, "
			        "name TEXT, "
			        "rtsp_url TEXT,"
			        "status  INTEGER, "
			        "algorithm_flag INTEGER);";
	
	rc = sqlite3_exec(db, camera, NULL, NULL, &err_msg); 
	if (rc != SQLITE_OK) {	
		log("sqlite create error : %s\n", err_msg);  
		sqlite3_free(err_msg);	
		return -2;	
	} else {  
		log("----- sqlite camera create success -----\r\n"); 
	} 
#endif
	// 创建 报警历史 表  
	char *alarm_history = "CREATE TABLE IF NOT EXISTS alarm_history ("  
							"alarm_id INTEGER PRIMARY KEY AUTOINCREMENT, "
							"alarm_time INTEGER NOT NULL, "
							"camera_name VARCHAR(15) NOT NULL, "  
							"alarm_image VARCHAR(128) NOT NULL UNIQUE, "	
							"person_alarm BOOLEAN DEFAULT (TRUE), "  // 人
							"animal_alarm BOOLEAN DEFAULT (TRUE), "  // 动物
							"fire_alarm BOOLEAN DEFAULT (TRUE), "  // 火
							"smoke_alarm BOOLEAN DEFAULT (TRUE), "  // 烟
							"water_alarm BOOLEAN DEFAULT (TRUE), "  // 水
							"detail TEXT NOT NULL"
						");";  
  
	rc = sqlite3_exec(db, alarm_history, NULL, NULL, &err_msg);	
	if (rc != SQLITE_OK) {	
		log("sqlite create error : %s\n", err_msg);  
		sqlite3_free(err_msg);	
		return -2;  
	} else {  
		log("----- sqlite alarm_history create success -----\r\n");	
	} 
#if 0
	char *alarm_history_detail = "CREATE TABLE IF NOT EXISTS alarm_history_detail ("  
									"alarm_id INTEGER NOT NULL, "
									"alarm_num INTEGER NOT NULL, "
									"alarm_type VARCHAR(12) NOT NULL, "
									"alarm_conf DOUBLE NOT NULL, "  
									"alarm_pos TEXT NOT NULL "
								");";  
  
	rc = sqlite3_exec(db, alarm_history_detail, NULL, NULL, &err_msg);	
	if (rc != SQLITE_OK) {	
		log("sqlite create error : %s\n", err_msg);  
		sqlite3_free(err_msg);	
		return -2;  
	} else {  
		log("----- sqlite alarm_histary create success -----\r\n");	
	}  
#endif
	// 查询报警记录数量
	rc = sqlite3_exec(db, "SELECT COUNT(*) FROM alarm_history;", set_alarm_id, NULL, &err_msg);	
	if (rc != SQLITE_OK) {	
		log("find max alarm_id error: %s\n", err_msg);  
		sqlite3_free(err_msg);	
	} else {  
		log("find max alarm_id success\n");  
	}  
  
	// 关闭数据库连接  
//	sqlite3_close(db);	

	return 0;  
}

int database_insert_alarm(char * alarm_sql)
{
	char * error_msg = NULL;
	int ret = sqlite3_exec(db, alarm_sql, NULL, NULL, &error_msg);
	if(ret != SQLITE_OK){
		log("sqlite insert error : %s\r\n", error_msg);
		sqlite3_free(error_msg);
		return -1;
	}else{
		log("----- sqlite insert alarm_history success -----\r\n");
	}
	return 0;
}

int database_insert_detail(char * detail_sql)
{
	char * error_msg = NULL;
	int ret = sqlite3_exec(db, detail_sql, NULL, NULL, &error_msg);
	if(ret != SQLITE_OK){
		log("sqlite insert error : %s\r\n", error_msg);
		sqlite3_free(error_msg);
		return -1;
	}else{
		log("----- sqlite insert alarm_history_detail success -----\r\n");
	}
	return 0;
}


int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
    NotUsed = NULL;

    for (int i = 0; i < argc; ++i)
    {
        printf("%s = %s\n", azColName[i], (argv[i] ? argv[i] : "NULL"));
    }

    printf("\n");

    return 0;
}

int database_select()
{
	char * error_msg = NULL;
	int ret = sqlite3_exec(db, "SELECT * FROM alarm_history ORDER BY alarm_id DESC LIMIT 1;", callback, NULL, &error_msg);
	if(ret != SQLITE_OK){
		log("sqlite select error : %s\r\n", error_msg);
		sqlite3_free(error_msg);
		return -1;
	}
	//else{
		//log("----- sqlite select success -----\r\n");
	//}
	return 0;
}

void database_exit()
{
	sqlite3_close(db);	
}

//int main()
//{
//	database_init("/userdata/alarm_history.db");
//	return 0;
//}

