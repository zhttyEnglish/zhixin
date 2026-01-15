
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

#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#include "libavcodec/codec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "sc_comm_vb.h"


#include "mpi_sys.h"
#include "audio_alarm_intf.h"
#include "audio_alarm_json_inf.h"

#include "stdio.h"
#include "stdlib.h"
#include "mpi_audio.h"
#include "epri_test_gpio.h"

/////////////////////////////////////////////////////////////
static char g_base_path[256] = {0};
static int audio_duration = 0;
static int light_duration = 0;
static char isAudio_start = 0;
static char isLight_start = 0;


typedef struct
{
    HalGpioProp ledctl;
    unsigned char blkEn;
    unsigned char blkDly100ms; /* blink delay 100ms, 闪烁间隔 单位100ms */
    unsigned char blkDlyCnt;   /* blink delay cnt, 记数, 为0时改变 */
    unsigned char res[1];
}LedDevices;




// WAVE file header format
typedef struct {
    unsigned char   chunk_id[4];        // RIFF string
    unsigned int    chunk_size;         // overall size of file in bytes (36 + data_size)
    unsigned char   sub_chunk1_id[8];   // WAVEfmt string with trailing null char
    unsigned int    sub_chunk1_size;    // 16 for PCM.  This is the size of the rest of the Subchunk which follows this number.
    unsigned short  audio_format;       // format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
    unsigned short  num_channels;       // Mono = 1, Stereo = 2
    unsigned int    sample_rate;        // 8000, 16000, 44100, etc. (blocks per second)
    unsigned int    byte_rate;          // SampleRate * NumChannels * BitsPerSample/8
    unsigned short  block_align;        // NumChannels * BitsPerSample/8
    unsigned short  bits_per_sample;    // bits per sample, 8- 8bits, 16- 16 bits etc
    unsigned char   sub_chunk2_id[4];   // Contains the letters "data"
    unsigned int    sub_chunk2_size;    // NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
} WAV_HEADER;

#define     AUDIO_BYTES_PER_FRAME       320
#define     WAV_HEADER_SIZE             44

typedef struct _TMP_VB_BUF
{
    SC_U32              pool_id;
    //VB_BLK              vb_block;
    //SC_U64  ATTRIBUTE   u64PhyAddr;
    //SC_U8   ATTRIBUTE   *p_buf;

}TMP_VB_BUF;

TMP_VB_BUF g_vb_info = {0};



SC_S32 sca200_sys_init(int vb_block_size, int vb_count, TMP_VB_BUF *p_vb_info)
{
    SC_S32 s32Ret = SC_FAILURE;

    SC_MPI_SYS_Exit();
    SC_MPI_VB_Exit();

    if(NULL == p_vb_info)
    {
        printf("[ERROR] Input args is null!\n");
        return SC_FAILURE;
    }

    VB_POOL_CONFIG_S stVbPoolCfg;
    memset(&stVbPoolCfg, 0, sizeof(VB_POOL_CONFIG_S));
    stVbPoolCfg.u64BlkSize = vb_block_size;
    stVbPoolCfg.u32BlkCnt = vb_count;
    stVbPoolCfg.enRemapMode = VB_REMAP_MODE_NONE;

    p_vb_info->pool_id = SC_MPI_VB_CreatePool(&stVbPoolCfg);
    if (p_vb_info->pool_id == VB_INVALID_POOLID)
    {
        printf("Maybe you not call sys init\n");
        return NULL;
    }

    s32Ret = SC_MPI_SYS_Init();

    if (SC_SUCCESS != s32Ret)
    {
        printf("SC_MPI_SYS_Init failed!\n");
        SC_MPI_VB_Exit();
        return SC_FAILURE;
    }

    return SC_SUCCESS;
}

static SC_S32 sca200_ao_start(SC_S32 AoDev, AIO_ATTR_S *pstAioAttr)
{
    SC_S32 ret = SC_SUCCESS;
    SC_S32 i;
    SC_U32 s32AoChnCnt;
    ret = SC_MPI_AO_SetPubAttr(AoDev, pstAioAttr);
    if (SC_SUCCESS != ret)
    {
        printf("SC_MPI_AO_SetPubAttr failed! %d\n", ret);
        return ret;
    }
    ret = SC_MPI_AO_Enable(AoDev);
    if (SC_SUCCESS != ret)
    {
        printf("SC_MPI_AO_Enable failed! %d\n", ret);
        return ret;
    }
    s32AoChnCnt = pstAioAttr->u32ChnCnt;
    for (i = 0; i < s32AoChnCnt >> pstAioAttr->enSoundmode; i++)
    {
        ret = SC_MPI_AO_EnableChn(AoDev, i);
        if (SC_SUCCESS != ret)
        {
            printf("%s: SC_MPI_AO_EnableChn(%d,%d) failed with %#x\n", __func__, AoDev, i, ret);
            return ret;
        }
    }
    return ret;
}

