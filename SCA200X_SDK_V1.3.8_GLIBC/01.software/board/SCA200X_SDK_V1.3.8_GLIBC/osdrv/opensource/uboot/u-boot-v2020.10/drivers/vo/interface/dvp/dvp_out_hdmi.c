#include "vo/interface/display_interface.h"
#define IDM_HDMI_640x480p60 0
enum hdmi_tx_timing {
	HDMI_TX_640x480p60 = IDM_HDMI_640x480p60,
	HDMI_TX_480p60,
	HDMI_TX_480p60_16x9,
	HDMI_TX_720p60,
	HDMI_TX_1080i60,
	HDMI_TX_480i60,
	HDMI_TX_480i60_16x9,
	HDMI_TX_1080p60,
	HDMI_TX_576p50,
	HDMI_TX_576p50_16x9,
	HDMI_TX_720p50,
	HDMI_TX_1080i50,
	HDMI_TX_576i50,
	HDMI_TX_576i50_16x9,
	HDMI_TX_1080p50,
	HDMI_TX_1080p24,
	HDMI_TX_1080p25,
	HDMI_TX_1080p30,
};
typedef struct {
	int format;
	int color_matric;
	int change_range;
} hdmi_tx_dev_t;

static hdmi_tx_dev_t hdmi_tx_dev;

static hdmi_tx_dev_t *get_hdmi_dev(void)
{
	return &hdmi_tx_dev;
}
void sc_ite_hdmi_tx_init(void *args);
void hdmi_set_csc_cfg(unsigned int user_itu, unsigned int user_limit, int change_range);
void ite_hdmi_tx_reset_command(void);
void hdmi_tx_set_timing(enum hdmi_tx_timing input_opt, int mode);

static display_interface_desc_t g_desc = {
	.out_color_mode = COLOR_MODE_YUV,
	.data_width = 8,
	.de_en = ENABLE_SIGNAL,
	.da_en = ENABLE_SIGNAL,
	.clock_en = ENABLE_SIGNAL,
	.de_polarity = POSITIVE_ACTIVE,
	.da_polarity = POSITIVE_ACTIVE,
	.clock_polarity = POSITIVE_ACTIVE,
	.hsync_polarity = POSITIVE_ACTIVE,
	.vsync_polarity = POSITIVE_ACTIVE,
};
static display_interface_res_infor_t g_res_infor = {
	/*int is_fix_res_out;*/
	0,
	/*int fix_res_index;*/
	0,
	/*int res_count;*/
	21,
	/*disp_res_infor_t res[MAX_SUPPORTED_RES];*/
	{
		{640, 800, 656, 752, 480, 525, 490, 492, 60},
		{800, 1056, 840, 968, 600, 628, 601, 605, 60},
		{1280, 1664, 1344, 1472, 768, 798, 771, 778, 60},
		{1280, 1680, 1296, 1408, 1024, 1071, 1025, 1028, 60},
		{3840, 4400, 4016, 4104, 2160, 2250, 2168, 2178, 60},
		{1280, 1650, 1390, 1430, 720, 750, 725, 730, 60},
		{1280, 1980, 1720, 1760, 720, 750, 725, 730, 50},
		{1920, 2200, 2008, 2052, 1080, 1125, 1084, 1089, 60},
		//modify for embeded output. timing bank is 0
		{1920, 2200, 1920, 2200, 1080, 1125, 1080, 1125, 60},
		{1920, 2200, 2008, 2052, 540, 562, 542, 547, 60}, //1080p 60 i seperated timming
		//{1920, 2200, 1920, 2200, 540, 562, 540, 562,60}, //1080p 60 i embeded timing
		{1920, 2640, 2448, 2492, 1080, 1125, 1084, 1089, 50},
		{1920, 2640, 2448, 2492, 540, 562, 542, 547, 50}, //1080p 50 i
		{1920, 2750, 2558, 2602, 540, 562, 542, 547, 48}, //1080i 48 i
		{1920, 2640, 2448, 2492, 1080, 1125, 1084, 1089, 50},
		{1920, 2640, 2448, 2492, 1080, 1125, 1084, 1089, 50},
		{1920, 2640, 2448, 2492, 1080, 1125, 1084, 1089, 25},
		//{1920, 2200, 2008, 2052, 1080, 1125, 1084, 1089,30},  // seperated timing
		{1920, 2200, 1920, 2200, 1080, 1125, 1080, 1125, 30},   //embeded timing
		{1920, 2750, 2558, 2602, 1080, 1125, 1084, 1089, 24},
		//720x480i
		//{720, 858, 739, 801, 240, 262, 244, 247,60},    // seperated timming
		{720, 858, 720, 858, 240, 262, 240, 262, 60},       // embeded timing
		//720x576i
		//{720, 864, 732, 795, 288, 312, 290, 293,50},
		{720, 864, 720, 864, 288, 312, 288, 312, 50},
		//720x576p
		{720, 864, 732, 796, 576, 625, 581, 586, 50},
		//720x480p
		{720, 858, 736, 798, 480, 525, 489, 495, 60},
	},
	//output bit count per channel
	8,
};

