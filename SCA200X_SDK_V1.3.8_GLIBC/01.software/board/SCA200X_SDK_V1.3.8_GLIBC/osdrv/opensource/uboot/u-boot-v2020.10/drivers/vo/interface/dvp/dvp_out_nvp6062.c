#include "vo/interface/display_interface.h"
#include <asm/arch/gpio.h>
#include <linux/types.h>
#include <config.h>
#include <i2c.h>
#define nvp60621_wr_addr 0x60
#define nvp60621_i2c_component 1
#define nvp60621_rest_port GPIO_PORT_A
#define nvp60621_rest_group GPIO_GROUP_0
#define nvp60621_rest_number 6
#define nvp60621_rest_pad     8  //GP6 AG1
//#define nvp60621_wr_addr 0x62
//#define nvp60621_wr_addr 0x64
//#define nvp60621_wr_addr 0x66
#define BANK0 0
#define BANK1 1
#define BANK2 2
static void nvp60621_reset(int delay1, int delay2, int delay3);
static int sc_cfg_get_dvp_timing(int width, int height, float fps);
enum nvp60621_timing {
	NVP60621_1920_1080_30P,
	NVP60621_1920_1080_25P,
	NVP60621_1280_720_60P,
	NVP60621_1280_720_50P,
	NVP60621_1280_720_30P,
	NVP60621_1280_720_25P,
	NVP60621_1280_720_30P_8BIT,
	NVP60621_1280_720_25P_8BIT,
};
static void nvp60621_set_timing(enum nvp60621_timing vmode);
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
	6,
	/*disp_res_infor_t res[MAX_SUPPORTED_RES];*/
	{
		{1280, 3960, 1390, 1430, 720, 750, 725, 730, 25},
		{1280, 3300, 1720, 1760, 720, 750, 725, 730, 30},
		{1280, 1980, 1720, 1760, 720, 750, 725, 730, 50},
		{1280, 1650, 1390, 1430, 720, 750, 725, 730, 60},
		{1920, 2640, 2448, 2492, 1080, 1125, 1084, 1089, 25},
		{1920, 2200, 2008, 2052, 1080, 1125, 1084, 1089, 30},
	},
};

static int  nvp60621_i2c_init()
{
	int ret = 0;
	i2c_set_bus_num(nvp60621_i2c_component);
	return ret;
}

static int  nvp60621_i2c_write(int address, int data)
{
	uint8_t buffer[2];
	int ret = 0;
	buffer[0] = (address) & 0xff;
	buffer[1] = (data) & 0xff;
	I2C_SET_BUS(nvp60621_i2c_component);
	ret = i2c_write(nvp60621_wr_addr >> 1, buffer, 1, &buffer[1], 1);
	return 0;
}

static int  nvp60621_i2c_read(int address, int *data)
{
	uint8_t buffer[2];
	int ret = 0;
	buffer[0] = (address) & 0xff;
	I2C_SET_BUS(nvp60621_i2c_component);
	ret = i2c_read(nvp60621_wr_addr >> 1, buffer, 1, &buffer[2], 1);
	*data = buffer[1];
	return ret;
}

static void nvp60621_reset(int delay1, int delay2, int delay3)
{
	/*
	 * GPIO config, use new GPIO API
	 * TODO
	 *
	      gpio_pad_set_func(nvp60621_rest_pad,0);
	      gpio_pad_set_direction(nvp60621_rest_group,nvp60621_rest_port,nvp60621_rest_number, GPIO_IS_OUTPUT);
	      gpio_pad_set_val(nvp60621_rest_group,nvp60621_rest_port, nvp60621_rest_number, GPIO_IS_HIGH);
	      sc_delay(delay1);
	      gpio_pad_set_val(nvp60621_rest_group,nvp60621_rest_port, nvp60621_rest_number, GPIO_IS_LOW);
	      sc_delay(delay2);
	      gpio_pad_set_val(nvp60621_rest_group,nvp60621_rest_port, nvp60621_rest_number, GPIO_IS_HIGH);
	      sc_delay(delay3);
	*/
}

static int init_interface_dev()
{
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
	enum nvp60621_timing input_opt = NVP60621_1920_1080_30P;
	// config dvp
	sc_always("fps = %d with w=%d h=%d", (int)fps, width, height);
	if((width == 1920) && (height == 1080)) {
		if(abs(25 - (uint32_t)fps) <= 1) {
			input_opt = NVP60621_1920_1080_25P;
		} else if (abs(30 - (uint32_t)fps) <= 1) {
			input_opt = NVP60621_1920_1080_30P;
		} else {
			sc_err("not supported fps = %d with w=%d h=%d", (int)fps, width, height);
		}
	} else if((width == 1280) && (height == 720)) {
		if(abs(25 - (uint32_t)fps) <= 1) {
			input_opt = NVP60621_1280_720_25P;
		} else if (abs(30 - (uint32_t)fps) <= 1) {
			input_opt = NVP60621_1280_720_30P;
		} else if (abs(50 - (uint32_t)fps) <= 1) {
			input_opt = NVP60621_1280_720_50P;
		} else if (abs(60 - (uint32_t)fps) <= 1) {
			input_opt = NVP60621_1280_720_60P;
		} else {
			sc_err("not supported fps = %d with w=%d h=%d", (int)fps, width, height);
		}
	} else {
		sc_err("not supported fps = %d with w=%d h=%d", (int)fps, width, height);
	}
	sc_always("input_opt=%d", input_opt);
	return (int)input_opt;
}

