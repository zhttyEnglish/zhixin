#include <common.h>
#include <display.h>
#include <dm.h>
#include <edid.h>
#include <log.h>
#include <time.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include "vo/sc_display.h"
#include "vo/display_core/display_type.h"
#include "vo/display_core/display_reg.h"
#include "vo/util.h"
//#include "uclass.h"

//#define DE_EMBEDED_INTERLACE
#define USE_RGB_DVP_OUT
//#define USE_BMP_LOGO
#define DIS_ALIGNE_TO(size,num) ( (num)*( ( (size)+(num)-1)/(num) ) )
struct sc_display_debug_info {
	uint32_t frame_count;
	uint32_t queue_depth;
	display_buffer_t current_buf;
	display_buffer_t next_buf;
};
static void power_on_display(disp_obj_t *obj);
static void display_first_overlay_frame(disp_obj_t *obj, display_buffer_t *buffer);
static void display_oneframe_ok(void *usr_data);
static display_server_t *get_display_server(void);
static int register_handle_to_server(display_handle_t handle);
static display_handle_t find_handle_by_layer_id(int layer_id);

static display_handle_t sc_creat_display_layer(void)
{
	static int layer_num = 0;
	layer_num++;
	if(layer_num <= 2) {
		disp_obj_t *disp_obj = sc_malloc(sizeof(disp_obj_t));
		memset(disp_obj, 0, sizeof(disp_obj_t));
		return (display_handle_t)disp_obj;
	} else {
		sc_err("can not creat layer.");
		return 0;
	}
}
display_handle_t sc_get_display_layer(int layer_id)
{
	sc_lock(get_display_server()->lock);
	display_handle_t handle = find_handle_by_layer_id(layer_id);
	if(!handle) {
		handle = sc_creat_display_layer();
		register_handle_to_server(handle);
		sc_init_display(handle, layer_id);
	}
	sc_unlock(get_display_server()->lock);
	return handle;
}

static int flush_disp_buffer(disp_obj_t *obj)
{
	sc_queue_t *disp_done_queue = (sc_queue_t *)obj->disp_done_queue;
	sc_queue_t *disp_queue = (sc_queue_t *)obj->disp_queue;
	void *buffer = NULL;

	while(disp_queue->get_queue_size(disp_queue) > 0) {
		disp_queue->queue_pop(disp_queue, &buffer);
		disp_done_queue->queue_insert(disp_done_queue, buffer);
	}
	return 0;
}

static int flush_hold_buffer(disp_obj_t *obj)
{
	sc_queue_t *disp_done_queue = (sc_queue_t *)obj->disp_done_queue;
	sc_queue_t *hold_odd = (sc_queue_t *)obj->hold_queue_odd;
	sc_queue_t *hold_even = (sc_queue_t *)obj->hold_queue_even;
	void *buffer = NULL;

	while(hold_odd->get_queue_size(hold_odd) > 0) {
		hold_odd->queue_pop(hold_odd, &buffer);
		disp_done_queue->queue_insert(disp_done_queue, buffer);
	}
	while(hold_even->get_queue_size(hold_even) > 0) {
		hold_even->queue_pop(hold_even, &buffer);
		disp_done_queue->queue_insert(disp_done_queue, buffer);
	}
	return 0;
}

static void disp_done_queue_insert_wrapper(disp_obj_t *obj, sc_queue_t *done_queue, display_buffer_t *buffer)
{
	uint32_t delay_time = (uint32_t)((buffer->timestamps_show - buffer->timestamps_enqueue) / 1000);
	if(delay_time < 100) {
		obj->deta_every_1ms_history[delay_time]++;
		obj->deta_every_1ms_history_log[delay_time]++;
		obj->next_buf_cfg_lock_deta = (buffer->timestamps_show - buffer->timestamps_enqueue) * 1000000;

	}
#if 0
	sc_printf("id %u, done %u, show %u, cfg %u delay time %d\n",
	    buffer->frame_id,
	    (uint32_t)(buffer->timestamps_done - buffer->timestamps_enqueue),
	    (buffer->timestamps_show) ? (uint32_t)(buffer->timestamps_show - buffer->timestamps_enqueue) : 0,
	    (buffer->timestamps_cfg) ? (uint32_t)(buffer->timestamps_cfg - buffer->timestamps_enqueue) : 0, delay_time );
#endif
	done_queue->queue_insert(done_queue, buffer);
	return;
}

