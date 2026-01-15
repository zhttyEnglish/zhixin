/**
 * @file     mpi_isp.h
 * @brief    ISP模块应用程序接口声明
 * @version  1.0.0
 * @since    1.0.0
 * @author   陶友水<taoyoushui@sgitg.sgcc.com.cn>
 * @date     2021-07-15 创建文件
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

#ifndef __MPI_ISP_H__
#define __MPI_ISP_H__

#include "sc_comm_sns.h"
#include "sc_comm_3a.h"
#include "sc_comm_vi.h"
#include "sc_ae_comm.h"
#include "sc_awb_comm.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

/***********************************************************************************************************************
 **********          ISP API DECLARATION                                                                      **********
 **********************************************************************************************************************/
/**
 * @brief      初始化ISP外部寄存器
 * @param[in]  ViPipe: VI PIPE号
 * @param[out] valOut:  无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_MemInit(VI_PIPE ViPipe);

/**
 * @brief      初始化ISP firmware
 * @param[in]  ViPipe: VI PIPE号
 * @param[out] valOut:  无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_Init(VI_PIPE ViPipe);

/**
 * @brief      运行ISP firmware
 * @param[in]  ViPipe: VI PIPE号
 * @param[out] valOut:  无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_Run(VI_PIPE ViPipe);

/**
 * @brief      退出ISP firmware
 * @param[in]  ViPipe: VI PIPE号
 * @param[out] valOut:  无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_Exit(VI_PIPE ViPipe);

/**
 * @brief      设置ISP firmware状态
 * @param[in]  ViPipe: VI PIPE号
 *             state:   ISP firmware状态
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetFMWState(VI_PIPE ViPipe, const ISP_FMW_STATE_E enState);

/**
 * @brief      获取ISP firmware状态
 * @param[in]  ViPipe: VI PIPE号
 * @param[out] state:   ISP firmware状态
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetFMWState(VI_PIPE ViPipe, ISP_FMW_STATE_E *penState);

/**
 * @brief      设置指定VI设备的MIPI设备号
 * @param[in]  ViDev:  VI设备号
 *             MipiDev: MIPI设备号
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:       失败
 */
SC_S32 SC_MPI_ISP_SetMipiBindDev(VI_DEV ViDev, MIPI_DEV MipiDev);

