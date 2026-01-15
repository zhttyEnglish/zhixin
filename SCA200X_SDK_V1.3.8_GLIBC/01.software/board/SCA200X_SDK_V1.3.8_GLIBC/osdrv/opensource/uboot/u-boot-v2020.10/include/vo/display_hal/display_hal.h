/**
 * @file display_hal.h
 * @brief  display hal include file
 * @author SmartChip Software Team
 * @version 0.0.1
 * @date 2021/07/19
 * @license 2021-2025, SmartChip. Co., Ltd.
**/
#ifndef __DISPLAY_HAL_H__
#define __DISPLAY_HAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_vo.h"

typedef struct {
	SC_BOOL              enable;
	STRU_SC_HAL_VO_POS   position;
	char                *file_name;
} STRU_DISP_CURSOR;

typedef struct {
	ENUM_SC_HAL_VO_CHN_STATE  e_state;
	STRU_SC_HAL_VO_CHN_ATTR   attr;
	STRU_SC_VO_CHN_PARAM_S    para;
	STRU_SC_HAL_VO_RECT       position;
} STRU_DISP_CHN;

typedef struct {
	SC_BOOL                    enable;
	STRU_SC_HAL_VO_LAYER_ATTR  attr;
	STRU_SC_HAL_VO_POS         position;
	STRU_SC_HAL_VO_LAYER_CSC   csc;
	STRU_DISP_CHN              chn[MAX_VO_CHN_NUM];
} STRU_DISP_LAYER;

typedef struct {
	SC_BOOL                  enable;
	STRU_SC_HAL_VO_DEV_ATTR  attr;
	ENUM_SC_HAL_VO_CSC       e_csc;
	ENUM_SC_HAL_VO_IMAGE     e_image;
	STRU_DISP_CURSOR         cursor;
} STRU_DISP_DEV;

typedef struct {
	SC_S32     width;
	SC_S32     height;
	SC_FLOAT   fps;
	int        interlace;
} STRU_DISP_RES;

const STRU_DISP_RES *display_hal_get_res_tbl(ENUM_SC_HAL_VO_DEV_TIMING_TEMPLATE e_timing);
int sc_display_hal_format_trans(ENUM_SC_HAL_VO_FMT vo_csc);
int sc_display_hal_csc_trans(ENUM_SC_HAL_VO_CSC vo_csc);

#endif

