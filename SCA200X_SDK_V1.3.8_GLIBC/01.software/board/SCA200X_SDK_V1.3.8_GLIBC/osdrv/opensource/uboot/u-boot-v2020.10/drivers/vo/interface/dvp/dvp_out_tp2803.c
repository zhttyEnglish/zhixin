#include "vo/interface/display_interface.h"
#include <asm/arch/gpio.h>
#include <config.h>
#include <i2c.h>

#define tp2803_wr_addr 0x8A //0x88
#define tp2803_i2c_component 3
#define tp2803_rest_port GPIO_PORT_A
#define tp2803_rest_group GPIO_GROUP_2
#define tp2803_rest_number 5
#define tp2803_rest_pad     87  //GP6 AG1

//Different pins have different pinmux function, check which mode is GPIO mode
#define THIS_PAD_GPIO_MODE  1 //pad 87 GPIO mode is '1'

#define BANK0 0
#define BANK1 1
#define BANK2 2

#define SC_CLK_ENABLE_I2S_CLK_FOR_SENSOR 0x40b00018

#define SC_CLK_AU_PLL_0    (0x6063213c)
#define SC_CLK_AU_PLL_1    (0x60632144)

#define SC_CLK_AU_PLL_DIG_REG_0   (0x60632A30)
#define SC_CLK_AU_PLL_DIG_REG_1   (0x60632A34)

#define AUPLL_FIX_CLK_FREQ  (40)    //MHz
#define PLL_DIVR_INTER_POS  (24)

static void tp2803_reset(int delay1, int delay2, int delay3);
static int sc_cfg_get_dvp_timing(int width, int height, float fps);
enum tp2803_timing {
	TP2803_1920_1080_30P,
	TP2803_1920_1080_25P,
	TP2803_1280_720_60P,
	TP2803_1280_720_50P,
	TP2803_1280_720_30P,
	TP2803_1280_720_25P,
	TP2803_1280_720_30P_8BIT,
	TP2803_1280_720_25P_8BIT,
};
static void tp2803_set_timing(enum tp2803_timing vmode);
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
	2,
	/*disp_res_infor_t res[MAX_SUPPORTED_RES];*/
	{
		{1280, 1920, 1280, 1430, 720, 750, 725, 730, 25},
		{1280, 1600, 1280, 1430, 720, 750, 725, 730, 30},
	},
};

static int  tp2803_i2c_init()
{
	int ret = 0;
	i2c_set_bus_num(tp2803_i2c_component);
	return ret;
}
static int  tp2803_i2c_read(int address, int *data);
static int  tp2803_i2c_write(int address, int data)
{
	uint8_t buffer[2];
	int ret = 0;
	buffer[0] = (address) & 0xff;
	buffer[1] = (data) & 0xff;
	I2C_SET_BUS(tp2803_i2c_component);
	ret = i2c_write(tp2803_wr_addr >> 1, buffer, 1, &buffer[1], 1);

	//int read_data=0;
	//tp2803_i2c_read(address,&read_data);
	//sc_always("address=0x%x, w_data=0x%x,read_data=0x%x",address,data,read_data);
	return 0;
}

static int  tp2803_i2c_read(int address, int *data)
{
	uint8_t buffer[2];
	int ret = 0;
	buffer[0] = (address) & 0xff;
	I2C_SET_BUS(tp2803_i2c_component);
	ret = i2c_read(tp2803_wr_addr >> 1, buffer, 1, &buffer[2], 1);
	*data = buffer[1];
	return ret;
}

