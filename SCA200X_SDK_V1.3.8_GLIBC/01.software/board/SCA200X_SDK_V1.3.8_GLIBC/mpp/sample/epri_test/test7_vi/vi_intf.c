
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <signal.h>

#include "mpi_sys.h"
#include "vi_intf.h"
#include "vi_json_inf.h"

/////////////////////////////////////////////////////////////
#define VI_COMMAND "snap"
#define VI_JPG_MAXSIZE 5000000

static char g_base_path[256] = {0};

int VI_DoTest_Task(Ng_vi_test *p_issue_info);

static int  vi_json_proc_cb(char *p_cJson)
{
    cJSON               *p_root_cJson = (cJSON*)p_cJson;
    cJSON               *p_response_cJson = NULL;
    Ng_test_res         test_json_res = {0};
    Ng_vi_test   *p_test_json = NULL;
    int res = 0;

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

        //printf("[Info] Get data base path is :%s\n", p_base_path);
    }

    //将json 字符串转换为 结构体
    p_test_json = (Ng_vi_test*)json_to_struct_Ng_vi_test(p_root_cJson);
    if(NULL != p_test_json)
    {
        /*****************************************************************************/
        //调用用户 测试 主程序.
        //do somthing ...

        res = VI_DoTest_Task(p_test_json);
        /*****************************************************************************/

        //回复结果，post 到服务器
        //get response url: xxx->response_url
        if(!res)
        {
            strcpy(test_json_res.status, "success");
        }
        else
        {
            strcpy(test_json_res.status, "fail");
        }

        strcpy(test_json_res.session, p_test_json->session);

        //change struct to json.
        p_response_cJson = struct_to_json_Ng_vi_test_res(&test_json_res);
        if(NULL == p_response_cJson)
        {
            printf("[Error] Function:%s, line:%d, get response json fail!\n", __FUNCTION__, __LINE__);
            return -1;
        }

        //post response to test web server.
        res = SC_MPI_SYS_HttpClient_PostData(p_test_json->response_url, MPI_SYS_POST_DATA_TYPE_CJSON, 0, p_response_cJson);

        //free response cjson obj.
        cJSON_Delete(p_response_cJson);

        //free input struct
        free(p_test_json);
        p_test_json = NULL;

        return res;
    }

    return -1;
}

int VI_DoTest_Task(Ng_vi_test *p_issue_info)
{
    int res = -1;
    char *pout_buf = NULL;
    int  out_size = VI_JPG_MAXSIZE;
    static SC_U64 pts = 0;

    //用户实际测试 函数
    printf("********** do vi test **********\n");

    //execute cpu test.
    if (memcmp(p_issue_info->command, VI_COMMAND, strlen(VI_COMMAND)))
    {
        printf("command:%s err\n", p_issue_info->command);
        return -1;
    }


    printf("VI_DoTest_Task->start!\n");
    printf("output_url:%s\n", p_issue_info->output_url);
    printf("response_url:%s\n", p_issue_info->response_url);

    pout_buf = (char *)malloc(VI_JPG_MAXSIZE);
	if (!pout_buf)
	{
		printf("VI_JPG_MAXSIZE malloc error\n");
		return SC_FAILURE;
	}

    res = visdk_startvenc(pout_buf, &out_size);
    if(res)
    {
        printf("visdk_startvenc err, res(%d)\n", res);
        return res;
    }
    printf("visdk_startvenc->out_size:%d res:%d\n", out_size, res);
    if (access("/tmp/one_jpg", F_OK) == 0)
    {
        remove("/tmp/one_jpg");
        write_jpeg(1, pout_buf, out_size, pts++, 0);
    }
    else if (access("/tmp/all_jpg", F_OK) == 0)
    {
        write_jpeg(1, pout_buf, out_size, pts++, 0);
    }

#if 1
    //post response to test web server.
    res = SC_MPI_SYS_HttpClient_PostData(p_issue_info->output_url,
                MPI_SYS_POST_DATA_TYPE_BIN, out_size, pout_buf);
    if(res)
    {
        printf("SC_MPI_SYS_HttpClient_PostData(url:%s) err, out_size(%d) res(%d)\n",
            p_issue_info->output_url, out_size, res);
        return res;
    }
#endif

    free(pout_buf);
    pout_buf = NULL;

    printf("VI_DoTest_Task->out_size:%d\n", out_size);

    return res;
}

int vi_http_intf_init()
{
    int res = 0;

    // 用户注册    Json 命令回调函数        command :"execute"
    res = SC_MPI_SYS_HttpServer_RegProc(VI_COMMAND, (SC_MPI_SYS_HttpSVR_ProcJson_Cb)vi_json_proc_cb);

    return res;
}

int VI_Test(void)
{
    Ng_vi_test issue_info = {0};
    strcpy(issue_info.command, VI_COMMAND);
    strcpy(issue_info.response_url, "test/response/vi/output");

    VI_DoTest_Task(&issue_info);

    return 0;
}

