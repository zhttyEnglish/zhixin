#include "vo/interface/display_interface.h"

#define VIDEO_IF_BASE_ADDRESS 0x01100000
#define DVP_CTRL_BASE_ADDR    0x010f0000
#define DVP_OUT_16_OFFSET_REG (VIDEO_IF_BASE_ADDRESS+0x00a4)
#define DVP_OUT_CLIP_CTL_OFFSET (0xac)
#define DVP_OUT_COEFF_CTL_OFFSET (0x160)
#define DVP_GROUP_CTRL_ADDR             (DVP_CTRL_BASE_ADDR+0x7c)

// disp
#define CGU_CTRL_BASE_ADDR 0x01070000
#define CLK_PIXEL_DISP_SEL_SHIFT    15
#define CLK_PIXEL_DISP_SEL_MASK     (1 << CLK_PIXEL_DISP_SEL_SHIFT)

#define DVP_OUT_CSC_CFG_FILE ("/usrdata/csc_dvp_out.json")

extern display_intgerface_ops_t dvp_dev_vga;
extern display_intgerface_ops_t dvp_dev_hdmi;
extern display_intgerface_ops_t dvp_dev_hx8264;
extern display_intgerface_ops_t dvp_dev_nvp60621;
extern display_intgerface_ops_t dvp_dev_tp2803;
extern display_intgerface_ops_t dvp_dev_tc3587;

static int intf_index = DVP_DEV_HDMI;

static display_intgerface_ops_t *dvp_dev_ops[] = {
	//  &dvp_dev_vga,
	//  &dvp_dev_hdmi,
	//  &dvp_dev_nvp60621,
	//  &dvp_dev_hx8264,
	//  &dvp_dev_tp2803,
	//  &dvp_dev_tc3587,
};
static void init_dvp_port(int out_color_mode, ENUM_SC_HAL_PIN_MODE pin_mode, int is_bt656, int clock_polarity;);
void dvp_rgb2yuvcoeff_custom(dvp_csc_pra_t *csc);
static void dvp_out_bprgb2yuvcoeff_config(int mode);

static int get_current_dvp_dev_index(void)
{
	return intf_index;
}

static int set_dvp_interface_index(int index)
{
	sc_always("index=%d", index);
	intf_index = index;
	return 0;
}

static int get_current_dvp_dev_sync_mode(void)
{
	return SYNC_MODE_EXTERNAL;
}

static int get_current_dvp_pad_line_cfg(void)
{
	return -1;
}

static int dvp_out_check_csccfg_custom(char *filename,  char **json)
{
	return -1;
}

int dvp_out_get_json_csccfg(char *content, dvp_csc_pra_t *csc)
{
	return -1;
}

static unsigned int get_dvp_out_clock_phase(void)
{
	return 0;
}

static display_intgerface_ops_t *get_dev_ops(void)
{
	int dev_index = get_current_dvp_dev_index();
	display_intgerface_ops_t *dev_ops = NULL;
	int i = 0;

	for(i = 0; i < sizeof(dvp_dev_ops) / sizeof(display_intgerface_ops_t *); i++) {
		dev_ops = dvp_dev_ops[i];
		if(dev_index == dev_ops->interface_index) {
			return dev_ops;
		}
	}

	sc_err("err intf index=%d", intf_index);
	return NULL;
}

