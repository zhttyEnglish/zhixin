
#ifndef _DISPLAY_TYPE_H_
#define _DISPLAY_TYPE_H_

#include <stdlib.h>
//#include <stdint.h>
#include "vo/sc_display.h"
#include "vo/interface/display_interface.h"
#include "vo/util.h"
#include "vo/display_hal/hal_vo.h"
#include "vo/display_hal/display_hal.h"

#define DISPLAY_HW_BURST_NUM 16
#define Y_UV_DIV_FACTOR  2

typedef enum {
	VO_HAL_MODE_LEGACY,
	VO_HAL_MODE_MPP
} ENUM_VO_MODE;

typedef enum _ROT_ANGLE_e {
	ROT_ROT0,
	ROT_FLIP_X,
	ROT_FLIP_Y,
	ROT_FLIP_XY, // 3
	ROT_ROT90,
	ROT_ROT180,
	ROT_ROT270,  // 6
	ROT_MAX
} ROT_ANGLE_e;

typedef enum _TileMode_e {
	TileMode_LINEAR, //00
	TileMode_TILED4X4, //01
	TileMode_SUPER_TILED_XMAJOR, //02
	TileMode_SUPER_TILED_YMAJOR, //03
	TileMode_TILE_MODE0, //04
	TileMode_TILE_MODE1, //05
	TileMode_TILE_MODE2, //06
	TileMode_TILE_MODE3, //07
	TileMode_TILE_MODE4, //08
	TileMode_TILE_MODE5, //09
	TileMode_TILE_MODE6, //0A
	TileMode_SUPER_TILED_XMAJOR8X4, //0B
	TileMode_SUPER_TILED_YMAJOR4X8, //0C
	TileMode_TILE_Y, //0D
	TileMode_MAX
} TileMode_e;

typedef enum _Swizzle_e {
	Swizzle_ARGB,
	Swizzle_RGBA,
	Swizzle_ABGR,
	Swizzle_BGRA,
	Swizzle_MAX
} Swizzle_e;

/*
 * Alpha blending enumeration define
 */
typedef enum _AlphaMode_e {
	AlphaMode_NORMAL,
	AlphaMode_INVERSED,
	AlphaMode_MAX
} AlphaMode_e;

typedef enum _GlobalAlphaMode_e {
	GlobalAlphaMode_NORMAL,
	GlobalAlphaMode_GLOBAL,
	GlobalAlphaMode_SCALED,
	GlobalAlphaMode_MAX
} GlobalAlphaMode_e;

typedef enum _BlendingMode_e {
	Blend_ZERO,
	Blend_ONE,
	Blend_NORMAL,
	Blend_INVERSED,
	Blend_COLOR,
	Blend_COLOR_INVERSED,
	Blend_SATURATED_ALPHA,
	Blend_SATURATED_DEST_ALPHA,
	Blend_MAX
} BlendingMode_e;

/*
 * The format of the framebuffer
 */
typedef display_format_t InputFormat_e;

/*
 * DPI interface data format
 */
typedef enum _OutputFormat_e {
	DPI_D16CFG1,
	DPI_D16CFG2,
	DPI_D16CFG3,
	DPI_D18CFG1,
	DPI_D18CFG2,
	DPI_D24,
	DPI_D30,    // ARGB2101010 ?????????????
	DPI_MAX
} OutputFormat_e;

/*
 * Cursor type.
 */
typedef enum _CursorFormat_e {
	CursorFormat_DISABLED,
	CursorFormat_MASKED,
	CursorFormat_ARGB8888,
	CursorFormat_MAX
} CursorFormat_e;

typedef enum _dc_status_type {
	dcSTATUS_TIMEOUT = -5,
	dcSTATUS_NOT_SUPPORT = -4,
	dcSTATUS_OOM = -3,
	dcSTATUS_FAILED = -2,
	dcSTATUS_INVALID_ARGUMENTS = -1,

	dcSTATUS_OK = 0,
} dc_status_type;

