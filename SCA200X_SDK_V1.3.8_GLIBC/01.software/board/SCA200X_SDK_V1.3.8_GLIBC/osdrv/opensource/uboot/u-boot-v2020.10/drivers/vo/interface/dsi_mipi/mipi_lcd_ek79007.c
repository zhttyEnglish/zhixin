#include "vo/interface/display_interface.h"
#include "vo/interface/dsi_mipi/dsi_reg.h"
#include "vo/interface/dsi_mipi/dphy_reg.h"
#include "vo/interface/dsi_mipi/dsi_mipi_common.h"
#include <fdtdec.h>
#include <asm/arch-sca200v100/gpio.h>

#define lcd_backlight_port               GPIO_PORT_B
#define lcd_backlight_group              GPIO_GROUP_1
#define lcd_backlight_number             0
#define lcd_backlight_pad                30
#define lcd_backlight_pad_gpio_fun_num   2

#define lcd_power_port                   GPIO_PORT_B
#define lcd_power_group                  GPIO_GROUP_1
#define lcd_power_number                 2
#define lcd_power_pad                    32
#define lcd_power_pad_gpio_fun_num       2

#define lcd_standby_port                 GPIO_PORT_A
#define lcd_standby_group                GPIO_GROUP_1
#define lcd_standby_number               6
#define lcd_standby_pad                  28
#define lcd_standby_pad_gpio_fun_num     2

#define lcd_rest_port                    GPIO_PORT_B
#define lcd_rest_group                   GPIO_GROUP_1
#define lcd_rest_number                  1
#define lcd_rest_pad                     31
#define lcd_rest_pad_gpio_fun_num        2

#define lcd_updown_scan_port             GPIO_PORT_D
#define lcd_updown_scan_group            GPIO_GROUP_1
#define lcd_updown_scan_number           7
#define lcd_updown_scan_pad              26
#define lcd_updown_pad_gpio_fun_num      3

#if 0
	#define lcd_shlr_port GPIO_PORT_A
	#define lcd_shlr_group GPIO_GROUP_1
	#define lcd_shlr_number 2
	#define lcd_shlr_pad     24
	#define lcd_shlr_pad_gpio_fun_num 1
#endif

static int get_lcd_cfg(char *name);

static display_interface_desc_t g_desc = {
	.out_color_mode = COLOR_MODE_RGB888,
	.data_width = 24,
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
	1,
	/*int fix_res_index;*/
	0,
	/*int res_count;*/
	1,
	/*disp_res_infor_t res[MAX_SUPPORTED_RES];*/
	{
		//       {1024, 1354, 1184, 1194, 600, 636, 612, 613, 60},
		//      {1024, 1352, 1284, 1292, 600, 635, 613, 623, 60},
		{1024, 1352, 1184, 1192, 600, 645, 612, 622, 60},
		//    {3840, 6040, 5040, 5640, 2160, 2250, 2190, 2220, 60},
	},
	8,
	0
};

static STRU_DSI_CFG g_dsi_cfg = {
	.dev_no         = 0,
	.lane           = {1, 1, 1, 1},
	.bits_per_pixel = 24,
	.sync_info      = {
		.hsa  = 6,
		.hbp  = 120,
		.hact = 1024,
		.hfp  = 120,

		.vsa  = 10,
		.vbp  = 23,
		.vact = 600,
		.vfp  = 12
	},
	.dphy_clk_mhz     = 313.9344,
	.pll_freq_reg_2c0 = 0x32F74FD1,
	.pll_freq_reg_38c = 0x2800
};

static int dcs_cmd_without_pra(int cmd)
{
	return cmd;
}

static int dcs_cmd_without_1pra(int cmd, int pra)
{
	return cmd;
}