/**
 * @brief      COMBO设备操作
 * @param[in]  pstComboDevAttr: COMBO设备结构体指针
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetComboDevAttr(const SC_COMBO_DEV_ATTR_S *pstDevAttr);

/**
 * @brief      设置ISP公共属性
 * @param[in]  ViPipe:  VI PIPE号
 *             pub_attr: ISP公共属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetPubAttr(VI_PIPE ViPipe, const ISP_PUB_ATTR_S *pstPubAttr);

/**
 * @brief      获取ISP公共属性
 * @param[in]  ViPipe:  VI PIPE号
 * @param[out] pub_attr: ISP公共属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetPubAttr(VI_PIPE ViPipe, ISP_PUB_ATTR_S *pstPubAttr);

/**
 * @brief      设置ISP功能模块的控制
 * @param[in]  ViPipe:  VI PIPE号
 *             mod_ctrl: 模块控制值
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetModuleControl(VI_PIPE ViPipe, const ISP_MODULE_CTRL_U *punModCtrl);

/**
 * @brief      获取ISP功能模块的控制
 * @param[in]  ViPipe:  VI PIPE号
 * @param[out] mod_ctrl: 模块控制值
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetModuleControl(VI_PIPE ViPipe, ISP_MODULE_CTRL_U *punModCtrl);

/**
 * @brief      ISP提供的SENSOR注册的回调接口
 * @param[in]  ViPipe:         VI PIPE号
 *             pstSnsAttrInfo: 向ISP注册的SENSOR的属性
 *             pstRegister:    SENSOR注册结构体指针
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SensorRegCallBack(VI_PIPE ViPipe,
    ISP_SNS_ATTR_INFO_S *pstSnsAttrInfo, ISP_SENSOR_REGISTER_S *pstRegister);

/**
 * @brief      ISP提供的SENSOR反注册的回调接口
 * @param[in]  ViPipe:   VI PIPE号
 *             SensorId: 向ISP注册的SENSOR的ID
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SensorUnRegCallBack(VI_PIPE ViPipe, SENSOR_ID SensorId);

/**
 * @brief      向ISP注册AE库
 * @param[in]  ViPipe:   VI PIPE号
 *             pstAeLib: AE算法库结构体指针
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_AE_Register(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib);

/**
 * @brief      向ISP反注册AE库
 * @param[in]  ViPipe:   VI PIPE号
 *             pstAeLib: AE算法库结构体指针
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_AE_UnRegister(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib);

/**
 * @brief      AE库提供的SENSOR注册的回调接口
 * @param[in]  ViPipe:         VI PIPE号
 *             pstAeLib:       AE算法库结构体指针
 *             pstSnsAttrInfo: 向AE注册的SENSOR的属性
 *             pstRegister:    SENSOR注册结构体指针
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_AE_SensorRegCallBack(VI_PIPE ViPipe,
    ALG_LIB_S *pstAeLib,
    ISP_SNS_ATTR_INFO_S *pstSnsAttrInfo,
    AE_SENSOR_REGISTER_S *pstRegister);

/**
 * @brief      AE库提供的SENSOR反注册的回调接口
 * @param[in]  ViPipe:   VI PIPE号
 *             pstAeLib: AE算法库结构体指针
 *             SensorId: 向AE反注册的SENSOR的ID
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_AE_SensorUnRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, SENSOR_ID SensorId);

/**
 * @brief      向ISP注册AWB库
 * @param[in]  ViPipe:   VI PIPE号
 *             pstAeLib: AWB算法库结构体指针
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_AWB_Register(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib);

/**
 * @brief      向ISP反注册AWB库
 * @param[in]  ViPipe:   VI PIPE号
 *             pstAeLib: AWB算法库结构体指针
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_AWB_UnRegister(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib);

/**
 * @brief      AWB库提供的SENSOR注册的回调接口
 * @param[in]  ViPipe:         VI PIPE号
 *             pstAwbLib:      AWB算法库结构体指针
 *             pstSnsAttrInfo: 向AWB注册的SENSOR的属性
 *             pstRegister:    SENSOR注册结构体指针
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_AWB_SensorRegCallBack(VI_PIPE ViPipe,
    ALG_LIB_S *pstAwbLib,
    ISP_SNS_ATTR_INFO_S *pstSnsAttrInfo,
    AWB_SENSOR_REGISTER_S *pstRegister);

/**
 * @brief      AWB库提供的SENSOR反注册的回调接口
 * @param[in]  ViPipe:    VI PIPE号
 *             pstAwbLib: AWB算法库结构体指针
 *             SensorId:  向AWB反注册的SENSOR的ID
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_AWB_SensorUnRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib, SENSOR_ID SensorId);

/**
 * @brief      ISP提供的AE库注册的回调接口
 * @param[in]  ViPipe:      VI PIPE号
 *             pstAeLib:    AE库结构体指针
 *             pstRegister: AE库注册结构体指针
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_AELibRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib,         ISP_AE_REGISTER_S *pstRegister);

/**
 * @brief      ISP提供的AE库反注册的回调接口
 * @param[in]  ViPipe:   VI PIPE号
 *             pstAeLib: AE库结构体指针
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_AELibUnRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib);

/**
 * @brief      ISP提供的AWB库注册的回调接口
 * @param[in]  ViPipe:      VI PIPE号
 *             pstAwbLib:   AWB库结构体指针
 *             pstRegister: AWB库注册结构体指针
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_AWBLibRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib,          ISP_AWB_REGISTER_S *pstRegister);

/**
 * @brief      ISP提供的AWB库反注册的回调接口
 * @param[in]  ViPipe:    VI PIPE号
 *             pstAwbLib: AWB库结构体指针
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_AWBLibUnRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib);

/**
 * @brief      设置寄存器值
 * @param[in]  ViPipe:   VI PIPE号
 *             u32Addr:  寄存器地址
 *             u32Value: 寄存器的值
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetRegister(VI_PIPE ViPipe, SC_U32 u32Addr, SC_U32 u32Value);

/**
 * @brief      获取寄存器值
 * @param[in]  ViPipe:    VI PIPE号
 *             u32Addr:   寄存器地址
 * @param[out] pu32Value: 寄存器值的指针
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetRegister(VI_PIPE ViPipe, SC_U32 u32Addr, SC_U32 *pu32Value);

/**
 * @brief      设置ISP的控制参数
 * @param[in]  ViPipe:          VI PIPE号
 *             pstIspCtrlParam: ISP控制参数结构体指针
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetCtrlParam(VI_PIPE ViPipe, const ISP_CTRL_PARAM_S *pstIspCtrlParam);

/**
 * @brief      获取ISP的控制参数
 * @param[in]  ViPipe: VI PIPE号
 * @param[out] pstIspCtrlParam: ISP控制参数结构体指针
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetCtrlParam(VI_PIPE ViPipe, ISP_CTRL_PARAM_S *pstIspCtrlParam);

/**
 * @brief      设置ISP 3A统计信息配置
 * @param[in]  ViPipe:     VI PIPE号
 *             pstStatCfg: ISP统计信息配置
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetStatisticsConfig(VI_PIPE ViPipe, const ISP_STATISTICS_CFG_S *pstStatCfg);

/**
 * @brief      获取ISP 3A统计信息配置
 * @param[in]  ViPipe:     VI PIPE号
 * @param[out] pstStatCfg: ISP统计信息配置
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetStatisticsConfig(VI_PIPE ViPipe, ISP_STATISTICS_CFG_S *pstStatCfg);

/**
 * @brief      获取AE统计信息
 * @param[in]  ViPipe:    VI PIPE号
 * @param[out] pstAeStat: AE统计信息
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetAEStatistics(VI_PIPE ViPipe, ISP_AE_STATISTICS_S *pstAeStat);

/**
 * @brief      获取AWB统计信息
 * @param[in]  ViPipe:    VI PIPE号
 * @param[out] pstWBStat: AWB统计信息
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetWBStatistics(VI_PIPE ViPipe, ISP_WB_STATISTICS_S *pstWBStat);

/**
 * @brief      获取ISP中断信息
 * @param[in]  ViPipe:      VI PIPE号
 *             enIspVDType: 场同步信号
 *             u32MilliSec: 超时时间，单位ms
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetVDTimeOut(VI_PIPE ViPipe, ISP_VD_TYPE_E enIspVDType, SC_U32 u32MilliSec);

/**
 * @brief      获取AF统计信息
 * @param[in]  ViPipe:    VI PIPE号
 * @param[out] pstAfStat: AF统计信息
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetFocusStatistics(VI_PIPE ViPipe, ISP_AF_STATISTICS_S *pstAfStat);

/**
 * @brief      设置对焦属性
 * @param[in]  ViPipe:       VI PIPE号
 *             pstFocusAttr: 对焦属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetFocusAttr(VI_PIPE ViPipe, const ISP_FOCUS_ATTR_S *pstFocusAttr);

/**
 * @brief      获取对焦属性
 * @param[in]  ViPipe:       VI PIPE号
 * @param[out] pstFocusAttr: 对焦属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetFocusAttr(VI_PIPE ViPipe, ISP_FOCUS_ATTR_S *pstFocusAttr);

/**
 * @brief      设置曝光属性
 * @param[in]  ViPipe:     VI PIPE号
 *             pstExpAttr: 曝光属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetExposureAttr(VI_PIPE ViPipe, const ISP_EXPOSURE_ATTR_S *pstExpAttr);

/**
 * @brief      获取曝光属性
 * @param[in]  ViPipe:     VI PIPE号
 * @param[out] pstExpAttr: 曝光属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetExposureAttr(VI_PIPE ViPipe, ISP_EXPOSURE_ATTR_S *pstExpAttr);

/**
 * @brief      设置WDR模式下的AE曝光属性
 * @param[in]  ViPipe:        VI PIPE号
 *             pstWDRExpAttr: WDR模式下的AE曝光属性结构体指针
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetWDRExposureAttr(VI_PIPE ViPipe, const ISP_WDR_EXPOSURE_ATTR_S *pstWDRExpAttr);

/**
 * @brief      获取WDR模式下的AE曝光属性
 * @param[in]  ViPipe:        VI PIPE号
 * @param[out] pstWDRExpAttr: WDR模式下的AE曝光属性结构体指针
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetWDRExposureAttr(VI_PIPE ViPipe, ISP_WDR_EXPOSURE_ATTR_S *pstWDRExpAttr);

/**
 * @brief      设置AE曝光分配策略属性
 * @param[in]  ViPipe:        VI PIPE号
 *             pstAERouteAttr: AE曝光分配策略结构体指针
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetAERouteAttr(VI_PIPE ViPipe, const ISP_AE_ROUTE_S *pstAERouteAttr);

/**
 * @brief      获取AE曝光分配策略属性
 * @param[in]  ViPipe:        VI PIPE号
 * @param[out] pstAERouteAttr: AE曝光分配策略结构体指针
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetAERouteAttr(VI_PIPE ViPipe, ISP_AE_ROUTE_S *pstAERouteAttr);

/**
 * @brief      获取AE内部状态信息
 * @param[in]  ViPipe:     VI PIPE号
 * @param[out] pstExpInfo: 曝光内部状态信息结构体指针
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_QueryExposureInfo(VI_PIPE ViPipe, ISP_EXP_INFO_S *pstExpInfo);

/**
 * @brief      设置Face ExposureInfo信息
 * @param[in]  ViPipe:         VI PIPE号
 * @param[in]  pstFaceSmpAttr: Face ExposureInfo信息结构体指针
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:       失败
 */