static int init_interface_dev(void)
{
	//do hmdi init, here do nothing current
	return 0;
}
static int get_display_interface_desc_dev(display_interface_desc_t *desc)
{
	if(desc) {
		*desc = g_desc;
	} else {
		sc_err("null input");
	}
	return 0;
}
static int get_display_interface_res_infor_dev(display_interface_res_infor_t *res_infor)
{
	if(res_infor) {
		*res_infor = g_res_infor;
	} else {
		sc_err("null input");
	}
	return 0;

}
static int sc_cfg_get_dvp_timing(int width, int height, float fps)
{
	enum hdmi_tx_timing input_opt = HDMI_TX_1080p60;
	// config dvp
	if((width == 1920) && (height == 1080)) {
		if(abs(60 - (uint32_t)fps) <= 1) {
			input_opt = HDMI_TX_1080p60;
		} else if (abs(50 - (uint32_t)fps) <= 1) {
			input_opt = HDMI_TX_1080p50;
		} else if (abs(30 - (uint32_t)fps) <= 1) {
			input_opt = HDMI_TX_1080p30;
		} else if (abs(25 - (uint32_t)fps) < 1) {
			input_opt = HDMI_TX_1080p25;
		} else if (abs(24 - (uint32_t)fps) < 1) {
			input_opt = HDMI_TX_1080p24;
		}
	} else if((width == 1280) && (height == 720)) {
		if(abs(60 - (uint32_t)fps) <= 1) {
			input_opt = HDMI_TX_720p60;
		} else if (abs(50 - (uint32_t)fps) <= 1) {
			input_opt = HDMI_TX_720p50;
		}
	} else if((width == 720) && (height == 576)) {
		input_opt = HDMI_TX_576p50;
	} else if((width == 720) && (height == 480)) {
		input_opt = HDMI_TX_480p60;
	} else if((width == 720) && (height == 288)) {
		input_opt = HDMI_TX_576i50;
	} else if((width == 720) && (height == 240)) {
		input_opt = HDMI_TX_480i60;
	} else if((width == 1920) && (height == 540)) {
		if(abs(60 - (uint32_t)fps) <= 1) {
			input_opt = HDMI_TX_1080i60;
		} else if (abs(50 - (uint32_t)fps) <= 1) {
			input_opt = HDMI_TX_1080i50;
		}
	}
	sc_always("dvp timing %d.", input_opt);
	return (int)input_opt;
}

static int set_display_interface_res_infor_dev(disp_res_infor_t *res_infor)
{
	if(res_infor) {
		//  TODO: add interlace type
		hdmi_tx_dev_t *hdmi_tx_dev = get_hdmi_dev();
		unsigned int user_itu = 0;
		unsigned int user_limit = 0;
		int hdmi_format = 0;
		switch(hdmi_tx_dev->color_matric) {
		case SC_VIDEO_MATRIX_NODATA_FULL: {
			user_itu = 2;
			user_limit = 0;
		}
		break;
		case SC_VIDEO_MATRIX_NODATA_LIMIT: {
			user_itu = 2;
			user_limit = 1;
		}
		break;
		case SC_VIDEO_MATRIX_BT601_FULL: {
			user_itu = 0;
			user_limit = 0;
		}
		break;
		case SC_VIDEO_MATRIX_BT601_LIMIT: {
			user_itu = 0;
			user_limit = 1;
		}
		break;
		case SC_VIDEO_MATRIX_BT709_FULL: {
			user_itu = 1;
			user_limit = 0;
		}
		break;
		case SC_VIDEO_MATRIX_BT709_LIMIT: {
			user_itu = 1;
			user_limit = 1;
		}
		break;
		default:
			sc_err("not supported color matric");
			break;
		}
		switch(hdmi_tx_dev->format) {
		case SC_VIDEO_FORMAT_RGB:
			hdmi_format = 0; //HDMI_RGB444;
			break;
		case SC_VIDEO_FORMAT_Y444:
			hdmi_format = 1; //HDMI_YUV444;
			break;
		case SC_VIDEO_FORMAT_Y42B:
			hdmi_format = 2; //HDMI_YUV422;
			break;
		default:
			hdmi_format = 0; //HDMI_RGB444;
		}

		//if(!get_display_dts("ui_mode"))
		{
			//hdmi_format = 2;/*just for hytest*/

			sc_always("orig = %d, user_itu=%d user_limit=%d hdmi_format=%d change_range=%d",
			    hdmi_tx_dev->color_matric, user_itu, user_limit, hdmi_format, hdmi_tx_dev->change_range);
			hdmi_set_csc_cfg(user_itu, user_limit, hdmi_tx_dev->change_range);
			hdmi_tx_set_timing(sc_cfg_get_dvp_timing(res_infor->h_valid_pixels, res_infor->v_valid_lines, res_infor->fps),
			    hdmi_format);
		}

	} else {
		sc_err("null input");
	}
	return 0;
}
static int reset_hdmi_tx_display_interface(void)
{
	ite_hdmi_tx_reset_command();
	return 0;
}
static int set_display_interface_ctl_dev(int clt_code, void *pra, int size)
{
	hdmi_tx_dev_t *hdmi_tx_dev = get_hdmi_dev();
	switch(clt_code) {
	case DISPLAY_INTERFACE_CTL_SET_HDMI_CSC: {
		hdmi_csc_t *csc = (hdmi_csc_t *)pra;
		hdmi_tx_dev->format = csc->format;
		hdmi_tx_dev->color_matric = csc->color_matric;
		hdmi_tx_dev->change_range = csc->change_range;
		sc_always("csc format %d, matrix %d change_range %d",
		    hdmi_tx_dev->format, hdmi_tx_dev->color_matric, hdmi_tx_dev->change_range);
	};
	break;
	default:
		break;
	}
	return 0;
}
display_intgerface_ops_t dvp_dev_hdmi = {
	.interface_index = DVP_DEV_HDMI,
	.init_display_interface = init_interface_dev,
	.get_display_interface_desc = get_display_interface_desc_dev,
	.get_display_interface_res_infor = get_display_interface_res_infor_dev,
	.set_display_interface_res_infor = set_display_interface_res_infor_dev,
	.on_display_interface = NULL,
	.off_display_interface = NULL,
	.reset_display_interface = reset_hdmi_tx_display_interface,
	.set_display_interface_ctl = set_display_interface_ctl_dev,
};
