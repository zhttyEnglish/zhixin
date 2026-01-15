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

#include "mpi_sys.h"
#include "issue_intf.h"
#include "vi_intf.h"
//#include "audio_alarm_intf.h"
#include "track_intf.h"
#include "distance_intf.h"
#include "filter_intf.h"


extern int VI_Test(void);

int main(int argc, int *argv[])
{
    int res = 0;

    // init http server.
    res = SC_MPI_SYS_HttpServer_Init(9000, "/home/dky/test");

    // register test service.
    res = issue_http_intf_init();
    res = vi_http_intf_init();
    //res = audio_alarm_http_intf_init();
    res = track_http_intf_init();
    res = distance_http_intf_init();
    res = filter_http_intf_init();





    //Filter_Test();
    //Track_Test();


    //distance_DoTest_Task(NULL,NULL);
    

    if(res)
    {
        printf("Demo fail, exit!\n");
        return -1;
    }

    printf("Demo run...\n");

    while(1)
    {
        sleep(10);
    }

    return res;
}