static void dvp_out_base_init(void)
{
	SC_SET_REG_BITS(__REG32__(DVP_GROUP_CTRL_ADDR), 0, 22, 23);

	write_reg32(DVP_CTRL_BASE_ADDR + 0x90, 0x0);  // set dvp portC as output

	uint32_t    sel = 0;
	unsigned int video_clk_ctrl = read_reg32(CGU_CTRL_BASE_ADDR);
	sc_always("video_clk_ctrl:%02x", video_clk_ctrl);
	sel = (video_clk_ctrl & CLK_PIXEL_DISP_SEL_MASK) >> CLK_PIXEL_DISP_SEL_SHIFT;

	if (sel == 1) {
		sc_always("1x dvp out clk.");
		SC_SET_REG_BITS(__REG32__(DVP_GROUP_CTRL_ADDR), 0, 15, 15); // set dvp 1x pixel clock
	} else {
		sc_always("2x dvp out clk.");
		SC_SET_REG_BITS(__REG32__(DVP_GROUP_CTRL_ADDR), 1, 15, 15); // set dvp 2x pixel clock
	}

	sc_always("reset the dvp port");

	SC_SET_REG_BITS(__REG32__(VIDEO_IF_BASE_ADDRESS + (0x3c << 2)), 0, 17, 17);  /*dvp out reset */
	sc_delay(1);
	SC_SET_REG_BITS(__REG32__(VIDEO_IF_BASE_ADDRESS + (0x3c << 2)), 1, 17, 17);
	SC_SET_REG_BITS(__REG32__(VIDEO_IF_BASE_ADDRESS + (0xaf << 2)), 0, 0, 0);   /* make sure vif is power on*/
}