typedef struct {
	uint32_t size;
	uint32_t width;
	uint32_t height;
	uint16_t planes;
	uint16_t bit_count;
	uint32_t compression;
	uint32_t size_img;
	uint32_t x_pelspermeter;
	uint32_t y_pelspermeter;
	uint32_t clrused;
	uint32_t clrimportant;
} bit_map_infor_header_t;

typedef struct {
	bit_map_infor_header_t header;
} bit_map_header_t;

typedef struct {
	uint8_t width;
	uint8_t height;
	uint8_t color_count;
	uint8_t reserved;
	uint16_t planes;
	uint16_t bit_count;
	uint32_t byte_count;
	uint32_t offset;
} icon_dir_entry_t;

typedef struct {
	uint16_t reserverd;
	uint16_t type;
	uint16_t count;
} __attribute__((aligned(1))) icon_header_t ;

#define vivFALSE                0
#define vivTRUE                 1

//#define NULL              0

/************************************************************************/

/* For function enable/disable parameter */
#define SET_ENABLE                                  1
#define SET_DISABLE                                 0

/* For polarity parameter */
#define SET_POSITIVE                                0
#define SET_NEGATIVE                                1

#define GAMMA_TABLE_SIZE 256
#define DC_OVERLAY_NUM 1

#define vivINFINITE ((unsigned int)(~0U))
#define VIV_ALIGN(data, offset)                 ((data + offset - 1) & ~(offset - 1))

#ifndef NULL
	#define NULL            ((void *)0)
#endif

typedef struct _Framebuffer_t {
	unsigned int fb_phys_addr[3];
	unsigned int fb_stride[3];

	unsigned int colorKey;
	unsigned int colorKeyHigh;

	// dcregFrameBufferConfig
	int valid;
	unsigned int clearFB;
	unsigned int transparency;
	ROT_ANGLE_e rotAngle;
	unsigned int fb_yuv_standard;
	TileMode_e tileMode;
	unsigned int scale;
	Swizzle_e swizzle;
	unsigned int uvSwizzle;
	InputFormat_e fb_format;

	// dcregFrameBufferScaleConfig
	unsigned int filterTap;
	unsigned int horizontalFilterTap;

	unsigned int bgColor;

	/* Original size in pixel before rotation and scale. */
	// dcregFrameBufferSize
	unsigned int width;
	unsigned int height;

	unsigned int scaleFactorX;
	unsigned int scaleFactorY;

	// dcregFrameBufferClearValue
	unsigned int clearValue;

	unsigned int initialOffsetX;
	unsigned int initialOffsetY;

	int nPlanes;
	int planeHeight[3];
} Framebuffer_t;

typedef struct _Overlay_t {
	// dcregOverlayConfig
	unsigned int transparency;
	ROT_ANGLE_e rotAngle;
	int yuv_standard;
	TileMode_e tileMode;
	Swizzle_e swizzle;
	unsigned int uvSwizzle;
	InputFormat_e format;
	unsigned int clearOverlay;
	int enable;
	// dcregOverlayAlphaBlendConfig
	AlphaMode_e srcAlphaMode;
	GlobalAlphaMode_e srcGlobalAlphaMode;
	BlendingMode_e scrBlendingMode;
	int srcAlphaFactor;

	AlphaMode_e dstAlphaMode;
	GlobalAlphaMode_e dstGlobalAlphaMode;
	BlendingMode_e dstBlendingMode;
	int dstAlphaFactor;

	// dcregOverlayAddress
	unsigned int address[3];
	unsigned int stride[3];

	unsigned int tlX;
	unsigned int tlY;
	unsigned int brX;
	unsigned int brY;

	unsigned int srcGlobalColor;
	unsigned int dstGlobalColor;

	unsigned int colorKey;
	unsigned int colorKeyHigh;

	// dcregOverlaySize
	/* Window size of overlay buffer in memory in pixels.
	 * If the overlay is rotated, this size may be different from size of overlay displayed.
	*/
	unsigned int width;
	unsigned int height;

	unsigned int clearValue;

	// dcregOverlayIndexColorTableData
	unsigned int lut[256];
} Overlay_t;

