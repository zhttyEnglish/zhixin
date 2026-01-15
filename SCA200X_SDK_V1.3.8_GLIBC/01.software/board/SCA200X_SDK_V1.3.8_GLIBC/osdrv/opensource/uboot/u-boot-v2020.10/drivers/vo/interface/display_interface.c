#include"vo/interface/display_interface.h"
display_intgerface_ops_t *get_mipi_ops(void);
display_intgerface_ops_t *get_dvp_ops(void);

static int intf_index = DISPLAY_INTERFACE_MIPI;

int get_current_display_interface()
{
	return intf_index;
}

int init_display_interface(int interface_type)
{
	sc_always("interfae_type=%d", interface_type);
	display_intgerface_ops_t *interface_ops = NULL;
	switch(interface_type) {
	case DISPLAY_INTERFACE_DVP:
		interface_ops = get_dvp_ops();
		if(interface_ops->init_display_interface) {
			interface_ops->init_display_interface();
		}
		break;
	case DISPLAY_INTERFACE_MIPI:
		interface_ops = get_mipi_ops();
		if(interface_ops->init_display_interface) {
			interface_ops->init_display_interface();
		}
		break;
	default:
		sc_err("not supported display interfaceb");
		break;
	}
	return 0;
}

int set_display_interface(int interface_type, int subintf)
{
	display_intgerface_ops_t *interface_ops = NULL;

	sc_always("interfae_type=%d subintf=%d", interface_type, subintf);
	if ((interface_type < 0) && (interface_type < DISPLAY_INTERFACE_MAX))
		return -1;

	intf_index = interface_type;

	switch(interface_type) {
	case DISPLAY_INTERFACE_DVP:
		interface_ops = get_dvp_ops();
		if(interface_ops->set_display_interface_index) {
			interface_ops->set_display_interface_index(subintf);
		}
		break;
	case DISPLAY_INTERFACE_MIPI:
		interface_ops = get_mipi_ops();
		if(interface_ops->set_display_interface_index) {
			interface_ops->set_display_interface_index(subintf);
		}
		break;
	default:
		sc_err("not supported display interface");
		break;
	}
	return 0;
}

int power_on_display_interface(int interface_type)
{
	sc_always("interfae_type=%d", interface_type);
	display_intgerface_ops_t *interface_ops = NULL;
	switch(interface_type) {
	case DISPLAY_INTERFACE_DVP:
		interface_ops = get_dvp_ops();
		if(interface_ops->on_display_interface) {
			interface_ops->on_display_interface();
		}
		break;
	case DISPLAY_INTERFACE_MIPI:
		interface_ops = get_mipi_ops();
		if(interface_ops->on_display_interface) {
			interface_ops->on_display_interface();
		}
		break;
	default:
		sc_err("not supported display interfaceb");
		break;
	}
	return 0;
}

int power_off_display_interface(int interface_type)
{
	sc_always("interfae_type=%d", interface_type);
	display_intgerface_ops_t *interface_ops = NULL;
	switch(interface_type) {
	case DISPLAY_INTERFACE_DVP:
		interface_ops = get_dvp_ops();
		if(interface_ops->off_display_interface) {
			interface_ops->off_display_interface();
		}
		break;
	case DISPLAY_INTERFACE_MIPI:
		break;
		interface_ops = get_mipi_ops();
		if(interface_ops->off_display_interface) {
			interface_ops->off_display_interface();
		}
		break;
	default:
		sc_err("not supported display interfaceb");
		break;
	}
	return 0;
}

