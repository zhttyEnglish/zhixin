/**
 * @file     mpi_vdec.h
 * @brief    视频解码模块的api接口
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

#ifndef __MPI_VDEC_H__
#define __MPI_VDEC_H__

#include "sc_comm_vdec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

/**
 * @brief      创建视频解码通道
 * @param[in]  VdChn: 视频解码通道号
 *             pstAttr: 解码通道属性指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VDEC_CreateChn(VDEC_CHN VdChn, const VDEC_CHN_ATTR_S *pstAttr);

//after stop recv stream
/**
 * @brief      销毁视频解码通道
 * @param[in]  VdChn: 视频解码通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VDEC_DestroyChn(VDEC_CHN VdChn);

//after chn open or start
/**
 * @brief      获取视频解码通道属性
 * @param[in]  VdChn: 视频解码通道号
 * @param[out] pstAttr: 解码通道属性指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VDEC_GetChnAttr(VDEC_CHN VdChn, VDEC_CHN_ATTR_S *pstAttr);

//after chn open
/**
 * @brief      设置视频解码通道属性
 * @param[in]  VdChn: 视频解码通道号
 *             pstAttr: 解码通道属性指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VDEC_SetChnAttr(VDEC_CHN VdChn, const VDEC_CHN_ATTR_S *pstAttr);

//after chn open
/**
 * @brief      解码器开始接收用户发送的码流
 * @param[in]  VdChn: 视频解码通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VDEC_StartRecvStream(VDEC_CHN VdChn);

//after chn start
/**
 * @brief      解码器停止接收用户发送的码流
 * @param[in]  VdChn: 视频解码通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VDEC_StopRecvStream(VDEC_CHN VdChn);

//after chn start
/**
 * @brief      查询解码通道状态
 * @param[in]  VdChn: 视频解码通道号
 * @param[out] pstStatus: 视频解码通道状态结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VDEC_QueryStatus(VDEC_CHN VdChn, VDEC_CHN_STATUS_S *pstStatus);

//after chn open or start
/**
 * @brief      获取视频解码通道的设备文件句柄
 * @param[in]  VdChn: 视频解码通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VDEC_GetFd(VDEC_CHN VdChn);

/**
 * @brief      关闭视频解码的设备文件句柄
 * @param[in]  VdChn: 视频解码通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VDEC_CloseFd(VDEC_CHN VdChn);

//after chn open
/**
 * @brief      复位视频解码通道
 * @param[in]  VdChn: 视频解码通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VDEC_ResetChn(VDEC_CHN VdChn);

//after chn open
/**
 * @brief      设置解码通道高级参数
 * @param[in]  VdChn: 视频解码通道号
 *             pstParam: 解码通道高级参数指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VDEC_SetChnParam(VDEC_CHN VdChn, const VDEC_CHN_PARAM_S *pstParam);

//after chn open or start
/**
 * @brief      获取解码通道高级参数
 * @param[in]  VdChn: 视频解码通道号
 * @param[out] pstParam: 解码通道高级参数指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VDEC_GetChnParam(VDEC_CHN VdChn, VDEC_CHN_PARAM_S *pstParam);

/* s32MilliSec: -1 is block,0 is no block,other positive number is timeout */
//after chn open or start
/**
 * @brief      向视频解码通道发送码流数据
 * @param[in]  VdChn: 视频解码通道号
 *             pstStream: 解码码流数据指针
 * @param[out] s32MilliSec: 送码流方式标志
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VDEC_SendStream(VDEC_CHN VdChn, const VDEC_STREAM_S *pstStream, SC_S32 s32MilliSec);

//after chn open or start
/**
 * @brief      获取视频解码通道的解码图像
 * @param[in]  VdChn: 视频解码通道号
 *             s32MilliSec: 获取图像方式标志
 * @param[out] pstFrameInfo: 获取的解码图像信息
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VDEC_GetFrame(VDEC_CHN VdChn, VIDEO_FRAME_INFO_S *pstFrameInfo, SC_S32 s32MilliSec);

/**
 * @brief      释放视频解码通道的图像
 * @param[in]  VdChn: 视频解码通道号
 *             pstFrameInfo: 解码后的图像信息指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VDEC_ReleaseFrame(VDEC_CHN VdChn, const VIDEO_FRAME_INFO_S *pstFrameInfo);

//after chn open or start
/**
 * @brief      获取视频解码通道的用户数据
 * @param[in]  VdChn: 视频解码通道号
 *             s32MilliSec: 获取用户数据方式标志
 * @param[out] pstUserData: 获取的解码用户数据
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VDEC_GetUserData(VDEC_CHN VdChn, VDEC_USERDATA_S *pstUserData, SC_S32 s32MilliSec);

//after chn open or start
/**
 * @brief      释放用户数据
 * @param[in]  VdChn: 视频解码通道号
 *             pstUserData: 解码后的用户数据指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VDEC_ReleaseUserData(VDEC_CHN VdChn, const VDEC_USERDATA_S *pstUserData);

/**
 * @brief      暂时不支持函数列表
 * @note       no support func
 */

