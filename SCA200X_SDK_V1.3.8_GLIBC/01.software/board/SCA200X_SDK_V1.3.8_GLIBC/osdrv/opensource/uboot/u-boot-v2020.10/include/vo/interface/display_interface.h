#ifndef __DISPLAY_INTERFACE_H__
#define __DISPLAY_INTERFACE_H__

#include <stdlib.h>
#include <stdint.h>
#include <vo/util.h>
#define MAX_SUPPORTED_RES 64

/* For function enable/disable parameter */
#define ENABLE_SIGNAL                                  1
#define DISABLE_SIGNAL                                 0

/* For polarity parameter */
#define POSITIVE_ACTIVE                                0
#define NEGATIVE_ACTIVE                                1

enum {
	DISPLAY_INTERFACE_NONE,
	DISPLAY_INTERFACE_DVP,
	DISPLAY_INTERFACE_DP,
	DISPLAY_INTERFACE_MIPI,
	DISPLAY_INTERFACE_MAX,
};

enum {
	DVP_DEV_NONE = 0,
	DVP_DEV_VGA,
	DVP_DEV_HDMI,
	DVP_DEV_NVP6021,
	DVP_DEV_TP2803,
	DVP_DEV_HX8264,
	DVP_DEV_TC3587,
	DVP_DEV_INVALID
};

enum {
	MIPI_DEV_NONE = 0,
	MIPI_DEV_EK79007,
	MIPI_DEV_MAX
};

enum {
	DP_DEV_COMMON,
	DP_DEV_MAX,
};

enum {
	COLOR_MODE_YUV,
	COLOR_MODE_RGB565,
	COLOR_MODE_RGB888,
};

enum {
	SYNC_MODE_EXTERNAL,
	SYNC_MODE_INTERNAL,
};

enum {
	SET_DVP_OUT_CSC_PRA,
	DISPLAY_INTERFACE_CTL_SET_HDMI_CSC,
	DISPLAY_INTERFACE_CTL_SET_INDEX,
	DISPLAY_INTERFACE_CTL_MAX,
};

enum {
	SC_DVP_OUT_CSC_BT709_LIMIT, //bt709 limit
	SC_DVP_OUT_CSC_BT709_FULL,  //bt709 full range
	SC_DVP_OUT_CSC_BT601_LIMIT, //bt601 limit
	SC_DVP_OUT_CSC_BT601_FULL,   //bt601 full
	SC_DVP_OUT_CSC_DEFAULT,
};

typedef struct {
	uint32_t Y_offset[2];
	uint32_t UV_offset[2];
	float coeff[9];
} dvp_csc_pra_t;

typedef enum {
	SC_HAL_PIN_MODE_BT1120_DEFAULT         =  0,
	SC_HAL_PIN_MODE_BT1120_CBY_CRY         =  1,
	SC_HAL_PIN_MODE_BT1120_YCB_YCR         =  2,
	SC_HAL_PIN_MODE_BT1120_CRY_CBY         =  3,
	SC_HAL_PIN_MODE_BT1120_YCR_YCB         =  4,
} ENUM_SC_HAL_PIN_MODE_BT1120;

typedef enum {
	SC_HAL_PIN_MODE_BT656_DEFAULT          =  0,
	SC_HAL_PIN_MODE_BT656_YCR_YCB_H        =  1,
	SC_HAL_PIN_MODE_BT656_YCB_YCR_H        =  2,
	SC_HAL_PIN_MODE_BT656_CRY_CBY_H        =  3,
	SC_HAL_PIN_MODE_BT656_CBY_CRY_H        =  4,

	SC_HAL_PIN_MODE_BT656_YCB_YCR_L        =  5,
	SC_HAL_PIN_MODE_BT656_YCR_YCB_L        =  6,
	SC_HAL_PIN_MODE_BT656_CRY_CBY_L        =  7,
	SC_HAL_PIN_MODE_BT656_CBY_CRY_L        =  8,
} ENUM_SC_HAL_PIN_MODE_BT656;