static void init_dvp_port(int out_color_mode, ENUM_SC_HAL_PIN_MODE pin_mode, int is_bt656, int clock_polarity)
{
	int mode = 0;
	unsigned int value = 0;

	sc_always("out_color_mode:%d pin_mode:%d is_bt656:%d clock_polarity:%d.", out_color_mode, pin_mode, is_bt656,
	    clock_polarity);

	dvp_out_base_init();

	//dvpo
	SC_SET_REG_BITS(value, 1, 8, 11);   // unknow
	SC_SET_REG_BITS(value, 1, 0, 0);   /* enable dvp */

	if(out_color_mode == COLOR_MODE_RGB565) {
		sc_always("COLOR_MODE_RGB565");
		SC_SET_REG_BITS(value, 1, 3, 3);   /* enable lcd mode */
		if ((pin_mode.rgb565 == SC_HAL_PIN_MODE_RGB565_DEFAULT) || (pin_mode.rgb565 == SC_HAL_PIN_MODE_RGB565_BGR)) {
			SC_SET_REG_BITS(value, 1, 12, 13);
			SC_SET_REG_BITS(value, 2, 14, 15);
		} else if (pin_mode.rgb565 == SC_HAL_PIN_MODE_RGB565_GRB) {
			SC_SET_REG_BITS(value, 2, 12, 13);
			SC_SET_REG_BITS(value, 0, 14, 15);
		} else
			sc_err("err rgb565 pin mode %d", pin_mode.rgb565);
	} else if (out_color_mode == COLOR_MODE_RGB888) {
		sc_always("COLOR_MODE_RGB888");
		SC_SET_REG_BITS(value, 1, 3, 3);   /* enable lcd mode */
		switch (pin_mode.rgb888) {
		case SC_HAL_PIN_MODE_RGB888_DEFAULT:
		case SC_HAL_PIN_MODE_RGB888_RGB:
			SC_SET_REG_BITS(value, 1, 12, 13);
			SC_SET_REG_BITS(value, 1, 14, 15);
			SC_SET_REG_BITS(value, 0, 16, 17);
			break;

		case SC_HAL_PIN_MODE_RGB888_GRB:
			SC_SET_REG_BITS(value, 0, 12, 13);
			SC_SET_REG_BITS(value, 0, 14, 15);
			SC_SET_REG_BITS(value, 0, 16, 17);
			break;

		case SC_HAL_PIN_MODE_RGB888_BRG:
			SC_SET_REG_BITS(value, 0, 12, 13);
			SC_SET_REG_BITS(value, 2, 14, 15);
			SC_SET_REG_BITS(value, 2, 16, 17);
			break;

		case SC_HAL_PIN_MODE_RGB888_RBG:
			SC_SET_REG_BITS(value, 2, 12, 13);
			SC_SET_REG_BITS(value, 1, 14, 15);
			SC_SET_REG_BITS(value, 2, 16, 17);
			break;

		case SC_HAL_PIN_MODE_RGB888_BGR:
			SC_SET_REG_BITS(value, 1, 12, 13);
			SC_SET_REG_BITS(value, 2, 14, 15);
			SC_SET_REG_BITS(value, 1, 16, 17);
			break;

		case SC_HAL_PIN_MODE_RGB888_GBR:
			SC_SET_REG_BITS(value, 2, 12, 13);
			SC_SET_REG_BITS(value, 0, 14, 15);
			SC_SET_REG_BITS(value, 1, 16, 17);
			break;

		default:
			sc_err("err rgb888 pin mode %d", pin_mode.rgb888);
			break;
		}
	} else if (out_color_mode == COLOR_MODE_YUV) {
		sc_always("COLOR_MODE_YUV");
		SC_SET_REG_BITS(value, 0, 3, 3);   /* disable lcd mode */
		if (is_bt656) {
			SC_SET_REG_BITS(__REG32__(DVP_GROUP_CTRL_ADDR), 1, 15, 15); // set dvp 2x pixel clock
			sc_always("cfg bt656 pin mode, must set 2x dvp output clock!");
			SC_SET_REG_BITS(value, 1, 1, 1);
			SC_SET_REG_BITS(value, 0, 12, 17);   /* if use DVP_CTRL_BASE_ADDR + 0x7c, should keep this reg [17:12] = 0 */

			switch (pin_mode.bt656) {
			case SC_HAL_PIN_MODE_BT656_DEFAULT:
			case SC_HAL_PIN_MODE_BT656_YCB_YCR_L:
				SC_SET_REG_BITS(__REG32__(DVP_GROUP_CTRL_ADDR), 0, 22, 23);
				SC_SET_REG_BITS(value, 2, 18, 19);
				break;

			case SC_HAL_PIN_MODE_BT656_YCR_YCB_H:
				SC_SET_REG_BITS(__REG32__(DVP_GROUP_CTRL_ADDR), 1, 22, 23);
				SC_SET_REG_BITS(value, 2, 18, 19);
				break;

			case SC_HAL_PIN_MODE_BT656_YCB_YCR_H:
				SC_SET_REG_BITS(__REG32__(DVP_GROUP_CTRL_ADDR), 1, 22, 23);
				SC_SET_REG_BITS(value, 0, 18, 19);
				break;

			case SC_HAL_PIN_MODE_BT656_CRY_CBY_H:
				SC_SET_REG_BITS(__REG32__(DVP_GROUP_CTRL_ADDR), 1, 22, 23);
				SC_SET_REG_BITS(value, 2, 18, 19);
				break;

			case SC_HAL_PIN_MODE_BT656_CBY_CRY_H:
				SC_SET_REG_BITS(__REG32__(DVP_GROUP_CTRL_ADDR), 1, 22, 23);
				SC_SET_REG_BITS(value, 1, 18, 19);
				break;

			case SC_HAL_PIN_MODE_BT656_YCR_YCB_L:
				SC_SET_REG_BITS(__REG32__(DVP_GROUP_CTRL_ADDR), 0, 22, 23);
				SC_SET_REG_BITS(value, 0, 18, 19);
				break;

			case SC_HAL_PIN_MODE_BT656_CRY_CBY_L:
				SC_SET_REG_BITS(__REG32__(DVP_GROUP_CTRL_ADDR), 0, 22, 23);
				SC_SET_REG_BITS(value, 3, 18, 19);
				break;

			case SC_HAL_PIN_MODE_BT656_CBY_CRY_L:
				SC_SET_REG_BITS(__REG32__(DVP_GROUP_CTRL_ADDR), 0, 22, 23);
				SC_SET_REG_BITS(value, 1, 18, 19);
				break;

			default:
				sc_err("err bt656 pin mode %d", pin_mode.bt1120);
				break;
			}
		} else {
			sc_always("cfg bt1120 pin mode");
			SC_SET_REG_BITS(value, 0, 1, 1);
			switch (pin_mode.bt1120) {
			case SC_HAL_PIN_MODE_BT1120_YCB_YCR:
				SC_SET_REG_BITS(value, 1, 18, 19);
				break;

			case SC_HAL_PIN_MODE_BT1120_CBY_CRY:
				SC_SET_REG_BITS(value, 0, 18, 19);
				break;

			case SC_HAL_PIN_MODE_BT1120_CRY_CBY:
				SC_SET_REG_BITS(value, 2, 18, 19);
				break;

			case SC_HAL_PIN_MODE_BT1120_DEFAULT:
			case SC_HAL_PIN_MODE_BT1120_YCR_YCB:
				SC_SET_REG_BITS(value, 3, 18, 19);
				break;

			default:
				sc_err("err bt1120 pin mode %d", pin_mode.bt1120);
				break;
			}
		}
	} else
		sc_err("err color mode %d", out_color_mode);

	//set embbed
	mode = get_current_dvp_dev_sync_mode();
	if(mode == SYNC_MODE_INTERNAL) {
		sc_always("set to internal sync");
		value |= (1 << 7);
	} else {
		sc_always("set to external sync");
		value &= ~(1 << 7);
	}

	sc_always("dvp cfg 0x%x", value);
	write_reg32(DVP_OUT_16_OFFSET_REG, value);  //yuv mode
	sc_always("dvp out ctrl *0x%x=0x%x", DVP_OUT_16_OFFSET_REG, read_reg32(DVP_OUT_16_OFFSET_REG));

	sc_always("dvp group ctrl: *0x%x=0x%x", DVP_GROUP_CTRL_ADDR, read_reg32(DVP_GROUP_CTRL_ADDR));
	write_reg32(VIDEO_IF_BASE_ADDRESS + (0x40 << 2), 0);
	sc_always("dvp clock polarity: 0x%x", clock_polarity);
	unsigned int new_clock_polarity = read_reg32(DVP_CTRL_BASE_ADDR + 0x84);
	if(clock_polarity) {
		SC_SET_REG_BITS(new_clock_polarity, 1, 0, 0);
	} else {
		SC_SET_REG_BITS(new_clock_polarity, 0, 0, 0);
	}
	write_reg32(DVP_CTRL_BASE_ADDR + 0x84, new_clock_polarity);
}