typedef struct _DCUltraL_t {
	/*frame buffer*/
	Framebuffer_t framebuffer;

	/* dither */
	int dither_enable;

	unsigned int dither_red_channel;
	unsigned int dither_green_channel;
	unsigned int dither_blue_channel;

	unsigned int dither_table_low;
	unsigned int dither_table_high;

	/* panel configuration */
	int panel_de_en;
	int panel_da_en;
	int panel_clock_en;
	int panel_de_polarity;
	int panel_da_polarity;
	int panel_clock_polarity;

	unsigned int panel_hline, panel_htotal;
	unsigned int panel_hsync_start, panel_hsync_end;
	int panel_hsync_polarity;

	unsigned int panel_vline, panel_vtotal;
	unsigned int panel_vsync_start, panel_vsync_end;
	int panel_vsync_polarity;

	float fps;
	/* gamma correction */
	int gamma_enable;

	//    unsigned int gamma[GAMMA_TABLE_SIZE][3];

	/* Cursor  */
	CursorFormat_e cursor_format; // include enable/disable
	int display_controller; // 0=>display0, 1=>display1
	unsigned int hot_spot_x; // Vertical offset to cursor hotspot.
	unsigned int hot_spot_y; // Horizontal offset to cursor hotspot.
	unsigned int alpha_factor;
	unsigned int flip_in_process;

	int loc_x, loc_y; // location

	unsigned int bg_color,
	         fg_color;

	void *cursor_addr;
	void *cursor_phy_addr;

	// These 4 parameters are used by software to clear cursor
	unsigned int    cursor_width;
	unsigned int    cursor_height;

	/* output/DPI configuration */
	int output_enable;

	OutputFormat_e output_dpi_format;

	/* Interrupt & Gating Registers */
	int intr_disp0_en;
	int intr_disp1_en;

	/* Filter and Index registers */
	unsigned int lut[256];

	unsigned int horKernel[128];
	unsigned int verKernel[128];
	unsigned int irq;
	Overlay_t overlay;
	int overlay_id;
	void *priv;
	int zoom_addr_offset[3];
	int dvp_out_en;
	int dsi_out_en;
	int rgb2yuv_en;
	int noising_en;
	int cea864_timing_en;
	int out_format;
	int low_noising_en;
	int blankvalule_en;
	int use_dsi_halt_en;
	int data_fifo_clr;
} DCUltraL_t;

typedef void (*DisNotify)(void *user_data);

typedef struct {
	int      use_ppm;
	uint32_t pix_fre_Khz;
	int32_t  total_step_num;
	int32_t  fine_step_ppm_or_time_ps;
	int32_t  adjust_skip_times_ms_or_frames;
	uint32_t adjust_percent_th_high;
	uint32_t adjust_percent_th_low;
	uint32_t deta_th_high;
	uint32_t deta_th_low;
} fre_adjust_pra;