int reset_display_interface(int interface_type)
{
	sc_always("interfae_type=%d", interface_type);
	display_intgerface_ops_t *interface_ops = NULL;
	switch(interface_type) {
	case DISPLAY_INTERFACE_DVP:
		interface_ops = get_dvp_ops();
		if(interface_ops->reset_display_interface) {
			interface_ops->reset_display_interface();
		}
		break;
	case DISPLAY_INTERFACE_MIPI:
		interface_ops = get_mipi_ops();
		if(interface_ops->reset_display_interface) {
			interface_ops->reset_display_interface();
		}
		break;
	default:
		sc_err("not supported display interfaceb");
		break;
	}
	return 0;
}
int get_display_interface_desc(int interfae_type, display_interface_desc_t *desc)
{
	sc_always("interfae_type=%d", interfae_type);
	display_intgerface_ops_t *interface_ops = NULL;
	switch(interfae_type) {
	case DISPLAY_INTERFACE_DVP:
		interface_ops = get_dvp_ops();
		if(interface_ops->get_display_interface_desc) {
			interface_ops->get_display_interface_desc(desc);
		}
		break;
	case DISPLAY_INTERFACE_MIPI:
		interface_ops = get_mipi_ops();
		if(interface_ops->get_display_interface_desc) {
			interface_ops->get_display_interface_desc(desc);
		}
		break;
	default:
		sc_err("not supported display interfaceb");
		break;
	}
	return 0;
}
int get_display_interface_res_infor(int interfae_type, display_interface_res_infor_t *res_infor)
{
	sc_always("interfae_type=%d", interfae_type);
	display_intgerface_ops_t *interface_ops = NULL;
	switch(interfae_type) {
	case DISPLAY_INTERFACE_DVP:
		interface_ops = get_dvp_ops();
		if(interface_ops->get_display_interface_res_infor) {
			interface_ops->get_display_interface_res_infor(res_infor);
		}
		break;
	case DISPLAY_INTERFACE_MIPI:
		interface_ops = get_mipi_ops();
		if(interface_ops->get_display_interface_res_infor) {
			interface_ops->get_display_interface_res_infor(res_infor);
		}
		break;
	default:
		sc_err("not supported display interfaceb");
		break;
	}
	return 0;
}
int set_display_interface_res_infor(int interfae_type, disp_res_infor_t *res_infor)
{
	sc_always("interfae_type=%d", interfae_type);
	display_intgerface_ops_t *interface_ops = NULL;
	switch(interfae_type) {
	case DISPLAY_INTERFACE_DVP:
		interface_ops = get_dvp_ops();
		if(interface_ops->set_display_interface_res_infor) {
			interface_ops->set_display_interface_res_infor(res_infor);
		}
		break;
	case DISPLAY_INTERFACE_MIPI:
		interface_ops = get_mipi_ops();
		if(interface_ops->set_display_interface_res_infor) {
			interface_ops->set_display_interface_res_infor(res_infor);
		}
		break;
	default:
		sc_err("not supported display interfaceb");
		break;
	}
	return 0;
}

int set_display_interface_format(int interfae_type, int format)
{
	sc_always("interfae_type=%d", interfae_type);
	display_intgerface_ops_t *interface_ops = NULL;
	switch(interfae_type) {
	case DISPLAY_INTERFACE_DVP:
		interface_ops = get_dvp_ops();
		if(interface_ops->set_display_interface_format) {
			interface_ops->set_display_interface_format(format);
		}
		break;
	case DISPLAY_INTERFACE_MIPI:
		interface_ops = get_mipi_ops();
		if(interface_ops->set_display_interface_format) {
			interface_ops->set_display_interface_format(format);
		}
		break;
	default:
		sc_err("not supported display interfaceb");
		break;
	}
	return 0;
}

int set_display_interface_ctl(int interfae_type, int clt_code, void *pra, int size)
{
	sc_always("interfae_type=%d", interfae_type);
	display_intgerface_ops_t *interface_ops = NULL;
	switch(interfae_type) {
	case DISPLAY_INTERFACE_DVP:
		interface_ops = get_dvp_ops();
		if(interface_ops->set_display_interface_ctl) {
			interface_ops->set_display_interface_ctl(clt_code, pra, size);
		}
		break;
	case DISPLAY_INTERFACE_MIPI:
		interface_ops = get_mipi_ops();
		if(interface_ops->set_display_interface_ctl) {
			interface_ops->set_display_interface_ctl(clt_code, pra, size);
		}
		break;
	default:
		sc_err("not supported display interfaceb");
		break;
	}
	return 0;
}