static int regw(int addr, int data)
{
	return dcs_cmd_without_1pra(addr, data);
}
static int get_lcd_cfg_init_setting(unsigned int *val, int count)
{
	int offset = -1;
	int ret, status = -1;
	void *blob = board_fdt_blob_setup();
	offset = fdt_node_offset_by_compatible(blob, 0, "tc3587_lcd_cfg");
	if (offset != -1) {
		ret = fdtdec_get_int_array_count(blob, offset, "init_setting", (unsigned int *)val, count * 2);
		sc_debug("ret=%d", ret);
		if(ret) {
			status = ret;
		} else {
			sc_err("%d", __LINE__);
		}
	} else {
		sc_err("%d", __LINE__);
	}
	return status;
}

static int lcd_init(void)
{
	int count = 0;
	unsigned int *val = NULL;
	int ret = 0;
	sc_always("start the lcd");
	count = get_lcd_cfg("init_setting_count");
	if(count > 0) {
		val = sc_malloc(count * sizeof(int) * 2);
		ret = get_lcd_cfg_init_setting(val, count);
		if(ret > 0) {
			int i = 0;
			ret = ret / 2;
			for(i = 0; i < ret; i++) {
				regw(val[i * 2], val[i * 2 + 1]);
				sc_always("add=0x%x data=0x%x", val[i * 2], val[i * 2 + 1]);
			}
		}
		sc_free(val);
	}
	return 0;
}
static int standby_lcd(int on)
{
	//    pin_share_config(lcd_standby_pad, lcd_standby_pad_gpio_fun_num);
	// gpio_set_direct(lcd_standby_group,lcd_standby_port,lcd_standby_number, GPIO_DIR_OUTPUT);
	if(on) {
		gpio_set_val(lcd_standby_group, lcd_standby_port, lcd_standby_number, GPIO_DATA_HIGH);
	} else {
		gpio_set_val(lcd_standby_group, lcd_standby_port, lcd_standby_number, GPIO_DATA_LOW);
	}
	return 0;
}

static int backlight_lcd(int on)
{
	//   pin_share_config(lcd_backlight_pad, lcd_backlight_pad_gpio_fun_num);
	//  gpio_set_direct(lcd_backlight_group,lcd_backlight_port,lcd_backlight_number, GPIO_DIR_OUTPUT);
	if(on) {
		gpio_set_val(lcd_backlight_group, lcd_backlight_port, lcd_backlight_number, GPIO_DATA_HIGH);
	} else {
		gpio_set_val(lcd_backlight_group, lcd_backlight_port, lcd_backlight_number, GPIO_DATA_LOW);
	}
	return 0;
}

static int power_lcd(int on)
{
	//pin_share_config(lcd_power_pad,lcd_power_pad_gpio_fun_num);
	//  gpio_set_direct(lcd_power_group,lcd_power_port,lcd_power_number, GPIO_DIR_OUTPUT);
	if(on) {
		gpio_set_val(lcd_power_group, lcd_power_port, lcd_power_number, GPIO_DATA_HIGH);
	} else {
		gpio_set_val(lcd_power_group, lcd_power_port, lcd_power_number, GPIO_DATA_LOW);
	}
	return 0;
}

static int reset_lcd(int delay1, int delay2, int delay3)
{
	//pin_share_config(lcd_rest_pad,lcd_rest_pad_gpio_fun_num);
	//   gpio_set_direct(lcd_rest_group,lcd_rest_port,lcd_rest_number, GPIO_DIR_OUTPUT);
	gpio_set_val(lcd_rest_group, lcd_rest_port, lcd_rest_number, GPIO_DATA_HIGH);
	sc_delay(delay1);
	gpio_set_val(lcd_rest_group, lcd_rest_port, lcd_rest_number, GPIO_DATA_LOW);
	sc_delay(delay2);
	gpio_set_val(lcd_rest_group, lcd_rest_port, lcd_rest_number, GPIO_DATA_HIGH);
	sc_delay(delay3);
	return 0;
}

static int get_lcd_cfg(char *name)
{
	int offset = -1;
	int ret, status = 0;
	unsigned int value = -1;
	void *blob = board_fdt_blob_setup();
	offset = fdt_node_offset_by_compatible(blob, 0, "ek79007_lcd_cfg");
	if (offset != -1) {
		ret = fdtdec_get_int_array(blob, offset, name, &value, 1);
		sc_always("ret=%d value=%d", ret, value);
		if(!ret) {
			status = value;

		} else {
			sc_err("%d", __LINE__);
			value = 0;
		}
	} else {
		sc_err("%d", __LINE__);
		value = 0;
	}
	sc_always("value=%d", value);
	return value;

}

