/**
 * @file     mpi_vo.h
 * @brief    视频输出模块的api接口
 * @version  1.0.0
 * @since    1.0.0
 * @author  张桂庆<zhangguiqing@sgitg.sgcc.com.cn>
 * @date    2021-07-15 创建文件
 */
/**
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

#ifndef __MPI_VO_H__
#define __MPI_VO_H__

#include "sc_comm_vo.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

//before SC_MPI_VO_Enable
/**
 * @brief      配置视频输出设备的公共属性
 * @param[in]  VoDev: 视频输出设备号
 *             pstPubAttr: 视频输出设备公共属性结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_SetPubAttr(VO_DEV VoDev, const VO_PUB_ATTR_S *pstPubAttr);

/**
 * @brief      获取视频输出设备的公共属性
 * @param[in]  VoDev: 视频输出设备号
 * @param[out] pstPubAttr: 视频输出设备公共属性结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_GetPubAttr(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr);

//after init valid
/**
 * @brief      启用视频输出设备
 * @param[in]  VoDev: 视频输出设备号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_Enable(VO_DEV VoDev);

/**
 * @brief      禁用视频输出设备
 * @param[in]  VoDev: 视频输出设备号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_Disable(VO_DEV VoDev);

/**
 * @brief      设置视频层属性
 * @param[in]  VoLayer: 视频输出视频层号
 *             pstLayerAttr: 视频层属性结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_SetVideoLayerAttr(VO_LAYER VoLayer, const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr);

/**
 * @brief      获取视频层属性
 * @param[in]  VoLayer: 视频输出视频层号
 * @param[out] pstLayerAttr: 视频层属性结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_GetVideoLayerAttr(VO_LAYER VoLayer, VO_VIDEO_LAYER_ATTR_S *pstLayerAttr);

/**
 * @brief      使能视频层
 * @param[in]  VoLayer: 视频输出视频层号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_EnableVideoLayer(VO_LAYER VoLayer);

/**
 * @brief      禁止视频层
 * @param[in]  VoLayer: 视频输出视频层号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_DisableVideoLayer(VO_LAYER VoLayer);

/**
 * @brief      设置视频层输出图像效果
 * @param[in]  VoLayer: 视频输出视频层号
 *             pstVideoCSC: 图像输出效果结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_SetVideoLayerCSC(VO_LAYER VoLayer, const VO_CSC_S *pstVideoCSC);

/**
 * @brief      获取设备输出图像效果
 * @param[in]  VoLayer: 视频输出视频层号
 * @param[out] pstVideoCSC: 图像输出效果结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_GetVideoLayerCSC(VO_LAYER VoLayer, VO_CSC_S *pstVideoCSC);

/**
 * @brief      获取输出屏幕图像数据
 * @param[in]  VoLayer: 视频输出视频层号
 *             s32MilliSec: 超时时间
 * @param[out] pstVFrame: 获取的输出屏幕图像数据信息结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_GetScreenFrame(VO_LAYER VoLayer, VIDEO_FRAME_INFO_S *pstVFrame, SC_S32 s32MilliSec);

/**
 * @brief      释放输出屏幕图像数据
 * @param[in]  VoLayer: 视频输出视频层号
 *             pstVFrame: 释放的输出屏幕图像数据信息结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_ReleaseScreenFrame(VO_LAYER VoLayer, const VIDEO_FRAME_INFO_S *pstVFrame);

//after dev enable and layer init
/**
 * @brief      配置指定视频输出通道的属性
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 *             pstChnAttr: 视频通道属性指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_SetChnAttr(VO_LAYER VoLayer, VO_CHN VoChn, const VO_CHN_ATTR_S *pstChnAttr);

/**
 * @brief      获取指定视频输出通道的属性
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 * @param[out] pstChnAttr: 视频通道属性指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_GetChnAttr(VO_LAYER VoLayer, VO_CHN VoChn, VO_CHN_ATTR_S *pstChnAttr);

//after layer enable and chn init
/**
 * @brief      启用指定的视频输出通道
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_EnableChn(VO_LAYER VoLayer, VO_CHN VoChn);

/**
 * @brief      禁用指定的视频输出通道
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_DisableChn(VO_LAYER VoLayer, VO_CHN VoChn);

//ater layer init and chn init
/**
 * @brief      配置指定视频输出通道的参数，用于幅型比功能
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 *             pstChnParam: 视频通道参数指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_SetChnParam(VO_LAYER VoLayer, VO_CHN VoChn, const VO_CHN_PARAM_S *pstChnParam);

/**
 * @brief      获取指定视频输出通道的参数
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 * @param[out] pstChnParam: 视频通道参数指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_GetChnParam(VO_LAYER VoLayer, VO_CHN VoChn, VO_CHN_PARAM_S *pstChnParam);

//after chn init
/**
 * @brief      设置指定视频输出通道的显示位置
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 *             pstDispPos: 通道显示位置指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_SetChnDisplayPosition(VO_LAYER VoLayer, VO_CHN VoChn, const POINT_S *pstDispPos);

/**
 * @brief      获取指定视频输出通道的显示位置
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 * @param[out] pstDispPos: 通道显示位置指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_GetChnDisplayPosition(VO_LAYER VoLayer, VO_CHN VoChn, POINT_S *pstDispPos);

//after chn enable
/**
 * @brief      获取输出通道图像数据
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 *             s32MilliSec: 超时时间
 * @param[out] pstFrame: 视频数据信息指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_GetChnFrame(VO_LAYER VoLayer, VO_CHN VoChn, VIDEO_FRAME_INFO_S *pstFrame, SC_S32 s32MilliSec);

/**
 * @brief      释放输出通道图像数据
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 *             pstFrame: 视频数据信息指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_ReleaseChnFrame(VO_LAYER VoLayer, VO_CHN VoChn, const VIDEO_FRAME_INFO_S *pstFrame);

//after chn enable or pause
/**
 * @brief      暂停指定的视频输出通道
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_PauseChn(VO_LAYER VoLayer, VO_CHN VoChn);

//after chn pause or enable
/**
 * @brief      恢复指定的视频输出通道
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_ResumeChn(VO_LAYER VoLayer, VO_CHN VoChn);

//after layer enable
/**
 * @brief      显示指定通道
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_ShowChn(VO_LAYER VoLayer, VO_CHN VoChn);

//after layer enable
/**
 * @brief      隐藏指定通道
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_HideChn(VO_LAYER VoLayer, VO_CHN VoChn);

//after chn enable
/**
 * @brief      将视频图像送入指定输出通道显示
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 *             s32MilliSec: 超时时间
 *             pstVFrame: 视频数据信息指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_SendFrame(VO_LAYER VoLayer, VO_CHN VoChn, VIDEO_FRAME_INFO_S *pstVFrame, SC_S32 s32MilliSec);

//no chn enable
/**
 * @brief      清空指定输出通道的缓存buffer数据
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 *             bClrAll: 是否将通道buffer中的数据全部清空
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_ClearChnBuf(VO_LAYER VoLayer, VO_CHN VoChn, SC_BOOL bClrAll);

/**
 * @brief      设置显示缓冲的长度
 * @param[in]  VoLayer: 视频输出视频层号
 *             u32BufLen: 显示缓冲的长度
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_SetDisplayBufLen(VO_LAYER VoLayer, SC_U32 u32BufLen);

/**
 * @brief      获取显示缓冲的长度
 * @param[in]  VoLayer: 视频输出视频层号
 * @param[out] pu32BufLen: 显示缓冲的长度指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_GetDisplayBufLen(VO_LAYER VoLayer, SC_U32 *pu32BufLen);

/**
 * @brief      设置设备用户时序下设备帧率
 * @param[in]  VoDev: 视频输出设备号
 *             u32FrameRate: 设备帧率
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_SetDevFrameRate(VO_DEV VoDev, SC_U32 u32FrameRate);

/**
 * @brief      获取设备用户时序下设备帧率
 * @param[in]  VoDev: 视频输出设备号
 * @param[out] pu32FrameRate: 设备帧率指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_GetDevFrameRate(VO_DEV VoDev, SC_U32 *pu32FrameRate);

/**
 * @brief      设置用户接口时序信息，用于配置时钟源、时钟大小和时钟分频比
 * @param[in]  VoDev: 视频输出设备号
 *             pstUserInfo: 用户时序信息指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetUserIntfSyncInfo(VO_DEV VoDev, VO_USER_INTFSYNC_INFO_S *pstUserInfo);

SC_S32 SC_MPI_VO_MipiTxSet(VO_DEV VoDev, SC_S32 Cmd, SC_PVOID pData, SC_S32 Size);

/**
 * @brief      VO模块资源退出
 * @param[in]  void
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VO_Exit(SC_VOID);

/**
 * @brief      暂时不支持函数列表
 * @note       no support func
 */