static void dvpo_init(display_interface_desc_t *dvp_desc)
{
	init_dvp_port(dvp_desc->out_color_mode, dvp_desc->pin_mode, dvp_desc->is_bt656, dvp_desc->clock_polarity);
}

static int init_dvp_interface(void)
{
	display_interface_desc_t desc;
	display_intgerface_ops_t *dev_ops = get_dev_ops();
	if(!dev_ops) {
		sc_err("err get dev_ops");
		return -1;
	}
	if(dev_ops->get_display_interface_desc) {
		dev_ops->get_display_interface_desc(&desc);
	}
	dvpo_init(&desc);
	if(dev_ops->init_display_interface) {
		dev_ops->init_display_interface();
	}
	return 0;
}
static int reset_dvp_interface(void)
{
	display_intgerface_ops_t *dev_ops = get_dev_ops();
	if(!dev_ops) {
		sc_err("err get dev_ops");
		return -1;
	}
	dev_ops->reset_display_interface();
	return 0;
}
static int get_dvp_display_interface_desc(display_interface_desc_t *desc)
{
	display_intgerface_ops_t *dev_ops = get_dev_ops();
	if(!dev_ops) {
		sc_err("err get dev_ops");
		return -1;
	}
	if(dev_ops->get_display_interface_desc) {
		dev_ops->get_display_interface_desc(desc);
	}
	return 0;
}
static int get_dvp_display_interface_res_infor(display_interface_res_infor_t *res_infor)
{
	display_intgerface_ops_t *dev_ops = get_dev_ops();
	if(!dev_ops) {
		sc_err("err get dev_ops");
		return -1;
	}
	if(dev_ops->get_display_interface_res_infor) {
		dev_ops->get_display_interface_res_infor(res_infor);
	}
	return 0;
}