int sc_display_res_get(display_hal_t *hal, display_interface_res_infor_t *res_infor,
    disp_res_infor_t *timing_info, SC_BOOL *interlace)
{
	int ret = SC_HAL_VO_SUCCESS;
	int   index = 0;

	if (hal->vo_dev.attr.e_timing_template > SC_HAL_VO_OUTPUT_USER) {
		ret = HAL_ERR_VO_DEV_TIMING_TEMPLATE;
	}
	if (hal->vo_dev.attr.e_timing_template == SC_HAL_VO_OUTPUT_USER) {
		timing_info->h_valid_pixels = hal->vo_dev.attr.timing_customize.hdp;

		timing_info->h_total_pixels =
		    hal->vo_dev.attr.timing_customize.hpw +
		    hal->vo_dev.attr.timing_customize.hbp +
		    hal->vo_dev.attr.timing_customize.hdp +
		    hal->vo_dev.attr.timing_customize.hfp;

		timing_info->start_hsync   =
		    hal->vo_dev.attr.timing_customize.hdp +
		    hal->vo_dev.attr.timing_customize.hfp;

		timing_info->end_hsync     =
		    hal->vo_dev.attr.timing_customize.hdp +
		    hal->vo_dev.attr.timing_customize.hfp +
		    hal->vo_dev.attr.timing_customize.hpw;

		timing_info->v_valid_lines  =
		    hal->vo_dev.attr.timing_customize.vdp;

		timing_info->v_total_lines  =
		    hal->vo_dev.attr.timing_customize.vpw +
		    hal->vo_dev.attr.timing_customize.vbp +
		    hal->vo_dev.attr.timing_customize.vdp +
		    hal->vo_dev.attr.timing_customize.vfp;

		timing_info->start_vsync    =
		    hal->vo_dev.attr.timing_customize.vdp +
		    hal->vo_dev.attr.timing_customize.vfp;

		timing_info->end_vsync      =
		    hal->vo_dev.attr.timing_customize.vdp +
		    hal->vo_dev.attr.timing_customize.vfp +
		    hal->vo_dev.attr.timing_customize.vpw;

		timing_info->fps            = hal->vo_dev.attr.timing_customize.fps;
		*interlace                  = hal->vo_dev.attr.timing_customize.interlace_mod;
	} else {
		const STRU_DISP_RES   *res = display_hal_get_res_tbl(hal->vo_dev.attr.e_timing_template);
		int size = res_infor->res_count;

		if (res == NULL) {
			sc_err("error timing template %d", hal->vo_dev.attr.e_timing_template);
			ret = HAL_ERR_VO_DEV_TIMING_TEMPLATE;
			goto End;
		}

		for(index = 0; index < size; index++) {
			if((res_infor->res[index].h_valid_pixels == res->width) && \
			    (res_infor->res[index].v_valid_lines == res->height) && \
			    (res->fps <= (res_infor->res[index].fps + 0.2)) && (res->fps > (res_infor->res[index].fps - 0.2))) {
				break;
			}
		}
		if(index >= size) {
			sc_err("can not find a timing for this res, (%d %d %d)", res->width, res->height, (int)(res->fps * 10000));
			ret = HAL_ERR_VO_DEV_TIMING_TEMPLATE;
			goto End;
		}

		timing_info->h_valid_pixels = res_infor->res[index].h_valid_pixels;
		timing_info->h_total_pixels = res_infor->res[index].h_total_pixels;
		timing_info->start_hsync    = res_infor->res[index].start_hsync;
		timing_info->end_hsync      = res_infor->res[index].end_hsync;

		timing_info->v_valid_lines  = res_infor->res[index].v_valid_lines;
		timing_info->v_total_lines  = res_infor->res[index].v_total_lines;
		timing_info->start_vsync    = res_infor->res[index].start_vsync;
		timing_info->end_vsync      = res_infor->res[index].end_vsync;

		timing_info->fps            = res_infor->res[index].fps;
		*interlace = res->interlace;
	}

End:
	return ret;
}

int sc_display_dev_enable(void)
{
	display_handle_t *handle = sc_get_display_layer(SC_HAL_VO_LAYER_ID_VIDEO_0);
	disp_obj_t *obj = (disp_obj_t *)handle;
	display_hal_t *hal = &get_display_server()->hal;
	int out_bitdepth = DPI_D24;
	int ret = SC_HAL_VO_SUCCESS;
	int bit_count_per_channel = 0;

	dc_display_reset(obj);    // clear all register

	ret = set_display_interface((int)hal->vo_dev.attr.e_interface, (int)hal->vo_dev.attr.u_sub_intance.e_mipi_pannel);
	if (ret != SC_HAL_VO_SUCCESS)
		goto End;

	obj->current_display_interface = get_current_display_interface();
	init_display_interface(obj->current_display_interface);

	get_display_interface_desc(obj->current_display_interface, &obj->desc);
	get_display_interface_res_infor(obj->current_display_interface, &obj->res_infor);

	ret = sc_display_res_get(hal, &obj->res_infor, &obj->res_timing, (SC_BOOL *)&obj->interlace_mod);
	if (ret != SC_HAL_VO_SUCCESS)
		goto End;

	obj->res_index = dc_set_panel2(
	        &obj->dc_inst,
	        obj->desc.de_en,
	        obj->desc.da_en,
	        obj->desc.clock_en,
	        obj->desc.de_polarity,
	        obj->desc.da_polarity,
	        obj->desc.clock_polarity,
	        obj->desc.hsync_polarity,
	        obj->desc.vsync_polarity,
	        &obj->res_timing
	    );

	obj->fps = obj->res_timing.fps;

	if(obj->current_display_interface == DISPLAY_INTERFACE_DVP) {
		obj->dc_inst.dvp_out_en = 1;
	} else if (obj->current_display_interface == DISPLAY_INTERFACE_MIPI) {
		obj->dc_inst.dsi_out_en = 1;
	}

	if(obj->desc.out_color_mode == COLOR_MODE_RGB565 || obj->desc.out_color_mode == COLOR_MODE_RGB888) {
		obj->dc_inst.out_format = 0;
		obj->dc_inst.rgb2yuv_en = 0;
		sc_always("rgb mode");
	} else {
		obj->dc_inst.out_format = 1;
		obj->dc_inst.rgb2yuv_en = 1;
		sc_always("yuv mode");
	}

	bit_count_per_channel = obj->res_infor.bit_count_per_channel;
	if(bit_count_per_channel == 0) {
		bit_count_per_channel = hal->vo_dev.attr.bit_count_per_channel;
		sc_always("use hal bit_count_per_channel=%d", bit_count_per_channel);
	}

	if(bit_count_per_channel == 8)
		out_bitdepth = DPI_D24;
	else if(bit_count_per_channel == 10)
		out_bitdepth = DPI_D30;
	else if(bit_count_per_channel == 5)
		out_bitdepth = DPI_D16CFG1;
	else {
		sc_err("err bit_count_per_channel=%d", obj->res_infor.bit_count_per_channel);
		out_bitdepth = DPI_D24;
	}

	sc_always("out_bitdepth=%d", out_bitdepth);

	// output parameters
	dc_set_output(
	    &obj->dc_inst,
	    SET_ENABLE,
	    out_bitdepth);

#if 0
	//set output csc
	dc_set_output_csc(hal->vo_dev.e_csc);
#endif

	// set interface csc
#if 0
	if (obj->desc.out_color_mode == COLOR_MODE_YUV) {
		sc_always("use yuv csc");
		sc_display_set_csc(handle, SC_VIDEO_FORMAT_RGB, sc_display_hal_csc_trans(hal->vo_dev.e_csc), 0);
	} else {
		sc_always("use rgb csc");
		sc_display_set_csc(handle, SC_VIDEO_FORMAT_RGB, SC_VIDEO_MATRIX_BT709_LIMIT, 0);
	}
#else
	sc_display_set_csc(handle, SC_VIDEO_FORMAT_RGB, sc_display_hal_csc_trans(hal->vo_dev.e_csc), 0);
#endif

	//set interlace
	if (obj->interlace_mod != SC_SYSTEM_INTERLACE_MOD_NULL) {
		*(unsigned int *)0x645C1F68 = (unsigned int )0x02003002;    //
		*(unsigned int *)0x645C1F40 = (unsigned int )0x10215;       // b[2] interlace
		*(unsigned int *)0x645C1F20 = (unsigned int )0x1;
		*(unsigned int *)0x645B00A4 = (unsigned int )0xd9183;       // BT656
		//*(unsigned int *)0x645B00A4 = (unsigned int )0x99181;   // BT1120
		sc_always("display bufconfig =%08x\n", *(unsigned int *)0x645c1518 );
	}

	power_on_display(obj);
	sc_always("vo dev init finished.");

End:
	return ret;
}