static void nvp60621_set_timing_v0(enum nvp60621_timing vmode)
{
	nvp60621_i2c_write(0xFF, BANK0);
	nvp60621_i2c_write(0x00, 0x01);
	sc_always("vmode =%d\r\n", vmode);

	switch(vmode) {
	case 0 :
		nvp60621_i2c_write(0x01, 0xF0);
		nvp60621_i2c_write(0x04, 0x00);
		break;
	case 1 :
		nvp60621_i2c_write(0x01, 0xF1);
		nvp60621_i2c_write(0x04, 0x00);
		break;
	case 2 :
		nvp60621_i2c_write(0x01, 0xE2);
		nvp60621_i2c_write(0x04, 0x00);
		break;
	case 3 :
		nvp60621_i2c_write(0x01, 0xE3);
		nvp60621_i2c_write(0x04, 0x00);
		break;
	case 4 :
		nvp60621_i2c_write(0x01, 0xEA);
		nvp60621_i2c_write(0x04, 0x00);
		break;
	case 5 :
		nvp60621_i2c_write(0x01, 0xEB);
		nvp60621_i2c_write(0x04, 0x00);
		break;
	case 6 :
		nvp60621_i2c_write(0x01, 0x6A);
		nvp60621_i2c_write(0x04, 0x00);
		break;
	case 7 :
		nvp60621_i2c_write(0x01, 0x6B);
		nvp60621_i2c_write(0x04, 0x00);
		break;
	}

	nvp60621_i2c_write(0xFF, BANK1);
	nvp60621_i2c_write(0x0E, 0x00);

	nvp60621_i2c_write(0xFF, BANK2);
	nvp60621_i2c_write(0x00, 0xFE);

	nvp60621_i2c_write(0x02, 0x00);

	nvp60621_i2c_write(0x04, 0x80);
	nvp60621_i2c_write(0x05, 0x00);
	nvp60621_i2c_write(0x06, 0x10);

	nvp60621_i2c_write(0x0C, 0x04);
	nvp60621_i2c_write(0x0D, 0x3F);
	nvp60621_i2c_write(0x0E, 0x00);

	nvp60621_i2c_write(0x10, 0xEB);
	nvp60621_i2c_write(0x11, 0x10);
	nvp60621_i2c_write(0x12, 0xF0);
	nvp60621_i2c_write(0x13, 0x10);
	nvp60621_i2c_write(0x14, 0x01);
	nvp60621_i2c_write(0x15, 0x00);
	nvp60621_i2c_write(0x16, 0x00);

	nvp60621_i2c_write(0x17, 0x00);
	nvp60621_i2c_write(0x18, 0x00);
	nvp60621_i2c_write(0x19, 0x00);

	nvp60621_i2c_write(0x1C, 0x80);
	nvp60621_i2c_write(0x1D, 0x80);
	nvp60621_i2c_write(0x1E, 0x80);

	nvp60621_i2c_write(0x20, 0x00);
	nvp60621_i2c_write(0x21, 0x00);
	nvp60621_i2c_write(0x22, 0x00);
	nvp60621_i2c_write(0x23, 0x00);
	nvp60621_i2c_write(0x24, 0x00);
	nvp60621_i2c_write(0x25, 0x00);
	nvp60621_i2c_write(0x26, 0x00);
	nvp60621_i2c_write(0x27, 0x00);
	nvp60621_i2c_write(0x28, 0x00);
	nvp60621_i2c_write(0x29, 0x00);
	nvp60621_i2c_write(0x2A, 0x00);
	nvp60621_i2c_write(0x2B, 0x00);
	nvp60621_i2c_write(0x2C, 0x00);
	nvp60621_i2c_write(0x2D, 0x00);
	nvp60621_i2c_write(0x2E, 0x00);
	nvp60621_i2c_write(0x2F, 0x00);
	nvp60621_i2c_write(0x30, 0x00);
	nvp60621_i2c_write(0x31, 0x00);
	nvp60621_i2c_write(0x32, 0x00);
	nvp60621_i2c_write(0x33, 0x00);
	nvp60621_i2c_write(0x34, 0x00);

	nvp60621_i2c_write(0x36, 0x01);
	nvp60621_i2c_write(0x37, 0x80);

	nvp60621_i2c_write(0x39, 0x00);

	nvp60621_i2c_write(0x3C, 0x00);
	nvp60621_i2c_write(0x3D, 0x00);
	nvp60621_i2c_write(0x3E, 0x00);

	nvp60621_i2c_write(0x40, 0x01);
	nvp60621_i2c_write(0x41, 0xFF);
	nvp60621_i2c_write(0x42, 0x80);

	nvp60621_i2c_write(0x48, 0x00);
	nvp60621_i2c_write(0x49, 0x00);
	nvp60621_i2c_write(0x4A, 0x00);
	nvp60621_i2c_write(0x4B, 0x00);
	nvp60621_i2c_write(0x4C, 0x00);
	nvp60621_i2c_write(0x4D, 0x00);
	nvp60621_i2c_write(0x4E, 0x00);
	nvp60621_i2c_write(0x4F, 0x00);
	nvp60621_i2c_write(0x50, 0x00);
	nvp60621_i2c_write(0x51, 0x00);
	nvp60621_i2c_write(0x52, 0x00);
	nvp60621_i2c_write(0x53, 0x00);
	nvp60621_i2c_write(0x54, 0x00);
	nvp60621_i2c_write(0x55, 0x00);
	nvp60621_i2c_write(0x56, 0x00);
	nvp60621_i2c_write(0x57, 0x00);
	nvp60621_i2c_write(0x59, 0x00);
	nvp60621_i2c_write(0x5A, 0x00);
	nvp60621_i2c_write(0x5C, 0x00);
	nvp60621_i2c_write(0x5D, 0x00);
	nvp60621_i2c_write(0x5E, 0x00);
	nvp60621_i2c_write(0x5F, 0x00);

	nvp60621_i2c_write(0x60, 0x80);
	nvp60621_i2c_write(0x61, 0x80);
	nvp60621_i2c_write(0x63, 0xE0);
	nvp60621_i2c_write(0x64, 0x19);
	nvp60621_i2c_write(0x65, 0x04);
	nvp60621_i2c_write(0x66, 0xEB);
	nvp60621_i2c_write(0x67, 0x60);
	nvp60621_i2c_write(0x68, 0x00);
	nvp60621_i2c_write(0x69, 0x00);
	nvp60621_i2c_write(0x6A, 0x00);
	nvp60621_i2c_write(0x6B, 0x00);

	switch(vmode) {
	case 0 :
		nvp60621_i2c_write(0x3A, 0x11);
		nvp60621_i2c_write(0x01, 0xF0);
		break;
	case 1 :
		nvp60621_i2c_write(0x3A, 0x11);
		nvp60621_i2c_write(0x01, 0xF1);
		break;
	case 2 :
		nvp60621_i2c_write(0x3A, 0x11);
		nvp60621_i2c_write(0x01, 0xE2);
		break;
	case 3 :
		nvp60621_i2c_write(0x3A, 0x11);
		nvp60621_i2c_write(0x01, 0xE3);
		break;
	case 4 :
		nvp60621_i2c_write(0x3A, 0x13);
		nvp60621_i2c_write(0x01, 0xEA);
		break;
	case 5 :
		nvp60621_i2c_write(0x3A, 0x13);
		nvp60621_i2c_write(0x01, 0xEB);
		break;
	case 6 :
		nvp60621_i2c_write(0x3A, 0x13);
		nvp60621_i2c_write(0x01, 0x6A);
		break;
	case 7 :
		nvp60621_i2c_write(0x3A, 0x13);
		nvp60621_i2c_write(0x01, 0x6B);
		break;

	}

	nvp60621_i2c_write(0xFF, BANK2);

	switch(vmode) {
	case 0 :    //1080p 30
		nvp60621_i2c_write(0x3D, 0x80);   //pn enable

		nvp60621_i2c_write(0x5C, 0x52);   //pn value
		nvp60621_i2c_write(0x5D, 0xCA);   //pn value
		nvp60621_i2c_write(0x5E, 0xF0);   //pn value
		nvp60621_i2c_write(0x5F, 0x2C);   //pn value

		nvp60621_i2c_write(0x1D, 0xB0);   //cb scale
		nvp60621_i2c_write(0x1E, 0xB0);   //cr scale

		nvp60621_i2c_write(0x37, 0xB0);   //burst scale
		break;
	case 1 :    //1080P 25
		nvp60621_i2c_write(0x3D, 0x80);   //pn enable

		nvp60621_i2c_write(0x5C, 0x52);   //pn value
		nvp60621_i2c_write(0x5D, 0xC3);   //pn value
		nvp60621_i2c_write(0x5E, 0x7D);   //pn value
		nvp60621_i2c_write(0x5F, 0xC8);   //pn value

		nvp60621_i2c_write(0x1D, 0xB0);   //cb scale
		nvp60621_i2c_write(0x1E, 0xB0);   //cr scale

		nvp60621_i2c_write(0x37, 0xB0);   //burst scale
		break;
	case 2 :    //720P 60
		nvp60621_i2c_write(0x3D, 0x80);   //pn enable

		nvp60621_i2c_write(0x5C, 0x52);   //pn value
		nvp60621_i2c_write(0x5D, 0xC5);   //pn value
		nvp60621_i2c_write(0x5E, 0xF9);   //pn value
		nvp60621_i2c_write(0x5F, 0x2C);   //pn value

		nvp60621_i2c_write(0x1D, 0xB0);   //cb scale
		nvp60621_i2c_write(0x1E, 0xB0);   //cr scale

		nvp60621_i2c_write(0x37, 0xB0);   //burst scale
		break;
	case 3 :    //720P 50
		nvp60621_i2c_write(0x3D, 0x80);   //pn enable

		nvp60621_i2c_write(0x5C, 0x52);   //pn value
		nvp60621_i2c_write(0x5D, 0xCF);   //pn value
		nvp60621_i2c_write(0x5E, 0xE7);   //pn value
		nvp60621_i2c_write(0x5F, 0x2C);   //pn value

		nvp60621_i2c_write(0x1D, 0xB0);   //cb scale
		nvp60621_i2c_write(0x1E, 0xB0);   //cr scale

		nvp60621_i2c_write(0x37, 0xB0);   //burst scale
		break;
	case 4 :    //720P 30
		break;
	case 5 :    //720P 25
		break;
	case 6:
		break;
	case 7:
		break;
	}
	//nvp60621_i2c_write(0xFF, BANK2);
	//nvp60621_i2c_write(0x04, 0x81);  //horizontal color bar test pattern outputs
	sc_always("OK\r\n");
}
static void nvp60621_set_timing_v1_720p50_60(enum nvp60621_timing vmode)
{
	nvp60621_i2c_write(0xFF, 0x00); // BANK0
	nvp60621_i2c_write(0x00, 0x07); // PLL & CLOCK PHASE manual setting
	if(vmode == NVP60621_1280_720_50P)
		nvp60621_i2c_write(0x01, 0xE3); //720P50 74.25MHz mode
	else
		nvp60621_i2c_write(0x01, 0xE2); //720P60 74.25MHz mode
	// PLL
	nvp60621_i2c_write(0x05, 0x00);
	nvp60621_i2c_write(0x06, 0x40);
	nvp60621_i2c_write(0x07, 0x18);

	nvp60621_i2c_write(0x05, 0xFF);
	sc_delay(1);
	nvp60621_i2c_write(0x05, 0x00);

	// CLOCK PHASE
	nvp60621_i2c_write(0x11, 0x00);
	nvp60621_i2c_write(0x12, 0x40);

	nvp60621_i2c_write(0xFF, 0x01); //BANK1
	nvp60621_i2c_write(0x0E, 0x00); //Y_C REVERSE

	nvp60621_i2c_write(0xFF, 0x02); //BANK2

	nvp60621_i2c_write(0x00, 0xFE);
	nvp60621_i2c_write(0x01, 0xEA);
	nvp60621_i2c_write(0x02, 0x00);
	nvp60621_i2c_write(0x03, 0x00);
	nvp60621_i2c_write(0x04, 0x50);
	nvp60621_i2c_write(0x05, 0x00);
	nvp60621_i2c_write(0x06, 0x10);
	nvp60621_i2c_write(0x07, 0x00);
	nvp60621_i2c_write(0x08, 0x00);
	nvp60621_i2c_write(0x09, 0x00);
	nvp60621_i2c_write(0x0A, 0x00);
	nvp60621_i2c_write(0x0B, 0x00);
	nvp60621_i2c_write(0x0C, 0x04);
	nvp60621_i2c_write(0x0D, 0x3F);
	nvp60621_i2c_write(0x0E, 0x00);
	nvp60621_i2c_write(0x0F, 0x00);

	nvp60621_i2c_write(0x10, 0xEB);
	nvp60621_i2c_write(0x11, 0x10);
	nvp60621_i2c_write(0x12, 0xF0);
	nvp60621_i2c_write(0x13, 0x10);
	nvp60621_i2c_write(0x14, 0x01);
	nvp60621_i2c_write(0x15, 0x00);
	nvp60621_i2c_write(0x16, 0x00);
	nvp60621_i2c_write(0x17, 0x00);
	nvp60621_i2c_write(0x18, 0x00);
	nvp60621_i2c_write(0x19, 0x00);
	nvp60621_i2c_write(0x1A, 0x00);
	nvp60621_i2c_write(0x1B, 0x00);
	nvp60621_i2c_write(0x1C, 0x80);
	nvp60621_i2c_write(0x1D, 0x80);
	nvp60621_i2c_write(0x1E, 0x80);
	nvp60621_i2c_write(0x1F, 0x00);

	nvp60621_i2c_write(0x20, 0x00);
	nvp60621_i2c_write(0x21, 0x00);
	nvp60621_i2c_write(0x22, 0x00);
	nvp60621_i2c_write(0x23, 0x00);
	nvp60621_i2c_write(0x24, 0x00);
	nvp60621_i2c_write(0x25, 0x00);
	nvp60621_i2c_write(0x26, 0x00);
	nvp60621_i2c_write(0x27, 0x00);
	nvp60621_i2c_write(0x28, 0x00);
	nvp60621_i2c_write(0x29, 0x00);
	nvp60621_i2c_write(0x2A, 0x00);
	nvp60621_i2c_write(0x2B, 0x00);
	nvp60621_i2c_write(0x2C, 0x00);
	nvp60621_i2c_write(0x2D, 0x00);
	nvp60621_i2c_write(0x2E, 0x00);
	nvp60621_i2c_write(0x2F, 0x00);

	nvp60621_i2c_write(0x30, 0x00);
	nvp60621_i2c_write(0x31, 0x00);
	nvp60621_i2c_write(0x32, 0x00);
	nvp60621_i2c_write(0x33, 0x00);
	nvp60621_i2c_write(0x34, 0x00);
	nvp60621_i2c_write(0x35, 0x00);
	nvp60621_i2c_write(0x36, 0x01);
	nvp60621_i2c_write(0x37, 0x80);
	nvp60621_i2c_write(0x38, 0x00);
	nvp60621_i2c_write(0x39, 0x00);
	nvp60621_i2c_write(0x3A, 0x13);
	nvp60621_i2c_write(0x3B, 0x00);
	nvp60621_i2c_write(0x3C, 0x00);
	nvp60621_i2c_write(0x3D, 0x00);
	nvp60621_i2c_write(0x3E, 0x00);
	nvp60621_i2c_write(0x3F, 0x00);

	nvp60621_i2c_write(0x40, 0x01);
	nvp60621_i2c_write(0x41, 0xFF);
	nvp60621_i2c_write(0x42, 0x80);
	nvp60621_i2c_write(0x43, 0x00);
	nvp60621_i2c_write(0x44, 0x00);
	nvp60621_i2c_write(0x45, 0x00);
	nvp60621_i2c_write(0x46, 0x00);
	nvp60621_i2c_write(0x47, 0x00);
	nvp60621_i2c_write(0x48, 0x00);
	nvp60621_i2c_write(0x49, 0x00);
	nvp60621_i2c_write(0x4A, 0x00);
	nvp60621_i2c_write(0x4B, 0x00);
	nvp60621_i2c_write(0x4C, 0x00);
	nvp60621_i2c_write(0x4D, 0x00);
	nvp60621_i2c_write(0x4E, 0x00);
	nvp60621_i2c_write(0x4F, 0x00);

	nvp60621_i2c_write(0x50, 0x00);
	nvp60621_i2c_write(0x51, 0x00);
	nvp60621_i2c_write(0x52, 0x00);
	nvp60621_i2c_write(0x53, 0x00);
	nvp60621_i2c_write(0x54, 0x00);
	nvp60621_i2c_write(0x55, 0x00);
	nvp60621_i2c_write(0x56, 0x00);
	nvp60621_i2c_write(0x57, 0x00);
	nvp60621_i2c_write(0x58, 0x00);
	nvp60621_i2c_write(0x59, 0x00);
	nvp60621_i2c_write(0x5A, 0x00);
	nvp60621_i2c_write(0x5B, 0x00);
	nvp60621_i2c_write(0x5C, 0x00);
	nvp60621_i2c_write(0x5D, 0x00);
	nvp60621_i2c_write(0x5E, 0x00);
	nvp60621_i2c_write(0x5F, 0x00);

	nvp60621_i2c_write(0x60, 0x80);
	nvp60621_i2c_write(0x61, 0x80);
	nvp60621_i2c_write(0x62, 0x00);
	nvp60621_i2c_write(0x63, 0xE0);
	nvp60621_i2c_write(0x64, 0x19);
	nvp60621_i2c_write(0x65, 0x04);
	nvp60621_i2c_write(0x66, 0xEB);
	nvp60621_i2c_write(0x67, 0x60);
	nvp60621_i2c_write(0x68, 0x00);
	nvp60621_i2c_write(0x69, 0x00);
	nvp60621_i2c_write(0x6A, 0x00);
	nvp60621_i2c_write(0x6B, 0x00);
	nvp60621_i2c_write(0x6C, 0x00);
	nvp60621_i2c_write(0x6D, 0x00);
	nvp60621_i2c_write(0x6E, 0x00);
	nvp60621_i2c_write(0x6F, 0x00);

	nvp60621_i2c_write(0x60, 0x80);
	nvp60621_i2c_write(0x61, 0x80);
	nvp60621_i2c_write(0x62, 0x00);
	nvp60621_i2c_write(0x63, 0xE0);
	nvp60621_i2c_write(0x64, 0x19);
	nvp60621_i2c_write(0x65, 0x04);
	nvp60621_i2c_write(0x66, 0xEB);
	nvp60621_i2c_write(0x67, 0x60);
	nvp60621_i2c_write(0x68, 0x00);
	nvp60621_i2c_write(0x69, 0x00);
	nvp60621_i2c_write(0x6A, 0x00);
	nvp60621_i2c_write(0x6B, 0x00);
	nvp60621_i2c_write(0x6C, 0x00);
	nvp60621_i2c_write(0x6D, 0x00);
	nvp60621_i2c_write(0x6E, 0x00);
	nvp60621_i2c_write(0x6F, 0x00);

	nvp60621_i2c_write(0x70, 0x00);
	nvp60621_i2c_write(0x71, 0x00);
	nvp60621_i2c_write(0x72, 0x00);
	nvp60621_i2c_write(0x73, 0x00);
	nvp60621_i2c_write(0x74, 0x00);
	nvp60621_i2c_write(0x75, 0x00);
	nvp60621_i2c_write(0x76, 0x00);
	nvp60621_i2c_write(0x77, 0x00);
	nvp60621_i2c_write(0x78, 0x00);
	nvp60621_i2c_write(0x79, 0x00);
	nvp60621_i2c_write(0x7A, 0x00);
	nvp60621_i2c_write(0x7B, 0x00);
	nvp60621_i2c_write(0x7C, 0x00);
	nvp60621_i2c_write(0x7D, 0x00);
	nvp60621_i2c_write(0x7E, 0x00);
	nvp60621_i2c_write(0x7F, 0x00);

	//  nvp60621_i2c_write(0xFF,0x01); // TEST PATTERN
	//  nvp60621_i2c_write(0x03,0xE0);
	//  nvp60621_i2c_write(0xFF,0x02);
	//  nvp60621_i2c_write(0x04,0x81);
}
void Init_TX_coax(void)
{
	nvp60621_i2c_write(0xFF, 0x02);
	nvp60621_i2c_write(0x63, 0xFF); // TX out-put Threshold
	nvp60621_i2c_write(0xFF, 0x03);

	nvp60621_i2c_write(0x20, 0x2F); // A-CP TX BOUD
	nvp60621_i2c_write(0x23, 0x08); // Line position1
	nvp60621_i2c_write(0x24, 0x00); // Line position2
	nvp60621_i2c_write(0x25, 0x07); // Lines count
	nvp60621_i2c_write(0x2B, 0x10); // A-CP mode choose
	nvp60621_i2c_write(0x2D, 0x0D); // Start point1
	nvp60621_i2c_write(0x2E, 0x01); // Start point2
	nvp60621_i2c_write(0xA9, 0x00); // i2c master mode off

	nvp60621_i2c_write(0x30, 0x55); // HEADER
	nvp60621_i2c_write(0x31, 0x24); // EQ Pattern [7:4]=device NVP6021 set, [3:2]=EQ Pattern [1]=Color status
	nvp60621_i2c_write(0x32, 0x00);
	nvp60621_i2c_write(0x33, 0x00);
	nvp60621_i2c_write(0x34, 0x00);
	nvp60621_i2c_write(0x35, 0x00);
	nvp60621_i2c_write(0x36, 0x00);
	nvp60621_i2c_write(0x37, 0x00);
	nvp60621_i2c_write(0x29, 0x08); // Status out
}
void Init_RX_coax(void)
{
	nvp60621_i2c_write(0xFF, 0x00);
	nvp60621_i2c_write(0x17, 0x00);     //RX Threshold on
	nvp60621_i2c_write(0xFF, 0x03);

	nvp60621_i2c_write(0x09, 0x00);
	nvp60621_i2c_write(0x80, 0x55);//RX ID
	nvp60621_i2c_write(0x82, 0x10);
	nvp60621_i2c_write(0x83, 0x01);
	nvp60621_i2c_write(0x86, 0x80);
	nvp60621_i2c_write(0x87, 0x01);
	nvp60621_i2c_write(0x88, 0x20);

	nvp60621_i2c_write(0x30, 0x55); //TX ID
}
static void nvp60621_set_timing_v1_720P_1080P_30_25(enum nvp60621_timing vmode)
{
	if (vmode == NVP60621_1920_1080_25P || NVP60621_1920_1080_30P == vmode) {
		if (vmode == NVP60621_1920_1080_25P) {
			nvp60621_i2c_write(0xFF, 0x00);
			nvp60621_i2c_write(0x01, 0xF1); //1080P25 74.25M 16bit
			nvp60621_i2c_write(0x04, 0x00);
		}

		else {
			nvp60621_i2c_write(0xFF, 0x00);
			nvp60621_i2c_write(0x01, 0xF0); //1080P30 74.25M 16bit
			nvp60621_i2c_write(0x04, 0x00);
		}
	} else { //720p mode
		if(1) { //(MP(BitWidth) == 16bit)
			if (vmode == NVP60621_1280_720_25P) {
				nvp60621_i2c_write(0xFF, 0x00);
				nvp60621_i2c_write(0x01, 0xEB); //720P25 74.25M 16bit
				nvp60621_i2c_write(0x04, 0x00);
			} else {
				nvp60621_i2c_write(0xFF, 0x00);
				nvp60621_i2c_write(0x01, 0xEA); //720P30 74.25M 16bit
				nvp60621_i2c_write(0x04, 0x00);
			}

		}
	}

	nvp60621_i2c_write(0x00, 0x01); //must be set

	nvp60621_i2c_write(0xFF, 0x01);
	nvp60621_i2c_write(0x0E, 0x00); //Bit swap off

	nvp60621_i2c_write(0xFF, 0x02);
	nvp60621_i2c_write(0x00, 0xFE);

	nvp60621_i2c_write(0x02, 0x00);

	nvp60621_i2c_write(0x04, 0x80); //2015.06.17
	nvp60621_i2c_write(0x05, 0x00); //swap cb-cr
	nvp60621_i2c_write(0x06, 0x10);

	nvp60621_i2c_write(0x0C, 0x04); //SYNC LEVEL
	nvp60621_i2c_write(0x0D, 0x3F); //BLACK LEVEL
	nvp60621_i2c_write(0x0E, 0x00);

	nvp60621_i2c_write(0x10, 0xEB);
	nvp60621_i2c_write(0x11, 0x10);
	nvp60621_i2c_write(0x12, 0xF0);
	nvp60621_i2c_write(0x13, 0x10);
	nvp60621_i2c_write(0x14, 0x01);
	nvp60621_i2c_write(0x15, 0x00);
	nvp60621_i2c_write(0x16, 0x00);
	nvp60621_i2c_write(0x17, 0x00);
	nvp60621_i2c_write(0x18, 0x00);
	nvp60621_i2c_write(0x19, 0x00);

	nvp60621_i2c_write(0x1C, 0x80); //y-scale
	nvp60621_i2c_write(0x1D, 0x80); //cb-scale
	nvp60621_i2c_write(0x1E, 0x80); //cr-scale

	nvp60621_i2c_write(0x20, 0x00);
	nvp60621_i2c_write(0x21, 0x00);
	nvp60621_i2c_write(0x22, 0x00);
	nvp60621_i2c_write(0x23, 0x00);
	nvp60621_i2c_write(0x24, 0x00);
	nvp60621_i2c_write(0x25, 0x00);
	nvp60621_i2c_write(0x26, 0x00);
	nvp60621_i2c_write(0x27, 0x00);
	nvp60621_i2c_write(0x28, 0x00);
	nvp60621_i2c_write(0x29, 0x00);
	nvp60621_i2c_write(0x2A, 0x00);
	nvp60621_i2c_write(0x2B, 0x00);
	nvp60621_i2c_write(0x2C, 0x00);
	nvp60621_i2c_write(0x2D, 0x00);
	nvp60621_i2c_write(0x2E, 0x00);
	nvp60621_i2c_write(0x2F, 0x00);
	nvp60621_i2c_write(0x30, 0x00);
	nvp60621_i2c_write(0x31, 0x00);
	nvp60621_i2c_write(0x32, 0x00);
	nvp60621_i2c_write(0x33, 0x00);
	nvp60621_i2c_write(0x34, 0x00);

	nvp60621_i2c_write(0x36, 0x01);
	nvp60621_i2c_write(0x37, 0x80);

	nvp60621_i2c_write(0x39, 0x00);

	nvp60621_i2c_write(0x3C, 0x00);
	nvp60621_i2c_write(0x3D, 0x00);
	nvp60621_i2c_write(0x3E, 0x00);

	nvp60621_i2c_write(0x40, 0x01);
	nvp60621_i2c_write(0x41, 0xFF);
	nvp60621_i2c_write(0x42, 0x80);

	nvp60621_i2c_write(0x48, 0x00);
	nvp60621_i2c_write(0x49, 0x00);
	nvp60621_i2c_write(0x4A, 0x00);
	nvp60621_i2c_write(0x4B, 0x00);
	nvp60621_i2c_write(0x4C, 0x00);
	nvp60621_i2c_write(0x4D, 0x00);
	nvp60621_i2c_write(0x4E, 0x00);
	nvp60621_i2c_write(0x4F, 0x00);
	nvp60621_i2c_write(0x50, 0x00);
	nvp60621_i2c_write(0x51, 0x00);
	nvp60621_i2c_write(0x52, 0x00);
	nvp60621_i2c_write(0x53, 0x00);
	nvp60621_i2c_write(0x54, 0x00);
	nvp60621_i2c_write(0x55, 0x00);
	nvp60621_i2c_write(0x56, 0x00);
	nvp60621_i2c_write(0x57, 0x00);
	nvp60621_i2c_write(0x59, 0x00);
	nvp60621_i2c_write(0x5A, 0x00);
	nvp60621_i2c_write(0x5C, 0x00);
	nvp60621_i2c_write(0x5D, 0x00);
	nvp60621_i2c_write(0x5E, 0x00);
	nvp60621_i2c_write(0x5F, 0x00);

	nvp60621_i2c_write(0x60, 0x80);
	nvp60621_i2c_write(0x61, 0x80);
	nvp60621_i2c_write(0x63, 0xE0);
	nvp60621_i2c_write(0x64, 0x19);
	nvp60621_i2c_write(0x65, 0x04);
	nvp60621_i2c_write(0x66, 0xEB);
	nvp60621_i2c_write(0x67, 0x60);
	nvp60621_i2c_write(0x68, 0x00);
	nvp60621_i2c_write(0x69, 0x00);
	nvp60621_i2c_write(0x6A, 0x00);
	nvp60621_i2c_write(0x6B, 0x00);

	if (vmode == NVP60621_1920_1080_25P || NVP60621_1920_1080_30P == vmode) {
		if (vmode == NVP60621_1920_1080_25P) {
			nvp60621_i2c_write(0xFF, 0x02);
			nvp60621_i2c_write(0x3A, 0x11);
			nvp60621_i2c_write(0x01, 0xF1); //1080P25
		} else {
			nvp60621_i2c_write(0xFF, 0x02);
			nvp60621_i2c_write(0x3A, 0x11);
			nvp60621_i2c_write(0x01, 0xF0); //1080P30
		}
	} else {
		if (vmode == NVP60621_1920_1080_25P) {
			nvp60621_i2c_write(0xFF, 0x02);
			nvp60621_i2c_write(0x3A, 0x13);
			nvp60621_i2c_write(0x01, 0xEB);//720P25
		} else {
			nvp60621_i2c_write(0xFF, 0x02);
			nvp60621_i2c_write(0x3A, 0x13);
			nvp60621_i2c_write(0x01, 0xEA);//720P30
		}
	}

	if(vmode == NVP60621_1920_1080_25P || NVP60621_1920_1080_30P == vmode) {
		if(vmode == NVP60621_1920_1080_25P) { //1080p25
			nvp60621_i2c_write(0xFF, 0x02);
			nvp60621_i2c_write(0x3D, 0x80);  //pn enable

			nvp60621_i2c_write(0x5c, 0x52);  //pn value
			nvp60621_i2c_write(0x5d, 0xC3);  //pn value
			nvp60621_i2c_write(0x5e, 0x7D);  //pn value
			nvp60621_i2c_write(0x5f, 0xC8);  //pn value

			nvp60621_i2c_write(0x1D, 0x80);  //cb scale -b0
			nvp60621_i2c_write(0x1E, 0x80);  //cr scale -b0

			nvp60621_i2c_write(0x37, 0x80);  //burst scale

			nvp60621_i2c_write(0x5A, 0x07);  //EXPANDER_MODE
		} else { //1080p30
			nvp60621_i2c_write(0xFF, 0x02);
			nvp60621_i2c_write(0x3D, 0x80);  //pn enable

			nvp60621_i2c_write(0x5c, 0x52);  //pn value
			nvp60621_i2c_write(0x5d, 0xCA);  //pn value
			nvp60621_i2c_write(0x5e, 0xF0);  //pn value
			nvp60621_i2c_write(0x5f, 0x2C);  //pn value

			nvp60621_i2c_write(0x1D, 0xA0);  //cb scale - b0
			nvp60621_i2c_write(0x1E, 0xA0);  //cr scale - b0

			nvp60621_i2c_write(0x37, 0xA0);  //burst scale
			nvp60621_i2c_write(0x5A, 0x07);  //EXPANDER_MODE
		}
	}
	Init_TX_coax();
	Init_RX_coax();
}