/**
 * @brief      设置协议相关的内存分配通道参数
 * @param[in]  VdChn: 视频解码通道号
 *             pstParam: 协议相关的内存分配参数指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VDEC_SetProtocolParam(VDEC_CHN VdChn, const VDEC_PRTCL_PARAM_S *pstParam);

/**
 * @brief      获取协议相关的内存分配通道参数
 * @param[in]  VdChn: 视频解码通道号
 * @param[out] pstParam: 协议相关的内存分配参数指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VDEC_GetProtocolParam(VDEC_CHN VdChn, VDEC_PRTCL_PARAM_S *pstParam);

/**
 * @brief      设置用户图片属性
 * @param[in]  VdChn: 视频解码通道号
 *             pstUsrPic: 用户图片属性结构指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VDEC_SetUserPic(VDEC_CHN VdChn, const VIDEO_FRAME_INFO_S *pstUsrPic);

/**
 * @brief      使能插入用户图片
 * @param[in]  VdChn: 视频解码通道号
 *             bInstant: 使能用户图片方式
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VDEC_EnableUserPic(VDEC_CHN VdChn, SC_BOOL bInstant);

/**
 * @brief      禁止使能插入用户图片
 * @param[in]  VdChn: 视频解码通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VDEC_DisableUserPic(VDEC_CHN VdChn);

/**
 * @brief      设置显示模式
 * @param[in]  VdChn: 视频解码通道号
 *             enDisplayMode: 显示模式枚举
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VDEC_SetDisplayMode(VDEC_CHN VdChn, VIDEO_DISPLAY_MODE_E enDisplayMode);

/**
 * @brief      获取显示模式
 * @param[in]  VdChn: 视频解码通道号
 * @param[out] penDisplayMode: 显示模式枚举指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VDEC_GetDisplayMode(VDEC_CHN VdChn, VIDEO_DISPLAY_MODE_E *penDisplayMode);

/**
 * @brief      设置解码图像旋转角度
 * @param[in]  VdChn: 视频解码通道号
 *             enRotation: 旋转角度枚举
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VDEC_SetRotation(VDEC_CHN VdChn, ROTATION_E enRotation);

/**
 * @brief      获取解码图像的旋转角度
 * @param[in]  VdChn: 视频解码通道号
 * @param[out] penRotation: 旋转角度枚举指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VDEC_GetRotation(VDEC_CHN VdChn, ROTATION_E *penRotation);

/**
 * @brief      将解码通道绑定到某个视频缓存VB池中
 * @param[in]  VdChn: 视频解码通道号
 *             pstPool: 视频缓存VB 池信息
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VDEC_AttachVbPool(VDEC_CHN VdChn, const VDEC_CHN_POOL_S *pstPool);

/**
 * @brief      将解码通道从某个视频缓存VB池中解绑定
 * @param[in]  VdChn: 视频解码通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VDEC_DetachVbPool(VDEC_CHN VdChn);

/**
 * @brief      设置解码模块参数
 * @param[in]  pstModParam: 解码模块参数结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VDEC_SetModParam(const VDEC_MOD_PARAM_S *pstModParam);

/**
 * @brief      获取解码模块参数
 * @param[out] pstModParam: 解码模块参数结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VDEC_GetModParam(VDEC_MOD_PARAM_S *pstModParam);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef  __MPI_VDEC_H__ */

