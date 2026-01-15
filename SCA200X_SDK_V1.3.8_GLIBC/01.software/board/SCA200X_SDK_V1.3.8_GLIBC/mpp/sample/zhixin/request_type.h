#include <stdint.h>


/*
	帧类型: 
	0 报警类型帧
	1 报警图片数据帧
	2 心跳数据帧
	3 模型类型帧 
	4 设备信息类型帧
	5 获取TTU信息帧 
	6 升级标志
	7 报警类型恢复帧
	8 抓拍查询
	9 报警参数查询
	10 召测类型数据帧
*/
struct Header{
	uint32_t 	head_flag;   	//固定值 0xFCFCFCFC
	uint8_t 	frame_type;			// 帧类型 
	uint8_t 	version;			//协议版本 0x01
	uint32_t 	package_size; 	//包长 低字节在前
};


struct alarmTypePayload{
	uint32_t 	alarm_id;		//报警id 0 ~ 0xffffffff (用于报警和图片保持一致)
	uint8_t 	alarm_count;		//报警类型总数	识别到的报警类型总数
	char 	device_name[12];	//报警设备名	参见设备信息
	int * 	alarm_type;			//报警类型集合
	uint16_t finish_flag;		//结束标志  0xFDFD
};

struct alarmImagePayload{
	uint32_t 	alarm_id;		//报警id 0 ~ 0xffffffff (用于报警和图片保持一致)
	uint32_t 	total_count;		//图片帧总数	图片大小按照图片数据体分成的总数
	uint32_t	image_num;			//图片帧序列号	从序号1开始计数…
	unsigned char * image_data; //图片数据体	必须满足N<1Mbyte
	uint16_t	finish_flag;
};

struct heartBeatPayload{
	uint8_t status_num;				//设备状态数量	设备数量(必须等于设备信息帧的数量)
	int * device_status;		//每个设备状态	0:正常、1摄像头或设备离线（后续可扩展）
	char heartbeat_time[30];	//心跳时间	格式：20190301T093009z 备注：多余字节为空字节
	uint16_t finish_flag;
};

struct modelTypePayload{
	uint8_t total_count;			//模型总数
	char ** model_type; 			//模型类型集合(单名长度<=12){person;animal;water;fire;smoke;waterdrop}
	uint16_t finish_flag;
};

typedef enum{
	person = 0, 
	animal = 1,
	water = 2,
	fire = 3,
	smoke = 4,
	waterdrop = 5
}alarmModel;

struct deviceInfoPayload{
	uint16_t heartbeat_frequency;	//心跳频率	默认300秒
	uint8_t device_num;				//设备数量	设备(1)+摄像头(N-1)
	char software_version[16];	//软件版本号	格式：[H1.1;Vxx.x-yyy]
	char device_name[14];		//设备名称	以设备为准{12char;
	char ** camera_name;		//摄像头名称	以摄像头为准
	uint16_t 	finish_flag;
};

struct getTTUInfoPayload{
	uint16_t parameter;			//获取参数列表	1---get_time
	uint16_t 	finish_flag;
};

struct UpdatePayload{
	uint16_t update_flag; 		//升级标志
	uint16_t finish_flag;
};

struct alarmTypeRecovery{
	uint8_t recover_num; 		//报警类型恢复总数
	char device_name[12];		// 报警设备名
	char * alarm_recovery;		//报警类型恢复集合
	uint16_t finish_flag;
};

struct uploadParamPayload{
	uint16_t snapshot_type;		// 查询抓拍类型
	uint16_t finish_flag;
};

struct alarmParamPayload{
	uint16_t alarm_param_type;		// 查询报警参数类型
	uint16_t finish_flag;
};

struct uploadImagePayload{
		uint16_t type;   		// 0x 召测 1x 告警主动上报 3x 主动抓取 4x 早上定时 5x 晚上定时 6x 远程许可打开 7x 远程许可关闭
								// x0 - x9 对应摄像机编号
		uint16_t camera_id;		// 1 -- N
		uint32_t image_count;	// 图片帧总数
		uint32_t image_num;		// 图片帧序列号
		uint8_t *image_data;	// 图片数据体
		uint16_t finish_flag;
};	


typedef struct alarmTypeFrame{
	struct Header header;
	struct alarmTypePayload payload;
}alarmTypeFrame_t;				// 报警类型帧

typedef struct alarmImageFrame{
	struct Header header;
	struct alarmImagePayload payload;
}alarmImageFrame_t;				// 报警图片数据帧

typedef struct heartBeatFrame{
	struct Header header;
	struct heartBeatPayload payload;
}heartBeatFrame_t;				// 心跳数据帧

typedef struct modelTypeFrame{
	struct Header header;
	struct modelTypePayload payload;
}modelTypeFrame_t;				// 模型类型帧

typedef struct deviceInfoFrame{
	struct Header header;
	struct deviceInfoPayload payload;
}deviceInfoFrame_t;				// 设备信息类型帧

typedef struct getTTUInfoFrame{
	struct Header header;
	struct getTTUInfoPayload payload;
}getTTUInfoFrame_t;				// 获取TTU信息帧

typedef struct getUpdateFrame{
	struct Header header;
	struct UpdatePayload payload;
}getUpdateFrame_t;				// 升级标志

typedef struct alarmTypeRecoveryFrame{
	struct Header header;
	struct alarmTypeRecovery payload;
}alarmTypeRecoveryFrame_t;		//报警类型恢复帧

typedef struct uploadParamFrame{
	struct Header header;
	struct uploadParamPayload payload;
}uploadParamFrame_t;				//抓拍查询

typedef struct alarmParamFrame{
	struct Header header;
	struct alarmParamPayload payload;
}alarmParamFrame_t;				//报警参数查询

typedef struct uploadImageFrame{
	struct Header header;
	struct uploadImagePayload payload;
}uploadImageFrame_t;				//召测类型数据帧
