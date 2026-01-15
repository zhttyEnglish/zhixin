1,替换mpi到板子/usr/bin/ 下,后重启板子，se为网络server,如运行demo程序,则需要首先运行./se &;
2,修改Makefile 中INC_FLAGS和LDFLAGS路径,然后编译
3,先使用vlc测试rtsp地址是否可以正常拉流,然后在config.ini中修改或者添加rtsp地址
4,由于没有算法,代码中添加了间隔1000帧存图和网络发送结果功能,const char *result_path_json = "/userdata/result";const char *image_path_jpg = "/userdata/image"; 目录需要提前创建
5,等联想提供算法库,代码搜索lenovo,完善相应函数即可