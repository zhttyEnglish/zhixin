#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <errno.h>

//#include "sc_math.h"
#include "sc_common.h"
#include "sc_comm_region.h"
#include "sc_comm_sys.h"
#include "mpi_region.h"

#define VALUE_BETWEEN(x,min,max) (((x)>=(min)) && ((x) <= (max)))

static RGN_HANDLE g_hgl = -1;
static SC_S32 g_need_update = 0;
static FILE *pfd = SC_NULL;
static SC_U32 u32SignalFlag = 0;

static SC_S32 RGN_Restore(RGN_HANDLE hdl)
{
    SC_S32 s32Ret = SC_FAILURE;

    if(pfd)
    {
        fclose(pfd);
        pfd = SC_NULL;
    }

    if(g_need_update)
    {
        s32Ret = SC_MPI_RGN_UpdateCanvas(hdl);
        if (SC_SUCCESS != s32Ret)
        {
            printf("update canvas error!!!\n");
        }
    }

    return SC_SUCCESS;
}

void VI_Chn_Dump_HandleSig(SC_S32 signo)
{
    if(u32SignalFlag)
    {
        exit(-1);
    }

    if (SIGINT == signo || SIGTERM == signo)
    {
        u32SignalFlag++;
        RGN_Restore(g_hgl);
        u32SignalFlag--;
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

void SAMPLE_RgnDump(RGN_HANDLE hdl)
{
    SC_CHAR szRgnName[128];
    SC_S32 s32Ret;
    RGN_CANVAS_INFO_S canvasInfo;
    char szPixFrm[16];

    s32Ret = SC_MPI_RGN_GetCanvasInfo(hdl, &canvasInfo);
    if (SC_SUCCESS != s32Ret)
    {
        printf("get canvas info error!!!\n");
        return;
    }
    g_need_update = 1;

    switch (canvasInfo.enPixelFmt)
    {
    case PIXEL_FORMAT_ARGB_1555:
        snprintf(szPixFrm, 10, "ARGB1555");
        break;
    case PIXEL_FORMAT_ARGB_4444:
        snprintf(szPixFrm, 10, "ARGB4444");
        break;
    case PIXEL_FORMAT_ARGB_8888:
        snprintf(szPixFrm, 10, "ARGB8888");
        break;
    default:
        snprintf(szPixFrm, 10, "--");
        break;
    }

    /* make file name */
    snprintf(szRgnName, 128, "./rgn_hdl%d_w%d_h%d_s%d_p%llx_%s.argb",
        hdl, canvasInfo.stSize.u32Width, canvasInfo.stSize.u32Height,
        canvasInfo.u32Stride, canvasInfo.u64PhyAddr, szPixFrm);
    printf("Dump RGN buffer of hdl %d  to file: \"%s\"\n", hdl, szRgnName);
    fflush(stdout);

    /* open file */
    pfd = fopen(szRgnName, "wb");

    if (SC_NULL == pfd)
    {
        printf("open file failed:%s!\n", strerror(errno));
        RGN_Restore(hdl);
        return;
    }

    fwrite((void*)(int)canvasInfo.u64VirtAddr, canvasInfo.u32Stride*canvasInfo.stSize.u32Height, 1, pfd);

    RGN_Restore(hdl);
    return;
}
static void usage(void)
{
    printf(
        "\n"
        "**********************************************************\n"
        "Usage: ./rgn_dump [hdl]\n"
        "1)hdl: \n"
        "   region hdl id\n"
        "*)Example:\n"
        "   e.g : ./rgn_dump 0\n"
        "**********************************************************\n"
        "\n");
}

SC_S32 main(int argc, char *argv[])
{
    printf("\nNOTICE: This tool only can be used for TESTING !!!\n");
    printf("\tTo see more usage, please enter: ./rgn_dump -h\n\n");
    if (argc > 1)
    {
        if (!strncmp(argv[1], "-h", 2))
        {
            usage();
            exit(SC_SUCCESS);
        }
    }

    if (argc < 2)
    {
        usage();
        exit(SC_SUCCESS);
    }

    g_hgl = atoi(argv[1]);
    if (!VALUE_BETWEEN(g_hgl, 0, RGN_HANDLE_MAX - 1))
    {
        printf("hdl id must be [0,%d]!!!!\n\n", RGN_HANDLE_MAX - 1);
        return -1;
    }

    signal(SIGINT, VI_Chn_Dump_HandleSig);
    signal(SIGTERM, VI_Chn_Dump_HandleSig);

    SAMPLE_RgnDump(g_hgl);

    return SC_SUCCESS;
}