static int set_dvp_display_interface_res_infor(disp_res_infor_t *res_infor)
{
	display_intgerface_ops_t *dev_ops = get_dev_ops();
	if(!dev_ops) {
		sc_err("err get dev_ops");
		return -1;
	}
	if(dev_ops->set_display_interface_res_infor) {
		dev_ops->set_display_interface_res_infor(res_infor);
	}
	return 0;
}

static int set_dvp_display_interface_format(int format)
{
	display_intgerface_ops_t *dev_ops = get_dev_ops();
	if(!dev_ops) {
		sc_err("err get dev_ops");
		return -1;
	}
	if(dev_ops->set_display_interface_format) {
		dev_ops->set_display_interface_format(format);
	}
	return 0;
}

static void csc_matrix_set_register(float *csc_data, uint32_t offset)
{
	for(int i = 0; i < 4; i++) {
		write_reg32(VIDEO_IF_BASE_ADDRESS + offset + i * 4,  \
		    (uint32_t)(csc_data[2 * i] > 0 ? csc_data[2 * i] * (1 << 10) :  \
		        (csc_data[2 * i] * (1 << 10) + (1 << 12))) |   \
		    ((uint32_t)(csc_data[2 * i + 1] > 0 ? csc_data[2 * i + 1] * (1 << 10) : \
		            (csc_data[2 * i + 1] * (1 << 10) + (1 << 12)))) << 12);
	}
	write_reg32(VIDEO_IF_BASE_ADDRESS + offset + 0x10, (uint32_t)(csc_data[8] * (1 << 10)));
}

static void dvp_out_enable_Y16_offset(uint32_t state)
{
	uint32_t reg_val;
	reg_val = read_reg32(DVP_OUT_16_OFFSET_REG);
	if(state > 0) {
		reg_val |= (1 << 8);
		write_reg32(DVP_OUT_16_OFFSET_REG, reg_val);
	} else {
		reg_val &= ~(1 << 8);
		write_reg32(DVP_OUT_16_OFFSET_REG, reg_val);
	}
}

static void dvp_out_bprgb2yuvclip_config(uint32_t offset, uint32_t clip, uint32_t enbit_pos)
{
	uint32_t val = 0;
	//set clip value
	write_reg32(offset + VIDEO_IF_BASE_ADDRESS, clip);
	//enable clip
	val = read_reg32(offset + VIDEO_IF_BASE_ADDRESS);
	val |= (1 << enbit_pos);
	write_reg32(offset + VIDEO_IF_BASE_ADDRESS, val);

}

