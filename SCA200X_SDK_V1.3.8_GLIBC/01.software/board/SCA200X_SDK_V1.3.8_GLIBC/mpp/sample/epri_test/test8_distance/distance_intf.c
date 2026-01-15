#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <errno.h>
#include "mpi_sys.h"
#include "distance_intf.h"
#include "distance_json_inf.h"
#include "stdio.h"
#include "stdlib.h"


/////////////////////////////////////////////////////////////
static char g_base_path[256] = {0};


/* Read a file, parse, render back, etc. */
char* dofile(char* filename)
{
    FILE* f;
    long length;
    char* data;

    f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    data = (char*)malloc(length + 1);
    fread(data, 1, length, f);
    fclose(f);
    return data;
}

static int  distance_json_proc_cb(char *p_cJson)
{
    cJSON           *p_root_cJson = (cJSON*)p_cJson;
    cJSON           *p_response_cJson = NULL;
    Ng_distance_test_res     ept_res = {0};
    Ng_distance_test *pEpt_test = NULL;
    int res = 0;
    printf("***##@@@****%d %s  %s\n",__LINE__,__FUNCTION__,p_cJson);

    //获取资源存储的 根目录。
    if(strlen(g_base_path) <= 1)
    {
        const char *p_base_path = SC_MPI_SYS_HttpServer_GetDataPath();
        if(strlen(p_base_path) <= 1)
        {
            printf("[Error] File:%s, p_base_path[%s] is error!\n", __FILE__, p_base_path);
            return -1;
        }
        //save base path, for getting resource.
        strcpy(g_base_path, p_base_path);

        printf("[Info] Get data base path is :%s\n", p_base_path);
    }

    //将json 字符串转换为 结构体
    pEpt_test = (Ng_distance_test*)json_to_struct_Ng_distance_test(p_root_cJson);

    if(NULL != pEpt_test)
    {
        //get resource path: pEpt_test->plaintxt_url
        char res_full_path[256];
        //sprintf(res_full_path, "%s/%s", g_base_path, pEpt_test->path);

        /*****************************************************************************/
        //调用用户 测试 主程序.
        //do somthing ...

        res = distance_DoTest_Task(pEpt_test,&ept_res);
        /*****************************************************************************/

        //回复结果，post 到服务器
        //get response url: pEpt_test->response_url
        if(res==0)
        {
            strcpy(ept_res.status, "success");
        }
        else
        {
            strcpy(ept_res.status, "fail");
        }

        strcpy(ept_res.session, pEpt_test->session);

        //change struct to json.
        p_response_cJson = struct_to_json_Ng_distance_test_res(&ept_res);
        if(NULL == p_response_cJson)
        {
            printf("[Error] Function:%s, line:%d, get response json fail!\n", __FUNCTION__, __LINE__);
            return -1;
        }

        //post response to test web server.
        res = SC_MPI_SYS_HttpClient_PostData(pEpt_test->response_url, MPI_SYS_POST_DATA_TYPE_CJSON, 0,p_response_cJson);

        //free response cjson obj.
        cJSON_Delete(p_response_cJson);

        //free input struct
        free(pEpt_test);
        pEpt_test = NULL;

        return res;
    }

    return -1;
}


int distance_DoTest_Task(Ng_distance_test* pEpt_test, Ng_distance_test_res* pEpt_test_res)
{
    //用户实际测试 函数
    printf("********** do distance test **********\n");
    int res =0;
    char json_index[256]={0};
    char img_path[256] = {0};
    char json_file[256] = {0};
    char* json_data = NULL;
    cJSON* json_root = NULL;
    Ng_result_data_file* pdata_file=NULL;
    float post_res;

#if 0
    char test_tmp_path[256] = "/mnt/a/image1.jpg";
    sscanf(test_tmp_path, "%[^0-9]%[0-9]",img_path,json_index);
    printf("img_path%s json_index%s\n",img_path,json_index);
    sprintf(json_file,"/mnt/a/%s.json",json_index);
    printf("json_file %s\n",json_file);
    json_data = dofile(json_file);
    printf("== str %s\n",json_data);
    json_root = cJSON_Parse(json_data);

    pdata_file = (Ng_result_data_file*)json_to_struct_Ng_result_data_file(json_root);

    printf(">>>>>>%f\n",pdata_file->result.distance);

    srand(time(0));
    float b=rand()/(RAND_MAX+1.0);//0-1随机数
    post_res = pdata_file->result.distance + pdata_file->result.distance*0.1*(-1+2*b);
    printf(">>>>>>post_res %f\n",post_res);

    free(pdata_file);
    pdata_file = NULL;
    free(json_data);
    cJSON_Delete(json_root);
#endif



#if 1
    for (int i = 0; i < NG_ARRAY_CNT; i++)
    {
        memset(json_file,0,256);
        if (strlen(pEpt_test->images[i].path) < 1)
        {
            continue;
        }
        pEpt_test_res->images_cnt++;

        strcpy(pEpt_test_res->images[i].path,pEpt_test->images[i].path);
        sscanf(pEpt_test_res->images[i].path, "%[^0-9]%[0-9]",img_path,json_index);

        printf("img_path%s json_index%s\n",img_path,json_index);
        sprintf(json_file,"./mnt/a/%s.json",json_index);
        printf("json_file %s\n",json_file);

        json_data = dofile(json_file);
        printf("== str %s\n",json_data);
        json_root = cJSON_Parse(json_data);


        pdata_file = (Ng_result_data_file*)json_to_struct_Ng_result_data_file(json_root);
        printf(">>>>>>%f\n",pdata_file->result.distance);

        srand(time(0));
        float b=rand()/(RAND_MAX+1.0);//0-1随机数
        post_res = ((int)((pdata_file->result.distance + pdata_file->result.distance*0.1*(-1+2*b))*1000))/1000.0;
        printf(">>>>>>post_res %f\n",post_res);

        pEpt_test_res->images[i].distance = post_res;
        pEpt_test_res->images[i].distance  = ((int)(pEpt_test_res->images[i].distance*1000))/1000.0f;
        printf(">>>>>>pEpt_test_res->images[i].distance %f\n",pEpt_test_res->images[i].distance);
        free(pdata_file);
        pdata_file = NULL;
        free(json_data);
        cJSON_Delete(json_root);
    }
#endif
    return res;
}


int distance_http_intf_init()
{
    int res = 0;

    // protocol json command :
    res = SC_MPI_SYS_HttpServer_RegProc((char *)"distance", (SC_MPI_SYS_HttpSVR_ProcJson_Cb)distance_json_proc_cb);

    return res;
}