void sca200_play_proc(char *p_data, int data_size, int chan_num, TMP_VB_BUF *p_vb_buf)
{
    SC_S32 s32Ret;
    VB_BLK  vb_blk;
    AUDIO_FRAME_S stFrame;
    char *p_cur_data = NULL;

    //FILE *pOutFile = fopen("./out.pcm", "wb");


    if(NULL == p_data || 0 == data_size || 0 == chan_num || NULL == p_vb_buf)
    {
        printf("[ERROR] Func:%s, input args is error!\n", __FUNCTION__);
        return;
    }

    SC_U64 u64BlkSize = AUDIO_BYTES_PER_FRAME*4;

    vb_blk = SC_MPI_VB_GetBlock(p_vb_buf->pool_id, u64BlkSize, SC_NULL);
    if (VB_INVALID_HANDLE == vb_blk)
    {
        printf("[ERROR] Func:%s, SC_MPI_VB_GetBlock err! size:%d\n", __FUNCTION__, AUDIO_BYTES_PER_FRAME*4);
        return;
    }

    stFrame.u64PhyAddr[0] = SC_MPI_VB_Handle2PhysAddr(vb_blk);
    if (0 == stFrame.u64PhyAddr[0])
    {
        printf("[ERROR] Func:%s, SC_MPI_VB_Handle2PhysAddr err!\n", __FUNCTION__);
        return;
    }
    stFrame.u64PhyAddr[1] = stFrame.u64PhyAddr[0] + AUDIO_BYTES_PER_FRAME*2;

    stFrame.u64VirAddr[0] = (SC_U8 *)SC_MPI_SYS_Mmap(stFrame.u64PhyAddr[0], AUDIO_BYTES_PER_FRAME*4);
    stFrame.u64VirAddr[1] = stFrame.u64VirAddr[0] + AUDIO_BYTES_PER_FRAME*2;

    stFrame.enBitwidth = AUDIO_BIT_WIDTH_16;
    stFrame.enSoundmode = AUDIO_SOUND_MODE_STEREO;
    stFrame.u32Len = AUDIO_BYTES_PER_FRAME*2;
    stFrame.u32PoolId[0] = p_vb_buf->pool_id;
    stFrame.u32PoolId[1] = p_vb_buf->pool_id;

    p_cur_data = p_data;
    //由于hal层重采样的原因前几帧播不出来，填上空帧使开头的声音能够正常
    s32Ret = SC_MPI_AO_SendFrame(0, 0, &stFrame, 1000);
    s32Ret = SC_MPI_AO_SendFrame(0, 0, &stFrame, 1000);s32Ret = SC_MPI_AO_SendFrame(0, 0, &stFrame, 1000);
    s32Ret = SC_MPI_AO_SendFrame(0, 0, &stFrame, 1000);s32Ret = SC_MPI_AO_SendFrame(0, 0, &stFrame, 1000);
    while (data_size > 0)
    {
        s32Ret = 0;

        /* get frame from buffer */
        if(data_size >= AUDIO_BYTES_PER_FRAME*2*chan_num)
        {
            for(int i = 0; i < AUDIO_BYTES_PER_FRAME * 2; i = i + 2)
            {
                stFrame.u64VirAddr[0][i] = *p_cur_data++;
                stFrame.u64VirAddr[0][i + 1] = *p_cur_data++;

                if(2 == chan_num)
                {
                    stFrame.u64VirAddr[1][i] = *p_cur_data++;
                    stFrame.u64VirAddr[1][i + 1] = *p_cur_data++;
                }
            }

            data_size = data_size - AUDIO_BYTES_PER_FRAME*2*chan_num;
        }
        else
        {
            memset(stFrame.u64VirAddr[0], 0, AUDIO_BYTES_PER_FRAME * 2);
            memset(stFrame.u64VirAddr[1], 0, AUDIO_BYTES_PER_FRAME * 2);

            for(int i = 0; i < (data_size/chan_num-1); i = i + 2)
            {
                stFrame.u64VirAddr[0][i] = *p_cur_data++;
                stFrame.u64VirAddr[0][i + 1] = *p_cur_data++;

                if(2 == chan_num)
                {
                    stFrame.u64VirAddr[1][i] = *p_cur_data++;
                    stFrame.u64VirAddr[1][i + 1] = *p_cur_data++;
                }
            }

            data_size = 0;
        }

        memcpy(stFrame.u64VirAddr[1],stFrame.u64VirAddr[0],stFrame.u32Len);

        //fwrite(stFrame.u64VirAddr[0], 1, AUDIO_BYTES_PER_FRAME * 2, pOutFile);
        s32Ret = SC_MPI_AO_SendFrame(0, 0, &stFrame, 1000);
        if( SC_SUCCESS != s32Ret)
        {
            printf("[ERROR] Func:%s, SC_MPI_AO_SendFrame err!\n", __FUNCTION__);
        }

        if(isAudio_start==0x55)
        {
            break;
        }
    }
    s32Ret = SC_MPI_SYS_Munmap(stFrame.u64VirAddr[0], AUDIO_BYTES_PER_FRAME*4);
    if (SC_SUCCESS != s32Ret)
    {
        printf("[ERROR] Func:%s, SC_MPI_SYS_Munmap err!\n", __FUNCTION__);
        return;
    }
    s32Ret = SC_MPI_VB_ReleaseBlock(vb_blk);
    if (SC_SUCCESS != s32Ret)
    {
        printf("[ERROR] Func:%s, SC_MPI_VB_ReleaseBlock err!\n", __FUNCTION__);
        return;
    }
    //sleep(1);

    //fflush(pOutFile);
    //fclose(pOutFile);

    return;
}

