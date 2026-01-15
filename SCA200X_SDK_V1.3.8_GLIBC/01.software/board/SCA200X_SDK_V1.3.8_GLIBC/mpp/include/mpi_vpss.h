/**
* @file     mpi_vpss.h
* @brief    视频处理子模块的api接口
* @version  1.0.0
* @since    1.0.0
* @author  张桂庆<zhangguiqing@sgitg.sgcc.com.cn>
* @date    2021-08-30 创建文件
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

#ifndef __MPI_VPSS_H__
#define __MPI_VPSS_H__

#include "sc_common.h"
#include "sc_comm_video.h"
#include "sc_comm_vb.h"
#include "sc_comm_vpss.h"
#include "sc_comm_gdc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/**
 * @brief      创建一个VPSS GROUP
 * @param[in]  VpssGrp: VPSS GROUP号
 *             pstGrpAttr: VPSS GROUP属性指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_CreateGrp(VPSS_GRP VpssGrp, const VPSS_GRP_ATTR_S *pstGrpAttr);

/**
 * @brief      销毁一个VPSS GROUP
 * @param[in]  VpssGrp: VPSS GROUP号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_DestroyGrp(VPSS_GRP VpssGrp);

/**
 * @brief      启用VPSS GROUP
 * @param[in]  VpssGrp: VPSS GROUP号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_StartGrp(VPSS_GRP VpssGrp);

/**
 * @brief      禁用VPSS GROUP
 * @param[in]  VpssGrp: VPSS GROUP号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_StopGrp(VPSS_GRP VpssGrp);

/**
 * @brief      复位VPSS GROUP
 * @param[in]  VpssGrp: VPSS GROUP号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_ResetGrp(VPSS_GRP VpssGrp);

/**
 * @brief      获取VPSS GROUP属性
 * @param[in]  VpssGrp: VPSS GROUP号
 * @param[out] pstGrpAttr: VPSS GROUP属性指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_GetGrpAttr(VPSS_GRP VpssGrp, VPSS_GRP_ATTR_S *pstGrpAttr);

/**
 * @brief      设置VPSS GROUP属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             pstGrpAttr: VPSS GROUP属性指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_SetGrpAttr(VPSS_GRP VpssGrp, const VPSS_GRP_ATTR_S *pstGrpAttr);

/**
 * @brief      设置VPSS CROP属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             pstCropInfo: CROP结构体
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_SetGrpCrop(VPSS_GRP VpssGrp, const VPSS_CROP_INFO_S *pstCropInfo);

/**
 * @brief      获取VPSS CROP属性
 * @param[in]  VpssGrp: VPSS GROUP号
 * @param[out] pstGrpAttr: CROP结构体
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_GetGrpCrop(VPSS_GRP VpssGrp, VPSS_CROP_INFO_S *pstCropInfo);

/**
 * @brief      用户向VPSS发送数据
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssGrpPipe: VPSS组的管道号
 *             pstVideoFrame: 待发送的图像信息
 *             s32MilliSec: 超时参数s32MilliSec设为-1时，为阻塞接口；0时为非阻塞接口；大于0时为超时等待时间，超时时间的单位为毫秒
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_SendFrame(VPSS_GRP VpssGrp, VPSS_GRP_PIPE VpssGrpPipe,
    const VIDEO_FRAME_INFO_S *pstVideoFrame, SC_S32 s32MilliSec);

/**
 * @brief      用户从GROUP获取一帧原始图像
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssGrpPipe: VPSS组的管道号
 * @param[out] pstVideoFrame: 图像信息
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_GetGrpFrame(VPSS_GRP VpssGrp, VPSS_GRP_PIPE VpssGrpPipe, VIDEO_FRAME_INFO_S *pstVideoFrame);

/**
 * @brief      用户释放一帧源图像
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssGrpPipe: VPSS组的管道号
 *             pstVideoFrame: 图像信息
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_ReleaseGrpFrame(VPSS_GRP VpssGrp, VPSS_GRP_PIPE VpssGrpPipe,
    const VIDEO_FRAME_INFO_S *pstVideoFrame);

/**
 * @brief      VPSS以非绑定方式接收图像时,使能用户自己做帧率控制,此时VPSS通道帧率控制不生效
 * @param[in]  VpssGrp: VPSS GROUP号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_EnableUserFrameRateCtrl(VPSS_GRP VpssGrp);

/**
 * @brief      VPSS以非绑定方式接收图像时,禁止用户自己做帧率控制,此时VPSS通道帧率控制生效
 * @param[in]  VpssGrp: VPSS GROUP号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_DisableUserFrameRateCtrl(VPSS_GRP VpssGrp);

/**
 * @brief      设置VPSS通道属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 *             pstChnAttr: VPSS通道属性
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_SetChnAttr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_CHN_ATTR_S *pstChnAttr);

/**
 * @brief      获取VPSS通道属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 * @param[out] pstChnAttr: VPSS通道属性
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_GetChnAttr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_CHN_ATTR_S *pstChnAttr);

/**
 * @brief      启用VPSS通道
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_EnableChn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);

/**
 * @brief      禁用VPSS通道
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_DisableChn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);

/**
 * @brief      设置VPSS通道裁剪功能属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 *             pstCropInfo: CROP功能参数
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_SetChnCrop(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_CROP_INFO_S *pstCropInfo);

/**
 * @brief      获取VPSS通道裁剪功能属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 * @param[out] pstCropInfo: CROP功能参数
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_GetChnCrop(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_CROP_INFO_S *pstCropInfo);

/**
 * @brief      设置VPSS通道图像固定角度旋转属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 *             enRotation: 旋转属性
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_SetChnRotation(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, ROTATION_E enRotation);

/**
 * @brief      获取VPSS通道图像固定角度旋转属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 * @param[out] penRotation: 旋转属性
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_GetChnRotation(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, ROTATION_E *penRotation);

/**
 * @brief      设置VPSS镜头畸变校正LDC属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 *             pstLDCAttr: LDC属性结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_SetChnLDCAttr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_LDC_ATTR_S *pstLDCAttr);

/**
 * @brief      获取VPSS镜头畸变校正LDC属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 * @param[out] pstLDCAttr: LDC属性结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_GetChnLDCAttr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_LDC_ATTR_S *pstLDCAttr);

/**
 * @brief      用户从通道获取一帧处理完成的图像
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 *             s32MilliSec: 超时时间
 * @param[out] pstVideoFrame: 图像信息
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_GetChnFrame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn,
    VIDEO_FRAME_INFO_S *pstVideoFrame, SC_S32 s32MilliSec);

/**
 * @brief      用户释放一帧通道图像
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 *             pstVideoFrame: 图像信息
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_ReleaseChnFrame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VIDEO_FRAME_INFO_S *pstVideoFrame);

/**
 * @brief      获取指定图像区域的亮度总和
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 *             pstRegionInfo: 区域信息
 *             s32MilliSec: 超时时间
 * @param[out] pu64LumaData: 接收区域亮度和统计信息的内存指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_GetRegionLuma(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VIDEO_REGION_INFO_S *pstRegionInfo,
    SC_U64 *pu64LumaData, SC_S32 s32MilliSec);

/**
 * @brief      设置VPSS扩展通道属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 *             pstExtChnAttr: VPSS扩展通道属性
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_SetExtChnAttr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_EXT_CHN_ATTR_S *pstExtChnAttr);

/**
 * @brief      获取VPSS扩展通道属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 * @param[out] pstExtChnAttr: VPSS扩展通道属性
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_GetExtChnAttr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_EXT_CHN_ATTR_S *pstExtChnAttr);

/**
 * @brief      将VPSS的通道绑定到某个视频缓存VB池
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 *             hVbPool: 视频缓存VB池信息
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_AttachVbPool(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VB_POOL hVbPool);

/**
 * @brief      将VPSS的通道从某个视频缓存VB池
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_DetachVbPool(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);

/**
 * @brief      设置VPSS 3DNR X接口参数
 * @param[in]  VpssGrp: VPSS GROUP号
 *             pstNRXParam: 3DNR参数
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_SetGrpNRXParam(VPSS_GRP VpssGrp, const VPSS_GRP_NRX_PARAM_S *pstNRXParam);

/**
 * @brief      获取VPSS 3DNR X接口参数
 * @param[in]  VpssGrp: VPSS GROUP号
 * @param[out] pstNRXParam: 3DNR参数
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_GetGrpNRXParam(VPSS_GRP VpssGrp, VPSS_GRP_NRX_PARAM_S *pstNRXParam);

/**
 * @brief      设置Sharpen属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             pstGrpSharpenAttr: Sharpen属性
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_SetGrpSharpen(VPSS_GRP VpssGrp, const VPSS_GRP_SHARPEN_ATTR_S *pstGrpSharpenAttr);

/**
 * @brief      获取Sharpen属性
 * @param[in]  VpssGrp: VPSS GROUP号
 * @param[out] pstGrpSharpenAttr: Sharpen属性
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_GetGrpSharpen(VPSS_GRP VpssGrp, VPSS_GRP_SHARPEN_ATTR_S *pstGrpSharpenAttr);

/**
 * @brief      VPSS模块资源退出
 * @param[in]  void
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VPSS_Exit(SC_VOID);

/**
 * @brief      暂时不支持函数列表
 * @note       no support func
 */

