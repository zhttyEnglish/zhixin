/**
 * @file     mpi_vgs.h
 * @brief    视频图形子系统模块的api接口
 * @version  1.0.0
 * @since    1.0.0
 * @author  张桂庆<zhangguiqing@sgitg.sgcc.com.cn>
 * @date    2021-08-20 创建文件
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

#ifndef __MPI_VGS_H__
#define __MPI_VGS_H__

#include "sc_common.h"
#include "sc_comm_video.h"
#include "sc_comm_vgs.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/**
 * @brief      启动一个job
 * @param[in]  phHandle: 用以返回HANDLE
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VGS_BeginJob(VGS_HANDLE *phHandle);

/**
 * @brief      提交一个job
 * @param[in]  phHandle: 已启动job句柄
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VGS_EndJob(VGS_HANDLE hHandle);

/**
 * @brief      取消一个 jobjob
 * @param[in]  phHandle: 已启动job句柄
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VGS_CancelJob(VGS_HANDLE hHandle);

/**
 * @brief      添加缩放task
 * @param[in]  phHandle: 已启动job句柄
 *             pstTask: task属性指针
 *             enScaleCoefMode: 缩放系数模式
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VGS_AddScaleTask(VGS_HANDLE hHandle, const VGS_TASK_ATTR_S *pstTask,
    VGS_SCLCOEF_MODE_E enScaleCoefMode);

/**
 * @brief      添加画线task
 * @param[in]  phHandle: 已启动job句柄
 *             pstTask: task属性指针
 *             pstVgsDrawLine: 画线属性配置指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VGS_AddDrawLineTask(VGS_HANDLE hHandle, const VGS_TASK_ATTR_S *pstTask,
    const VGS_DRAW_LINE_S *pstVgsDrawLine);

/**
 * @brief      添加打COVER task
 * @param[in]  phHandle: 已启动job句柄
 *             pstTask: task属性指针
 *             pstVgsAddCover: 打COVER属性配置指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VGS_AddCoverTask(VGS_HANDLE hHandle, const VGS_TASK_ATTR_S *pstTask,
    const VGS_ADD_COVER_S *pstVgsAddCover);

/**
 * @brief      添加打Osd task
 * @param[in]  phHandle: 已启动job句柄
 *             pstTask: task属性指针
 *             pstVgsAddOsd: 打OSD属性配置指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VGS_AddOsdTask(VGS_HANDLE hHandle, const VGS_TASK_ATTR_S *pstTask,
    const VGS_ADD_OSD_S *pstVgsAddOsd);

/**
 * @brief      添加打马赛克
 * @param[in]  phHandle: 已启动job句柄
 *             pstTask: task属性指针
 *             pstVgsAddMosaic: 打马赛克属性配置指针
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VGS_AddMosaicTask(VGS_HANDLE hHandle, const VGS_TASK_ATTR_S *pstTask,
    const VGS_ADD_MOSAIC_S *pstVgsAddMosaic);

/**
 * @brief      添加批量画线task
 * @param[in]  phHandle: 已启动job句柄
 *             pstTask: task属性指针
 *             astVgsDrawLine: 画线属性配置
 *             u32ArraySize: 画线数目，范围[1, 100]
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VGS_AddDrawLineTaskArray(VGS_HANDLE hHandle, const VGS_TASK_ATTR_S *pstTask,
    const VGS_DRAW_LINE_S astVgsDrawLine[], SC_U32 u32ArraySize);

/**
 * @brief      添加打批量COVER task
 * @param[in]  phHandle: 已启动job句柄
 *             pstTask: task属性指针
 *             astVgsAddCover: 打COVER属性配置
 *             u32ArraySize: 打COVER的数目，范围[1, 100]
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VGS_AddCoverTaskArray(VGS_HANDLE hHandle, const VGS_TASK_ATTR_S *pstTask,
    const VGS_ADD_COVER_S astVgsAddCover[], SC_U32 u32ArraySize);

/**
 * @brief      添加打批量Osd task
 * @param[in]  phHandle: 已启动job句柄
 *             pstTask: task属性指针
 *             astVgsAddOsd: 打OSD属性配置
 *             u32ArraySize: 打OSD的数目，范围[1, 100]
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VGS_AddOsdTaskArray(VGS_HANDLE hHandle, const VGS_TASK_ATTR_S *pstTask,
    const VGS_ADD_OSD_S astVgsAddOsd[], SC_U32 u32ArraySize);

/**
 * @brief      添加旋转task
 * @param[in]  phHandle: 已启动job句柄
 *             pstTask: task属性指针
 *             enRotationAngle: 旋转角度
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note
 */
SC_S32 SC_MPI_VGS_AddRotationTask(VGS_HANDLE hHandle, const VGS_TASK_ATTR_S *pstTask, ROTATION_E enRotationAngle);

/**
 * @brief      暂时不支持函数列表
 * @note       no support func
 */

/**
 * @brief      添加获取亮度和task
 * @param[in]  phHandle: 已启动job句柄
 *             pstTask: task属性指针
 *             astVgsLumaRect: 亮度和区域数组首地址
 *             u32ArraySize: 亮度和区域数量，范围: [1, 100]
 * @param[out] au64LumaData: 亮度和写出数组首地址
 * @return     SC_S32
 *             SC_SUCCESS: 成功
 *             其它: 失败
 * @note       no support
 */
SC_S32 SC_MPI_VGS_AddLumaTaskArray(VGS_HANDLE hHandle, VGS_TASK_ATTR_S *pstTask, const RECT_S astVgsLumaRect[],
    SC_U32 u32ArraySize, SC_U64 au64LumaData[]);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* end of __MPI_VGS_H__ */