typedef struct {
	DCUltraL_t dc_inst;
	int overlay_id;
	int init;
	int start_flag;
	int fre_adjust_flag;
	int fre_adjust_hw_en;
	uint64_t dis_irq_count;
	float fps_stats;
	float push_fps;
	int src_push_cnt;
	uint32_t src_push_first_time;
	float input_fps;
	int  frame_count;
	unsigned int dis_frame_num;
	unsigned int top_field_cnt;
	unsigned int bot_field_cnt;
	unsigned int drop_frame_cnt;
	uint32_t dis_loss_count;
	uint32_t repeat_count;
	int loss_come_too_much;
	int repeat_not_match;
	unsigned int repeat_next_null;
	void *disp_queue;
	void *disp_done_queue;
	void *hold_queue_odd;
	void *hold_queue_even;
	display_buffer_t *current_buf;
	display_buffer_t *next_buf;
	int interlace_mod;
	int current_display_interface;
	display_interface_desc_t desc;
	display_interface_res_infor_t res_infor;
	disp_res_infor_t   res_timing;
	int res_index;

	uint32_t stats_frame_count;
	uint32_t stats_frame_count_th;
	uint32_t adjust_percent_th_high;
	uint32_t adjust_percent_th_low;
	uint32_t exception_low_count;
	uint32_t exception_high_count;
	uint32_t normal_count;
	uint32_t deta_err_count_too_large;
	uint32_t deta_err_count_low_zero;

	uint64_t next_buf_cfg_time;
	int64_t next_buf_cfg_lock_deta;
	int64_t deta_th_low;
	int64_t deta_th_high;
	int64_t deta_th_err;
	uint64_t vsync_interval;
	uint64_t average_low_time;
	uint64_t average_high_time;
	uint64_t adjust_step;
	uint64_t average_normal_time;
	int64_t average_deta;
	int64_t current_time;
	int64_t dest_time;
	int64_t init_time;
	float fre_adjust_th_high;
	float fre_adjust_th_low;
	int   log_fre_adjust;
	int   log_fre_adjust_hist;
	int   stats_count;
	int   enable_fine_adjust;
	int64_t   fine_adjust_step;
	int   fine_adjust_count;
	int   fine_adjust_skip;
	int   fine_adjust_skip_num;
	int   fine_adjust_delay_ms;
	int   use_windows;
	int enable_av_freq_sync;
	int enable_fifo_mode;
	int fifo_depth;
	int interlace_depth;
	int fre_group_count;
	fre_adjust_pra adjsut_pra;
	uint32_t deta_every_1ms_history[100];
	uint32_t adjust_status;
	uint32_t return_adjust_flag;
	int      low_fine_adjusting;
	int      low_adjusting;
	int      high_fine_adjusting;
	int      high_adjusting;
	int      idleing;
	uint32_t deta_every_1ms_history_log[100];
	int low_adjust_count;
	int hight_adjust_count;
	int return_adjust_count;
	int64_t average_deta_log;
	uint32_t stats_frame_count_log;
	uint32_t exception_low_count_log;
	uint32_t exception_high_count_log;
	uint32_t normal_count_log;
	display_buffer_t frame_buf4k2k;
	int width;
	int height;
	int out_width;
	int out_height;
	int format;
	float fps;
	int y_stride;
	int cbcr_stride;
	DisNotify dis_notify;
	int is_first_frame;
	int enable_irq_process;
	uint64_t first_time;
	uint64_t first_deta;
	void  *display_post_proc;
	void  *frame_buffer_vsync;
	int (*flush_display_queue)(void *usr_data);
	void *usr_data;
	sc_lock_t queue_lock;
	void *priv;
	sc_queue_t *deta_queue;
	sc_queue_t *small_step_req_queue;
	sc_signal_t small_req_singal;
	int64_t deta_data[1024];
	int invert_interlace;
	volatile int stop_req;

	unsigned int cpld_interlace_pin_num;
	unsigned int cpld_sd_hd_pin_num;
	int overlay_x;
	int overlay_y;
	display_buffer_t *background;
	int use_background;
	int ui_max_len;
	int ui_color;
	int ui_w;
	int ui_h;
	int ui_fps;
	int ui_hline;
	int ui_vline;
	int csc_pra;

	STRU_DISP_LAYER  hal_layer;
} disp_obj_t;
typedef struct {
	sc_lock_t        lock;
	uint32_t         mode;
	STRU_DISP_DEV    vo_dev;
	STRU_DISP_LAYER  vo_layer[SC_HAL_VO_LAYER_ID_MAX];
} display_hal_t;

typedef struct {
	display_handle_t handle[2];
	display_hal_t    hal;
	sc_lock_t        lock;
	int              ui_mode;
} display_server_t;

typedef struct {
	uint64_t  current_time;
	uint64_t  dest_time;
} small_req_data_t;

typedef struct {
	int width;
	int height;
	int y_stride;
	int cbcr_stride;
	int format;
	int overlay_x;
	int overlay_y;
	int fps;
} display_dev_data_t;

typedef struct {
	ENUM_SC_HAL_VO_DEV_ID e_dev_id;
} vo_dev_data_t;

typedef struct {
	ENUM_SC_HAL_VO_LAYER_ID e_layer_id;
} vo_layer_data_t;

typedef struct {
	unsigned int Yr;
	unsigned int Ur;
	unsigned int Vr;
	unsigned int Cr;
	unsigned int Yg;
	unsigned int Ug;
	unsigned int Vg;
	unsigned int Cg;
	unsigned int Yb;
	unsigned int Ub;
	unsigned int Vb;
	unsigned int Cb;
	unsigned int YuvClipEn;
	unsigned int YClip;
	unsigned int UClip;
	unsigned int VClip;
} STRU_DC_CSC;