typedef enum {
	SC_HAL_PIN_MODE_RGB888_DEFAULT         =  0,
	SC_HAL_PIN_MODE_RGB888_GRB             =  1,
	SC_HAL_PIN_MODE_RGB888_RGB             =  2,
	SC_HAL_PIN_MODE_RGB888_BRG             =  3,
	SC_HAL_PIN_MODE_RGB888_RBG             =  4,
	SC_HAL_PIN_MODE_RGB888_BGR             =  5,
	SC_HAL_PIN_MODE_RGB888_GBR             =  6
} ENUM_SC_HAL_PIN_MODE_RGB888;

typedef enum {
	SC_HAL_PIN_MODE_RGB565_DEFAULT         =  0,
	SC_HAL_PIN_MODE_RGB565_GRB             =  1,
	SC_HAL_PIN_MODE_RGB565_BGR             =  2
} ENUM_SC_HAL_PIN_MODE_RGB565;

typedef union {
	ENUM_SC_HAL_PIN_MODE_BT656   bt656;
	ENUM_SC_HAL_PIN_MODE_BT1120  bt1120;
	ENUM_SC_HAL_PIN_MODE_RGB888  rgb888;
	ENUM_SC_HAL_PIN_MODE_RGB565  rgb565;
} ENUM_SC_HAL_PIN_MODE;

typedef struct {
	int out_color_mode;
	int data_width;
	int de_en;
	int da_en;
	int clock_en;
	int de_polarity;
	int da_polarity;
	int clock_polarity;
	unsigned int hsync_polarity;
	unsigned int vsync_polarity;
	ENUM_SC_HAL_PIN_MODE pin_mode;
	int is_bt656;
} display_interface_desc_t;

typedef struct {
	int h_valid_pixels;
	int h_total_pixels;
	int start_hsync;
	int end_hsync;

	int v_valid_lines;
	int v_total_lines;
	int start_vsync;
	int end_vsync;
	float fps;
} disp_res_infor_t;

typedef struct {
	int is_fix_res_out;
	int fix_res_index;
	int res_count;
	disp_res_infor_t res[MAX_SUPPORTED_RES];
	int bit_count_per_channel;
} display_interface_res_infor_t;

typedef struct {
	int format;
	int color_matric;
	int change_range;
} hdmi_csc_t;

int set_display_interface(int interface_type, int subintf);
int get_current_display_interface(void);
int init_display_interface(int interface_type);
int power_on_display_interface(int interface_type);
int power_off_display_interface(int interface_type);
int reset_display_interface(int interface_type);
int get_display_interface_desc(int interface_type, display_interface_desc_t *desc);
int get_display_interface_res_infor(int interface_type, display_interface_res_infor_t *res_infor);
int set_display_interface_res_infor(int interfae_type, disp_res_infor_t *res_infor);
int set_display_interface_format(int interfae_type, int format);
int set_display_interface_ctl(int interfae_type, int clt_code, void *pra, int size);

typedef int (*pfn_init_display_interface)(void);
typedef int (*pfn_on_display_interface)(void);
typedef int (*pfn_off_display_interface)(void);
typedef int (*pfn_reset_display_interface)(void);
typedef int (*pfn_get_display_interface_desc)(display_interface_desc_t *desc);
typedef int (*pfn_get_display_interface_res_infor)(display_interface_res_infor_t *res_infor);
typedef int (*pfn_set_display_interface_res_infor)(disp_res_infor_t *res_infor);
typedef int (*pfn_set_display_interface_format)(int format);
typedef int (*pfn_set_display_interface_ctl)(int clt_code, void *pra, int size);
typedef int (*pfn_set_display_interface_index)(int index);

typedef struct {
	int interface_index;
	pfn_set_display_interface_index set_display_interface_index;
	pfn_init_display_interface  init_display_interface;
	pfn_get_display_interface_desc get_display_interface_desc;
	pfn_get_display_interface_res_infor get_display_interface_res_infor;
	pfn_set_display_interface_res_infor set_display_interface_res_infor;
	pfn_on_display_interface on_display_interface;
	pfn_off_display_interface off_display_interface;
	pfn_reset_display_interface reset_display_interface;
	pfn_set_display_interface_format set_display_interface_format;
	pfn_set_display_interface_ctl     set_display_interface_ctl;
} display_intgerface_ops_t;

#endif
