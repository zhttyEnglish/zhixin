#include "vo/display_hal/display_hal.h"
#include "vo/sc_display.h"
#include "vo/util.h"

const STRU_DISP_RES  disp_res_tbl[SC_HAL_VO_OUTPUT_USER] = {
	{720, 576, 25, SC_SYSTEM_INTERLACE_MOD_720_576I_PAL}, /* PAL */
	{720, 480, 30, SC_SYSTEM_INTERLACE_MOD_720_480I_NTSC},/* NTSC */

	{1920, 1080, 24, SC_SYSTEM_INTERLACE_MOD_NULL},       /* 1920 x 1080 at 24 Hz. */
	{1920, 1080, 25, SC_SYSTEM_INTERLACE_MOD_NULL},       /* 1920 x 1080 at 25 Hz. */
	{1920, 1080, 30, SC_SYSTEM_INTERLACE_MOD_NULL},       /* 1920 x 1080 at 30 Hz. */
	{1920, 1080, 50, SC_SYSTEM_INTERLACE_MOD_NULL},       /* 1920 x 1080 at 50 Hz. */
	{1920, 1080, 60, SC_SYSTEM_INTERLACE_MOD_NULL},       /* 1920 x 1080 at 60 Hz. */
	{1920, 1080, 48, SC_SYSTEM_INTERLACE_MOD_1080I},      /* 1920 x 1080I at 48 Hz. */
	{1920, 1080, 50, SC_SYSTEM_INTERLACE_MOD_1080I},      /* 1920 x 1080I at 50 Hz. */
	{1920, 1080, 60, SC_SYSTEM_INTERLACE_MOD_1080I},      /* 1920 x 1080I at 60 Hz. */

	{1280, 720, 25, SC_SYSTEM_INTERLACE_MOD_NULL},        /* 1280 x  720 at 25 Hz. */
	{1280, 720, 30, SC_SYSTEM_INTERLACE_MOD_NULL},        /* 1280 x  720 at 30 Hz. */
	{1280, 720, 50, SC_SYSTEM_INTERLACE_MOD_NULL},        /* 1280 x  720 at 50 Hz. */
	{1280, 720, 60, SC_SYSTEM_INTERLACE_MOD_NULL},        /* 1280 x  720 at 60 Hz. */

	{720, 576, 50, SC_SYSTEM_INTERLACE_MOD_NULL},         /* 720  x  576 at 50 Hz. */
	{720, 480, 60, SC_SYSTEM_INTERLACE_MOD_NULL},         /* 720  x  480 at 60 Hz. */

	{800,  600,  60, SC_SYSTEM_INTERLACE_MOD_NULL},       /* VESA 800 x 600 at 60 Hz */
	{1024, 600,  60, SC_SYSTEM_INTERLACE_MOD_NULL},       /* 1024 x 600 at 60 Hz */
	{1280, 1024, 60, SC_SYSTEM_INTERLACE_MOD_NULL},       /* VESA 1280 x 1024 at 60 Hz*/
	{1280, 768,  60, SC_SYSTEM_INTERLACE_MOD_NULL},       /* 1280*768@60Hz VGA@60Hz*/
	{640,  480,  60, SC_SYSTEM_INTERLACE_MOD_NULL},       /* VESA 640 x 480 at 60 Hz*/
	{3840, 2160, 60, SC_SYSTEM_INTERLACE_MOD_NULL},       /* 3840x2160_60 */
	{800,  480,  60, SC_SYSTEM_INTERLACE_MOD_NULL}        /* 1920 x 1080 at 60 Hz. */
};

const STRU_DISP_RES *display_hal_get_res_tbl(ENUM_SC_HAL_VO_DEV_TIMING_TEMPLATE e_timing)
{
	return (e_timing < SC_HAL_VO_OUTPUT_USER) ? &disp_res_tbl[e_timing] : NULL;
}

int sc_display_hal_csc_trans(ENUM_SC_HAL_VO_CSC vo_csc)
{
	int sc_csc = SC_VIDEO_MATRIX_NODATA_LIMIT;

	switch (vo_csc) {
	case SC_HAL_VO_CSC_BT709_LIMIT:
		sc_csc = SC_VIDEO_MATRIX_BT709_LIMIT;
		break;

	case SC_HAL_VO_CSC_BT709_FULL:
		sc_csc = SC_VIDEO_MATRIX_BT709_LIMIT;
		break;

	case SC_HAL_VO_CSC_BT601_LIMIT:
		sc_csc = SC_VIDEO_MATRIX_BT601_LIMIT;
		break;

	case SC_HAL_VO_CSC_BT601_FULL:
		sc_csc = SC_VIDEO_MATRIX_BT601_FULL;
		break;

	default:
		break;
	}

	return sc_csc;
}

int sc_display_hal_format_trans(ENUM_SC_HAL_VO_FMT vo_csc)
{
	int sc_format = Input_YV12;

	switch (vo_csc) {
	case SC_HAL_VO_FMT_ARGB1555:
		sc_format = Input_ARGB1555;
		break;

	case SC_HAL_VO_FMT_RGB565:
		sc_format = Input_RGB565;
		break;

	case SC_HAL_VO_FMT_ARGB8888:
		sc_format = Input_ARGB8888;
		break;

	case SC_HAL_VO_FMT_YV12:
		sc_format = Input_YV12;
		break;

	default:
		break;
	}

	return sc_format;
}