SC_S32  sca200_ao_stop(SC_S32 AoDevId, SC_S32 s32AoChnCnt)
{
    SC_S32 i;
    SC_S32 ret;

    for (i = 0; i < s32AoChnCnt; i++)
    {
        ret = SC_MPI_AO_DisableChn(AoDevId, i);
        if (SC_SUCCESS != ret)
        {
            printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
            return ret;
        }
    }

    ret = SC_MPI_AO_Disable(AoDevId);
    if (SC_SUCCESS != ret)
    {
        printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
        return ret;
    }

    //printf("[Debug] Func:%s, Line:%d \n", __FUNCTION__, __LINE__);

    if(VB_INVALID_POOLID != g_vb_info.pool_id)
    {
        ret = SC_MPI_VB_DestroyPool(g_vb_info.pool_id);
        if(ret)
        {
            printf("[Func]:%s [Line]:%d SC_MPI_VB_DestroyPool fail[ret:%d]!\n", __FUNCTION__, __LINE__, ret);
            return ret;
        }
    }
    return SC_SUCCESS;
}


SC_S32 sca200_ao_open(int volume, int sample_rate)
{
    SC_S32 ret = -1;
    SC_S32      AoChn = 0;

    ret = sca200_sys_init(AUDIO_BYTES_PER_FRAME*4, 2, &g_vb_info);
    if (SC_SUCCESS != ret)
    {
        printf("%s: system init failed with %d!\n", __FUNCTION__, ret);
        return SC_FAILURE;
    }

    AIO_ATTR_S stAioAttr;
    stAioAttr.enSamplerate   = sample_rate;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_STEREO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 8;
    stAioAttr.u32PtNumPerFrm = AUDIO_BYTES_PER_FRAME;
    stAioAttr.u32ChnCnt      = 2;
    stAioAttr.u32ClkSel      = 0;
    stAioAttr.enI2sType      = AIO_I2STYPE_INNERCODEC;

    ret = sca200_ao_start(0, &stAioAttr);
    if(ret)
    {
        printf("[ERROR] Func:%s, ao open fail!\n", __FUNCTION__);
        return ret;
    }

    if(volume>=0&&volume<=100)
    {
        volume = -2 + volume/20;                
        ret = SC_MPI_AO_SetVolume(0, volume);//建议调节范围限制为[-2, 8]
        if(ret)
        {
            printf("[ERROR] Func:%s, ao SetVolume fail! volume%d\n", __FUNCTION__,volume);            
        }
    }   
    return ret;
}