static void tp2803_reset(int delay1, int delay2, int delay3)
{
	sc_always("enter %d %d %d", delay1, delay2, delay3);
	pin_share_config(tp2803_rest_pad, THIS_PAD_GPIO_MODE);
	smartx_gpio_set_direction(tp2803_rest_group, tp2803_rest_port, tp2803_rest_number, GPIO_IS_OUTPUT);
	smartx_gpio_set_value(tp2803_rest_group, tp2803_rest_port, tp2803_rest_number, GPIO_IS_HIGH);
	sc_delay(delay1);
	smartx_gpio_set_value(tp2803_rest_group, tp2803_rest_port, tp2803_rest_number, GPIO_IS_LOW);
	sc_delay(delay2);
	smartx_gpio_set_value(tp2803_rest_group, tp2803_rest_port, tp2803_rest_number, GPIO_IS_HIGH);
	sc_delay(delay3);
	sc_always("exit");

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
static int sc_cfg_get_dvp_timing(int width, int height, float fps)
{
	enum tp2803_timing input_opt = TP2803_1920_1080_30P;
	// config dvp
	sc_always("fps = %d with w=%d h=%d", (int)fps, width, height);
	if((width == 1920) && (height == 1080)) {
		if(abs(25 - (uint32_t)fps) <= 1) {
			input_opt = TP2803_1920_1080_25P;
		} else if (abs(30 - (uint32_t)fps) <= 1) {
			input_opt = TP2803_1920_1080_30P;
		} else {
			sc_err("not supported fps = %d with w=%d h=%d", (int)fps, width, height);
		}
	} else if((width == 1280) && (height == 720)) {
		if(abs(25 - (uint32_t)fps) <= 1) {
			input_opt = TP2803_1280_720_25P;
		} else if (abs(30 - (uint32_t)fps) <= 1) {
			input_opt = TP2803_1280_720_30P;
		} else if (abs(50 - (uint32_t)fps) <= 1) {
			input_opt = TP2803_1280_720_50P;
		} else if (abs(60 - (uint32_t)fps) <= 1) {
			input_opt = TP2803_1280_720_60P;
		} else {
			sc_err("not supported fps = %d with w=%d h=%d", (int)fps, width, height);
		}
	} else {
		sc_err("not supported fps = %d with w=%d h=%d", (int)fps, width, height);
	}
	sc_always("input_opt=%d", input_opt);
	return (int)input_opt;
}

static void  tp2803_set_timing_1280_720_25()
{
	sc_always("enter");
	tp2803_i2c_write(0xFF, 0x00); //Page 00
	tp2803_i2c_write(0x00, 0x08);
	tp2803_i2c_write(0x01, 0x00);
	tp2803_i2c_write(0x02, 0x8B);
	tp2803_i2c_write(0x03, 0x42);
	tp2803_i2c_write(0x04, 0x00);
	tp2803_i2c_write(0x05, 0x6C);
	tp2803_i2c_write(0x06, 0x00);
	tp2803_i2c_write(0x07, 0x41);
	tp2803_i2c_write(0x08, 0x76);
	tp2803_i2c_write(0x09, 0x76);
	tp2803_i2c_write(0x0A, 0x76);
	tp2803_i2c_write(0x0B, 0x05);
	tp2803_i2c_write(0x0C, 0x04);
	tp2803_i2c_write(0x0D, 0x04);
	tp2803_i2c_write(0x0E, 0x82);
	tp2803_i2c_write(0x0F, 0x80);
	tp2803_i2c_write(0x10, 0x17);
	tp2803_i2c_write(0x11, 0x80);
	tp2803_i2c_write(0x12, 0x00);
	tp2803_i2c_write(0x13, 0x3C);
	tp2803_i2c_write(0x14, 0x38);
	tp2803_i2c_write(0x15, 0x38);
	tp2803_i2c_write(0x16, 0xEB);
	tp2803_i2c_write(0x17, 0x08);
	tp2803_i2c_write(0x18, 0x10);
	tp2803_i2c_write(0x19, 0xF0);
	tp2803_i2c_write(0x1A, 0x10);
	tp2803_i2c_write(0x1B, 0xA4);
	tp2803_i2c_write(0x1C, 0x55);
	tp2803_i2c_write(0x1D, 0x76);
	tp2803_i2c_write(0x1E, 0x80);
	tp2803_i2c_write(0x1F, 0x00);

	//tp2803_i2c_write(0x20,0x28);
	//tp2803_i2c_write(0x21,0xc4);
	//tp2803_i2c_write(0x22,0x44);
	//tp2803_i2c_write(0x23,0x44);

	tp2803_i2c_write(0x20, 0x51);
	tp2803_i2c_write(0x21, 0x88);
	tp2803_i2c_write(0x22, 0x88);
	tp2803_i2c_write(0x23, 0x88);

	tp2803_i2c_write(0x24, 0x00);
	tp2803_i2c_write(0x25, 0x00);
	tp2803_i2c_write(0x26, 0x00);
	tp2803_i2c_write(0x27, 0x56);
	tp2803_i2c_write(0x28, 0x00);
	tp2803_i2c_write(0x29, 0x35);
	tp2803_i2c_write(0x2A, 0xE0);
	tp2803_i2c_write(0x2B, 0x00);
	tp2803_i2c_write(0x2C, 0x00);
	tp2803_i2c_write(0x2D, 0x00);
	tp2803_i2c_write(0x2E, 0x00);
	tp2803_i2c_write(0x2F, 0x00);
	tp2803_i2c_write(0x30, 0x00);
	tp2803_i2c_write(0x31, 0x00);
	tp2803_i2c_write(0x32, 0x00);
	tp2803_i2c_write(0x33, 0x00);
	tp2803_i2c_write(0x34, 0x00);
	tp2803_i2c_write(0x35, 0x00);
	tp2803_i2c_write(0x36, 0x00);
	tp2803_i2c_write(0x37, 0x00);
	tp2803_i2c_write(0x38, 0x00);
	tp2803_i2c_write(0x39, 0x00);
	tp2803_i2c_write(0x3A, 0x00);
	tp2803_i2c_write(0x3B, 0x00);
	tp2803_i2c_write(0x3C, 0x10);
	tp2803_i2c_write(0x3D, 0x80);
	tp2803_i2c_write(0x3E, 0x18);
	tp2803_i2c_write(0x3F, 0x00);
	tp2803_i2c_write(0x40, 0x00);
	tp2803_i2c_write(0x41, 0x01);
	tp2803_i2c_write(0x42, 0x12);
	tp2803_i2c_write(0x43, 0x08);
	tp2803_i2c_write(0x44, 0x4E);
	tp2803_i2c_write(0x45, 0xCB);
	tp2803_i2c_write(0x46, 0xAA);
	tp2803_i2c_write(0x47, 0x1F);
	tp2803_i2c_write(0x48, 0xFA);
	tp2803_i2c_write(0x49, 0x00);
	tp2803_i2c_write(0x4A, 0x07);
	tp2803_i2c_write(0x4B, 0x08);
	tp2803_i2c_write(0x4C, 0x00);
	tp2803_i2c_write(0x4D, 0x00);
	tp2803_i2c_write(0x4E, 0x00);
	tp2803_i2c_write(0x4F, 0x00);
	tp2803_i2c_write(0x50, 0x00);
	tp2803_i2c_write(0x51, 0x00);
	tp2803_i2c_write(0x52, 0x00);
	tp2803_i2c_write(0x53, 0x00);
	tp2803_i2c_write(0x54, 0x00);
	tp2803_i2c_write(0x55, 0x00);
	tp2803_i2c_write(0x56, 0x27);
	tp2803_i2c_write(0x57, 0x04);
	tp2803_i2c_write(0x58, 0xBD);
	tp2803_i2c_write(0x59, 0xA1);
	tp2803_i2c_write(0x5A, 0x0D);
	tp2803_i2c_write(0x5B, 0x00);
	tp2803_i2c_write(0x5C, 0x0B);
	tp2803_i2c_write(0x5D, 0x0C);
	tp2803_i2c_write(0x5E, 0x21);
	tp2803_i2c_write(0x5F, 0x00);
	tp2803_i2c_write(0x60, 0x00);
	tp2803_i2c_write(0x61, 0x00);
	tp2803_i2c_write(0x62, 0x00);
	tp2803_i2c_write(0x63, 0x00);
	tp2803_i2c_write(0x64, 0x00);
	tp2803_i2c_write(0x65, 0x00);
	tp2803_i2c_write(0x66, 0x00);
	tp2803_i2c_write(0x67, 0x00);
	tp2803_i2c_write(0x68, 0x00);
	tp2803_i2c_write(0x69, 0x00);
	tp2803_i2c_write(0x6A, 0xFA);
	tp2803_i2c_write(0x6B, 0x90);
	tp2803_i2c_write(0x6C, 0x00);
	tp2803_i2c_write(0x6D, 0x00);
	tp2803_i2c_write(0x6E, 0x00);
	tp2803_i2c_write(0x6F, 0x00);
	tp2803_i2c_write(0x70, 0x00);
	tp2803_i2c_write(0x71, 0x00);
	tp2803_i2c_write(0x72, 0x00);
	tp2803_i2c_write(0x73, 0x00);
	tp2803_i2c_write(0x74, 0x00);
	tp2803_i2c_write(0x75, 0x00);
	tp2803_i2c_write(0x76, 0x00);
	tp2803_i2c_write(0x77, 0x00);
	tp2803_i2c_write(0x78, 0x00);
	tp2803_i2c_write(0x79, 0x00);
	tp2803_i2c_write(0x7A, 0x00);
	tp2803_i2c_write(0x7B, 0x8A);
	tp2803_i2c_write(0x7C, 0x00);
	tp2803_i2c_write(0x7D, 0x00);
	tp2803_i2c_write(0x7E, 0x00);
	tp2803_i2c_write(0x7F, 0x00);
	tp2803_i2c_write(0x80, 0x00);
	tp2803_i2c_write(0x81, 0x00);
	tp2803_i2c_write(0x82, 0x00);
	tp2803_i2c_write(0x83, 0x00);
	tp2803_i2c_write(0x84, 0x00);
	tp2803_i2c_write(0x85, 0x00);
	tp2803_i2c_write(0x86, 0x00);
	tp2803_i2c_write(0x87, 0x00);
	tp2803_i2c_write(0x88, 0x00);
	tp2803_i2c_write(0x89, 0x00);
	tp2803_i2c_write(0x8A, 0x00);
	tp2803_i2c_write(0x8B, 0x00);
	tp2803_i2c_write(0x8C, 0x00);
	tp2803_i2c_write(0x8D, 0x00);
	tp2803_i2c_write(0x8E, 0x00);
	tp2803_i2c_write(0x8F, 0x00);
	tp2803_i2c_write(0x90, 0x00);
	tp2803_i2c_write(0x91, 0x00);
	tp2803_i2c_write(0x92, 0x00);
	tp2803_i2c_write(0x93, 0x00);
	tp2803_i2c_write(0x94, 0x00);
	tp2803_i2c_write(0x95, 0x00);
	tp2803_i2c_write(0x96, 0x00);
	tp2803_i2c_write(0x97, 0x00);
	tp2803_i2c_write(0x98, 0x00);
	tp2803_i2c_write(0x99, 0x00);
	tp2803_i2c_write(0x9A, 0x00);
	tp2803_i2c_write(0x9B, 0x00);
	tp2803_i2c_write(0x9C, 0x00);
	tp2803_i2c_write(0x9D, 0x00);
	tp2803_i2c_write(0x9E, 0x00);
	tp2803_i2c_write(0x9F, 0x00);
	tp2803_i2c_write(0xA0, 0x00);
	tp2803_i2c_write(0xA1, 0x00);
	tp2803_i2c_write(0xA2, 0x00);
	tp2803_i2c_write(0xA3, 0x00);
	tp2803_i2c_write(0xA4, 0x00);
	tp2803_i2c_write(0xA5, 0x00);
	tp2803_i2c_write(0xA6, 0x00);
	tp2803_i2c_write(0xA7, 0x00);
	tp2803_i2c_write(0xA8, 0x00);
	tp2803_i2c_write(0xA9, 0x00);
	tp2803_i2c_write(0xAA, 0x00);
	tp2803_i2c_write(0xAB, 0x00);
	tp2803_i2c_write(0xAC, 0x00);
	tp2803_i2c_write(0xAD, 0x00);
	tp2803_i2c_write(0xAE, 0x00);
	tp2803_i2c_write(0xAF, 0x00);
	tp2803_i2c_write(0xB0, 0x00);
	tp2803_i2c_write(0xB1, 0x00);
	tp2803_i2c_write(0xB2, 0x00);
	tp2803_i2c_write(0xB3, 0x00);
	tp2803_i2c_write(0xB4, 0x00);
	tp2803_i2c_write(0xB5, 0x00);
	tp2803_i2c_write(0xB6, 0x00);
	tp2803_i2c_write(0xB7, 0x00);
	tp2803_i2c_write(0xB8, 0x00);
	tp2803_i2c_write(0xB9, 0x00);
	tp2803_i2c_write(0xBA, 0x00);
	tp2803_i2c_write(0xBB, 0x00);
	tp2803_i2c_write(0xBC, 0x00);
	tp2803_i2c_write(0xBD, 0x00);
	tp2803_i2c_write(0xBE, 0x00);
	tp2803_i2c_write(0xBF, 0x00);
	tp2803_i2c_write(0xC0, 0x00);
	tp2803_i2c_write(0xC1, 0x00);
	tp2803_i2c_write(0xC2, 0x00);
	tp2803_i2c_write(0xC3, 0x00);
	tp2803_i2c_write(0xC4, 0x00);
	tp2803_i2c_write(0xC5, 0x00);
	tp2803_i2c_write(0xC6, 0x00);
	tp2803_i2c_write(0xC7, 0x00);
	tp2803_i2c_write(0xC8, 0x00);
	tp2803_i2c_write(0xC9, 0x00);
	tp2803_i2c_write(0xCA, 0x00);
	tp2803_i2c_write(0xCB, 0x00);
	tp2803_i2c_write(0xCC, 0x00);
	tp2803_i2c_write(0xCD, 0x00);
	tp2803_i2c_write(0xCE, 0x00);
	tp2803_i2c_write(0xCF, 0x00);
	tp2803_i2c_write(0xD0, 0x00);
	tp2803_i2c_write(0xD1, 0x00);
	tp2803_i2c_write(0xD2, 0x00);
	tp2803_i2c_write(0xD3, 0x00);
	tp2803_i2c_write(0xD4, 0x00);
	tp2803_i2c_write(0xD5, 0x00);
	tp2803_i2c_write(0xD6, 0x00);
	tp2803_i2c_write(0xD7, 0x00);
	tp2803_i2c_write(0xD8, 0x00);
	tp2803_i2c_write(0xD9, 0x00);
	tp2803_i2c_write(0xDA, 0x00);
	tp2803_i2c_write(0xDB, 0x00);
	tp2803_i2c_write(0xDC, 0x00);
	tp2803_i2c_write(0xDD, 0x00);
	tp2803_i2c_write(0xDE, 0x00);
	tp2803_i2c_write(0xDF, 0x00);
	tp2803_i2c_write(0xE0, 0x00);
	tp2803_i2c_write(0xE1, 0x00);
	tp2803_i2c_write(0xE2, 0x00);
	tp2803_i2c_write(0xE3, 0x00);
	tp2803_i2c_write(0xE4, 0x00);
	tp2803_i2c_write(0xE5, 0x00);
	tp2803_i2c_write(0xE6, 0x00);
	tp2803_i2c_write(0xE7, 0x00);
	tp2803_i2c_write(0xE8, 0x00);
	tp2803_i2c_write(0xE9, 0x00);
	tp2803_i2c_write(0xEA, 0x00);
	tp2803_i2c_write(0xEB, 0x00);
	tp2803_i2c_write(0xEC, 0x00);
	tp2803_i2c_write(0xED, 0x00);
	tp2803_i2c_write(0xEE, 0x00);
	tp2803_i2c_write(0xEF, 0x00);
	tp2803_i2c_write(0xF0, 0x00);
	tp2803_i2c_write(0xF1, 0x00);
	tp2803_i2c_write(0xF2, 0x00);
	tp2803_i2c_write(0xF3, 0x00);
	tp2803_i2c_write(0xF4, 0x00);
	tp2803_i2c_write(0xF5, 0x00);
	tp2803_i2c_write(0xF6, 0x00);
	tp2803_i2c_write(0xF7, 0x00);
	tp2803_i2c_write(0xF8, 0x00);
	tp2803_i2c_write(0xF9, 0x00);
	tp2803_i2c_write(0xFA, 0x00);
	tp2803_i2c_write(0xFB, 0x00);
	tp2803_i2c_write(0xFC, 0x12);
	tp2803_i2c_write(0xFD, 0x00);
	tp2803_i2c_write(0xFE, 0x29);
	tp2803_i2c_write(0xFF, 0x10);
	sc_always("exit");
}
static void  tp2803_set_timing_1280_720_30()
{
	sc_always("enter");
	tp2803_i2c_write(0xFF, 0x00); //Page 00
	tp2803_i2c_write(0x00, 0x08);
	tp2803_i2c_write(0x01, 0x00);
	tp2803_i2c_write(0x02, 0x8B);
	//tp2803_i2c_write(0x02,0xCB);//color bar
	tp2803_i2c_write(0x03, 0x42);
	//tp2803_i2c_write(0x03,0x62);
	tp2803_i2c_write(0x04, 0x00);
	tp2803_i2c_write(0x05, 0x6C);
	tp2803_i2c_write(0x06, 0x00);
	tp2803_i2c_write(0x07, 0x41);
	tp2803_i2c_write(0x08, 0x76);
	tp2803_i2c_write(0x09, 0x76);
	tp2803_i2c_write(0x0A, 0x76);
	tp2803_i2c_write(0x0B, 0x05);
	tp2803_i2c_write(0x0C, 0x04);
	tp2803_i2c_write(0x0D, 0x04);
	tp2803_i2c_write(0x0E, 0x82);
	tp2803_i2c_write(0x0F, 0x40);
	tp2803_i2c_write(0x10, 0x06);
	tp2803_i2c_write(0x11, 0x3E);
	tp2803_i2c_write(0x12, 0x00);
	tp2803_i2c_write(0x13, 0x3C);
	tp2803_i2c_write(0x14, 0x38);
	tp2803_i2c_write(0x15, 0x38);
	tp2803_i2c_write(0x16, 0xEB);
	tp2803_i2c_write(0x17, 0x08);
	tp2803_i2c_write(0x18, 0x10);
	tp2803_i2c_write(0x19, 0xF0);
	tp2803_i2c_write(0x1A, 0x10);
	tp2803_i2c_write(0x1B, 0xA4);
	tp2803_i2c_write(0x1C, 0x55);
	tp2803_i2c_write(0x1D, 0x76);
	tp2803_i2c_write(0x1E, 0x80);
	tp2803_i2c_write(0x1F, 0x00);
	//tp2803_i2c_write(0x20,0x28);
	//tp2803_i2c_write(0x21,0xAE);
	//tp2803_i2c_write(0x22,0x14);
	//tp2803_i2c_write(0x23,0x7A);

	tp2803_i2c_write(0x20, 0x51);
	tp2803_i2c_write(0x21, 0x5c);
	tp2803_i2c_write(0x22, 0x28);
	tp2803_i2c_write(0x23, 0xf4);

	tp2803_i2c_write(0x24, 0x00);
	tp2803_i2c_write(0x25, 0x00);
	tp2803_i2c_write(0x26, 0x00);
	tp2803_i2c_write(0x27, 0x56);
	tp2803_i2c_write(0x28, 0xd0);
	tp2803_i2c_write(0x29, 0x35);
	tp2803_i2c_write(0x2A, 0xE0);
	tp2803_i2c_write(0x2B, 0x00);
	tp2803_i2c_write(0x2C, 0x00);
	tp2803_i2c_write(0x2D, 0x00);
	tp2803_i2c_write(0x2E, 0x00);
	tp2803_i2c_write(0x2F, 0x00);
	tp2803_i2c_write(0x30, 0x00);
	tp2803_i2c_write(0x31, 0x00);
	tp2803_i2c_write(0x32, 0x00);
	tp2803_i2c_write(0x33, 0x00);
	tp2803_i2c_write(0x34, 0x00);
	tp2803_i2c_write(0x35, 0x00);
	tp2803_i2c_write(0x36, 0x00);
	tp2803_i2c_write(0x37, 0x00);
	tp2803_i2c_write(0x38, 0x00);
	tp2803_i2c_write(0x39, 0x00);
	tp2803_i2c_write(0x3A, 0x00);
	tp2803_i2c_write(0x3B, 0x00);
	tp2803_i2c_write(0x3C, 0x10);
	tp2803_i2c_write(0x3D, 0x80);
	tp2803_i2c_write(0x3E, 0x18);
	tp2803_i2c_write(0x3F, 0x00);
	tp2803_i2c_write(0x40, 0x00);
	tp2803_i2c_write(0x41, 0x01);
	tp2803_i2c_write(0x42, 0x12);
	tp2803_i2c_write(0x43, 0x08);
	tp2803_i2c_write(0x44, 0x4E);
	tp2803_i2c_write(0x45, 0xCB);
	tp2803_i2c_write(0x46, 0xAA);
	tp2803_i2c_write(0x47, 0x1F);
	tp2803_i2c_write(0x48, 0xFA);
	tp2803_i2c_write(0x49, 0x00);
	tp2803_i2c_write(0x4A, 0x07);
	tp2803_i2c_write(0x4B, 0x08);
	tp2803_i2c_write(0x4C, 0x00);
	tp2803_i2c_write(0x4D, 0x00);
	tp2803_i2c_write(0x4E, 0x00);
	tp2803_i2c_write(0x4F, 0x00);
	tp2803_i2c_write(0x50, 0x00);
	tp2803_i2c_write(0x51, 0x00);
	tp2803_i2c_write(0x52, 0x00);
	tp2803_i2c_write(0x53, 0x00);
	tp2803_i2c_write(0x54, 0x00);
	tp2803_i2c_write(0x55, 0x00);
	tp2803_i2c_write(0x56, 0x27);
	tp2803_i2c_write(0x57, 0x04);
	tp2803_i2c_write(0x58, 0xBD);
	tp2803_i2c_write(0x59, 0xA1);
	tp2803_i2c_write(0x5A, 0x0D);
	tp2803_i2c_write(0x5B, 0x00);
	tp2803_i2c_write(0x5C, 0x0B);
	tp2803_i2c_write(0x5D, 0x0C);
	tp2803_i2c_write(0x5E, 0x21);
	tp2803_i2c_write(0x5F, 0x00);
	tp2803_i2c_write(0x60, 0x00);
	tp2803_i2c_write(0x61, 0x00);
	tp2803_i2c_write(0x62, 0x00);
	tp2803_i2c_write(0x63, 0x00);
	tp2803_i2c_write(0x64, 0x00);
	tp2803_i2c_write(0x65, 0x00);
	tp2803_i2c_write(0x66, 0x00);
	tp2803_i2c_write(0x67, 0x00);
	tp2803_i2c_write(0x68, 0x00);
	tp2803_i2c_write(0x69, 0x00);
	tp2803_i2c_write(0x6A, 0xFA);
	tp2803_i2c_write(0x6B, 0x90);
	tp2803_i2c_write(0x6C, 0x00);
	tp2803_i2c_write(0x6D, 0x00);
	tp2803_i2c_write(0x6E, 0x00);
	tp2803_i2c_write(0x6F, 0x00);
	tp2803_i2c_write(0x70, 0x00);
	tp2803_i2c_write(0x71, 0x00);
	tp2803_i2c_write(0x72, 0x00);
	tp2803_i2c_write(0x73, 0x00);
	tp2803_i2c_write(0x74, 0x00);
	tp2803_i2c_write(0x75, 0x00);
	tp2803_i2c_write(0x76, 0x00);
	tp2803_i2c_write(0x77, 0x00);
	tp2803_i2c_write(0x78, 0x00);
	tp2803_i2c_write(0x79, 0x00);
	tp2803_i2c_write(0x7A, 0x00);
	tp2803_i2c_write(0x7B, 0x8A);
	tp2803_i2c_write(0x7C, 0x00);
	tp2803_i2c_write(0x7D, 0x00);
	tp2803_i2c_write(0x7E, 0x00);
	tp2803_i2c_write(0x7F, 0x00);
	tp2803_i2c_write(0x80, 0x00);
	tp2803_i2c_write(0x81, 0x00);
	tp2803_i2c_write(0x82, 0x00);
	tp2803_i2c_write(0x83, 0x00);
	tp2803_i2c_write(0x84, 0x00);
	tp2803_i2c_write(0x85, 0x00);
	tp2803_i2c_write(0x86, 0x00);
	tp2803_i2c_write(0x87, 0x00);
	tp2803_i2c_write(0x88, 0x00);
	tp2803_i2c_write(0x89, 0x00);
	tp2803_i2c_write(0x8A, 0x00);
	tp2803_i2c_write(0x8B, 0x00);
	tp2803_i2c_write(0x8C, 0x00);
	tp2803_i2c_write(0x8D, 0x00);
	tp2803_i2c_write(0x8E, 0x00);
	tp2803_i2c_write(0x8F, 0x00);
	tp2803_i2c_write(0x90, 0x00);
	tp2803_i2c_write(0x91, 0x00);
	tp2803_i2c_write(0x92, 0x00);
	tp2803_i2c_write(0x93, 0x00);
	tp2803_i2c_write(0x94, 0x00);
	tp2803_i2c_write(0x95, 0x00);
	tp2803_i2c_write(0x96, 0x00);
	tp2803_i2c_write(0x97, 0x00);
	tp2803_i2c_write(0x98, 0x00);
	tp2803_i2c_write(0x99, 0x00);
	tp2803_i2c_write(0x9A, 0x00);
	tp2803_i2c_write(0x9B, 0x00);
	tp2803_i2c_write(0x9C, 0x00);
	tp2803_i2c_write(0x9D, 0x00);
	tp2803_i2c_write(0x9E, 0x00);
	tp2803_i2c_write(0x9F, 0x00);
	tp2803_i2c_write(0xA0, 0x00);
	tp2803_i2c_write(0xA1, 0x00);
	tp2803_i2c_write(0xA2, 0x00);
	tp2803_i2c_write(0xA3, 0x00);
	tp2803_i2c_write(0xA4, 0x00);
	tp2803_i2c_write(0xA5, 0x00);
	tp2803_i2c_write(0xA6, 0x00);
	tp2803_i2c_write(0xA7, 0x00);
	tp2803_i2c_write(0xA8, 0x00);
	tp2803_i2c_write(0xA9, 0x00);
	tp2803_i2c_write(0xAA, 0x00);
	tp2803_i2c_write(0xAB, 0x00);
	tp2803_i2c_write(0xAC, 0x00);
	tp2803_i2c_write(0xAD, 0x00);
	tp2803_i2c_write(0xAE, 0x00);
	tp2803_i2c_write(0xAF, 0x00);
	tp2803_i2c_write(0xB0, 0x00);
	tp2803_i2c_write(0xB1, 0x00);
	tp2803_i2c_write(0xB2, 0x00);
	tp2803_i2c_write(0xB3, 0x00);
	tp2803_i2c_write(0xB4, 0x00);
	tp2803_i2c_write(0xB5, 0x00);
	tp2803_i2c_write(0xB6, 0x00);
	tp2803_i2c_write(0xB7, 0x00);
	tp2803_i2c_write(0xB8, 0x00);
	tp2803_i2c_write(0xB9, 0x00);
	tp2803_i2c_write(0xBA, 0x00);
	tp2803_i2c_write(0xBB, 0x00);
	tp2803_i2c_write(0xBC, 0x00);
	tp2803_i2c_write(0xBD, 0x00);
	tp2803_i2c_write(0xBE, 0x00);
	tp2803_i2c_write(0xBF, 0x00);
	tp2803_i2c_write(0xC0, 0x00);
	tp2803_i2c_write(0xC1, 0x00);
	tp2803_i2c_write(0xC2, 0x00);
	tp2803_i2c_write(0xC3, 0x00);
	tp2803_i2c_write(0xC4, 0x00);
	tp2803_i2c_write(0xC5, 0x00);
	tp2803_i2c_write(0xC6, 0x00);
	tp2803_i2c_write(0xC7, 0x00);
	tp2803_i2c_write(0xC8, 0x00);
	tp2803_i2c_write(0xC9, 0x00);
	tp2803_i2c_write(0xCA, 0x00);
	tp2803_i2c_write(0xCB, 0x00);
	tp2803_i2c_write(0xCC, 0x00);
	tp2803_i2c_write(0xCD, 0x00);
	tp2803_i2c_write(0xCE, 0x00);
	tp2803_i2c_write(0xCF, 0x00);
	tp2803_i2c_write(0xD0, 0x00);
	tp2803_i2c_write(0xD1, 0x00);
	tp2803_i2c_write(0xD2, 0x00);
	tp2803_i2c_write(0xD3, 0x00);
	tp2803_i2c_write(0xD4, 0x00);
	tp2803_i2c_write(0xD5, 0x00);
	tp2803_i2c_write(0xD6, 0x00);
	tp2803_i2c_write(0xD7, 0x00);
	tp2803_i2c_write(0xD8, 0x00);
	tp2803_i2c_write(0xD9, 0x00);
	tp2803_i2c_write(0xDA, 0x00);
	tp2803_i2c_write(0xDB, 0x00);
	tp2803_i2c_write(0xDC, 0x00);
	tp2803_i2c_write(0xDD, 0x00);
	tp2803_i2c_write(0xDE, 0x00);
	tp2803_i2c_write(0xDF, 0x00);
	tp2803_i2c_write(0xE0, 0x00);
	tp2803_i2c_write(0xE1, 0x00);
	tp2803_i2c_write(0xE2, 0x00);
	tp2803_i2c_write(0xE3, 0x00);
	tp2803_i2c_write(0xE4, 0x00);
	tp2803_i2c_write(0xE5, 0x00);
	tp2803_i2c_write(0xE6, 0x00);
	tp2803_i2c_write(0xE7, 0x00);
	tp2803_i2c_write(0xE8, 0x00);
	tp2803_i2c_write(0xE9, 0x00);
	tp2803_i2c_write(0xEA, 0x00);
	tp2803_i2c_write(0xEB, 0x00);
	tp2803_i2c_write(0xEC, 0x00);
	tp2803_i2c_write(0xED, 0x00);
	tp2803_i2c_write(0xEE, 0x00);
	tp2803_i2c_write(0xEF, 0x00);
	tp2803_i2c_write(0xF0, 0x00);
	tp2803_i2c_write(0xF1, 0x00);
	tp2803_i2c_write(0xF2, 0x00);
	tp2803_i2c_write(0xF3, 0x00);
	tp2803_i2c_write(0xF4, 0x00);
	tp2803_i2c_write(0xF5, 0x00);
	tp2803_i2c_write(0xF6, 0x00);
	tp2803_i2c_write(0xF7, 0x00);
	tp2803_i2c_write(0xF8, 0x00);
	tp2803_i2c_write(0xF9, 0x00);
	tp2803_i2c_write(0xFA, 0x00);
	tp2803_i2c_write(0xFB, 0x00);
	tp2803_i2c_write(0xFC, 0x12);
	tp2803_i2c_write(0xFD, 0x00);
	tp2803_i2c_write(0xFE, 0x29);
	tp2803_i2c_write(0xFF, 0x10);
	sc_always("exit");
}
static void tp2803_set_timing_v0(enum tp2803_timing vmode)
{
	//to read the id register,
	int data = 0;
	tp2803_i2c_read(0x00, &data);
	sc_always("read 0x00:%d,excepted 0x08 vmode=%d", data, vmode);
	//set the init setting
	switch(vmode) {
	case TP2803_1280_720_25P:
		tp2803_set_timing_1280_720_25();
		break;
	case TP2803_1280_720_30P:
		tp2803_set_timing_1280_720_30();
		break;
	default:
		sc_err("not supported setting");
		break;
	}
}

static void tp2803_set_timing(enum tp2803_timing vmode)
{
	tp2803_set_timing_v0(vmode);
}

static int set_i2s_MCLK_2803(int freq)
{
	double divr_manual_float;
	unsigned int divr_manual;
	// int post_div2;

	divr_manual_float = ( (double)freq) / AUPLL_FIX_CLK_FREQ;
	// sc_always("divr_manual_float %d/10000", (int)(divr_manual_float*10000));
	divr_manual = (unsigned int)( divr_manual_float * (1 << PLL_DIVR_INTER_POS) );

	sc_always("input clk %d/10000 - divr manual 0x%x", (int)(freq * 10000), divr_manual);

	//set
	int val = read_reg32(SC_CLK_ENABLE_I2S_CLK_FOR_SENSOR);
	val |= (1 << 4); //i2s1 -> master
	val &= ~(1 << 19);
	val &= ~(1 << 28);
	val &= ~(0x7 << 20);

	write_reg32(SC_CLK_ENABLE_I2S_CLK_FOR_SENSOR, val);    //enable PLL

	write_reg32(SC_CLK_AU_PLL_0, 0x40);//B79
	write_reg32(SC_CLK_AU_PLL_1, 0x08);//B81 + PosDiv2 = 1

	// write_reg32(SC_CLK_AU_PLL_DIG_REG_0, 0x00ED9999);
	write_reg32(SC_CLK_AU_PLL_DIG_REG_0, divr_manual);
	write_reg32(SC_CLK_AU_PLL_DIG_REG_1, 0x0000501E);
	write_reg32(SC_CLK_AU_PLL_DIG_REG_1, 0x00305016);
	write_reg32(SC_CLK_AU_PLL_DIG_REG_1, 0x0030501E);

	return 0;
}
static int set_display_interface_res_infor_dev(disp_res_infor_t *res_infor)
{
	if(res_infor) {
		//  TODO: add interlace type
		//clk
		/*
		 * GPIO config, use new GPIO API
		 * TODO
		 *
		        gpio_pad_set_func(67,0);
		*/
		set_i2s_MCLK_2803(27);
		//sc_clk_set_i2s_clock_for_sensor_double(27);
		tp2803_i2c_init();
		tp2803_reset(10, 30, 10);
		tp2803_set_timing(sc_cfg_get_dvp_timing(res_infor->h_valid_pixels, res_infor->v_valid_lines, res_infor->fps));

	} else {
		sc_err("null input");
	}
	return 0;
}
display_intgerface_ops_t dvp_dev_tp2803 = {
	.interface_index = DVP_DEV_TP2803,
	.init_display_interface = init_interface_dev,
	.get_display_interface_desc = get_display_interface_desc_dev,
	.get_display_interface_res_infor = get_display_interface_res_infor_dev,
	.set_display_interface_res_infor = set_display_interface_res_infor_dev,
};