/**
 * @brief      关闭所有视频输出设备的Fd
 * @param[in]  void
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_CloseFd(SC_VOID);

/**
 * @brief      绑定视频层到某个设备
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoDev: 视频输出设备号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_BindVideoLayer(VO_LAYER VoLayer, VO_DEV VoDev);

/**
 * @brief      解绑定视频层到某个设备
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoDev: 视频输出设备号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_UnBindVideoLayer(VO_LAYER VoLayer, VO_DEV VoDev);

/**
 * @brief      设置视频层的优先级
 * @param[in]  VoLayer: 视频输出视频层号
 *             u32Priority: 视频层优先级设置等级
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetVideoLayerPriority(VO_LAYER VoLayer, SC_U32 u32Priority);

/**
 * @brief      获取视频层的优先级
 * @param[in]  VoLayer: 视频输出视频层号
 * @param[out] pu32Priority: 视频层优先级指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetVideoLayerPriority(VO_LAYER VoLayer, SC_U32 *pu32Priority);

/**
 * @brief      设置视频层的分割模式
 * @param[in]  VoLayer: 视频输出视频层号
 *             enPartMode: 视频层处理的分割模式
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetVideoLayerPartitionMode(VO_LAYER VoLayer, VO_PART_MODE_E enPartMode);

/**
 * @brief      获取视频层的分割模式
 * @param[in]  VoLayer: 视频输出视频层号
 * @param[out] penPartMode: 视频层分割模式指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetVideoLayerPartitionMode(VO_LAYER VoLayer, VO_PART_MODE_E *penPartMode);

/**
 * @brief      设置视频层上的通道的设置属性开始
 * @param[in]  VoLayer: 视频输出视频层号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_BatchBegin(VO_LAYER VoLayer);

/**
 * @brief      设置视频层上的通道的设置属性结束
 * @param[in]  VoLayer: 视频输出视频层号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_BatchEnd(VO_LAYER VoLayer);

/**
 * @brief      设置视频层内所有区域的边框的宽度和颜色
 * @param[in]  VoLayer: 视频输出视频层号
 *             pstLayerBoundary: 视频层区域边框结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetVideoLayerBoundary(VO_LAYER VoLayer, const VO_LAYER_BOUNDARY_S *pstLayerBoundary);

/**
 * @brief      获取视频层内所有区域的边框的宽度和颜色
 * @param[in]  VoLayer: 视频输出视频层号
 * @param[out] pstAttr: 视频层区域边框结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetVideoLayerBoundary(VO_LAYER VoLayer, VO_LAYER_BOUNDARY_S *pstLayerBoundary);

/**
 * @brief      配置指定视频层的参数，可用于幅型比功能
 * @param[in]  VoLayer: 视频输出视频层号
 *             pstLayerParam: 视频层参数指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetVideoLayerParam(VO_LAYER VoLayer, const VO_LAYER_PARAM_S *pstLayerParam);

/**
 * @brief      获取指定视频层的参数
 * @param[in]  VoLayer: 视频输出视频层号
 * @param[out] pstLayerParam: pstLayerParam
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetVideoLayerParam(VO_LAYER VoLayer, VO_LAYER_PARAM_S *pstLayerParam);

/**
 * @brief      设置视频层是否开启解压缩功能，仅MULTI模式下有效
 * @param[in]  VoLayer: 视频输出视频层号
 *             bSupportDecompress: 是否开启解压缩功能
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetVideoLayerDecompress(VO_LAYER VoLayer, SC_BOOL bSupportDecompress);

/**
 * @brief      获取视频层是否开启解压缩功能
 * @param[in]  VoLayer: 视频输出视频层号
 *             pbSupportDecompress: 是否开启解压缩功能指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetVideoLayerDecompress(VO_LAYER VoLayer, SC_BOOL *pbSupportDecompress);

/**
 * @brief      设置视频层CROP 功能属性
 * @param[in]  VoLayer: 视频输出视频层号
 *             pstCropInfo: 视频层CROP 属性结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetVideoLayerCrop(VO_LAYER VoLayer, const CROP_INFO_S *pstCropInfo);

/**
 * @brief      获取视频层CROP 功能属性
 * @param[in]  VoLayer: 视频输出视频层号
 * @param[out] pstCropInfo: 视频层CROP 属性结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetVideoLayerCrop(VO_LAYER VoLayer, CROP_INFO_S *pstCropInfo);

/**
 * @brief      设置播放容忍度
 * @param[in]  VoLayer: 视频输出视频层号
 *             u32Toleration: 播放容忍度
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetPlayToleration(VO_LAYER VoLayer, SC_U32 u32Toleration);

/**
 * @brief      获取播放容忍度
 * @param[in]  VoLayer: 视频输出视频层号
 *             pu32Toleration: 播放容忍度指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetPlayToleration(VO_LAYER VoLayer, SC_U32 *pu32Toleration);

/**
 * @brief      设置指定视频输出通道的显示帧率
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 *             s32ChnFrmRate: 视频通道显示帧率
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetChnFrameRate(VO_LAYER VoLayer, VO_CHN VoChn, SC_S32 s32ChnFrmRate);

/**
 * @brief      获取指定视频输出通道的显示帧率
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 * @param[out] ps32ChnFrmRate: 视频通道显示帧率指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetChnFrameRate(VO_LAYER VoLayer, VO_CHN VoChn, SC_S32 *ps32ChnFrmRate);

/**
 * @brief      单帧播放指定的视频输出通道
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_StepChn(VO_LAYER VoLayer, VO_CHN VoChn);

/**
 * @brief      刷新指定的视频输出通道
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */

