#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "sample_comm.h"
//#include "loadbmp.h"
#include "mpi_ive.h"
#include "sample_ive_main.h"

void SAMPLE_IVE_Usage(char *sPrgNm)
{
    printf("Usage : %s <index>\n", sPrgNm);

    printf("index:\n"
        "\t 0) SAMPLE_IVE_Md\n"
        "\t 1) SAMPLE_IVE_RESZIE\n"
        "\t 2) SAMPLE_IVE_CSC\n");

    printf("\t If you have any questions, please look at readme.txt!\n");
    return;
}

/******************************************************************************
* function    : main()
* Description : video vgs sample
******************************************************************************/
SC_S32 main(SC_S32 argc, SC_CHAR *argv[])
{
    SC_S32 s32Ret = SC_FAILURE;
    SC_S32 s32Index = -1;

    if (argc < 2)
    {
        SAMPLE_IVE_Usage(argv[0]);
        return s32Ret;
    }

    if (!strncmp(argv[1], "-h", 2))
    {
        SAMPLE_IVE_Usage(argv[0]);
        return SC_SUCCESS;
    }

    s32Index = atoi(argv[1]);
    if (!s32Index && strncmp(argv[1], "0", 1))
    {
        s32Index = -1;
    }

    usleep(100 * 1000);

    switch (s32Index)
    {
    case 0:
        SAMPLE_IVE_Md();
        break;
    case 1:
        printf("resize\n");
        SAMPLE_IVE_RESZIE();
        break;
    case 2:
        SAMPLE_IVE_CSC();;
        break;
    default:
        SAMPLE_PRT("the index is invaild!\n");
        SAMPLE_IVE_Usage(argv[0]);
        break;
    }

    return 0;
}
