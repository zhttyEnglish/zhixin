#include "vo/interface/display_interface.h"
#include <asm/arch/gpio.h>
#include <config.h>
#include <i2c.h>
#define tc3587_wr_addr 0x1c//0x0e
#define tc3587_i2c_component 3

#define lcd_backlight_port GPIO_PORT_D
#define lcd_backlight_group GPIO_GROUP_1
#define lcd_backlight_number 6
#define lcd_backlight_pad     56
#define lcd_backlight_pad_gpio_fun_num 1

#define lcd_power_port GPIO_PORT_D
#define lcd_power_group GPIO_GROUP_2
#define lcd_power_number 1
#define lcd_power_pad     77
#define lcd_power_pad_gpio_fun_num 2

#define lcd_standby_port GPIO_PORT_A
#define lcd_standby_group GPIO_GROUP_1
#define lcd_standby_number 3
#define lcd_standby_pad     25
#define lcd_standby_pad_gpio_fun_num 1

#define lcd_rest_port GPIO_PORT_A
#define lcd_rest_group GPIO_GROUP_1
#define lcd_rest_number 4
#define lcd_rest_pad     26
#define lcd_rest_pad_gpio_fun_num 1

#define lcd_updown_scan_port GPIO_PORT_A
#define lcd_updown_scan_group GPIO_GROUP_1
#define lcd_updown_scan_number 1
#define lcd_updown_scan_pad     23
#define lcd_updown_pad_gpio_fun_num 1

#define lcd_shlr_port GPIO_PORT_A
#define lcd_shlr_group GPIO_GROUP_1
#define lcd_shlr_number 2
#define lcd_shlr_pad     24
#define lcd_shlr_pad_gpio_fun_num 1

#define tc3587_rest_port GPIO_PORT_A
#define tc3587_rest_group GPIO_GROUP_1
#define tc3587_rest_number 0
#define tc3587_rest_pad     22
#define tc3587_rest_pad_gpio_fun_num 1

static int  tc3587_i2c_read(int address, int *data);
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
		{1024, 1354, 1184, 1194, 600, 636, 612, 613, 60},
	},
	8,
};

static int  tc3587_i2c_init(void)
{
	int ret = 0;
	sc_always("tc3587_i2c_component=%d,tc3587_wr_addr=%d", tc3587_i2c_component, tc3587_wr_addr);
	i2c_set_bus_num(tc3587_i2c_component);
	return ret;
}

static int  tc3587_i2c_write(int address, int data)
{
	uint8_t buffer[4];
	int val = 0;
	int ret = 0;
	buffer[0] = (address >> 8) & 0xff;
	buffer[1] = (address) & 0xff;
	buffer[2] = (data >> 8) & 0xff;
	buffer[3] = (data) & 0xff;
	ret = i2c_write(tc3587_wr_addr >> 1, address, 2, &buffer[2], 2);

	//tc3587_i2c_read(address,&val);
	//sc_always("addr=0x%x w:0x%x r:0x%x",address,data,val);
	return 0;
}

static int  tc3587_i2c_read(int address, int *data)
{
	uint8_t buffer[4];
	int ret = 0;
	buffer[0] = (address >> 8) & 0xff;
	buffer[1] = (address) & 0xff;
	buffer[2] = 0;
	buffer[3] = 0;
	ret = i2c_read(tc3587_wr_addr >> 1, address, 2, &buffer[2], 2);
	*data = (buffer[2] << 8) | buffer[3];
	return ret;
}

static int read_chip_id(void)
{
	int id = 0;
	tc3587_i2c_read(0x0000, &id);
	printf("id:0x%x\n", id);
	return id;
}

static int dcs_short_write_cmd_without_pra(int cmd)
{
	uint16_t reg_val = cmd & 0xff;
	tc3587_i2c_write(0x0602, 0x1005);
	tc3587_i2c_write(0x0604, 0x0000);
	tc3587_i2c_write(0x0610, reg_val);
	tc3587_i2c_write(0x0600, 0x0001);
	udelay(100);
	return reg_val;
}

static int dcs_short_write_cmd_without_1pra(int cmd, int pra)
{
	uint16_t reg_val = cmd & 0xff;
	reg_val |= ((pra & 0xff) << 8);
	tc3587_i2c_write(0x0602, 0x1015);
	tc3587_i2c_write(0x0604, 0x0000);
	tc3587_i2c_write(0x0610, reg_val);
	tc3587_i2c_write(0x0600, 0x0001);
	sc_delay(1);
	return reg_val;
}