int sc_display_dev_disable(void)
{
	disp_obj_t *obj = (disp_obj_t *)sc_get_display_layer(SC_HAL_VO_LAYER_ID_VIDEO_0);
	int ret = SC_HAL_VO_SUCCESS;

	dc_display_reset(obj);    // clear all register
	obj->current_display_interface = get_current_display_interface();
	reset_display_interface(obj->current_display_interface);
	sc_always("vo dev deinit finished.");

	return ret;
}

int sc_init_display(display_handle_t handle, int overlay_id)
{
	//malloc display buffer
	disp_obj_t *obj = (disp_obj_t *)handle;
	//char value[64]={0};
	int64_t base = 1000000000;
	//   int i=0;
	if(!obj->init) {
		obj->init = 1;
	} else {
		return 0;
	}
	obj->overlay_id = overlay_id;
	obj->dc_inst.overlay_id = obj->overlay_id;
	obj->dc_inst.priv = (void *)obj;
	obj->is_first_frame = 1;
	obj->enable_irq_process = 0;
	obj->stats_frame_count = 0;
	obj->exception_high_count = 0;
	obj->exception_low_count = 0;
	obj->average_low_time = 0;
	obj->average_high_time = 0;
	obj->normal_count = 0;
	obj->src_push_cnt = 0;
	obj->input_fps = 0.0;
	obj->deta_err_count_too_large = 0;
	obj->deta_err_count_low_zero = 0;
	obj->stats_frame_count_th = 1000;
	obj->stats_count = 45;
	obj->deta_th_err = obj->stats_count * base;
	obj->next_buf_cfg_lock_deta = 0;
	obj->average_deta = 0;
	obj->fre_adjust_th_high = 148.85;
	obj->fre_adjust_th_low = 148.28;
	obj->adjust_status = 0;
	obj->return_adjust_flag = 0;
	obj->low_fine_adjusting = 0;
	obj->low_adjusting = 0;
	obj->high_fine_adjusting = 0;
	obj->high_adjusting = 0;
	obj->idleing = 0;
	obj->frame_count = 0;
	memset(obj->deta_every_1ms_history_log, 0, sizeof(obj->deta_every_1ms_history_log));
	obj->low_adjust_count = 0;
	obj->hight_adjust_count = 0;
	obj->return_adjust_count = 0;
	obj->interlace_mod = SC_SYSTEM_INTERLACE_MOD_NULL;
	memset(obj->deta_every_1ms_history, 0, sizeof(obj->deta_every_1ms_history));
	obj->frame_buf4k2k.priv = NULL;
	obj->dc_inst.zoom_addr_offset[0] = 0;
	obj->dc_inst.zoom_addr_offset[1] = 0;
	obj->dc_inst.zoom_addr_offset[2] = 0;
	//creat a task to process the fre adjust police,the adjust must be completed in 16.7 ms after interrupt
	//so the taks pro is highest in the system
	obj->display_post_proc = (void *)sc_create_signal();
	obj->frame_buffer_vsync = (void *)sc_create_signal();
	//init the queue
	obj->disp_queue = sc_creat_queue(16, "display_queue");
	obj->disp_done_queue = sc_creat_queue(16, "display_done_queue");
	obj->hold_queue_odd = sc_creat_queue(16, "hold_queue_odd");
	obj->hold_queue_even = sc_creat_queue(16, "hold_queue_even");
	obj->queue_lock = sc_creat_lock();

	//all the task and singal have created ,so init the dc
	dc_init(obj, display_oneframe_ok);
	return 0;
}

int sc_display_set_overlay_id(display_handle_t handle, int id)
{
	disp_obj_t *obj = (disp_obj_t *)handle;
	obj->overlay_id = id;
	obj->dc_inst.overlay_id = obj->overlay_id;
	return 0;
}

int sc_disp_get_buffer_width(uint32_t width)
{
	uint32_t bit_num_per_burst = 128;
	uint32_t bitdepth = 8;
	uint32_t burst_num = DISPLAY_HW_BURST_NUM * Y_UV_DIV_FACTOR;
	uint32_t tmp_width = DIS_ALIGNE_TO(width, (bit_num_per_burst / bitdepth));
	uint32_t ret_width = DIS_ALIGNE_TO(tmp_width / (bit_num_per_burst / bitdepth), burst_num) * bit_num_per_burst / 8;
	sc_always("width=%d ret_width=%d bit_num_per_burst=%d bitdepth=%d burst_num=%d ", \
	    width, ret_width, bit_num_per_burst, bitdepth, burst_num);
	return ret_width;
}

int sc_disp_get_buffer_width_div2(uint32_t width)
{
	uint32_t bit_num_per_burst = 128;
	uint32_t bitdepth = 8;
	uint32_t burst_num = DISPLAY_HW_BURST_NUM;
	uint32_t tmp_width = DIS_ALIGNE_TO(width, (bit_num_per_burst / bitdepth));
	uint32_t ret_width = DIS_ALIGNE_TO(tmp_width / (bit_num_per_burst / bitdepth), burst_num) * bit_num_per_burst / 8;
	sc_always("width=%d ret_width=%d bit_num_per_burst=%d bitdepth=%d burst_num=%d ", \
	    width, ret_width, bit_num_per_burst, bitdepth, burst_num);
	return ret_width;
}