static int playaudio_check_wav_header(WAV_HEADER *p_wav_header, int *p_data_size/* Output, pcm data size */, int *p_chan_num, int *p_sample_rate)
{
    if(NULL == p_wav_header || NULL == p_data_size || NULL == p_chan_num || NULL == p_sample_rate)
    {
        printf("[ERROR] Func:%s input args is null!\n", __FUNCTION__);
        return -1;
    }

    if(strncmp("RIFF", p_wav_header->chunk_id, 4))
    {
        printf("[ERROR] Func:%s chunk_id is not RIFF!\n", __FUNCTION__);
        return -1;
    }

    if(strncmp("WAVEfmt", p_wav_header->sub_chunk1_id, 7))
    {
        printf("[ERROR] Func:%s sub_chunk1_id[%s] is not WAVEfmt!\n", __FUNCTION__, p_wav_header->sub_chunk1_id);
        return -1;
    }

    if(16 != p_wav_header->bits_per_sample)
    {
        printf("[ERROR] Func:%s bits_per_sample[%d] is not support!\n", __FUNCTION__, p_wav_header->bits_per_sample);
        return -1;
    }

    if(1 != p_wav_header->audio_format)
    {
        printf("[ERROR] Func:%s audio_format[%d] is not support!\n", __FUNCTION__, p_wav_header->audio_format);
        return -1;
    }

    if((AUDIO_SAMPLE_RATE_8000 != p_wav_header->sample_rate)
    //    && (AUDIO_SAMPLE_RATE_12000 != p_wav_header->sample_rate)
    //    && (AUDIO_SAMPLE_RATE_11025 != p_wav_header->sample_rate)
    //    && (AUDIO_SAMPLE_RATE_16000 != p_wav_header->sample_rate)
    //    && (AUDIO_SAMPLE_RATE_22050 != p_wav_header->sample_rate)
    //    && (AUDIO_SAMPLE_RATE_24000 != p_wav_header->sample_rate)
    //    && (AUDIO_SAMPLE_RATE_32000 != p_wav_header->sample_rate)
    //    && (AUDIO_SAMPLE_RATE_44100 != p_wav_header->sample_rate)
    //    && (AUDIO_SAMPLE_RATE_48000 != p_wav_header->sample_rate)
    )
    {
        printf("[ERROR] Func:%s sample_rate[%d] is not support!\n", __FUNCTION__, p_wav_header->sample_rate);
        return -1;
    }

    *p_chan_num = p_wav_header->num_channels;
    *p_sample_rate = p_wav_header->sample_rate;
    //*p_bits_sample = p_wav_header->bits_per_sample;
    *p_data_size = p_wav_header->chunk_size - 36;


    //printf("[Debug Info] sample:%d, channel num:%d, data size:%d\n", p_wav_header->sample_rate, p_wav_header->num_channels, *p_data_size);
    return 0;

}


int play_audio(const char* p_wav_file, int volume)
{
    FILE *pFile = NULL;
    char header[WAV_HEADER_SIZE] = {0};
    int ret = -1, data_size = 0, chan_num = 0, sample_rate = 0;
    char *p_data = NULL;
    printf("***##@@@****%d %s  p_wav_file:%s\n",__LINE__,__FUNCTION__,p_wav_file);

    if(NULL == p_wav_file)
    {
        printf("[ERROR] Func:%s input file is null!\n", __FUNCTION__);
        return -1;
    }

    pFile = fopen(p_wav_file, "rb");
    if(NULL == pFile)
    {
        printf("[ERROR] Func:%s open file %s fail!\n", __FUNCTION__, p_wav_file);
        return -1;
    }

    ret = fread(header, 1, WAV_HEADER_SIZE, pFile);
    if(WAV_HEADER_SIZE != ret)
    {
        printf("[ERROR] Func:%s read wav header fail!\n", __FUNCTION__);
        goto exit_err;
    }

    ret = playaudio_check_wav_header(header, &data_size, &chan_num, &sample_rate);
    if(0 != ret)
    {
        printf("[ERROR] Func:%s check wav header fail!\n", __FUNCTION__);
        goto exit_err;
    }

    printf("[Debug Info] sample:%d, channel num:%d, data size:%d\n", sample_rate, chan_num, data_size);

    p_data = (char*)malloc(data_size);
    if(NULL == p_data)
    {
        printf("[ERROR] Func:%s malloc fail!\n", __FUNCTION__);
        goto exit_err;
    }

    ret = fread(p_data, 1, data_size, pFile);
    if(data_size != ret)
    {
        printf("[ERROR] Func:%s read wav data fail![read size:%d,ret:%d]\n", __FUNCTION__, data_size, ret);
        goto exit_err;
    }


    sca200_play_proc(p_data, data_size, chan_num,&g_vb_info);


exit_err:

    if(NULL != pFile)
    {
        fclose(pFile);
        pFile = NULL;
    }

    if(NULL != p_data)
    {
        free(p_data);
        p_data = NULL;
    }
    return 0;
}