static int regw(int addr, int data)
{
	return dcs_short_write_cmd_without_1pra(addr, data);
}
static int get_lcd_cfg_init_setting(unsigned int *val, int count)
{
}

static int lcd_init()
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
	/*
	 * GPIO config, use new GPIO API
	 * TODO
	 *
	   gpio_pad_set_func(lcd_standby_pad, lcd_standby_pad_gpio_fun_num);
	   gpio_pad_set_direction(lcd_standby_group,lcd_standby_port,lcd_standby_number, GPIO_IS_OUTPUT);
	   if(on){
	      gpio_pad_set_val(lcd_standby_group,lcd_standby_port, lcd_standby_number, GPIO_IS_HIGH);
	   }else
	   {
	      gpio_pad_set_val(lcd_standby_group,lcd_standby_port, lcd_standby_number, GPIO_IS_LOW);
	   }
	*/
	return 0;
}

static int backlight_lcd(int on)
{
	/*
	 * GPIO config, use new GPIO API
	 * TODO
	 *
	    gpio_pad_set_func(lcd_backlight_pad, lcd_backlight_pad_gpio_fun_num);
	    gpio_pad_set_direction(lcd_backlight_group,lcd_backlight_port,lcd_backlight_number, GPIO_IS_OUTPUT);
	    if(on){
	        gpio_pad_set_val(lcd_backlight_group,lcd_backlight_port, lcd_backlight_number, GPIO_IS_HIGH);
	    }else
	    {
	        gpio_pad_set_val(lcd_backlight_group,lcd_backlight_port, lcd_backlight_number, GPIO_IS_LOW);
	    }
	*/
	return 0;
}

static int power_lcd(int on)
{
	/*
	 * GPIO config, use new GPIO API
	 * TODO
	 *
	    gpio_pad_set_func(lcd_power_pad,lcd_power_pad_gpio_fun_num);
	    gpio_pad_set_direction(lcd_power_group,lcd_power_port,lcd_power_number, GPIO_IS_OUTPUT);
	    if(on)
	    {
	       gpio_pad_set_val(lcd_power_group,lcd_power_port, lcd_power_number, GPIO_IS_HIGH);
	    }else
	    {
	       gpio_pad_set_val(lcd_power_group,lcd_power_port, lcd_power_number, GPIO_IS_LOW);
	    }
	*/
	return 0;
}

static int reset_lcd(int delay1, int delay2, int delay3)
{
	/*
	 * GPIO config, use new GPIO API
	 * TODO
	 *
	   gpio_pad_set_func(lcd_rest_pad,lcd_rest_pad_gpio_fun_num);
	   gpio_pad_set_direction(lcd_rest_group,lcd_rest_port,lcd_rest_number, GPIO_IS_OUTPUT);
	   gpio_pad_set_val(lcd_rest_group,lcd_rest_port, lcd_rest_number, GPIO_IS_HIGH);
	   sc_delay(delay1);
	   gpio_pad_set_val(lcd_rest_group,lcd_rest_port,lcd_rest_number, GPIO_IS_LOW);
	   sc_delay(delay2);
	   gpio_pad_set_val(lcd_rest_group,lcd_rest_port,lcd_rest_number, GPIO_IS_HIGH);
	   sc_delay(delay3);
	*/
	return 0;
}

static int reset_tc3587(int delay1, int delay2, int delay3)
{
	/*
	 * GPIO config, use new GPIO API
	 * TODO
	 *
	    gpio_pad_set_func(tc3587_rest_pad,tc3587_rest_pad_gpio_fun_num);
	    gpio_pad_set_direction(tc3587_rest_group,tc3587_rest_port,tc3587_rest_number, GPIO_IS_OUTPUT);
	    gpio_pad_set_val(tc3587_rest_group,tc3587_rest_port, tc3587_rest_number, GPIO_IS_HIGH);
	    sc_delay(delay1);
	    gpio_pad_set_val(tc3587_rest_group,tc3587_rest_port, tc3587_rest_number, GPIO_IS_LOW);
	    sc_delay(delay2);
	    gpio_pad_set_val(tc3587_rest_group,tc3587_rest_port, tc3587_rest_number, GPIO_IS_HIGH);
	    sc_delay(delay3);
	*/
	return 0;
}