static void flush_display_top_queue(disp_obj_t *obj)
{
	//flush all the display queue and done queue
	display_buffer_t *buffer = NULL;
	sc_queue_t *disp_done_queue = (sc_queue_t *)obj->disp_done_queue;
	sc_queue_t *disp_queue = (sc_queue_t *)obj->disp_queue;
	do {
		disp_done_queue->queue_pop(disp_done_queue, (void **)&buffer);
	} while(buffer);
	do {
		disp_queue->queue_pop(disp_queue, (void **)&buffer);
	} while(buffer);
	return;
}

int sc_cfg_display_out(display_handle_t handle, int width, int height)
{
	disp_obj_t *obj = (disp_obj_t *)handle;
	obj->out_width = width;
	obj->out_height = height;
	return 0;
}

int sc_cfg_display_interlace_mode(display_handle_t handle, int interlace_mode)
{
	disp_obj_t *obj = (disp_obj_t *)handle;
	obj->interlace_mod = interlace_mode;
	sc_always("interlace_mod=%d", obj->interlace_mod);

	if(obj->cpld_interlace_pin_num == 0xffffffff || obj->cpld_sd_hd_pin_num == 0xffffffff) {
		sc_err("err pin num");
		return 0;
	}
#if 0
	if(obj->overlay_id == 0) {
		gpio_set_output(obj->cpld_interlace_pin_num);
		gpio_set_output(obj->cpld_sd_hd_pin_num);
		if(obj->interlace_mod == SC_SYSTEM_INTERLACE_MOD_NULL) { //progressive
			gpio_setpin(obj->cpld_interlace_pin_num, 0);
		} else {
			gpio_setpin(obj->cpld_interlace_pin_num, 1);
			switch(obj->interlace_mod) {
			case SC_SYSTEM_INTERLACE_MOD_1080I:
				gpio_setpin(obj->cpld_sd_hd_pin_num, 1);
				break;
			case SC_SYSTEM_INTERLACE_MOD_720_480I_NTSC:
			case SC_SYSTEM_INTERLACE_MOD_720_576I_PAL:
				gpio_setpin(obj->cpld_sd_hd_pin_num, 0);
				break;
			}
		}
	} else {
		sc_always("over layer layer, not to cfg fpga");
	}
#endif
	return 0;
}

int sc_cfg_flush_cb(display_handle_t handle, pfn_display_cb cb, void *usrdata)
{
	disp_obj_t *obj = (disp_obj_t *)handle;
	obj->flush_display_queue = cb;
	obj->usr_data = usrdata;
	return 0;
}

void sc_display_zoom(display_handle_t handle, float zoom)
{
	disp_obj_t *obj = (disp_obj_t *)handle;
	int w = (float)obj->dc_inst.framebuffer.width / zoom;
	int h = (float)obj->dc_inst.framebuffer.height / zoom;
	dc_set_zoom(&obj->dc_inst, zoom);
	obj->dc_inst.zoom_addr_offset[0] = (obj->dc_inst.framebuffer.height - h) / 2 * obj->dc_inst.framebuffer.fb_stride[0] +
	    (obj->dc_inst.framebuffer.width - w) / 2;
	obj->dc_inst.zoom_addr_offset[2] = obj->dc_inst.zoom_addr_offset[1] = (obj->dc_inst.framebuffer.height - h) / 4 *
	        obj->dc_inst.framebuffer.fb_stride[1] + (obj->dc_inst.framebuffer.width - w) / 4;
	sc_always("zoom=%d w=%d h=%d offset(%d %d %d)", float2int(zoom), w, h, \
	    obj->dc_inst.zoom_addr_offset[0], obj->dc_inst.zoom_addr_offset[1], obj->dc_inst.zoom_addr_offset[2]);
}

int sc_display_interface_cfg_csc(int csc_pra)
{
	display_handle_t handle = find_handle_by_layer_id(0);
	disp_obj_t *obj = (disp_obj_t *)handle;
	obj->csc_pra = csc_pra;
	int ret = set_display_interface_ctl(obj->current_display_interface, SET_DVP_OUT_CSC_PRA, &obj->csc_pra,
	        sizeof(obj->csc_pra));
	return ret;
}

int sc_pause_display(display_handle_t handle)
{
	disp_obj_t *obj = (disp_obj_t *)handle;
	//display_buffer_t *buffer=NULL;
	//int x=0;
	sc_queue_t *disp_done_queue = (sc_queue_t *)obj->disp_done_queue;
	if(!get_display_server()->ui_mode) {
		dc_display_pause(obj);
	} else {
		if(!obj->use_background) {
			dc_display_pause(obj);
		}
	}
	sc_always("stop the display hw .....");

	sc_lock(obj->queue_lock);
	obj->start_flag = 0;
	flush_disp_buffer(obj);

	if(obj->interlace_mod == SC_SYSTEM_INTERLACE_MOD_NULL) {
		if(obj->next_buf) {
			disp_done_queue->queue_insert(disp_done_queue, obj->next_buf);
			obj->next_buf = NULL;
		}
		if(obj->current_buf) {
			disp_done_queue->queue_insert(disp_done_queue, obj->current_buf);
			obj->current_buf = NULL;
		}
	} else {
		flush_hold_buffer(obj);
	}

	sc_unlock(obj->queue_lock);
	obj->out_height = 0;
	obj->out_width = 0;
	obj->is_first_frame = 1;
	obj->enable_irq_process = 0;
	obj->dis_irq_count = 0;
	obj->dis_frame_num = 0;
	obj->dis_loss_count = 0;
	obj->repeat_count = 0;
	obj->stats_frame_count = 0;
	obj->exception_high_count = 0;
	obj->exception_low_count = 0;
	obj->average_low_time = 0;
	obj->average_high_time = 0;
	obj->normal_count = 0;
	obj->deta_err_count_too_large = 0;
	obj->deta_err_count_low_zero = 0;
	obj->next_buf_cfg_lock_deta = 0;
	obj->average_deta = 0;
	obj->low_adjust_count = 0;
	obj->hight_adjust_count = 0;
	obj->return_adjust_count = 0;
	memset(obj->deta_every_1ms_history_log, 0, sizeof(obj->deta_every_1ms_history_log));
	obj->low_adjust_count = 0;
	obj->hight_adjust_count = 0;
	obj->return_adjust_count = 0;
	memset(obj->deta_every_1ms_history, 0, sizeof(obj->deta_every_1ms_history));
	sc_always(MOD_DISPLAY, "Stop.");
	return 0;
}
int sc_display_set_csc(display_handle_t handle, int format, int convert_matric, int change_range)
{
	disp_obj_t *obj = (disp_obj_t *)handle;
	hdmi_csc_t csc;
	csc.format = format;
	csc.color_matric = convert_matric;
	csc.change_range = change_range;
	sc_always("csc format %d, matrix %d change_range: %d", format, convert_matric, change_range);

	if ((change_range) &&
	    (convert_matric == SC_VIDEO_MATRIX_BT601_FULL) &&
	    (convert_matric == SC_VIDEO_MATRIX_BT709_FULL) &&
	    (convert_matric == SC_VIDEO_MATRIX_NODATA_FULL) ) {
		sc_err("full range can't change csc");
		return -1;
	}

	return set_display_interface_ctl(obj->current_display_interface, DISPLAY_INTERFACE_CTL_SET_HDMI_CSC, &csc,
	        sizeof(hdmi_csc_t));
}