void *audio_alarm_proc_thread(void *args)
{
    int res = 0;
    LedDevices leddevice = {0};
    halGPIOInit();
    leddevice.ledctl.port = 2;
    leddevice.ledctl.pinnum = 27;
    leddevice.ledctl.val = 0;
    while(1)
    {   
        if(isLight_start==0xaa)
        {
            if(leddevice.blkDlyCnt >= 1)
            {
                leddevice.blkDlyCnt = 0;
                leddevice.ledctl.val = leddevice.ledctl.val ? 0 : 1;
                halGPIOWrite(&leddevice.ledctl);
            }
            leddevice.blkDlyCnt++;
        }
        if(isLight_start==0x55)
        {
            leddevice.blkDlyCnt = 0;
            leddevice.ledctl.val =0;
            halGPIOWrite(&leddevice.ledctl);
        }
        if(isAudio_start==0xaa)
        {
            sca200_ao_open(3, 8000);
            
            play_audio("/mnt/a/7.wav",3);

        }
        if(isAudio_start==0x55)
        {
            sleep(1);
            sca200_ao_stop(0, 1);
            isAudio_start = 0;
        }
        sleep(1);
    }
}
void someone_func_handler(int param)
{
    isLight_start = 0x55;
    isAudio_start = 0x55;
}

static int create_audio_alarm_proc_thread(void)
{
    pthread_t thread_id;
    int res = 0;

    typedef void (*sighandler_t)(int);

    sighandler_t handler = someone_func_handler;
    signal(SIGALRM, handler);
   

    res = pthread_create(&thread_id, NULL, audio_alarm_proc_thread, NULL);
    if(res)
    {
        printf("[Error] Create audio_alarm_proc_thread failed! res = %d\n", res);
        return -1;
    }

    return 0;
}

int Audio_Alarm_DoTest_Task(char* p_data_full_path, Ng_audio_alarm_test *p_issue_info)
{
    //用户实际测试 函数
    printf("********** do audio_alarm test **********\n");
    int res =0;

    if(strcmp(p_issue_info->command,"start_audio_alarm")==0)
    {
        audio_duration = p_issue_info->duration;
        if(audio_duration>0)
        {
            alarm(audio_duration);
        }
        isAudio_start = 0xaa;
    }

    if(strcmp(p_issue_info->command,"stop_audio_alarm")==0)
    {        
        isAudio_start = 0x55;
    }

    if(strcmp(p_issue_info->command,"start_light_alarm")==0)
    {            
        light_duration = p_issue_info->duration;
        if(light_duration>0)
        {
            alarm(light_duration);
        }
       
        isLight_start = 0xaa;
        
    }

    if(strcmp(p_issue_info->command,"stop_light_alarm")==0)
    {   
        isLight_start = 0x55;
    }
    return res;
}


static int  audio_alarm_json_proc_cb(char *p_cJson)
{
    cJSON           *p_root_cJson = (cJSON*)p_cJson;
    cJSON           *p_response_cJson = NULL;
    Ng_test_res     ept_res = {0};
    Ng_audio_alarm_test *pEpt_test = NULL;
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

        printf("[Info] Get data base path is :%s\n", p_base_path);
    }

    //将json 字符串转换为 结构体
    pEpt_test = (Ng_audio_alarm_test*)json_to_struct_Ng_audio_alarm_test(p_root_cJson);

    if(NULL != pEpt_test)
    {
        //get resource path: pEpt_test->plaintxt_url
        char res_full_path[256];
        //sprintf(res_full_path, "%s/%s", g_base_path, pEpt_test->path);
       
        /*****************************************************************************/
        //调用用户 测试 主程序.
        //do somthing ...

        res = Audio_Alarm_DoTest_Task(res_full_path, pEpt_test);
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
        p_response_cJson = struct_to_json_Ng_audio_alarm_test_res(&ept_res);
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



int audio_alarm_http_intf_init()
{
    int res = 0;

    // protocol json command :
    res = SC_MPI_SYS_HttpServer_RegProc((char *)"start_audio_alarm", (SC_MPI_SYS_HttpSVR_ProcJson_Cb)audio_alarm_json_proc_cb);
    res |= SC_MPI_SYS_HttpServer_RegProc((char *)"stop_audio_alarm", (SC_MPI_SYS_HttpSVR_ProcJson_Cb)audio_alarm_json_proc_cb);
    res |= SC_MPI_SYS_HttpServer_RegProc((char *)"start_light_alarm", (SC_MPI_SYS_HttpSVR_ProcJson_Cb)audio_alarm_json_proc_cb);
    res |= SC_MPI_SYS_HttpServer_RegProc((char *)"stop_light_alarm", (SC_MPI_SYS_HttpSVR_ProcJson_Cb)audio_alarm_json_proc_cb);


    create_audio_alarm_proc_thread();


    return res;
}



