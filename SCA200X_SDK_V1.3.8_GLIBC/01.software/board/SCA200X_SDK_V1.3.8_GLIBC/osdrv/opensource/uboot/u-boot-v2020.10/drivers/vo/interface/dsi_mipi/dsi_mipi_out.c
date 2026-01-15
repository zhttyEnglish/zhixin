#include "vo/interface/display_interface.h"
#include "vo/interface/dsi_mipi/dsi_reg.h"
#include "vo/interface/dsi_mipi/dphy_reg.h"

extern display_intgerface_ops_t mipi_dev_ek79007;
extern display_intgerface_ops_t mipi_dev_HIB050H1087;
extern display_intgerface_ops_t mipi_dev_E309144;
static int intf_index = MIPI_DEV_EK79007;

static display_intgerface_ops_t *mipi_dev_ops[] = {
	&mipi_dev_ek79007,
	// &mipi_dev_HIB050H1087,
	// &mipi_dev_E309144,
};

static int get_current_mipi_dev_index(void)
{
	return intf_index;
}

static display_intgerface_ops_t *get_dev_ops(void)
{
	int dev_index = get_current_mipi_dev_index();
	display_intgerface_ops_t *dev_ops = NULL;
	int i = 0;
	//  display_interface_desc_t desc;
	for(i = 0; i < sizeof(mipi_dev_ops) / sizeof(display_intgerface_ops_t *); i++) {
		dev_ops = mipi_dev_ops[i];
		if(dev_index == dev_ops->interface_index) {
			return dev_ops;
		}
	}

	sc_err("err intf index=%d", intf_index);
	return NULL;
}

int dsi_check_str(char *str)
{
	int str_len = strlen(str);
	for(int i = 0; i < str_len; i++) {
		if (    ( (str[i] >= '0') && (str[i] <= '9') ) \
		    || ( (str[i] >= 'a') && (str[i] <= 'f') ) \
		    || ( (str[i] >= 'A') && (str[i] <= 'F') )    ) {
			continue;
		} else {
			return -1;
		}
	}
	return 0;
}

uint8_t dsi_char2int(char str)
{
	if( (str >= '0') && (str <= '9') )
		return (str - '0');

	if( (str >= 'a') && (str <= 'f') )
		return (str - 'a' + 10);

	if( (str >= 'A') && (str <= 'F') )
		return (str - 'A' + 10);

	return 0;
}

/*
   str = "0x0" ~ "0x12345678"
   */
int dsi_str2hex(char *str, uint32_t *value)
{
	int str_len = 0;
	uint32_t val_tmp = 0;

	str_len = strlen(str);
	if( (str_len < 3) || (str_len > 10) )
		return -1;

	if( (str[0] != '0') || (str[1] != 'x') )
		return -2;

	if(0 != dsi_check_str(str + 2))
		return -3;

	for(int i = 2; i < str_len; i++) {
		val_tmp = (val_tmp << 4);
		val_tmp += dsi_char2int(str[i]);
	}

	*value = val_tmp;
	return 0;
}

static void mipi_init(display_interface_desc_t *mipi_desc)
{
}

static int set_mipi_interface_index(int index)
{
	sc_always("index=%d", index);
	intf_index = index;
	return 0;
}

static int init_mipi_interface(void)
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
	if(dev_ops->init_display_interface) {
		dev_ops->init_display_interface();
	}

	mipi_init(&desc);
	return 0;
}

static int reset_mipi_interface(void)
{
	display_intgerface_ops_t *dev_ops = get_dev_ops();
	if(!dev_ops) {
		sc_err("err get dev_ops");
		return -1;
	}

	if (dev_ops->reset_display_interface) {
		dev_ops->reset_display_interface();
	}
	return 0;
}
static int get_mipi_display_interface_desc(display_interface_desc_t *desc)
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
static int get_mipi_display_interface_res_infor(display_interface_res_infor_t *res_infor)
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

static int set_mipi_display_interface_res_infor(disp_res_infor_t *res_infor)
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

static int set_mipi_display_interface_format(int format)
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

static int set_mipi_display_interface_ctl(int clt_code, void *pra, int size)
{
	display_intgerface_ops_t *dev_ops = get_dev_ops();

	switch(clt_code) {
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
static display_intgerface_ops_t mipi_ops = {
	.interface_index = DISPLAY_INTERFACE_MIPI,
	.set_display_interface_index = set_mipi_interface_index,
	.init_display_interface = init_mipi_interface,
	.get_display_interface_desc = get_mipi_display_interface_desc,
	.get_display_interface_res_infor = get_mipi_display_interface_res_infor,
	.set_display_interface_res_infor = set_mipi_display_interface_res_infor,
	.on_display_interface = NULL,
	.off_display_interface = NULL,
	.reset_display_interface = reset_mipi_interface,
	.set_display_interface_format = set_mipi_display_interface_format,
	.set_display_interface_ctl = set_mipi_display_interface_ctl
};

display_intgerface_ops_t *get_mipi_ops(void)
{
	return &mipi_ops;
}