int sc_display_set_layer_csc(display_handle_t handle, STRU_SC_HAL_VO_LAYER_CSC *csc)
{
	disp_obj_t *obj = (disp_obj_t *)handle;
	int  ret = SC_HAL_VO_SUCCESS;
	sc_always("set layer(%d) csc  input:%d, output:%d.", obj->overlay_id,
	    csc->input, csc->output);
	ret = dc_set_layer_csc(obj->overlay_id, csc->input);
	if (ret != SC_HAL_VO_SUCCESS)
		return ret;

	/* only one output module*/
	ret = dc_set_output_csc(csc->output);
	if (ret != SC_HAL_VO_SUCCESS)
		return ret;

	return SC_HAL_VO_SUCCESS;
}

void sc_display_overlay_set(display_handle_t handle, unsigned int x, unsigned int y)
{
	disp_obj_t *obj = (disp_obj_t *)handle;
	obj->overlay_x = x;
	obj->overlay_y = y;
	dc_set_overlay_pos(&obj->dc_inst, x, y);
}

static void display_first_overlay_frame(disp_obj_t *obj, display_buffer_t *buffer)
{
	sc_always("display_first overlay frame");
	Overlay_t *overlay = &(obj->dc_inst.overlay);
	unsigned int ybase_addr = (unsigned int)buffer->pannel[0].buffer_pa;
	unsigned int cbbase_addr = (unsigned int)buffer->pannel[1].buffer_pa;
	unsigned int crbase_addr = (unsigned int)buffer->pannel[2].buffer_pa;
	unsigned int frame_width = obj->width;
	unsigned int frame_height = obj->height;
	unsigned char format = obj->format;
	unsigned int y_stride =  obj->y_stride;
	unsigned int cb_stride = obj->cbcr_stride;
	unsigned int cr_stride = obj->cbcr_stride;
	// set frame parameters
	overlay->address[0] = ybase_addr;
	overlay->address[1] = cbbase_addr;
	overlay->address[2] = crbase_addr;
	overlay->stride[0] = y_stride;
	overlay->stride[1] = cb_stride;
	overlay->stride[2] = cr_stride;
	overlay->colorKey = 0;
	overlay->colorKeyHigh = 0;
	overlay->transparency = 0;
	overlay->rotAngle = ROT_ROT0;
	overlay->yuv_standard = 1;
	overlay->tileMode = TileMode_LINEAR;
	overlay->swizzle = Swizzle_ARGB;
	overlay->uvSwizzle = 0;
	overlay->format = format;
	overlay->clearOverlay = 0;

	// dcregOverlayAlphaBlendConfig
	if(format == Input_ARGB8888 || format == Input_ARGB1555) {
		overlay->srcAlphaMode = 0;
		overlay->srcGlobalAlphaMode = 0;
		overlay->scrBlendingMode = 2;
		overlay->srcAlphaFactor = 1;

		overlay->dstAlphaMode = 0;
		overlay->dstGlobalAlphaMode = 0;
		overlay->dstBlendingMode = 3;
		overlay->dstAlphaFactor = 0;

	} else {
		overlay->srcAlphaMode = 0;
		overlay->srcGlobalAlphaMode = 0;
		overlay->scrBlendingMode = 1;
		overlay->srcAlphaFactor = 1;

		overlay->dstAlphaMode = 0;
		overlay->dstGlobalAlphaMode = 0;
		overlay->dstBlendingMode = 0;
		overlay->dstAlphaFactor = 1;
	}

	overlay->tlX = 0;
	overlay->tlY = 0;
	overlay->brX = frame_width;
	overlay->brY = frame_height;

	overlay->srcGlobalColor = 0;
	overlay->dstGlobalColor = 0;
	overlay->width = frame_width;
	overlay->height = frame_height;
	overlay->clearValue = 0;
	//start first overlay display frame, first disable the overlay
	overlay->enable = 0;
	dc_set_overlay(obj);
	sc_func_exit();
}