SC_S32 SC_MPI_ISP_SetFaceSimpleAttr(VI_PIPE ViPipe, const ISP_FACE_AEC_S *pstFaceSmpAttr);
/**

 * @brief      设置白平衡属性
 * @param[in]  ViPipe:    VI PIPE号
 *             pstWBAttr: 白平衡的参数属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetWBAttr(VI_PIPE ViPipe, const ISP_WB_ATTR_S *pstWBAttr);

/**
 * @brief      获取白平衡属性
 * @param[in]  ViPipe:    VI PIPE号
 * @param[out] pstWBAttr: 白平衡的参数属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetWBAttr(VI_PIPE ViPipe, ISP_WB_ATTR_S *pstWBAttr);

/**
 * @brief      获取当前白平衡增益系数，检测色温，饱和度值，颜色校正矩阵系数
 * @param[in]  ViPipe:    VI PIPE号
 * @param[out] pstWBInfo: 颜色相关状态参数结构体指针
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_QueryWBInfo(VI_PIPE ViPipe, ISP_WB_INFO_S *pstWBInfo);

/**
 * @brief      设置亮度简单属性
 * @param[in]  ViPipe:        VI PIPE号
 *             pstBriSmpAttr: 亮度简单属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetBrightnessSimpleAttr(VI_PIPE ViPipe, const ISP_BRIGHTNESS_SIMPLE_ATTR_S *pstBriSmpAttr);

/**
 * @brief      获取亮度简单属性
 * @param[in]  ViPipe:        VI PIPE号
 * @param[out] pstBriSmpAttr: 亮度简单属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetBrightnessSimpleAttr(VI_PIPE ViPipe, ISP_BRIGHTNESS_SIMPLE_ATTR_S *pstBriSmpAttr);

/**
 * @brief      设置锐度简单属性
 * @param[in]  ViPipe:        VI PIPE号
 *             pstShpSmpAttr: 锐度简单属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetSharpnessSimpleAttr(VI_PIPE ViPipe, const ISP_SHARPNESS_SIMPLE_ATTR_S *pstShpSmpAttr);

/**
 * @brief      获取锐度简单属性
 * @param[in]  ViPipe:        VI PIPE号
 * @param[out] pstShpSmpAttr: 锐度简单属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetSharpnessSimpleAttr(VI_PIPE ViPipe, ISP_SHARPNESS_SIMPLE_ATTR_S *pstShpSmpAttr);

/**
 * @brief      设置色度简单属性
 * @param[in]  ViPipe:        VI PIPE号
 *             pstHueSmpAttr: 色度简单属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetHueSimpleAttr(VI_PIPE ViPipe, const ISP_HUE_SIMPLE_ATTR_S *pstHueSmpAttr);

/**
 * @brief      获取色度简单属性
 * @param[in]  ViPipe:        VI PIPE号
 * @param[out] pstHueSmpAttr: 色度简单属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetHueSimpleAttr(VI_PIPE ViPipe, ISP_HUE_SIMPLE_ATTR_S *pstHueSmpAttr);

/**
 * @brief      设置对比度简单属性
 * @param[in]  ViPipe:        VI PIPE号
 *             pstConSmpAttr: 对比度简单属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetContrastSimpleAttr(VI_PIPE ViPipe, const ISP_CONTRAST_SIMPLE_ATTR_S *pstConSmpAttr);

/**
 * @brief      获取对比度简单属性
 * @param[in]  ViPipe:        VI PIPE号
 * @param[out] pstConSmpAttr: 对比度简单属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetContrastSimpleAttr(VI_PIPE ViPipe, ISP_CONTRAST_SIMPLE_ATTR_S *pstConSmpAttr);

/**
 * @brief      设置颜色饱和度属性
 * @param[in]  ViPipe:     VI PIPE号
 *             pstSatAttr: 颜色饱和度属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetSaturationAttr(VI_PIPE ViPipe, const ISP_SATURATION_ATTR_S *pstSatAttr);

/**
 * @brief      获取颜色饱和度属性
 * @param[in]  ViPipe:     VI PIPE号
 * @param[out] pstSatAttr: 颜色饱和度属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetSaturationAttr(VI_PIPE ViPipe, ISP_SATURATION_ATTR_S *pstSatAttr);

/**
 * @brief      设置曝光限制简单属性
 * @param[in]  ViPipe:           VI PIPE号
 *             pstExpLmtSmpAttr: 曝光限制简单属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetExpLimitSimpleAttr(VI_PIPE ViPipe, const ISP_EXP_LIMIT_SIMPLE_ATTR_S *pstExpLmtSmpAttr);

/**
 * @brief      获取曝光限制简单属性
 * @param[in]  ViPipe:           VI PIPE号
 * @param[out] pstExpLmtSmpAttr: 曝光限制简单属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetExpLimitSimpleAttr(VI_PIPE ViPipe, ISP_EXP_LIMIT_SIMPLE_ATTR_S *pstExpLmtSmpAttr);

/**
 * @brief      设置抗闪简单属性
 * @param[in]  ViPipe:        VI PIPE号
 *             pstFlkSmpAttr: 抗闪简单属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetAntiflickerSimpleAttr(VI_PIPE ViPipe, const ISP_ANTIFLICKER_SIMPLE_ATTR_S *pstFlkSmpAttr);

/**
 * @brief      获取抗闪简单属性
 * @param[in]  ViPipe:        VI PIPE号
 * @param[out] pstFlkSmpAttr: 抗闪简单属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetAntiflickerSimpleAttr(VI_PIPE ViPipe, ISP_ANTIFLICKER_SIMPLE_ATTR_S *pstFlkSmpAttr);

/**
 * @brief      设置场景模式简单属性
 * @param[in]  ViPipe:        VI PIPE号
 *             pstScnSmpAttr: 场景模式简单属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetSceneSimpleAttr(VI_PIPE ViPipe, const ISP_SCENE_SIMPLE_ATTR_S *pstScnSmpAttr);

/**
 * @brief      获取模式简单属性
 * @param[in]  ViPipe:        VI PIPE号
 * @param[out] pstScnSmpAttr: 场景模式简单属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetSceneSimpleAttr(VI_PIPE ViPipe, ISP_SCENE_SIMPLE_ATTR_S *pstScnSmpAttr);

/**
 * @brief      设置白平衡简单属性
 * @param[in]  ViPipe:        VI PIPE号
 *             pstAwbSmpAttr: 白平衡简单属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetAwbSimpleAttr(VI_PIPE ViPipe, const ISP_AWB_SIMPLE_ATTR_S *pstAwbSmpAttr);

/**
 * @brief      设置去雾简单属性
 * @param[in]  ViPipe:        VI PIPE号
 *             pstDfgSmpAttr: 去雾简单属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetDefogSimpleAttr(VI_PIPE ViPipe, const ISP_DEFOG_SIMPLE_ATTR_S *pstDfgSmpAttr);

/**
 * @brief      获取去雾简单属性
 * @param[in]  ViPipe:        VI PIPE号
 * @param[out] pstDfgSmpAttr: 去雾简单属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetDefogSimpleAttr(VI_PIPE ViPipe, ISP_DEFOG_SIMPLE_ATTR_S *pstDfgSmpAttr);

/**
 * @brief      设置2D降噪简单属性
 * @param[in]  ViPipe:         VI PIPE号
 *             pstNr2dSmpAttr: 2D降噪简单属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetNr2dSimpleAttr(VI_PIPE ViPipe, const ISP_NR2D_SIMPLE_ATTR_S *pstNr2dSmpAttr);

/**
 * @brief      获取2D降噪简单属性
 * @param[in]  ViPipe:         VI PIPE号
 * @param[out] pstNr2dSmpAttr: 2D降噪简单属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetNr2dSimpleAttr(VI_PIPE ViPipe, ISP_NR2D_SIMPLE_ATTR_S *pstNr2dSmpAttr);

/**
 * @brief      设置3D降噪简单属性
 * @param[in]  ViPipe:         VI PIPE号
 *             pstNr3dSmpAttr: 3D降噪简单属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetNr3dSimpleAttr(VI_PIPE ViPipe, const ISP_NR3D_SIMPLE_ATTR_S *pstNr3dSmpAttr);

/**
 * @brief      获取3D降噪简单属性
 * @param[in]  ViPipe:         VI PIPE号
 * @param[out] pstNr3dSmpAttr: 3D降噪简单属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetNr3dSimpleAttr(VI_PIPE ViPipe, ISP_NR3D_SIMPLE_ATTR_S *pstNr3dSmpAttr);

/**
 * @brief      设置DRC简单属性
 * @param[in]  ViPipe:        VI PIPE号
 *             pstDrcSmpAttr: DRC简单属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetDrcSimpleAttr(VI_PIPE ViPipe, const ISP_DRC_SIMPLE_ATTR_S *pstDrcSmpAttr);

/**
 * @brief      获取DRC简单属性
 * @param[in]  ViPipe:        VI PIPE号
 * @param[out] pstDrcSmpAttr: DRC简单属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetDrcSimpleAttr(VI_PIPE ViPipe, ISP_DRC_SIMPLE_ATTR_S *pstDrcSmpAttr);

/**
 * @brief      设置颜色校正基础矩阵属性
 * @param[in]  ViPipe:     VI PIPE号
 *             pstCCMAttr: 颜色矩阵属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetCCMAttr(VI_PIPE ViPipe, const ISP_COLORMATRIX_ATTR_S *pstCCMAttr);

/**
 * @brief      获取颜色校正基础矩阵属性
 * @param[in]  ViPipe:     VI PIPE号
 * @param[out] pstCCMAttr: 颜色矩阵属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetCCMAttr(VI_PIPE ViPipe, ISP_COLORMATRIX_ATTR_S *pstCCMAttr);

/**
 * @brief      设置图像锐化属性
 * @param[in]  ViPipe:        VI PIPE号
 *             pstIspShpAttr: 图像锐化属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetIspSharpenAttr(VI_PIPE ViPipe, const ISP_SHARPEN_ATTR_S *pstIspShpAttr);

/**
 * @brief      获取图像锐化属性
 * @param[in]  ViPipe:        VI PIPE号
 * @param[out] pstIspShpAttr: 图像锐化属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetIspSharpenAttr(VI_PIPE ViPipe, ISP_SHARPEN_ATTR_S *pstIspShpAttr);

/**
 * @brief      设置Gamma属性
 * @param[in]  ViPipe:       VI PIPE号
 *             pstGammaAttr: Gamma属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetGammaAttr(VI_PIPE ViPipe, const ISP_GAMMA_ATTR_S *pstGammaAttr);

/**
 * @brief      获取Gamma属性
 * @param[in]  ViPipe:       VI PIPE号
 * @param[out] pstGammaAttr: Gamma属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetGammaAttr(VI_PIPE ViPipe, ISP_GAMMA_ATTR_S *pstGammaAttr);

/**
 * @brief      设置动态范围压缩参数
 * @param[in]  ViPipe: VI PIPE号
 *             pstDRC: 动态范围压缩参数
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetDRCAttr(VI_PIPE ViPipe, const ISP_DRC_ATTR_S *pstDRC);

/**
 * @brief      获取动态范围压缩参数
 * @param[in]  ViPipe: VI PIPE号
 * @param[out] pstDRC: 动态范围压缩参数
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetDRCAttr(VI_PIPE ViPipe, ISP_DRC_ATTR_S *pstDRC);

/**
 * @brief      设置动态范围压缩参数
 * @param[in]  ViPipe:         VI PIPE号
 *             pstDPCalibrate: 静态坏点标定参数
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetDPCalibrate(VI_PIPE ViPipe, const ISP_DP_STATIC_CALIBRATE_S *pstDPCalibrate);

/**
 * @brief      获取动态范围压缩参数
 * @param[in]  ViPipe:         VI PIPE号
 * @param[out] pstDPCalibrate: 静态坏点标定参数
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetDPCalibrate(VI_PIPE ViPipe, ISP_DP_STATIC_CALIBRATE_S *pstDPCalibrate);

/**
 * @brief      设置静态坏点校正属性
 * @param[in]  ViPipe:          VI PIPE号
 *             pstDPStaticAttr: 静态坏点校正属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetDPStaticAttr(VI_PIPE ViPipe, const ISP_DP_STATIC_ATTR_S *pstDPStaticAttr);

/**
 * @brief      获取静态坏点校正属性
 * @param[in]  ViPipe:          VI PIPE号
 * @param[out] pstDPStaticAttr: 静态坏点校正属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetDPStaticAttr(VI_PIPE ViPipe, ISP_DP_STATIC_ATTR_S *pstDPStaticAttr);

/**
 * @brief      设置动态坏点校正属性
 * @param[in]  ViPipe:           VI PIPE号
 *             pstDPDynamicAttr: 动态坏点校正属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetDPDynamicAttr(VI_PIPE ViPipe, const ISP_DP_DYNAMIC_ATTR_S *pstDPDynamicAttr);

/**
 * @brief      获取动态坏点校正属性
 * @param[in]  ViPipe:           VI PIPE号
 * @param[out] pstDPDynamicAttr: 动态坏点校正属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetDPDynamicAttr(VI_PIPE ViPipe, ISP_DP_DYNAMIC_ATTR_S *pstDPDynamicAttr);

/**
 * @brief      设置Crosstalk属性
 * @param[in]  ViPipe:    VI PIPE号
 *             pstCRAttr: Crosstalk属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetCrosstalkAttr(VI_PIPE ViPipe, const ISP_CR_ATTR_S *pstCRAttr);

/**
 * @brief      获取Crosstalk属性
 * @param[in]  ViPipe:    VI PIPE号
 * @param[out] pstCRAttr: Crosstalk属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetCrosstalkAttr(VI_PIPE ViPipe, ISP_CR_ATTR_S *pstCRAttr);

/**
 * @brief      设置噪声抑制参数
 * @param[in]  ViPipe:    VI PIPE号
 *             pstNRAttr: 噪声抑制参数
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetNRAttr(VI_PIPE ViPipe, const ISP_NR_ATTR_S *pstNRAttr);

/**
 * @brief      获取噪声抑制参数
 * @param[in]  ViPipe:    VI PIPE号
 * @param[out] pstNRAttr: 噪声抑制参数
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetNRAttr(VI_PIPE ViPipe, ISP_NR_ATTR_S *pstNRAttr);

/**
 * @brief      设置Demosaic参数
 * @param[in]  ViPipe:          VI PIPE号
 *             pstDemosaicAttr: Demosaic参数
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetDemosaicAttr(VI_PIPE ViPipe, const ISP_DEMOSAIC_ATTR_S *pstDemosaicAttr);

/**
 * @brief      获取Demosaic参数
 * @param[in]  ViPipe:          VI PIPE号
 * @param[out] pstDemosaicAttr: Demosaic参数
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetDemosaicAttr(VI_PIPE ViPipe, ISP_DEMOSAIC_ATTR_S *pstDemosaicAttr);

/**
 * @brief      设置黑电平属性
 * @param[in]  ViPipe:        VI PIPE号
 *             pstBlackLevel: 黑电平属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetBlackLevelAttr(VI_PIPE ViPipe, const ISP_BLACK_LEVEL_S *pstBlackLevel);

/**
 * @brief      获取黑电平属性
 * @param[in]  ViPipe:        VI PIPE号
 * @param[out] pstBlackLevel: 黑电平属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetBlackLevelAttr(VI_PIPE ViPipe, ISP_BLACK_LEVEL_S *pstBlackLevel);

/**
 * @brief      设置帧合成参数
 * @param[in]  ViPipe:       VI PIPE号
 *             pstFSWDRAttr: 帧合成参数
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetFSWDRAttr(VI_PIPE ViPipe, const ISP_WDR_FS_ATTR_S *pstFSWDRAttr);

/**
 * @brief      获取帧合成参数
 * @param[in]  ViPipe:       VI PIPE号
 * @param[out] pstFSWDRAttr: 帧合成参数
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetFSWDRAttr(VI_PIPE ViPipe, ISP_WDR_FS_ATTR_S *pstFSWDRAttr);

/**
 * @brief      设置去雾属性
 * @param[in]  ViPipe:        VI PIPE号
 *             pstDehazeAttr: 去雾属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetDehazeAttr(VI_PIPE ViPipe, const ISP_DEHAZE_ATTR_S *pstDehazeAttr);

/**
 * @brief      获取去雾属性
 * @param[in]  ViPipe:        VI PIPE号
 * @param[out] pstDehazeAttr: 去雾属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetDehazeAttr(VI_PIPE ViPipe, ISP_DEHAZE_ATTR_S *pstDehazeAttr);

/**
 * @brief      设置3D降噪属性
 * @param[in]  ViPipe:      VI PIPE号
 *             pstNrXParam: 3D降噪属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetPipeNRXParam(VI_PIPE ViPipe, const ISP_PIPE_NRX_PARAM_S *pstNrXParam);

/**
 * @brief      获取3D降噪属性
 * @param[in]  ViPipe:      VI PIPE号
 * @param[out] pstNrXParam: 3D降噪属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetPipeNRXParam(VI_PIPE ViPipe, ISP_PIPE_NRX_PARAM_S *pstNrXParam);

/**
 * @brief      设置色彩空间转换属性
 * @param[in]  ViPipe:     VI PIPE号
 *             pstCSCAttr: CSC属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetCSCAttr(VI_PIPE ViPipe, const ISP_CSC_ATTR_S *pstCSCAttr);

/**
 * @brief      获取色彩空间转换属性
 * @param[in]  ViPipe:     VI PIPE号
 * @param[out] pstCSCAttr: CSC属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetCSCAttr(VI_PIPE ViPipe, ISP_CSC_ATTR_S *pstCSCAttr);

/**
 * @brief      设置ISP调试信息属性
 * @param[in]  ViPipe:      VI PIPE号
 *             pstIspDebug: ISP调试信息属性
 * @param[out] 无
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_SetDebug(VI_PIPE ViPipe, const ISP_DEBUG_INFO_S *pstIspDebug);

/**
 * @brief      获取ISP调试信息属性
 * @param[in]  ViPipe:      VI PIPE号
 * @param[out] pstIspDebug: ISP调试信息属性
 * @return     SC_SUCCESS或其它
 * @retval     SC_SUCCESS: 成功
 * @retval     其它:         失败
 */
SC_S32 SC_MPI_ISP_GetDebug(VI_PIPE ViPipe, ISP_DEBUG_INFO_S *pstIspDebug);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __MPI_ISP_H__ */
