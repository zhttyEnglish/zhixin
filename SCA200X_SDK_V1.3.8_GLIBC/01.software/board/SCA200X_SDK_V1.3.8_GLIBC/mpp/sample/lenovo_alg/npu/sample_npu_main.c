#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>

#include "cJSON.h"
#include "sample_npu_main.h"
#include "sample_npu.h"
// #include "opencv/cv.h"
// #include "opencv/highgui.h"

/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_SVP_Usage(char *pchPrgName)
{
    printf("Usage : %s <image_path> <model_path> \n", pchPrgName);
}

int get_lic_file(char * file)
{
	char front[32] = {0};
	char tail[8] = {0};
	char * p = NULL;
	DIR * dir = opendir("/userdata/lenovo_alg/");

	if(dir == NULL)
	{
		printf("dir == NULL\r\n");
		return -1;
	}

	struct dirent * dirp;
	while(1)
	{
		dirp = readdir(dir);
		if(dirp == NULL)
		{
		    break;
		}

		p = strchr(dirp->d_name, '.');
		if(p != NULL)
		{
			p += 1;
			if(strcmp(p, "lic") == 0)
			{
				memcpy(file, dirp->d_name, strlen(dirp->d_name));
			}
		}
	}
	//关闭目录
	closedir(dir);
	return 0;
}


/******************************************************************************
* function : npu sample
******************************************************************************/

int main(int argc, char *argv[])
{
    SC_S32 s32Ret = SC_SUCCESS;

    if (argc < 3)
    {
        SAMPLE_SVP_Usage(argv[0]);
        return SC_FAILURE;
    }

    const char* pcSrcFile  = argv[1];
    const char* pcModel    = argv[2];

    /* JSON 根对象 */
    cJSON *root = cJSON_CreateObject();

    // 生成Req文件
    SVP_NPU_GenerateLicense();

    /********************************************************
     *  1. 加载模型
     ********************************************************/
    SAMPLE_SVP_NPU_MODEL_S yolov8Model;
    SAMPLE_SVP_NPU_PARAM_S yolov8Param;
    SAMPLE_SVP_TRACE_INFO("----------------:\n");
    // license文件
    //const char *lic_file_path = "ef2bea9a18ee4cc5a58aedb173ed3a6e.lic";
    //const char *lic_file_path = "0c0f4da4.lic";//d15813db;f1232d9f;09eed5b0;f0357b4a
    char lic_file_path[16] = {0};
    get_lic_file(lic_file_path); 
    printf("lic_file_path = %s\r\n", lic_file_path);

    // 公钥文件
    const char *public_key_path = "public_key.pem";
    s32Ret = SVP_NPU_YOLOv8_LoadModel(pcModel, 
                                      &yolov8Model, 
                                      &yolov8Param,
                                      lic_file_path,
                                      public_key_path);


    if (s32Ret != SC_SUCCESS){
        SAMPLE_SVP_TRACE_ERR("Load model failed!\n");
        cJSON_Delete(root);
        return SC_FAILURE;
    }
    
    /********************************************************
     *  2. 推理一个图片
     ********************************************************/
    int oriImgWidth = 1920;
    int oriImgHeight = 1080;
    int resizeImgWidth = 640;
    int resizeImgHeight = 640;
    float iou_th = 0.3;
    float conf_th = 0.6;
    int class_num = 8;
    int saveFlag = 1;
    const char* saveFolder = "./result";

    SVP_NPU_YOLOv8_Predict(&yolov8Model,
                           &yolov8Param,
                           pcSrcFile,
                           oriImgWidth,
                           oriImgHeight,
                           resizeImgWidth,
                           resizeImgHeight,
                           iou_th,
                           conf_th,
                           class_num,
                           saveFlag,
                           saveFolder,
                           root);

    /********************************************************
     *  3. 保存结果
     ********************************************************/
    char *jsonString = cJSON_Print(root);
    FILE *file = fopen("result.json", "w");
    if (!file)
    {
        perror("Failed to open JSON output file");
        free(jsonString);
        cJSON_Delete(root);
        SVP_NPU_YOLOv8_Deinit(&yolov8Model, &yolov8Param);
        return SC_FAILURE;
    }

    fprintf(file, "%s\n", jsonString);
    fclose(file);

    free(jsonString);
    cJSON_Delete(root);

    /********************************************************
     * 5. 释放模型资源
     ********************************************************/
    SVP_NPU_YOLOv8_Deinit(&yolov8Model, &yolov8Param);

    return SC_SUCCESS;
}