static void display_video_first_frame(disp_obj_t *obj, display_buffer_t *buffer)
{
	sc_func_enter();
	Framebuffer_t *fb = &(obj->dc_inst.framebuffer);
	unsigned int ybase_addr = (unsigned int)NULL;
	unsigned int cbbase_addr = (unsigned int)NULL;
	unsigned int crbase_addr = (unsigned int)NULL;
	unsigned int frame_width = obj->width;
	unsigned int frame_height = obj->height;
	unsigned char format = obj->format;
	unsigned int y_stride =  obj->y_stride;
	unsigned int cb_stride = obj->cbcr_stride;
	unsigned int cr_stride = obj->cbcr_stride;

	if (buffer != NULL) {
		ybase_addr = (unsigned int)buffer->pannel[0].buffer_pa;
		cbbase_addr = (unsigned int)buffer->pannel[1].buffer_pa;
		crbase_addr = (unsigned int)buffer->pannel[2].buffer_pa;
	}

	if ( (obj->dc_inst.panel_hline != obj->width) ||
	    (obj->dc_inst.panel_vline != obj->height) ) {
		fb->scale = 1;
		sc_always("vo layer's rect(%d, %d) does not match the vo dev's(%d, %d).",
		    obj->width, obj->height, obj->dc_inst.panel_hline, obj->dc_inst.panel_vline);
	}

	sc_always("frame_width=%d frame_height=%d hline=%d vline=%d",
	    obj->width, obj->height, obj->dc_inst.panel_hline, obj->dc_inst.panel_vline);
	sc_always("enter disp y_stride=0x%x,cb_stride=0x%x,cr_stride=0x%x\n", y_stride, cb_stride, cr_stride);
	obj->dis_frame_num = 0;
	obj->adjust_status = 0;

	// set frame parameters
	fb->fb_phys_addr[0] = ybase_addr;
	fb->fb_phys_addr[1] = cbbase_addr;
	fb->fb_phys_addr[2] = crbase_addr;
	fb->fb_stride[0] = y_stride;
	fb->fb_stride[1] = cb_stride;
	fb->fb_stride[2] = cr_stride;

	fb->colorKey = 0;
	fb->colorKeyHigh = 0;

	// dcregFrameBufferConfig
	fb->valid = 0;
	fb->clearFB = 0;
	fb->transparency = 0;
	fb->rotAngle = 0;
	fb->fb_yuv_standard = 1;
	fb->tileMode = TileMode_LINEAR;
	fb->swizzle = Swizzle_ARGB;
	fb->uvSwizzle = 0;
	fb->fb_format = format;

	// dcregFrameBufferScaleConfig
	fb->filterTap = 3;
	fb->horizontalFilterTap = 3;

	fb->bgColor = 0;

	/* Original size in pixel before rotation and scale. */
	// dcregFrameBufferSize
	fb->width = frame_width;
	fb->height = frame_height;

	// dcregFrameBufferClearValue
	fb->clearValue = 0;

	fb->initialOffsetX = 0x8000;
	fb->initialOffsetY = 0x8000;

	// start display frame
	dc_set_framebuffer(
	    &obj->dc_inst
	);
	sc_always("First video frame show.");
	sc_func_exit();
}

int sc_display_layer_enable(display_handle_t handle, int width, int height, int format, float fps, int luma_stride,
    int chroma_stride)
{
	disp_obj_t *obj = (disp_obj_t *)handle;
	if(obj->start_flag) {
		sc_err("layer has already enabled!");
		return HAL_ERR_VO_LAYER_HAS_ENABLED;
	}

	//int64_t *p_deta=NULL;
	//int i=0;
	obj->width = width;
	obj->height = height;
	obj->format = format;
	obj->fps = fps;
	obj->y_stride = luma_stride;
	obj->cbcr_stride = chroma_stride;
	sc_always("width=%d height=%d y stride %u, chroma stride %u.format=%d fps=%d", width, height, obj->y_stride,
	    obj->cbcr_stride, format, (int)fps);

	//init some internal use stats
	obj->current_buf = NULL;
	obj->next_buf = NULL;
	obj->is_first_frame = 1;
	obj->dis_irq_count = 0;
	obj->dis_frame_num = 0;
	obj->top_field_cnt = 0;
	obj->bot_field_cnt = 0;
	obj->dis_loss_count = 0;
	obj->repeat_count = 0;
	obj->start_flag = 1;
	obj->stats_frame_count = 0;
	obj->exception_high_count = 0;
	obj->exception_low_count = 0;
	obj->average_low_time = 0;
	obj->average_high_time = 0;
	obj->normal_count = 0;
	obj->deta_err_count_too_large = 0;
	obj->deta_err_count_low_zero = 0;
	obj->next_buf_cfg_lock_deta = 0;
	obj->average_deta = 0;
	obj->vsync_interval = (uint64_t)1000 * 1000 * 1000 * 1000 / obj->fps; //ps

	memset(obj->deta_every_1ms_history_log, 0, sizeof(obj->deta_every_1ms_history_log));
	obj->low_adjust_count = 0;
	obj->hight_adjust_count = 0;
	obj->return_adjust_count = 0;
	memset(obj->deta_every_1ms_history, 0, sizeof(obj->deta_every_1ms_history));
	//clear the small step.

#if 0
	small_req_data_t *req = NULL;
	do {
		obj->small_step_req_queue->queue_pop(obj->small_step_req_queue, (void **)&req);
		if(req) {
			sc_free(req);
		}
	} while(req);
#endif

	flush_display_top_queue(obj);

	if(obj->overlay_id == 0) {
#if 0
		if(!get_display_server()->ui_mode) {
			dc_display_reset(obj);
		}
#endif
		display_video_first_frame(obj, NULL);
	} else {
		display_first_overlay_frame(obj, NULL);
	}
	return 0;
}

int sc_display_layer_flush_all_buffer(display_handle_t handle, SC_BOOL clr_all)
{
	disp_obj_t *obj = (disp_obj_t *)handle;
	sc_queue_t *disp_done_queue = (sc_queue_t *)obj->disp_done_queue;

	sc_lock(obj->queue_lock);
	flush_disp_buffer(obj);

	if(obj->interlace_mod == SC_SYSTEM_INTERLACE_MOD_NULL) {
		if(obj->next_buf) {
			disp_done_queue->queue_insert(disp_done_queue, obj->next_buf);
			obj->next_buf = NULL;
		}

		if ((obj->current_buf) && (clr_all)) {
			disp_done_queue->queue_insert(disp_done_queue, obj->current_buf);
			obj->current_buf = NULL;
		}
	} else {
		flush_hold_buffer(obj);
	}

	sc_unlock(obj->queue_lock);
	return 0;
}

