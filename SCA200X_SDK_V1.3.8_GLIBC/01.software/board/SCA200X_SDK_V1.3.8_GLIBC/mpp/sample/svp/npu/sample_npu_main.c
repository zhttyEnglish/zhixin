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

#include "sample_npu_main.h"
#include "sc_comm_svp.h"
#include "mpi_npu.h"
static SC_CHAR **s_ppChCmdArgv = NULL;
/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void SAMPLE_SVP_HandleSig(int s32Signo)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    if (SIGINT == s32Signo || SIGTERM == s32Signo)
    {
        switch (*s_ppChCmdArgv[1])
        {
        case '0':
        {
            SAMPLE_SVP_NPU_Dcnn_HandleSig();
        }
        break;
        case '1':
        {
            SAMPLE_SVP_NPU_Ssd_HandleSig();
        }
        break;
        case '2':
        {
            SAMPLE_SVP_NPU_Yolov3_HandleSig();
        }
        break;
        case '3':
        {
            SAMPLE_SVP_NPU_CB_HandleSig();
        }
        break;

        default :
        {

        }
        break;
        }

        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_SVP_Usage(char *pchPrgName)
{
    printf("Usage : %s <index> \n", pchPrgName);
    printf("index:\n");
    printf("\t 0) Cnn(Read File).\n");
    printf("\t 1) SSD(Read File).\n");
    printf("\t 2) Yolov3(Read YUV420P File).\n");
    printf("\t 3) Cnn CallBack(Read File).\n");
}

/******************************************************************************
* function : npu sample
******************************************************************************/
int main(int argc, char *argv[])
{
    SC_S32 s32Ret = SC_SUCCESS;

    if (argc < 2)
    {
        SAMPLE_SVP_Usage(argv[0]);
        return SC_FAILURE;
    }
    s_ppChCmdArgv = argv;

    signal(SIGINT, SAMPLE_SVP_HandleSig);
    signal(SIGTERM, SAMPLE_SVP_HandleSig);

    switch (*argv[1])
    {
    case '0':
    {
        SAMPLE_SVP_NPU_Dcnn();
    }
    break;
    case '1':
    {
        SAMPLE_SVP_NPU_Ssd();
    }
    break;

    case '2':
    {
        SAMPLE_SVP_NPU_Yolov3();
    }
    break;

    case '3':
    {
        SAMPLE_SVP_NPU_CB();
    }
    break;

    default :
    {
        SAMPLE_SVP_Usage(argv[0]);
    }
    break;
    }

    return s32Ret;
}

