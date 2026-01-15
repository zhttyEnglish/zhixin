/*
 * @file     sc_comm_aenc.h
 * @brief    audio
 * @version  1.0.1
 * @since    1.0.0
 * @author
 */

/*
 ********************************************************************************************************
 * Copyright 2021, BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
 *             ALL RIGHTS RESERVED
 * Permission is hereby granted to licensees of BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
 * to use or abstract this computer program for the sole purpose of implementing a product based on
 * BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. No other rights to reproduce, use,
 * or disseminate this computer program,whether in part or in whole, are granted.
 * BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. makes no representation or warranties
 * with respect to the performance of this computer program, and specifically disclaims
 * any responsibility for any damages, special or consequential, connected with the use of this program.
 * For details, see http://www.sgitg.sgcc.com.cn/
 ********************************************************************************************************
 */

#ifndef  __SC_COMM_AENC_H__
#define  __SC_COMM_AENC_H__

#include "sc_type.h"
#include "sc_common.h"
#include "sc_comm_aio.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct scAENC_ATTR_G711_S
{
    SC_U32 resv;            /*reserve item*/
} AENC_ATTR_G711_S;

typedef struct scAENC_ATTR_G726_S
{
    G726_BPS_E enG726bps;
} AENC_ATTR_G726_S;

typedef struct scAENC_ATTR_ADPCM_S
{
    ADPCM_TYPE_E enADPCMType;
} AENC_ATTR_ADPCM_S;

typedef struct scAENC_ATTR_LPCM_S
{
    SC_U32 resv;            /* reserve item */
} AENC_ATTR_LPCM_S;

typedef struct scAENC_ENCODER_S
{
    PAYLOAD_TYPE_E  enType;
    SC_U32          u32MaxFrmLen;
    SC_CHAR         aszName[17];    /* encoder type,be used to print proc information */
    SC_S32          (*pfnOpenEncoder)(SC_VOID *pEncoderAttr,
        SC_VOID **ppEncoder); /* pEncoder is the handle to control the encoder */
    SC_S32          (*pfnEncodeFrm)(SC_VOID *pEncoder, const AUDIO_FRAME_S *pstData,
        SC_U8 *pu8Outbuf, SC_U32 *pu32OutLen);
    SC_S32          (*pfnCloseEncoder)(SC_VOID *pEncoder);
} AENC_ENCODER_S;

typedef struct scAENC_CHN_ATTR_S
{
    PAYLOAD_TYPE_E      enType;
    SC_U32              u32PtNumPerFrm;
    SC_U32              u32BufSize;      /* buf size [2~MAX_AUDIO_FRAME_NUM] */
    SC_VOID ATTRIBUTE   *pValue;  /* point to attribute of definite audio encoder */
} AENC_CHN_ATTR_S;

typedef enum scEN_AENC_ERR_CODE_E
{
    AENC_ERR_ENCODER_ERR     = 64,
    AENC_ERR_VQE_ERR         = 65,
} EN_AENC_ERR_CODE_E;

/*samples per frame for AACLC and aacPlus */
#define AACLD_SAMPLES_PER_FRAME         512
#define AACLC_SAMPLES_PER_FRAME         1024
#define AACPLUS_SAMPLES_PER_FRAME       2048

/*max length of AAC stream by bytes */
#define MAX_AAC_MAINBUF_SIZE    768*2

typedef enum
{
    AAC_TYPE_AACLC      = 0,            /* AAC LC */
    AAC_TYPE_EAAC       = 1,            /* eAAC  (HEAAC or AAC+  or aacPlusV1) */
    AAC_TYPE_EAACPLUS   = 2,            /* eAAC+ (AAC++ or aacPlusV2) */
    AAC_TYPE_AACLD      = 3,
    AAC_TYPE_AACELD     = 4,
    AAC_TYPE_BUTT,
} AAC_TYPE_E;

typedef enum
{
    AAC_BPS_8K      = 8000,
    AAC_BPS_16K     = 16000,
    AAC_BPS_22K     = 22000,
    AAC_BPS_24K     = 24000,
    AAC_BPS_32K     = 32000,
    AAC_BPS_48K     = 48000,
    AAC_BPS_64K     = 64000,
    AAC_BPS_96K     = 96000,
    AAC_BPS_128K    = 128000,
    AAC_BPS_256K    = 256000,
    AAC_BPS_320K    = 320000,
    AAC_BPS_BUTT
} AAC_BPS_E;

typedef enum
{
    AAC_TRANS_TYPE_ADTS = 0,
    AAC_TRANS_TYPE_LOAS = 1,
    AAC_TRANS_TYPE_LATM_MCP1 = 2,
    AAC_TRANS_TYPE_BUTT
} AAC_TRANS_TYPE_E;