int sc_display_layer_disable(display_handle_t handle)
{
	disp_obj_t *obj = (disp_obj_t *)handle;
	//display_buffer_t *buffer=NULL;
	//int x=0;
	sc_always("disable the layer %d.", obj->overlay_id);
	dc_display_pause(obj);

	obj->start_flag = 0;
	sc_display_layer_flush_all_buffer(handle, SC_TRUE);

	obj->out_height = 0;
	obj->out_width = 0;
	obj->is_first_frame = 1;
	obj->enable_irq_process = 0;
	obj->dis_irq_count = 0;
	obj->dis_frame_num = 0;
	obj->dis_loss_count = 0;
	obj->repeat_count = 0;
	obj->stats_frame_count = 0;
	obj->exception_high_count = 0;
	obj->exception_low_count = 0;
	obj->average_low_time = 0;
	obj->average_high_time = 0;
	obj->normal_count = 0;
	obj->deta_err_count_too_large = 0;
	obj->deta_err_count_low_zero = 0;
	obj->next_buf_cfg_lock_deta = 0;
	obj->average_deta = 0;
	obj->low_adjust_count = 0;
	obj->hight_adjust_count = 0;
	obj->return_adjust_count = 0;
	memset(obj->deta_every_1ms_history_log, 0, sizeof(obj->deta_every_1ms_history_log));
	obj->low_adjust_count = 0;
	obj->hight_adjust_count = 0;
	obj->return_adjust_count = 0;
	memset(obj->deta_every_1ms_history, 0, sizeof(obj->deta_every_1ms_history));
	sc_always("Stop.");
	return 0;
}

static int sc_display_enqueue_normal(display_handle_t handle, display_buffer_t *buffer)
{
	int status = 0;
	disp_obj_t *obj = (disp_obj_t *)handle;
	//status=obj->disp_queue->queue_insert_irq(obj->disp_queue,buffer);
	//  uint32_t x=0;
	int start_flag = 0;
	sc_queue_t *disp_done_queue = (sc_queue_t *)obj->disp_done_queue;
	int delay_time = 0;

	sc_lock(obj->queue_lock);
	start_flag = obj->start_flag;
	if(!start_flag) {
		disp_done_queue->queue_insert(disp_done_queue, buffer);
		sc_unlock(obj->queue_lock);
		return status;
	}
	if(obj->is_first_frame == 1) {
		obj->current_buf = buffer;
		delay_time = PS2MS(obj->vsync_interval >> 1);
		sc_delay(delay_time);
		dc_set_frameaddr2(&obj->dc_inst, buffer);
		dc_display_start(obj);
		obj->enable_irq_process = 1;
		if(obj->overlay_id == 0) {
			sc_always("start the display hw, delay %d .....", delay_time);
			reg_filter_index_set(&obj->dc_inst);
			//here we start the lcd
			//give some delay
			sc_delay(100);
			set_display_interface_res_infor(obj->current_display_interface, &obj->res_infor.res[obj->res_index]);
		}
		obj->is_first_frame = 0;
		obj->dis_loss_count = 0;
		obj->repeat_count = 0;
		obj->dis_irq_count = 0;
		obj->first_time = sc_get_timestamp_us() * 1000000;
		obj->first_deta = 0;

	} else {
		//cfg the next address
		dc_set_frameaddr2(&obj->dc_inst, buffer);
		buffer->timestamps_us = sc_get_timestamp_us() * 1000000;
		if(obj->next_buf) { //set the lasted queue buffer, as next buffer ,so back the last next buffer
			obj->dis_loss_count++;
			disp_done_queue->queue_insert(disp_done_queue, obj->next_buf);
		}
		obj->next_buf = buffer;
		obj->next_buf_cfg_time = sc_get_timestamp_us() * 1000000;
	}
	sc_unlock(obj->queue_lock);
	return status;
}

static int sc_display_enqueue_normal_fifo(display_handle_t handle, display_buffer_t *buffer)
{
	int status = 0;
	disp_obj_t *obj = (disp_obj_t *)handle;
	//  uint32_t x=0;
	int start_flag = 0;
	sc_queue_t *disp_done_queue = (sc_queue_t *)obj->disp_done_queue;
	sc_queue_t *disp_queue = (sc_queue_t *)obj->disp_queue;
	display_buffer_t *cfg_buffer = NULL, *tmp_buffer = NULL;
	sc_lock(obj->queue_lock);
	start_flag = obj->start_flag;
	if(!start_flag) {
		disp_done_queue->queue_insert(disp_done_queue, buffer);
		sc_unlock(obj->queue_lock);
		return status;
	}
	buffer->timestamps_enqueue = sc_get_timestamp_us();
	disp_queue->queue_insert(disp_queue, buffer);
	if(disp_queue->get_queue_size(disp_queue) > obj->fifo_depth) {
		disp_queue->queue_pop(disp_queue, (void **)&tmp_buffer);
		tmp_buffer->timestamps_cfg = 0;
		tmp_buffer->timestamps_show = 0;
		tmp_buffer->timestamps_done = sc_get_timestamp_us();
		obj->dis_loss_count++;
		//disp_done_queue->queue_insert(disp_done_queue,tmp_buffer);
		disp_done_queue_insert_wrapper(obj, disp_done_queue, tmp_buffer);
	}
	if(obj->is_first_frame == 1) {
		disp_queue->queue_pop(disp_queue, (void **)&cfg_buffer);
		dc_set_frameaddr2(&obj->dc_inst, cfg_buffer);
		dc_display_start(obj);
		if(obj->overlay_id == 0) {
			reg_filter_index_set(&obj->dc_inst);
			//here we start the lcd
			//give some delay
			sc_delay(100);
			set_display_interface_res_infor(obj->current_display_interface, &obj->res_infor.res[obj->res_index]);
		}
		obj->is_first_frame = 0;
		obj->dis_loss_count = 0;
		obj->first_time = sc_get_timestamp_us() * 1000000;
		obj->first_time = sc_get_timestamp_us();
		obj->current_buf = cfg_buffer;
		obj->next_buf = NULL;
		cfg_buffer->timestamps_cfg = sc_get_timestamp_us();
		cfg_buffer->timestamps_show = cfg_buffer->timestamps_cfg;
	} else {
		if(!obj->next_buf) {
			disp_queue->queue_pop(disp_queue, (void **)&cfg_buffer);
			dc_set_frameaddr2(&obj->dc_inst, cfg_buffer);
			obj->next_buf = cfg_buffer;
			cfg_buffer->timestamps_cfg = sc_get_timestamp_us();
		}
	}
	sc_unlock(obj->queue_lock);
	return status;
}
int sc_display_enqueue(display_handle_t handle, display_buffer_t *buffer)
{
	disp_obj_t *obj = (disp_obj_t *)handle;
	uint32_t end_time = 0;

	if(obj->src_push_first_time == 0)
		obj->src_push_first_time = sc_get_timestamp();

	obj->src_push_cnt++;
	if (obj->src_push_cnt == 1000) {
		obj->src_push_cnt = 0;
		end_time = sc_get_timestamp();
		obj->input_fps = 1000 * 1000 / ((end_time - obj->src_push_first_time));
		obj->src_push_first_time = end_time;
	}
	buffer->x = obj->overlay_x;
	buffer->y = obj->overlay_y;
	if(obj->interlace_mod == SC_SYSTEM_INTERLACE_MOD_NULL) {
		if(obj->enable_fifo_mode)
			sc_display_enqueue_normal_fifo(handle, buffer);
		else
			sc_display_enqueue_normal(handle, buffer);
	} else {
		//sc_display_enqueue_interlace(handle,buffer);
	}
	return 0;
}
display_buffer_t *sc_display_dequeue(display_handle_t handle)
{
	int status = 0;
	disp_obj_t *obj = (disp_obj_t *)handle;
	display_buffer_t *buffer = NULL;
	sc_queue_t *disp_done_queue = (sc_queue_t *)obj->disp_done_queue;
	sc_queue_t *disp_queue = (sc_queue_t *)obj->disp_queue;

	status = status;
	if(obj->interlace_mod == SC_SYSTEM_INTERLACE_MOD_NULL) {
		status = disp_done_queue->queue_pop(disp_done_queue, (void **)&buffer);
		if(!buffer) {
			if(disp_queue->get_queue_size(disp_queue) > 1) {
				status = disp_queue->queue_pop(disp_queue, (void **)&buffer);
			}
		}
	} else {
		status = disp_done_queue->queue_pop(disp_done_queue, (void **)&buffer);
	}
	return buffer;
}
static void display_oneframe_ok(void *usr_data)
{
}