typedef struct {
	unsigned int Yr;
	unsigned int Ur;
	unsigned int Vr;
	unsigned int Cr;
	unsigned int Yg;
	unsigned int Ug;
	unsigned int Vg;
	unsigned int Cg;
	unsigned int Yb;
	unsigned int Ub;
	unsigned int Vb;
	unsigned int Cb;
	unsigned int YuvClipEn;
	unsigned int YClipLow;
	unsigned int YClipHigh;
	unsigned int UClipLow;
	unsigned int UClipHigh;
	unsigned int VClipLow;
	unsigned int VClipHigh;
} STRU_DC_CSC_VALUE;

void dc_display_start(disp_obj_t *obj);
void dc_display_pause(disp_obj_t *obj);
void dc_display_reset(disp_obj_t *obj);
int dc_cfg_qos(disp_obj_t *obj);
void dc_set_frameaddr(
    DCUltraL_t *dc,
    unsigned int address
);
void dc_set_frameaddr2(
    DCUltraL_t *dc,
    display_buffer_t *buffer
);
void dc_set_overlay_pos(
    DCUltraL_t *dc,
    int x, int y
);

void dc_set_framebuffer(
    DCUltraL_t *dc
);

void dc_set_zoom(
    DCUltraL_t *dc, float zoom
);

int dc_set_layer_csc(ENUM_SC_HAL_VO_LAYER_ID LayerId, ENUM_SC_HAL_VO_CSC csc_type);
int dc_set_output_csc(int csc_type);

int dc_set_panel(
    DCUltraL_t *dc,
    int de_en,
    int da_en,
    int clock_en,
    int de_polarity,
    int da_polarity,
    int clock_polarity,
    unsigned int hsync_polarity,
    unsigned int vsync_polarity,
    unsigned int hline,
    unsigned int vline,
    float fps
);
float dc_power_on(DCUltraL_t *dc);
int  dc_set_panel2(
    DCUltraL_t *dc,
    int de_en,
    int da_en,
    int clock_en,
    int de_polarity,
    int da_polarity,
    int clock_polarity,
    unsigned int hsync_polarity,
    unsigned int vsync_polarity,
    disp_res_infor_t  *res
);

/* Set output.
 * enable:   Enable output or not.
 * format:   DPI data format.
 *           DPI_D16CFG1/DPI_D16CFG2/DPI_D16CFG3/DPI_D18CFG1/DPI_D18CFG2/DPI_D24/DPI_D30
 */
void dc_set_output(
    DCUltraL_t *dc,
    int enable,
    OutputFormat_e dpi_format
);

void dc_set_dither(
    DCUltraL_t *dc,
    int enable,
    InputFormat_e fb_format,
    unsigned int low,
    unsigned int high
);

void dc_clear_dither(
    DCUltraL_t *dc
);

void dc_set_gamma(
    DCUltraL_t *dc,
    int enable,
    unsigned int (*gamma_tab)[3]
);

void dc_set_cursor(
    DCUltraL_t *dc,
    unsigned int width,
    unsigned int height,
    unsigned int x,
    unsigned int y,
    unsigned int fg,
    unsigned int bg,
    char *p_cursor
);

void dc_move_cursor(
    DCUltraL_t *dc,
    unsigned int x,
    unsigned int y
);
void dc_get_cursor(
    DCUltraL_t *dc,
    unsigned int *x,
    unsigned int *y
);
void dc_cursor_disable(DCUltraL_t *dc);
void dc_cursor_enable(DCUltraL_t *dc);

void dc_set_overlay(disp_obj_t *obj);
void dc_init(
    disp_obj_t *obj,
    DisNotify notify
);
void dc_deinit(disp_obj_t *obj);
void dc_reg_check(DCUltraL_t *dc);
int sc_disp_get_buffer_width(uint32_t width);
int sc_disp_get_buffer_width_div2(uint32_t width);

#endif

