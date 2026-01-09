20251204
1,更新so库
2,更新mpi
3,算法模型指定目录为当前目录lenovo_alg/yolov8_onnx.bin
4,如需开启算法存图功能,配置int saveFlag = 1; 会使用tmp目录，即使拷贝走，1000帧推理一次
5,程序运行是会使用tmp/目录一些内存
6,userdata 目录数据使用完，需要删除





1,替换mpi到板子/usr/bin/ 下,后重启板子，se为网络server,如运行demo程序,则需要首先运行./se &;
2,修改Makefile 中INC_FLAGS和LDFLAGS路径,然后编译
3,先使用vlc测试rtsp地址是否可以正常拉流,然后在config.ini中修改或者添加rtsp地址
4,由于没有算法,代码中添加了间隔1000帧存图和网络发送结果功能,const char *result_path_json = "/userdata/result";const char *image_path_jpg = "/userdata/image"; 目录需要提前创建
5,等联想提供算法库,代码搜索lenovo,完善相应函数即可