typedef struct
{
    SC_S32 s32Samplerate;   /* sample rate*/
    SC_S32 s32BitRate;                  /* bitrate */
    SC_S32 s32Profile;                  /* profile*/
    SC_S32 s32TnsUsed;                  /* TNS Tools*/
    SC_S32 s32PnsUsed;                  /* PNS Tools*/
} AAC_FRAME_INFO_S;

/*
    AAC Commendatory Parameter:
    Sampling Rate(HZ)    LC BitRate(Kbit/s)    EAAC BitRate (Kbit/s)    EAAC+ BitRate (Kbit/s)
    48000                128                    48                        32,24
    44100                128                    48                        32,24
    32000                96                    22                        16
    24000                64
    22050                64
    16000                48
*/
typedef struct
{
    AAC_TYPE_E          enAACType;   /* AAC profile type */
    AAC_BPS_E           enBitRate;   /* AAC bitrate (LC:16~320, EAAC:24~128, EAAC+:16~64, AACLD:16~320, AACELD:32~320)*/
    AUDIO_SAMPLE_RATE_E enSmpRate;   /* AAC sample rate (LC:8~48, EAAC:16~48, EAAC+:16~48, AACLD:8~48, AACELD:8~48)*/
    AUDIO_BIT_WIDTH_E   enBitWidth;  /* AAC bit width (only support 16bit)*/
    AUDIO_SOUND_MODE_E  enSoundMode; /* sound mode of inferent audio frame */
    AAC_TRANS_TYPE_E    enTransType;

    SC_S16              s16BandWidth; /* targeted audio bandwidth in Hz (0 or 1000~enSmpRate/2), the default is 0*/

} AENC_ATTR_AAC_S;

typedef struct
{
    void       *pstAACState;
    AENC_ATTR_AAC_S     stAACAttr;
} AENC_AAC_ENCODER_S;

typedef struct
{
    AAC_TRANS_TYPE_E enTransType;

} ADEC_ATTR_AAC_S;

typedef struct
{
    void         *pstAACState;
    ADEC_ATTR_AAC_S     stAACAttr;
} ADEC_AAC_DECODER_S;

/* invlalid device ID */
#define SC_ERR_AENC_INVALID_DEVID     SC_DEF_ERR(SC_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
/* invlalid channel ID */
#define SC_ERR_AENC_INVALID_CHNID     SC_DEF_ERR(SC_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/* at lease one parameter is illagal ,eg, an illegal enumeration value  */
#define SC_ERR_AENC_ILLEGAL_PARAM     SC_DEF_ERR(SC_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/* channel exists */
#define SC_ERR_AENC_EXIST             SC_DEF_ERR(SC_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
/* channel unexists */
#define SC_ERR_AENC_UNEXIST           SC_DEF_ERR(SC_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/* using a NULL point */
#define SC_ERR_AENC_NULL_PTR          SC_DEF_ERR(SC_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/* try to enable or initialize system,device or channel, before configing attribute */
#define SC_ERR_AENC_NOT_CONFIG        SC_DEF_ERR(SC_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/* operation is not supported by NOW */
#define SC_ERR_AENC_NOT_SUPPORT       SC_DEF_ERR(SC_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/* operation is not permitted ,eg, try to change static attribute */
#define SC_ERR_AENC_NOT_PERM          SC_DEF_ERR(SC_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/* failure caused by malloc memory */
#define SC_ERR_AENC_NOMEM             SC_DEF_ERR(SC_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/* failure caused by malloc buffer */
#define SC_ERR_AENC_NOBUF             SC_DEF_ERR(SC_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/* no data in buffer */
#define SC_ERR_AENC_BUF_EMPTY         SC_DEF_ERR(SC_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/* no buffer for new data */
#define SC_ERR_AENC_BUF_FULL          SC_DEF_ERR(SC_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/* system is not ready,had not initialed or loaded */
#define SC_ERR_AENC_SYS_NOTREADY      SC_DEF_ERR(SC_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
/* encoder internal err */
#define SC_ERR_AENC_ENCODER_ERR       SC_DEF_ERR(SC_ID_AENC, EN_ERR_LEVEL_ERROR, AENC_ERR_ENCODER_ERR)
/* vqe internal err */
#define SC_ERR_AENC_VQE_ERR       SC_DEF_ERR(SC_ID_AENC, EN_ERR_LEVEL_ERROR, AENC_ERR_VQE_ERR)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif/* End of #ifndef __SC_COMM_AENC_H__ */