static void dvp_out_bprgb2yuvcoeff_config(int mode)
{
	//note: vif-dvp_out rgbtoyuv CB channel and CR channael be reversed
	//so the matrix is diffrent with the nomal matrix CB<===>CR
	static float csc_bt709_full_range[9] = \
	{
		0.213, 0.715, 0.072, \
		0.511, -0.464, -0.047, \
		-0.117, -0.394, 0.511\
	};

	static float csc_bt601_full_range[9] = \
	{
		0.299, 0.587, 0.114, \
		0.500, -0.419, -0.081, \
		-0.169, -0.331, 0.500\
	};

	static float csc_bt709_limit_range[9] = \
	{
		0.183, 0.614, 0.062, \
		0.439, -0.399, -0.040, \
		-0.101, -0.338, 0.439\
	};

	static float csc_bt601_limit_range[9] = \
	{
		0.257, 0.504, 0.098, \
		0.439, -0.368, -0.071, \
		-0.148, -0.291, 0.439\
	};
	//full range disable Y + 16, clip[0, 255]
	switch(mode) {
	case SC_DVP_OUT_CSC_BT709_LIMIT: {
		dvp_out_enable_Y16_offset(1);
		csc_matrix_set_register(csc_bt709_limit_range, DVP_OUT_COEFF_CTL_OFFSET);
		//Y 8bits, u,v 8bits, u,v low 6bits
		dvp_out_bprgb2yuvclip_config(DVP_OUT_CLIP_CTL_OFFSET, 0x10f0eb, 23);
	}
	break;
	case SC_DVP_OUT_CSC_BT709_FULL: {
		dvp_out_enable_Y16_offset(0);
		csc_matrix_set_register(csc_bt709_full_range, DVP_OUT_COEFF_CTL_OFFSET);
		dvp_out_bprgb2yuvclip_config(DVP_OUT_CLIP_CTL_OFFSET, 0x00ffff, 23);
	}
	break;
	case SC_DVP_OUT_CSC_BT601_LIMIT: {
		dvp_out_enable_Y16_offset(1);
		csc_matrix_set_register(csc_bt601_limit_range, DVP_OUT_COEFF_CTL_OFFSET);
		dvp_out_bprgb2yuvclip_config(DVP_OUT_CLIP_CTL_OFFSET, 0x10f0eb, 23);
	}
	break;
	case SC_DVP_OUT_CSC_BT601_FULL: {
		dvp_out_enable_Y16_offset(0);
		csc_matrix_set_register(csc_bt601_full_range, DVP_OUT_COEFF_CTL_OFFSET);
		dvp_out_bprgb2yuvclip_config(DVP_OUT_CLIP_CTL_OFFSET, 0x00ffff, 23);
	}
	break;
	default:
		break;
	}

}

void dvp_rgb2yuvcoeff_custom(dvp_csc_pra_t *csc)
{
	//sc_always("y_offset[%x, %x], uv[%x, %x]", csc->Y_offset[0], csc->Y_offset[1], csc->UV_offset[0], csc->UV_offset[1]);
	//for (int i = 0; i < 9; i++)
	//sc_always("coffe[%d] = %d", i, float2int(csc->coeff[i]));
	//set Y low offset
	if (csc->Y_offset[0] < 16)
		dvp_out_enable_Y16_offset(0);
	else
		dvp_out_enable_Y16_offset(1);

	//set y, uv clip
	dvp_out_bprgb2yuvclip_config(DVP_OUT_CLIP_CTL_OFFSET, csc->Y_offset[1] | \
	    (csc->UV_offset[1] << 8) | (csc->UV_offset[0] << 24), 16);
	//set coeff
	csc_matrix_set_register(csc->coeff, DVP_OUT_COEFF_CTL_OFFSET);

}

static int set_dvp_display_interface_ctl(int clt_code, void *pra, int size)
{
	display_intgerface_ops_t *dev_ops = get_dev_ops();

	switch(clt_code) {
	case SET_DVP_OUT_CSC_PRA: {
		//set dvp out register to change CSC matrix
		dvp_out_bprgb2yuvcoeff_config(*(int *)pra);
	}
	break;
	default:
		break;
	}

	if(!dev_ops) {
		sc_err("err get dev_ops");
		return -1;
	}
	if(dev_ops->set_display_interface_ctl) {
		dev_ops->set_display_interface_ctl(clt_code, pra, size);
	}
	return 0;
}
static display_intgerface_ops_t dvp_ops = {
	.interface_index = DISPLAY_INTERFACE_DVP,
	.set_display_interface_index = set_dvp_interface_index,
	.init_display_interface = init_dvp_interface,
	.get_display_interface_desc = get_dvp_display_interface_desc,
	.get_display_interface_res_infor = get_dvp_display_interface_res_infor,
	.set_display_interface_res_infor = set_dvp_display_interface_res_infor,
	.on_display_interface = NULL,
	.off_display_interface = NULL,
	.reset_display_interface = reset_dvp_interface,
	.set_display_interface_format = set_dvp_display_interface_format,
	.set_display_interface_ctl = set_dvp_display_interface_ctl
};

display_intgerface_ops_t *get_dvp_ops(void)
{
	return &dvp_ops;
}