static void power_on_display(disp_obj_t *obj)
{
	float fre = 148.25;
	DCUltraL_t *p_dc = &obj->dc_inst;
	p_dc->fps = obj->fps;
	fre = dc_power_on(p_dc);

	sc_enter_critical();
	SC_SET_REG_BITS(__REG32__(CGU_REG_BASE + 0x4018), 1, 0, 0);
	SC_SET_REG_BITS(__REG32__(CGU_REG_BASE + 0x4018), 1, 1, 1);
	SC_SET_REG_BITS(__REG32__(CGU_REG_BASE + 0x4018), 0, 2, 2);
	SC_SET_REG_BITS(__REG32__(CGU_REG_BASE + 0x4018), 1, 12, 12); // enable display clock
	sc_exit_critical();

	dc_cfg_qos(obj);
}

static display_server_t *display_server = NULL;

display_server_t *get_display_server(void)
{
	return  display_server;
}

display_handle_t *get_display_dev(void)
{
	return (display_handle_t *)get_display_server();
}

void sc_display_init(void)
{
	if(!display_server) {
		display_server = sc_malloc(sizeof(display_server_t));
		memset(display_server, 0, sizeof(display_server_t));
		display_server->lock = sc_creat_lock();

		/* hal init */
		display_server->hal.lock = sc_creat_lock();
		display_server->hal.mode = VO_HAL_MODE_MPP;
	}
	printf("vo init finish.\n");
}

static int register_handle_to_server(display_handle_t handle)
{
	int count = sizeof(display_server->handle) / sizeof(display_handle_t);
	int i = 0;
	for(i = 0; i < count; i++) {
		if(display_server->handle[i] == handle) {
			sc_err("have registed");
			return 0;
		}
	}
	for(i = 0; i < count; i++) {
		if(!display_server->handle[i]) {
			display_server->handle[i] = handle;
			return 0;
		}
	}
	sc_err("register failed ,full");
	return 0;
}

static display_handle_t find_handle_by_layer_id(int layer_id)
{
	int count = sizeof(display_server->handle) / sizeof(display_handle_t);
	disp_obj_t *obj = NULL;
	int i = 0;
	for(i = 0; i < count; i++) {
		obj = (disp_obj_t *)display_server->handle[i];
		if(obj && obj->overlay_id == layer_id) {
			return (display_handle_t)obj;
		}
	}
	return 0;
}

int uboot_sc_display_server_init(void)
{
	sc_display_init();
	// sc_vo_test();

#if 0
	for (int i = 0; i < 3; i++) {
		sc_always("uboot_sc_display_server_init %d", i);
		sc_delay(500);
	}
#endif
	return 0;
}

static int sc_display_probe(struct udevice *dev)
{
	printf("abc probe\r\n");
	return 0;
}

static int sc_display_bind(struct udevice *dev)
{
	printf("abc bind\r\n");
	return 0;
}

#if 0
static const struct udevice_id display_ids[] = {
	{ .compatible = "smartchip,display" },
	{ }
};

static int display_111read_timing(struct udevice *dev,
    struct display_timing *timing)
{
	return 0;
}

static int display_111lcd_enable(struct udevice *dev, int bpp,
    const struct display_timing *edid)
{
	return 0;
}

static const struct dm_display_ops display_ops = {
	.read_timing = display_111read_timing,
	.enable = display_111lcd_enable,
};

U_BOOT_DRIVER(display) = {
	.name   = "display",
	.id     = UCLASS_DISPLAY,
	.of_match   = display_ids,
	.ops    = &display_ops,
	.probe  = sc_display_probe,
	.priv_auto_alloc_size = 16,
};

#if 0
U_BOOT_DRIVER(display) = {
	.name   = "display",
	.id = UCLASS_DISPLAY,
	.of_match   = display_ids,
	.ops    = NULL,
	.bind       = sc_display_bind,
	.probe  = sc_display_probe,
	.priv_auto_alloc_size = 16,
	.platdata_auto_alloc_size = 16,
};
#endif
#endif

