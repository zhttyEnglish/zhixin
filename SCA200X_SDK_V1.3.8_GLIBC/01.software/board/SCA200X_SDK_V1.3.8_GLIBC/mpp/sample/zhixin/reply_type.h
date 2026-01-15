#include <stdint.h>

struct replyHeader{
	uint32_t 	head_flag;   	//固定值 0xFCFCFCFC
	uint8_t 	frame_type;		// 帧类型 
	uint32_t 	package_size; 	//包长 低字节在前
};

struct getTTUReplyPayload{
	uint16_t param_type;		//参数返回类型 1--获取时间
	uint8_t len;				//参数长度
	char  * data;				//参数内容  多余为空
	uint16_t finish_flag;
};

struct getUpdateReplyPayload{
	uint16_t para_type;			//参数返回类型 1--获取时间
	uint8_t update_flag;		//升级标志  0 无升级包  1 有升级包
	uint16_t finish_flag;
};

struct hostIssuePayload{
	uint16_t snapshot_type;     //抓拍类型  1 定时上报 2 召测 3 远程许可
	uint16_t data; 				// 类型为 3 时 远程许可状态 值为 0 打开远程许可 1 关闭远程许可
								// 类型为 2 时 召测相机ID    召测和抓拍 2 选 1
	uint8_t snapshot_switch;	// 定时抓拍开关 0 关 1 开
	char    snapshot_time[13];		// 格式 {08:00; 17:30}
	uint16_t finish_flag;
};

struct alarmLevelSetPayload{
	uint16_t alarm_param;       // 1  报警参数  2 报警分类
	// alalrm_param == 1
	uint8_t	alarm_level_count;  // 报警等级数量 默认4 最大9
	uint16_t lv1_alarm_time;	// 一级报警时间 单位 秒 默认 300
	uint16_t lv2_alarm_time;	// 二级报警时间 单位 秒 默认 60
	uint16_t lv3_alarm_time;	// 三级报警时间 单位 秒 默认 10  0 表示实时发送
	
	// alarm_param == 2
	uint16_t alarm_model_len;   //模型报警长度
	char * model;				// model 等级格式 {person=1;dire=2;water=3}
	uint16_t finish_flag;
};


typedef struct getTTUReplyFrame{
	struct replyHeader header;
	struct getTTUReplyPayload payload;
}getTTUReplyFrame_t;    			//TTU 返回报文

typedef struct getUpdateReplyFrame{
	struct replyHeader header;
	struct getUpdateReplyPayload payload;
}getUpdateReplyFrame_t;			//获取升级返回报文

typedef struct hostIssueFrame{
	struct replyHeader header;
	struct hostIssuePayload payload;
}hostIssueFrame_t;				// 主机主动下发或响应查询设置参数及远程许可设定

typedef struct alarmLevelSetFrame{
	struct replyHeader header;
	struct alarmLevelSetPayload payload;
}alarmLevelSetFrame_t;				// 报警级别参数设定

