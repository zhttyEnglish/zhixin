#include <stdio.h>

#define TRUE 1
#define FALSE 0

#define COMMON_INI_PATH "/userdata/common.ini"
#define PARAM_INI_PATH "/userdata/param.ini"
#define CONFIG_INI_PATH "/userdata/config.ini"
#define ALARM_HISTORY_PATH	"/userdata/history_alarm.db"
#define CAMERA_PATH	"/userdata/camera.db"

#define DEBUG_OPEN 1

#if DEBUG_OPEN
#define log(format, ...) \
  			printf("[%s:%d] "format, __func__, __LINE__,  ##__VA_ARGS__)
#else
#define log(format, ...)
#endif

