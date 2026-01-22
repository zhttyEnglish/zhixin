#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp> 
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cJSON.h"

using namespace std;
using namespace cv;

int area_num = 0;
int area_x1[16] = {0};
int area_x2[16] = {0};
int area_y1[16] = {0};
int area_y2[16] = {0};

void show_usage()
{
	printf("usage: \r\n");
	printf("./draw_text image_path result_path\r\n");
}

int read_result(char * result_path)
{
	char buf[1024] = {0};
	FILE * fp = fopen(result_path, "r");
	fread(buf, 1, 1024, fp);
	fclose(fp);

	cJSON * root = NULL;
	cJSON * num = NULL;
	cJSON * detections = NULL;
	
	root = cJSON_Parse(buf);
	num = cJSON_GetObjectItem(root, "num_detections");
	detections = cJSON_GetObjectItem(root, "detections");

	area_num = num->valueint;
	
	for(int i = 0; i < num->valueint; i ++)
	{
		cJSON * array = cJSON_GetArrayItem(detections, i);
		cJSON * conf = cJSON_GetObjectItem(array, "conf");
		cJSON * cls_id = cJSON_GetObjectItem(array, "cls_id");
		cJSON * x1 = cJSON_GetObjectItem(array, "x1");
		cJSON * y1 = cJSON_GetObjectItem(array, "y1");
		cJSON * x2 = cJSON_GetObjectItem(array, "x2");
		cJSON * y2 = cJSON_GetObjectItem(array, "y2");

		area_x1[i] = (int)(x1->valuedouble + 0.5);
		area_x2[i] = (int)(x2->valuedouble + 0.5);
		area_y1[i] = (int)(y1->valuedouble + 0.5);
		area_y2[i] = (int)(y2->valuedouble + 0.5);
	}
	return 0;
}

int draw_text(char *image_path)
{
	char text[256] = {0};
	cv::Mat img = cv::imread(image_path);
	
	// 文本内容
    time_t timep;
    struct tm *p;
    time (&timep);
    p = gmtime(&timep);
    sprintf(text,"Alarm Time: %04d-%02d-%02d %02d:%02d:%02d",1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, 8 + p->tm_hour, p->tm_min, p->tm_sec);

    // 文本位置
    cv::Point org(50, 150);

    // 字体类型
    int fontFace = FONT_HERSHEY_SIMPLEX;

    // 字体缩放
    double fontScale = 1.5;

    // 颜色 (红色) 
    cv::Scalar color(0, 0, 255);

    // 线条粗细
    int thickness = 2;

    // 绘制文本 putText(canvas,"(50,300)",Point(50,300),1,1,Scalar(0));
    cv::putText(img, text, org, fontFace, fontScale, color, thickness);
	for(int i = 0; i < area_num; i ++)
	{
		cv::Point pt1(area_x1[i], area_y1[i]);
		cv::Point pt2(area_x2[i], area_y2[i]);
		cv::rectangle(img, pt1, pt2, color, thickness);
	}

	cv::imwrite(image_path, img);
    return 0;
}

int main(int argc, char ** argv)
{
	if(argc != 3){
		show_usage();
		return -1;
	}

	char * image_path = argv[1];
	char * result_path = argv[2];
	
	if (access(image_path, F_OK) != -1) {
		printf("%s exist\n", image_path);
	} else {
		printf("%s not exist !!!\n", image_path);
	}

	if (access(result_path, F_OK) != -1) {
		printf("%s exist\n", result_path);
	} else {
		printf("%s not exist !!!\n", result_path);
	}

	read_result(result_path);

	draw_text(image_path);

	return 0;
}

