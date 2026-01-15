/**
 * @file     mpi_sys.h
 * @brief    sys模块定义
 * @version  1.0.0
 * @since    1.0.0
 * @author  刘彬<liubin@sgitg.sgcc.com.cn>
 * @date    2021-07-15 创建文件
 */
/************************************************************
 *@note
    Copyright 2021, BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
               ALL RIGHTS RESERVED
    Permission is hereby granted to licensees of  BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. to use
    or abstract this computer program for the sole purpose of implementing a product based on BEIJIING SMARTCHIP
    MICROELECTRONICS TECHNOLOGY CO., LTD. No other rights to reproduce, use, or disseminate this computer program,
    whether in part or in whole, are granted. BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. makes no
    representation or warranties with respect to the performance of this computer program, and specifically disclaims
    any responsibility for any damages, special or consequential, connected with the use of this program.
　　For details, see http://www.sgitg.sgcc.com.cn/
**********************************************************/

#ifndef __MPI_SYS_H__
#define __MPI_SYS_H__

#include "sc_type.h"
#include "sc_debug.h"
#include "sc_common.h"
#include "sc_comm_sys.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

//Used for init.
SC_S32 SC_MPI_SYS_Init(SC_VOID);
SC_S32 SC_MPI_SYS_Exit(SC_VOID);
SC_S32 SC_MPI_SYS_SetConfig(const MPP_SYS_CONFIG_S *pstSysConfig);
SC_S32 SC_MPI_SYS_GetConfig(MPP_SYS_CONFIG_S *pstSysConfig);

//Used for bind
SC_S32 SC_MPI_SYS_Bind(const MPP_CHN_S *pstSrcChn, const MPP_CHN_S *pstDestChn);
SC_S32 SC_MPI_SYS_UnBind(const MPP_CHN_S *pstSrcChn, const MPP_CHN_S *pstDestChn);
SC_S32 SC_MPI_SYS_GetBindbyDest(const MPP_CHN_S *pstDestChn, MPP_CHN_S *pstSrcChn);
SC_S32 SC_MPI_SYS_GetBindbySrc(const MPP_CHN_S *pstSrcChn, MPP_BIND_DEST_S *pstBindDest);

//Used for get soc info
SC_S32 SC_MPI_SYS_GetVersion(MPP_VERSION_S *pstVersion);
SC_S32 SC_MPI_SYS_GetChipId(SC_U32 *pu32ChipId);
SC_S32 SC_MPI_SYS_GetCustomCode(SC_U64 *pu64CustomCode);

//Used for pts
SC_S32 SC_MPI_SYS_GetCurPTS(SC_U64 *pu64CurPTS);
SC_S32 SC_MPI_SYS_InitPTSBase(SC_U64 u64PTSBase);
SC_S32 SC_MPI_SYS_SyncPTS(SC_U64 u64PTSBase);
SC_S32 SC_MPI_SYS_SetTimeZone(SC_S32 s32TimeZone);
SC_S32 SC_MPI_SYS_GetTimeZone(SC_S32 *ps32TimeZone);

//Used for mmz
SC_S32 SC_MPI_SYS_MmzAlloc(SC_U64 *pu64PhyAddr, SC_VOID **ppVirAddr,
    const SC_CHAR *strMmb, const SC_CHAR *strZone, SC_U32 u32Len);
SC_S32 SC_MPI_SYS_MmzAlloc_Align(SC_U64 *pu64_phy_addr, SC_VOID **p_vir_addr,
    const SC_CHAR *pstr_mmb, const SC_CHAR *pstr_zone, SC_U32 u32_len, SC_U32 u32_align);

SC_S32 SC_MPI_SYS_MmzAlloc_Cached(SC_U64 *pu64PhyAddr, SC_VOID **ppVirAddr,
    const SC_CHAR *pstrMmb, const SC_CHAR *pstrZone, SC_U32 u32Len);
SC_S32 SC_MPI_SYS_MmzAlloc_Cached_Align(SC_U64 *pu64_phy_addr, SC_VOID **p_vir_addr,
    const SC_CHAR *pstr_mmb, const SC_CHAR *pstr_zone, SC_U32 u32_len, SC_U32 u32_align);

