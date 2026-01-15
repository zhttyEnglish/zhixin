#include "vo/interface/display_interface.h"

static display_interface_desc_t g_desc = {
	.out_color_mode = COLOR_MODE_RGB565,
	.data_width = 16,
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
	18,
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
		{1920, 2200, 2008, 2052, 540, 562, 542, 547, 60}, //1080p 60 i
		{1920, 2640, 2448, 2492, 1080, 1125, 1084, 1089, 50},
		{1920, 2640, 2448, 2492, 540, 562, 542, 547, 50}, //1080p 50 i
		{1920, 2640, 2448, 2492, 1080, 1125, 1084, 1089, 50},
		{1920, 2640, 2448, 2492, 1080, 1125, 1084, 1089, 50},
		{1920, 2200, 2008, 2052, 1080, 1125, 1084, 1089, 25},
		{1920, 2640, 2448, 2492, 1080, 1125, 1084, 1089, 30},
		{1920, 2750, 2558, 2602, 1080, 1125, 1084, 1089, 24},
		//720x480i
		{720, 858, 739, 801, 240, 262, 255, 258, 60},
		//720x576i
		{720, 864, 732, 795, 288, 312, 307, 312, 60},
	},
};

static int init_interface_dev()
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
display_intgerface_ops_t dvp_dev_vga = {
	.interface_index = DVP_DEV_VGA,
	.init_display_interface = init_interface_dev,
	.get_display_interface_desc = get_display_interface_desc_dev,
	.get_display_interface_res_infor = get_display_interface_res_infor_dev,
};

