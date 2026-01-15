#include "vo/interface/display_interface.h"

static display_interface_desc_t g_desc = {
	.out_color_mode = COLOR_MODE_RGB888,
	.data_width = 16,
	.de_en = ENABLE_SIGNAL,
	.da_en = ENABLE_SIGNAL,
	.clock_en = ENABLE_SIGNAL,
	.de_polarity = POSITIVE_ACTIVE,
	.da_polarity = POSITIVE_ACTIVE,
	.clock_polarity = POSITIVE_ACTIVE,
	.hsync_polarity = NEGATIVE_ACTIVE,
	.vsync_polarity = NEGATIVE_ACTIVE,
};
static display_interface_res_infor_t g_res_infor = {
	/*int is_fix_res_out;*/
	1,
	/*int fix_res_index;*/
	0,
	/*int res_count;*/
	1,
	/*disp_res_infor_t res[MAX_SUPPORTED_RES];*/
	{
		{800, 1056, 1010, 1042, 480, 525, 502, 510, 60},
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
display_intgerface_ops_t dvp_dev_hx8264 = {
	.interface_index = DVP_DEV_HX8264,
	.init_display_interface = init_interface_dev,
	.get_display_interface_desc = get_display_interface_desc_dev,
	.get_display_interface_res_infor = get_display_interface_res_infor_dev,
};