static int get_lcd_cfg(char *name)
{
	int offset = -1;
	int ret, status = 0;
	unsigned int value = -1;
	void *blob = board_fdt_blob_setup();
	offset = fdt_node_offset_by_compatible(blob, 0, "tc3587_lcd_cfg");
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

static int lcd_set_scan_mode()
{
	int value = 0;
	/*
	 * GPIO config, use new GPIO API
	 * TODO
	 *
	   gpio_pad_set_func(lcd_updown_scan_pad,lcd_updown_pad_gpio_fun_num);
	   gpio_pad_set_direction(lcd_updown_scan_group,lcd_updown_scan_port,lcd_updown_scan_number, GPIO_IS_OUTPUT);
	   value=get_lcd_cfg("flip");
	   gpio_pad_set_val(lcd_updown_scan_group,lcd_updown_scan_port, lcd_updown_scan_number, value);

	   gpio_pad_set_func(lcd_shlr_pad,lcd_shlr_pad_gpio_fun_num);
	   gpio_pad_set_direction(lcd_shlr_group,lcd_shlr_port,lcd_shlr_number, GPIO_IS_OUTPUT);
	   value=get_lcd_cfg("mirror");
	   gpio_pad_set_val(lcd_shlr_group,lcd_shlr_port, lcd_shlr_number, value);
	*/
	return 0;
}

static int  tc3587_init()
{
	//static unsigned int TC358768XBG_INIT_OR_RESUME[] = {
	// **************************************************
	// First initialization Sequence or RESUME Sequence
	// **************************************************
	// **************************************************
	// Power on TC358768XBG according to recommended power-on sequence, if power is cut off
	// Assert Reset (RESX="L")
	// Deassert Reset (RESX="H")
	// Start input REFCK and PCLK
	// **************************************************
	// **************************************************
	// TC358768XBG Software Reset
	// **************************************************
	tc3587_i2c_write(0x0002, 0x0001); //SYSctl, S/W Reset
	sc_delay(1);
	tc3587_i2c_write(0x0002, 0x0000); //SYSctl, S/W Reset release
	sc_delay(10);
	// TC358768XBG PLL,Clock Setting
	// **************************************************
	tc3587_i2c_write(0x0016, 0x2054); //PLL Control Register 0 (PLL_PRD,PLL_FBD)
	tc3587_i2c_write(0x0018, 0x0603); //PLL_FRS,PLL_LBWS, PLL oscillation enable
	sc_delay(10);
	tc3587_i2c_write(0x0018, 0x0613); //PLL_FRS,PLL_LBWS, PLL clock out enable
	// **************************************************
	// TC358768XBG DPI Input Control
	// **************************************************
	tc3587_i2c_write(0x0006, 0x0050); //FIFO Control Register
	// **************************************************
	// TC358768XBG D-PHY Setting
	// **************************************************
	tc3587_i2c_write(0x0140, 0x0000); //D-PHY Clock lane enable
	tc3587_i2c_write(0x0142, 0x0000); //
	tc3587_i2c_write(0x0144, 0x0000); //D-PHY Data lane0 enable
	tc3587_i2c_write(0x0146, 0x0000); //
	tc3587_i2c_write(0x0148, 0x0000); //D-PHY Data lane1 enable
	tc3587_i2c_write(0x014A, 0x0000); //
	tc3587_i2c_write(0x014C, 0x0000); //D-PHY Data lane2 enable
	tc3587_i2c_write(0x014E, 0x0000); //
	tc3587_i2c_write(0x0150, 0x0000); //D-PHY Data lane3 enable
	tc3587_i2c_write(0x0152, 0x0000); //
	tc3587_i2c_write(0x0100, 0x0002); //D-PHY Clock lane control
	tc3587_i2c_write(0x0102, 0x0000); //
	tc3587_i2c_write(0x0104, 0x0002); //D-PHY Data lane0 control
	tc3587_i2c_write(0x0106, 0x0000); //
	tc3587_i2c_write(0x0108, 0x0002); //D-PHY Data lane1 control
	tc3587_i2c_write(0x010A, 0x0000); //
	tc3587_i2c_write(0x010C, 0x0002); //D-PHY Data lane2 control
	tc3587_i2c_write(0x010E, 0x0000); //
	tc3587_i2c_write(0x0110, 0x0002); //D-PHY Data lane3 control
	tc3587_i2c_write(0x0112, 0x0000); //
	// **************************************************
	// TC358768XBG DSI-TX PPI Control
	// **************************************************
	tc3587_i2c_write(0x0210, 0x1644); //LINEINITCNT
	tc3587_i2c_write(0x0212, 0x0000); //
	tc3587_i2c_write(0x0214, 0x0002); //LPTXTIMECNT
	tc3587_i2c_write(0x0216, 0x0000); //
	tc3587_i2c_write(0x0218, 0x2002); //TCLK_HEADERCNT
	tc3587_i2c_write(0x021A, 0x0000); //
	tc3587_i2c_write(0x0220, 0x0603); //THS_HEADERCNT
	tc3587_i2c_write(0x0222, 0x0000); //
	tc3587_i2c_write(0x0224, 0x4268); //TWAKEUPCNT
	tc3587_i2c_write(0x0226, 0x0000); //
	tc3587_i2c_write(0x022C, 0x0000); //THS_TRAILCNT
	tc3587_i2c_write(0x022E, 0x0000); //
	tc3587_i2c_write(0x0230, 0x0005); //HSTXVREGCNT
	tc3587_i2c_write(0x0232, 0x0000); //
	tc3587_i2c_write(0x0234, 0x001F); //HSTXVREGEN enable
	tc3587_i2c_write(0x0236, 0x0000); //
	tc3587_i2c_write(0x0238, 0x0001); //DSI clock Enable/Disable during LP
	tc3587_i2c_write(0x023A, 0x0000); //
	tc3587_i2c_write(0x023C, 0x0001); //BTACNTRL1
	tc3587_i2c_write(0x023E, 0x0002); //
	tc3587_i2c_write(0x0204, 0x0001); //STARTCNTRL
	tc3587_i2c_write(0x0206, 0x0000); //
	// **************************************************
	// TC358768XBG DSI-TX Timing Control
	// **************************************************
	tc3587_i2c_write(0x0620, 0x0001); //Sync Pulse/Sync Event mode setting
	tc3587_i2c_write(0x0622, 0x0018); //V Control Register1
	tc3587_i2c_write(0x0624, 0x0017); //V Control Register2
	tc3587_i2c_write(0x0626, 0x0258); //V Control Register3
	tc3587_i2c_write(0x0628, 0x0230); //H Control Register1
	tc3587_i2c_write(0x062A, 0x020F); //H Control Register2
	tc3587_i2c_write(0x062C, 0x0C00); //H Control Register3
	tc3587_i2c_write(0x0518, 0x0001); //DSI Start
	tc3587_i2c_write(0x051A, 0x0000); //
	// **************************************************
	// LCDD (Peripheral) Setting
	lcd_init();
	// **************************************************
	// **************************************************
	// Set to HS mode
	// **************************************************
	tc3587_i2c_write(0x0500, 0x0086); //DSI lane setting, DSI mode=HS
	tc3587_i2c_write(0x0502, 0xA300); //bit set
	tc3587_i2c_write(0x0500, 0x8000); //Switch to DSI mode
	tc3587_i2c_write(0x0502, 0xC300); //
	// **************************************************
	// Host: RGB(DPI) input start
	// **************************************************
	tc3587_i2c_write(0x0008, 0x0037); //DSI-TX Format setting
	tc3587_i2c_write(0x0050, 0x003E); //DSI-TX Pixel stream packet Data Type setting
	tc3587_i2c_write(0x0032, 0x0000); //HSYNC Polarity
	tc3587_i2c_write(0x0004, 0x0044); //Configuration Control Register
	// **************************************************
	// LCDD (Peripheral) Setting
	// **************************************************
	//};
	return 0;
}
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

static int set_display_interface_res_infor_dev(disp_res_infor_t *res_infor)
{
	//power on the lcd
	power_lcd(0);
	backlight_lcd(0);

	//standby the lcd
	lcd_set_scan_mode();
	standby_lcd(1);
	sc_delay(10);

	//reset the lcd
	reset_lcd(1, 10, 1);
	reset_tc3587(1, 10, 10);

	tc3587_i2c_init();
	read_chip_id();
	if(res_infor) {
		tc3587_init();
	} else {
		sc_err("***null input");
	}
	//final we backlight the lcd
	backlight_lcd(1);
	return 0;
}
display_intgerface_ops_t dvp_dev_tc3587 = {
	.interface_index = DVP_DEV_TC3587,
	.init_display_interface = init_interface_dev,
	.get_display_interface_desc = get_display_interface_desc_dev,
	.get_display_interface_res_infor = get_display_interface_res_infor_dev,
	.set_display_interface_res_infor = set_display_interface_res_infor_dev,
};