SC_S32 SC_MPI_SYS_MmzFlushCache(SC_U64 u64PhyAddr, SC_VOID *pVirAddr, SC_U32 u32Size);
SC_S32 SC_MPI_SYS_MmzInvalidCache(SC_U64 u64_phy_addr, SC_VOID *p_vir_addr, SC_U32 u32_size);
SC_VOID *SC_MPI_SYS_Mmap(SC_U64 u64PhyAddr, SC_U32 u32Size);
SC_VOID *SC_MPI_SYS_MmapCache(SC_U64 u64PhyAddr, SC_U32 u32Size);
SC_S32 SC_MPI_SYS_Munmap(SC_VOID *pVirAddr, SC_U32 u32Size);
SC_S32 SC_MPI_SYS_MflushCache(SC_U64 u64PhyAddr, SC_VOID *pVirAddr, SC_U32 u32Size);
SC_S32 SC_MPI_SYS_MmzFree(SC_U64 u64PhyAddr, SC_VOID *pVirAddr);

//Not support now.
SC_S32 SC_MPI_SYS_SetMemConfig(const MPP_CHN_S *pstMppChn, const SC_CHAR *pcMmzName);
SC_S32 SC_MPI_SYS_GetMemConfig(const MPP_CHN_S *pstMppChn, SC_CHAR *pcMmzName);
SC_S32 SC_MPI_SYS_CloseFd(SC_VOID);
SC_S32 SC_MPI_SYS_GetVirMemInfo(const void *pVirAddr, SYS_VIRMEM_INFO_S *pstMemInfo);

/* Set/get Scale coefficient level for VPSS/VGS */
SC_S32 SC_MPI_SYS_SetScaleCoefLevel(const SCALE_RANGE_S *pstScaleRange,
    const SCALE_COEFF_LEVEL_S *pstScaleCoeffLevel);
SC_S32 SC_MPI_SYS_GetScaleCoefLevel(const SCALE_RANGE_S *pstScaleRange, SCALE_COEFF_LEVEL_S *pstScaleCoeffLevel);

SC_S32 SC_MPI_SYS_SetVIVPSSMode(const VI_VPSS_MODE_S *pstVIVPSSMode);
SC_S32 SC_MPI_SYS_GetVIVPSSMode(VI_VPSS_MODE_S *pstVIVPSSMode);
SC_S32 SC_MPI_SYS_GetVPSSVENCWrapBufferLine(VPSS_VENC_WRAP_PARAM_S *pWrapParam, SC_U32 *pu32BufLine);

SC_S32 SC_MPI_SYS_SetRawFrameCompressParam(const RAW_FRAME_COMPRESS_PARAM_S *pstCompressParam);
SC_S32 SC_MPI_SYS_GetRawFrameCompressParam(RAW_FRAME_COMPRESS_PARAM_S *pstCompressParam);



/* EPRI Test Interface */
#define     MPI_SYS_POST_DATA_TYPE_CJSON            0x49
#define     MPI_SYS_POST_DATA_TYPE_BIN              0xE2

typedef SC_S32 (*SC_MPI_SYS_HttpSVR_ProcJson_Cb)(SC_VOID *p_cJson);
SC_S32 SC_MPI_SYS_HttpServer_RegProc(SC_CHAR *p_json_cmd_str, SC_MPI_SYS_HttpSVR_ProcJson_Cb p_json_proc);
SC_S32 SC_MPI_SYS_HttpServer_Init(SC_S32 http_server_port, SC_CHAR *p_store_base_path);
SC_S32 SC_MPI_SYS_HttpServer_SetDataPath(SC_CHAR *p_base_path);
const SC_CHAR* SC_MPI_SYS_HttpServer_GetDataPath(SC_VOID);
SC_S32 SC_MPI_SYS_HttpClient_PostData(SC_CHAR *p_post_url, int data_type, int data_size/*valid when data type is bin*/, SC_VOID *p_cJson);

/* waiting for log module ready.
SC_S32 SC_MPI_LOG_SetLevelConf(LOG_LEVEL_CONF_S *pstConf);
SC_S32 SC_MPI_LOG_GetLevelConf(LOG_LEVEL_CONF_S *pstConf);
SC_S32 SC_MPI_LOG_SetWaitFlag(SC_BOOL bWait);
SC_S32 SC_MPI_LOG_Read(SC_CHAR *pBuf, SC_U32 u32Size);
SC_VOID SC_MPI_LOG_Close(SC_VOID);
*/

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __MPI_SYS_H__ */