SC_S32 SC_MPI_VO_RefreshChn(VO_LAYER VoLayer, VO_CHN VoChn);

/**
 * @brief      设置视频输出局部放大窗口
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 *             pstZoomAttr: 局部放大属性结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetZoomInWindow(VO_LAYER VoLayer, VO_CHN VoChn, const VO_ZOOM_ATTR_S *pstZoomAttr);

/**
 * @brief      获取视频输出局部放大窗口
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 * @param[out] pstZoomAttr: 局部放大属性结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetZoomInWindow(VO_LAYER VoLayer, VO_CHN VoChn, VO_ZOOM_ATTR_S *pstZoomAttr);

/**
 * @brief      获取指定视频输出通道正在显示图像的时间戳
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 * @param[out] pu64ChnPTS: 通道时间戳指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetChnPTS(VO_LAYER VoLayer, VO_CHN VoChn, SC_U64 *pu64ChnPTS);

/**
 * @brief      查询视频输出通道状态
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 * @param[out] pstStatus: 视频输出通道状态结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_QueryChnStatus(VO_LAYER VoLayer, VO_CHN VoChn, VO_QUERY_STATUS_S *pstStatus);

/**
 * @brief      设置单通道的边框属性
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 *             pstBorder: 视频通道边框属性指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetChnBorder(VO_LAYER VoLayer, VO_CHN VoChn, const VO_BORDER_S *pstBorder);

/**
 * @brief      获取单通道的边框属性
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 * @param[out] pstBorder: 视频通道边框属性指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetChnBorder(VO_LAYER VoLayer, VO_CHN VoChn, VO_BORDER_S *pstBorder);

/**
 * @brief      设置多通道的边框属性：边框宽度和颜色
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 *             pstChnBoundary: 视频通道边框属性指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetChnBoundary(VO_LAYER VoLayer, VO_CHN VoChn, const VO_CHN_BOUNDARY_S *pstChnBoundary);

/**
 * @brief      获取多通道的边框属性
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 * @param[out] pstChnBoundary: 视频通道边框属性指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetChnBoundary(VO_LAYER VoLayer, VO_CHN VoChn, VO_CHN_BOUNDARY_S *pstChnBoundary);

/**
 * @brief      设置视频输出通道的显示门限值
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 *             u32Threshold: 视频输出通道的显示门限值
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetChnRecvThreshold(VO_LAYER VoLayer, VO_CHN VoChn, SC_U32 u32Threshold);

/**
 * @brief      获取视频输出通道的显示门限值
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 * @param[out] u32Threshold: 视频输出通道的显示门限值指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetChnRecvThreshold(VO_LAYER VoLayer, VO_CHN VoChn, SC_U32 *pu32Threshold);

/**
 * @brief      设置指定视频输出通道的旋转角度
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 *             enRotation: 旋转角度枚举值
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetChnRotation(VO_LAYER VoLayer, VO_CHN VoChn, ROTATION_E enRotation);

/**
 * @brief      获取指定视频输出通道的旋转角度
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 * @param[out] penRotation: 旋转角度枚举值指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetChnRotation(VO_LAYER VoLayer, VO_CHN VoChn, ROTATION_E *penRotation);

/**
 * @brief      获取指定图像区域的亮度总和
 * @param[in]  VoLayer: 视频输出视频层号
 *             VoChn: 视频输出通道号
 *             pstRegionInfo: 区域信息指针
 *             s32MilliSec: 超时时间
 * @param[out] VoLayer: 接收区域亮度和统计信息的内存指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetChnRegionLuma(VO_LAYER VoLayer, VO_CHN VoChn, VO_REGION_INFO_S *pstRegionInfo,
    SC_U64 *pu64LumaData, SC_S32 s32MilliSec);

/**
 * @brief      设置回写设备的回写源，可设置从设备回写还是从设备的视频层回写
 * @param[in]  VoWBC: 回写设备号
 *             pstWBCSource: 回写源结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetWBCSource(VO_WBC VoWBC, const VO_WBC_SOURCE_S *pstWBCSource);

/**
 * @brief      获取回写设备的回写源
 * @param[in]  VoWBC: 回写设备号
 * @param[out] pstWBCSource: 回写源结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetWBCSource(VO_WBC VoWBC, VO_WBC_SOURCE_S *pstWBCSources);

/**
 * @brief      设置回写设备属性
 * @param[in]  VoWBC: 回写设备号
 *             pstWBCAttr: 回写设备属性结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetWBCAttr(VO_WBC VoWBC, const VO_WBC_ATTR_S *pstWBCAttr);

/**
 * @brief      获取回写设备属性
 * @param[in]  VoWBC: 回写设备号
 * @param[out] pstWBCAttr: 回写设备属性结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetWBCAttr(VO_WBC VoWBC, VO_WBC_ATTR_S *pstWBCAttr);

/**
 * @brief      使能回写设备
 * @param[in]  VoWBC: 回写设备号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_EnableWBC(VO_WBC VoWBC);

/**
 * @brief      禁用回写设备
 * @param[in]  VoWBC: 回写设备号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_DisableWBC(VO_WBC VoWBC);

/**
 * @brief      设置回写设备的回写模式
 * @param[in]  VoWBC: 回写设备号
 *             enWBCMode: 回写模式枚举
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetWBCMode(VO_WBC VoWBC, VO_WBC_MODE_E enWBCMode);

/**
 * @brief      获取回写设备的回写模式
 * @param[in]  VoWBC: 回写设备号
 * @param[out] penWBCMode: 回写模式枚举指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetWBCMode(VO_WBC VoWBC, VO_WBC_MODE_E *penWBCMode);

/**
 * @brief      设置回写设备的回写深度
 * @param[in]  VoWBC: 回写设备号
 *             u32Depth: 回写设备回写深度
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetWBCDepth(VO_WBC VoWBC, SC_U32 u32Depth);

/**
 * @brief      获取回写设备的回写深度
 * @param[in]  VoWBC: 回写设备号
 * @param[out] pu32Depth: 回写设备回写深度指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetWBCDepth(VO_WBC VoWBC, SC_U32 *pu32Depth);

/**
 * @brief      获取回写设备的输出图像数据
 * @param[in]  VoWBC: 回写设备号
 *             s32MilliSec: 超时时间
 * @param[out] pstVFrame: 获取的回写输出图像数据信息结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetWBCFrame(VO_WBC VoWBC, VIDEO_FRAME_INFO_S *pstVFrame, SC_S32 s32MilliSec);

/**
 * @brief      释放回写设备的输出图像数据
 * @param[in]  VoWBC: 回写设备号
 *             pstVFrame: 释放的输出通道图像数据信息结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_ReleaseWBCFrame(VO_WBC VoWBC, const VIDEO_FRAME_INFO_S *pstVFrame);

/**
 * @brief      绑定图形层到某个设备
 * @param[in]  GraphicLayer: 图形层号
 *             VoDev: 视频输出设备号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_BindGraphicLayer(GRAPHIC_LAYER GraphicLayer, VO_DEV VoDev);

/**
 * @brief      解绑定图形层到某个设备
 * @param[in]  GraphicLayer: 图形层号
 *             VoDev: 视频输出设备号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_UnBindGraphicLayer(GRAPHIC_LAYER GraphicLayer, VO_DEV VoDev);

/**
 * @brief      设置图形层输出图像效果
 * @param[in]  GraphicLayer: 图形层号
 *             pstCSC: 图形层输出效果结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetGraphicLayerCSC(GRAPHIC_LAYER GraphicLayer, const VO_CSC_S *pstCSC);

/**
 * @brief      获取图形层输出图像效果
 * @param[in]  GraphicLayer: 图形层号
 * @param[out] pstCSC: 图形层输出效果结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetGraphicLayerCSC(GRAPHIC_LAYER GraphicLayer, VO_CSC_S *pstCSC);

/**
 * @brief      设置模块参数
 * @param[in]  pstModParam: 模块参数结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetModParam(const VO_MOD_PARAM_S *pstModParam);

/**
 * @brief      获取模块参数
 * @param[out] pstModParam: 模块参数结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetModParam(VO_MOD_PARAM_S *pstModParam);

/**
 * @brief      设置设备垂直时序中断门限，单位：行
 * @param[in]  VoDev: 视频输出设备号
 *             u32Vtth: 门限值
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetVtth(VO_DEV VoDev, SC_U32 u32Vtth);

/**
 * @brief      获取设备垂直时序中断门限值
 * @param[in]  VoDev: 视频输出设备号
 * @param[out] pu32Vtth: 门限值指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetVtth(VO_DEV VoDev, SC_U32 *pu32Vtth);

/**
 * @brief      设置设备垂直时序2中断门限
 * @param[in]  VoDev: 视频输出设备号
 *             u32Vtth: 门限值
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_SetVtth2(VO_DEV VoDev, SC_U32 u32Vtth);

/**
 * @brief      获取设备垂直时序2中断门限值
 * @param[in]  VoDev: 视频输出设备号
 * @param[out] pu32Vtth: 门限值指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VO_GetVtth2(VO_DEV VoDev, SC_U32 *pu32Vtth);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __MPI_VO_H__ */

