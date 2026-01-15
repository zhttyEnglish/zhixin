>>>>>>【测试使用说明】

	使用 SDK中的 libmpi.so 或者 libmpi.a   + libcjson.a 库，用户可以添加自己的 测试程序，示例代码在 sample/epri_test 目录。

>>>>>>【 代码】
    	xx_intf.c : 接口实现文件
    	xx_intf.h : 提供给 epri_test.c 的回调函数声明
    	xx_json_inf.c:  json 和 struct 相互转换的结构体。
    	xx_json_inf.h:  struct 结构体定义。
			epri_test.c: 测试主程序，注册回调等。
     
>>>>>>【 代码对接说明】

	1. 根据协议在 xx_issue_intf.c 文件中， 使用 xx_http_intf_init 函数注册json 协议中 command 字段对应的命令字符串的处理函数xx_json_proc_cb。例如： "command": "mobilenet_model",
	2. 实现 xx_json_proc_cb 函数： 解析 json --> struct, 获取资源路径(二进制文件、Images文件)调用 XX_DoTest_Task 函数做具体测试，调用struct -> json, 调用 http 回复函数。需要有二进制数据post 到服务器时，使用SC_MPI_SYS_HttpClient_PostData, 将data_type 设为MPI_SYS_POST_DATA_TYPE_BIN
	3. 协议解析使用 xx_json_inf.c  函数， xx_json_inf.h 定义结构体。协议变更时，可以参考已有函数编写。

【注意】
	需要更新 sdk 1.3.6 以上版本才能使用。