static int lcd_set_scan_mode(void)
{
	int value = 0;
	//pin_share_config(lcd_updown_scan_pad,lcd_updown_pad_gpio_fun_num);
	gpio_set_direct(lcd_updown_scan_group, lcd_updown_scan_port, lcd_updown_scan_number, GPIO_DIR_OUTPUT);
	//value=get_lcd_cfg("flip");
	gpio_set_val(lcd_updown_scan_group, lcd_updown_scan_port, lcd_updown_scan_number, value);

#if 0
	pin_share_config(lcd_shlr_pad, lcd_shlr_pad_gpio_fun_num);
	smartx_gpio_set_direction(lcd_shlr_group, lcd_shlr_port, lcd_shlr_number, GPIO_IS_OUTPUT);
	value = get_lcd_cfg("mirror");
	smartx_gpio_set_value(lcd_shlr_group, lcd_shlr_port, lcd_shlr_number, value);
#endif
	return 0;

}

static int init_interface_dev(void)
{
	int ret = 0;

	gpio_set_direct(lcd_standby_group, lcd_standby_port, lcd_standby_number, GPIO_DIR_OUTPUT);
	gpio_set_direct(lcd_backlight_group, lcd_backlight_port, lcd_backlight_number, GPIO_DIR_OUTPUT);
	gpio_set_direct(lcd_power_group, lcd_power_port, lcd_power_number, GPIO_DIR_OUTPUT);
	gpio_set_direct(lcd_rest_group, lcd_rest_port, lcd_rest_number, GPIO_DIR_OUTPUT);
	gpio_set_direct(lcd_updown_scan_group, lcd_updown_scan_port, lcd_updown_scan_number, GPIO_DIR_OUTPUT);

#if 1
	ret = dsi_init();
	if (ret != 0) {
		sc_err("dsi_init ret=%d", ret);
		goto End;
	}
#else
	ret = dsi_set_timing(&g_dsi_cfg);
	if (ret != 0) {
		sc_err("dsi_init ret=%d", ret);
		goto End;
	}

	dsi_exit_cmd_mode();
#endif
	sc_trace_line();
End:
	return ret;
}
static int get_display_interface_desc_dev(display_interface_desc_t *desc)
{
	sc_trace_line();

	if(desc) {
		*desc = g_desc;
	} else {
		sc_err("null input");
	}
	return 0;
}
static int get_display_interface_res_infor_dev(display_interface_res_infor_t *res_infor)
{
	sc_trace_line();

	if(res_infor) {
		*res_infor = g_res_infor;
	} else {
		sc_err("null input");
	}
	return 0;
}

static int set_display_interface_res_infor_dev(disp_res_infor_t *res_infor)
{
	int                 ret = 0;

	ret = dsi_set_timing(&g_dsi_cfg);
	if (ret != 0) {
		sc_err("dsi_init ret=%d", ret);
		goto End;
	}

	dsi_exit_cmd_mode();

	sc_always("poweron_lcd");
	//power on the lcd
	power_lcd(1);
	backlight_lcd(1);

	//standby the lcd
	lcd_set_scan_mode();
	standby_lcd(1);
	sc_delay(10);

	//reset the lcd
	reset_lcd(1, 10, 1);

	//final we backlight the lcd
	backlight_lcd(1);

	sc_delay(1000);
	sc_always("get_display_mode");

End:
	return 0;
}

display_intgerface_ops_t mipi_dev_ek79007 = {
	.interface_index = MIPI_DEV_EK79007,
	.init_display_interface = init_interface_dev,
	.get_display_interface_desc = get_display_interface_desc_dev,
	.get_display_interface_res_infor = get_display_interface_res_infor_dev,
	.set_display_interface_res_infor = set_display_interface_res_infor_dev,
};