#define USE_V0
//efine USE_V1

static void nvp60621_set_timing(enum nvp60621_timing vmode)
{
#ifdef USE_V0
	nvp60621_set_timing_v0(vmode);
#endif

#ifdef USE_V1
	if(vmode == NVP60621_1280_720_60P || NVP60621_1280_720_50P == vmode) {
		nvp60621_set_timing_v1_720p50_60(vmode);
	} else {
		nvp60621_set_timing_v1_720P_1080P_30_25(vmode);
	}
#endif
}

static int set_display_interface_res_infor_dev(disp_res_infor_t *res_infor)
{
	if(res_infor) {
		//  TODO: add interlace type
		sc_delay(500); //wait for clk stable
		nvp60621_i2c_init();
		nvp60621_reset(100, 300, 100);
		nvp60621_set_timing(sc_cfg_get_dvp_timing(res_infor->h_valid_pixels, res_infor->v_valid_lines, res_infor->fps));

	} else {
		sc_err("null input");
	}
	return 0;
}
display_intgerface_ops_t dvp_dev_nvp60621 = {
	.interface_index = DVP_DEV_NVP6021,
	.init_display_interface = init_interface_dev,
	.get_display_interface_desc = get_display_interface_desc_dev,
	.get_display_interface_res_infor = get_display_interface_res_infor_dev,
	.set_display_interface_res_infor = set_display_interface_res_infor_dev,
};

