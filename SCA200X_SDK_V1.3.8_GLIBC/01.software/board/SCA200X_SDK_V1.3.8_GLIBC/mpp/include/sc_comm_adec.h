/*
 * @file     sc_comm_adec.h
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

#ifndef  __SC_COMM_ADEC_H__
#define  __SC_COMM_ADEC_H__

#include "sc_type.h"
#include "sc_common.h"
#include "sc_comm_aio.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct scADEC_ATTR_G711_S
{
    SC_U32 resv;            /*reserve item*/
} ADEC_ATTR_G711_S;

typedef struct scADEC_ATTR_G726_S
{
    G726_BPS_E enG726bps;
} ADEC_ATTR_G726_S;

typedef struct scADEC_ATTR_ADPCM_S
{
    ADPCM_TYPE_E enADPCMType;
} ADEC_ATTR_ADPCM_S;

typedef struct scADEC_ATTR_LPCM_S
{
    SC_U32 resv;
} ADEC_ATTR_LPCM_S;

typedef enum scADEC_MODE_E
{
    ADEC_MODE_PACK = 0, /* require input is valid dec pack(a
                           complete frame encode result),
                           e.g.the stream get from AENC is a
                           valid dec pack, the stream know actually
                           pack len from file is also a dec pack.
                           this mode is scgh-performative */
    ADEC_MODE_STREAM,   /* input is stream, low-performative,
                           if you couldn't find out whether a stream is
                           vaild dec pack,you could use
                           this mode */
    ADEC_MODE_BUTT
} ADEC_MODE_E;

typedef struct scADEC_CH_ATTR_S
{
    PAYLOAD_TYPE_E enType;
    SC_U32         u32BufSize;  /* buf size[2~MAX_AUDIO_FRAME_NUM] */
    ADEC_MODE_E    enMode;      /* decode mode */
    SC_VOID ATTRIBUTE      *pValue;
} ADEC_CHN_ATTR_S;

typedef struct scADEC_CHN_STATE_S
{
    SC_BOOL bEndOfStream;             /* EOS flag */
    SC_U32 u32BufferFrmNum;           /* total number of channel buffer */
    SC_U32 u32BufferFreeNum;          /* free number of channel buffer */
    SC_U32 u32BufferBusyNum;          /* busy number of channel buffer */
} ADEC_CHN_STATE_S;

typedef struct scADEC_DECODER_S
{
    PAYLOAD_TYPE_E  enType;
    SC_CHAR         aszName[17];
    SC_S32          (*pfnOpenDecoder)(SC_VOID *pDecoderAttr, SC_VOID **ppDecoder); /* struct ppDecoder is packed by user,
    user malloc and free memory for this struct */
    SC_S32          (*pfnDecodeFrm)(SC_VOID *pDecoder, SC_U8 **pu8Inbuf, SC_S32 *ps32LeftByte,
        SC_U16 *pu16Outbuf, SC_U32 *pu32OutLen, SC_U32 *pu32Chns);
    SC_S32          (*pfnGetFrmInfo)(SC_VOID *pDecoder, SC_VOID *pInfo);
    SC_S32          (*pfnCloseDecoder)(SC_VOID *pDecoder);
    SC_S32          (*pfnResetDecoder)(SC_VOID *pDecoder);
} ADEC_DECODER_S;

typedef enum scEN_ADEC_ERR_CODE_E
{
    ADEC_ERR_DECODER_ERR     = 64,
    ADEC_ERR_BUF_LACK,
} EN_ADEC_ERR_CODE_E;

/* invlalid device ID */
#define SC_ERR_ADEC_INVALID_DEVID     SC_DEF_ERR(SC_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
/* invlalid channel ID */
#define SC_ERR_ADEC_INVALID_CHNID     SC_DEF_ERR(SC_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/* at lease one parameter is illagal ,eg, an illegal enumeration value  */
#define SC_ERR_ADEC_ILLEGAL_PARAM     SC_DEF_ERR(SC_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/* channel exists */
#define SC_ERR_ADEC_EXIST             SC_DEF_ERR(SC_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
/* channel unexists */
#define SC_ERR_ADEC_UNEXIST           SC_DEF_ERR(SC_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/* using a NULL point */
#define SC_ERR_ADEC_NULL_PTR          SC_DEF_ERR(SC_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/* try to enable or initialize system,device or channel, before configing attribute */
#define SC_ERR_ADEC_NOT_CONFIG        SC_DEF_ERR(SC_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/* operation is not supported by NOW */
#define SC_ERR_ADEC_NOT_SUPPORT       SC_DEF_ERR(SC_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/* operation is not permitted ,eg, try to change stati attribute */
#define SC_ERR_ADEC_NOT_PERM          SC_DEF_ERR(SC_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/* failure caused by malloc memory */
#define SC_ERR_ADEC_NOMEM             SC_DEF_ERR(SC_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/* failure caused by malloc buffer */
#define SC_ERR_ADEC_NOBUF             SC_DEF_ERR(SC_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/* no data in buffer */
#define SC_ERR_ADEC_BUF_EMPTY         SC_DEF_ERR(SC_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/* no buffer for new data */
#define SC_ERR_ADEC_BUF_FULL          SC_DEF_ERR(SC_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/* system is not ready,had not initialed or loaded */
#define SC_ERR_ADEC_SYS_NOTREADY      SC_DEF_ERR(SC_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
/* decoder internal err */
#define SC_ERR_ADEC_DECODER_ERR       SC_DEF_ERR(SC_ID_ADEC, EN_ERR_LEVEL_ERROR, ADEC_ERR_DECODER_ERR)
/* input buffer not enough to decode one frame */
#define SC_ERR_ADEC_BUF_LACK          SC_DEF_ERR(SC_ID_ADEC, EN_ERR_LEVEL_ERROR, ADEC_ERR_BUF_LACK)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif/* End of #ifndef __SC_COMM_ADEC_H__*/