/**
 * @brief      使能Backup帧
 * @param[in]  VpssGrp: VPSS GROUP号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_EnableBackupFrame(VPSS_GRP VpssGrp);

/**
 * @brief      不使能Backup帧
 * @param[in]  VpssGrp: VPSS GROUP号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_DisableBackupFrame(VPSS_GRP VpssGrp);

/**
 * @brief      设置延迟启动VPSS处理的延时队列长度
 * @param[in]  VpssGrp: VPSS GROUP号
 *             u32Delay: 延时队列长度
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_SetGrpDelay(VPSS_GRP VpssGrp, SC_U32 u32Delay);

/**
 * @brief      获取延迟启动VPSS处理的延时队列长度
 * @param[in]  VpssGrp: VPSS GROUP号
 *             pu32Delay: 延时队列长度
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_GetGrpDelay(VPSS_GRP VpssGrp, SC_U32 *pu32Delay);

/**
 * @brief      设置VPSS GROUP对应的鱼眼镜头LMF参数配置
 * @param[in]  VpssGrp: VPSS GROUP号
 *             pstFisheyeConfig: 鱼眼镜头LMF参数结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_SetGrpFisheyeConfig(VPSS_GRP VpssGrp, const FISHEYE_CONFIG_S *pstFisheyeConfig);

/**
 * @brief      获取VPSS GROUP对应的鱼眼镜头LMF参数配置
 * @param[in]  VpssGrp: VPSS GROUP号
 * @param[out] pstFisheyeConfig: 鱼眼镜头LMF参数结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_GetGrpFisheyeConfig(VPSS_GRP VpssGrp, FISHEYE_CONFIG_S *pstFisheyeConfig);

/**
 * @brief      设置VPSS在线时使用的帧中断属性结构体
 * @param[in]  VpssGrp: VPSS GROUP号
 *             pstFrameIntAttr: 帧中断属性结构体
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_SetGrpFrameInterruptAttr(VPSS_GRP VpssGrp, const FRAME_INTERRUPT_ATTR_S *pstFrameIntAttr);

/**
 * @brief      获取VPSS在线时使用的帧中断属性结构体
 * @param[in]  VpssGrp: VPSS GROUP号
 * @param[out] pstFrameIntAttr: 帧中断属性结构体
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_GetGrpFrameInterruptAttr(VPSS_GRP VpssGrp, FRAME_INTERRUPT_ATTR_S *pstFrameIntAttr);

/**
 * @brief      设置VPSS的任意角度旋转属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 *             pstRotationExAttr: 任意角度旋转属性结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_SetChnRotationEx(VPSS_GRP VpssGrp, VPSS_CHN VpssChn,
    const VPSS_ROTATION_EX_ATTR_S *pstRotationExAttr);

/**
 * @brief      获取VPSS的任意角度旋转属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 * @param[out] pstRotationExAttr: 任意角度旋转属性结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_GetChnRotationEx(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_ROTATION_EX_ATTR_S *pstRotationExAttr);

/**
 * @brief      设置VPSS通道展宽属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 *             pstSpreadAttr: 展宽属性结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_SetChnSpreadAttr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const SPREAD_ATTR_S *pstSpreadAttr);

/**
 * @brief      获取VPSS通道展宽属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 * @param[out] pstSpreadAttr: 展宽属性结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_GetChnSpreadAttr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, SPREAD_ATTR_S *pstSpreadAttr);

/**
 * @brief      设置低延时属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 *             pstLowDelayInfo: 低延时属性
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_SetLowDelayAttr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_LOW_DELAY_INFO_S *pstLowDelayInfo);

/**
 * @brief      获取低延时属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 * @param[out] pstLowDelayInfo: 低延时属性
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_GetLowDelayAttr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_LOW_DELAY_INFO_S *pstLowDelayInfo);

/**
 * @brief      设置低延时卷绕属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 *             pstVpssChnBufWrap: 通道buffer卷绕属性结构体
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_SetChnBufWrapAttr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_CHN_BUF_WRAP_S *pstVpssChnBufWrap);

/**
 * @brief      获取低延时卷绕属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 * @param[out] pstVpssChnBufWrap: 通道buffer卷绕属性结构体
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_GetChnBufWrapAttr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_CHN_BUF_WRAP_S *pstVpssChnBufWrap);

/**
 * @brief      触发抓拍帧
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 *             u32FrameCnt: 触发抓拍帧的总数
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_TriggerSnapFrame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, SC_U32 u32FrameCnt);

/**
 * @brief      使能输入输出复用buffer
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_EnableBufferShare(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);

/**
 * @brief      不使能输入输出复用buffer
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_DisableBufferShare(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);

/**
 * @brief      设置VPSS通道输出YUV数据的行stride对齐
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 *             u32Align: VPSS通道输出YUV数据的行stride对齐，单位为Byte
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_SetChnAlign(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, SC_U32 u32Align);

/**
 * @brief      获取VPSS通道输出YUV数据的行stride对齐
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 * @param[out] pu32Align: VPSS通道输出YUV数据的行stride对齐，单位为Byte
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_GetChnAlign(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, SC_U32 *pu32Align);

/**
 * @brief      设置VPSS通道处理模式
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 *             enVpssChnProcMode: VPSS通道处理模式
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_SetChnProcMode(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_CHN_PROC_MODE_E enVpssChnProcMode);

/**
 * @brief      获取VPSS通道处理模式
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 * @param[out] enVpssChnProcMode: VPSS通道处理模式
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_GetChnProcMode(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_CHN_PROC_MODE_E *penVpssChnProcMode);

/**
 * @brief      设置VPSS扩展通道对应的鱼眼属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 *             pstFishEyeAttr: VPSS扩展通道的鱼眼属性结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_SetExtChnFisheye(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const FISHEYE_ATTR_S *pstFishEyeAttr);

/**
 * @brief      获取VPSS扩展通道对应的鱼眼属性
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 * @param[out] pstFishEyeAttr: VPSS扩展通道的鱼眼属性结构体指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_GetExtChnFisheye(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, FISHEYE_ATTR_S *pstFishEyeAttr);

/**
 * @brief      根据鱼眼校正输出图像坐标点查找源图像坐标点
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 *             u32RegionIndex: 鱼眼校正区域号
 *             pstDstPointIn: 鱼眼校正图上需要查找映射关系的坐标点
 * @param[out] pstSrcPointOut: 鱼眼原图上查找到的坐标点
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_FisheyePosQueryDst2Src(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, SC_U32 u32RegionIndex,
    const POINT_S *pstDstPointIn, POINT_S *pstSrcPointOut);

/**
 * @brief      设置VPSS模块参数
 * @param[in]  pstModParam: 模块参数
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_SetModParam(const VPSS_MOD_PARAM_S *pstModParam);

/**
 * @brief      获取VPSS模块参数
 * @param[out] pstModParam: 模块参数
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_GetModParam(VPSS_MOD_PARAM_S *pstModParam);

/**
 * @brief      获取VPSS通道对应的设备文件句柄
 * @param[in]  VpssGrp: VPSS GROUP号
 *             VpssChn: VPSS通道号
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_GetChnFd(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);

/**
 * @brief      关闭设备和通道的文件描述符
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VPSS_CloseFd(SC_VOID);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __MPI_VPSS_H__ */

