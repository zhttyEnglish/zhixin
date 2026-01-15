#include "vo/display_core/display_type.h"
#include "vo/display_core/display_reg.h"
#include "vo/util.h"

/*******************************************************************************
** Register access.
*/
unsigned int reg_read_dc(
    unsigned int addr
)
{
	return read_reg32(DC_BASE_ADDR + addr);
}

void reg_write_dc(
    unsigned int addr,
    unsigned int data
)
{
	write_reg32(DC_BASE_ADDR + addr, data);
}

/*******************************************************************************
** Function operations
*/

void reg_config_en()
{
	// Write 0 to this bit to reset the display controller, then configure the other registers and lastly
	// write a 1 to this bit to let the display controller start.
	reg_write_dc(dcregFrameBufferConfig0, 0);
}

void reg_display_start()
{
	// write a 1 to this bit to let the display controller start.
	unsigned int val = reg_read_dc(dcregFrameBufferConfig0);
	reg_write_dc(dcregFrameBufferConfig0, val | 0x10);
}

void reg_display_pause()
{
	// write a 1 to this bit to let the display controller start.
	unsigned int val = reg_read_dc(dcregFrameBufferConfig0);
	reg_write_dc(dcregFrameBufferConfig0, val & ~0x10);
}

void reg_display_overlay_start()
{
	// write a 1 to this bit to let the display controller start.
	unsigned int val = reg_read_dc(dcregOverlayConfig0);
	reg_write_dc(dcregOverlayConfig0, val | (1 << 24));
}

void reg_display_overlay_pause()
{
	// write a 1 to this bit to let the display controller start.
	unsigned int val = reg_read_dc(dcregOverlayConfig0);
	reg_write_dc(dcregOverlayConfig0, val & ~(1 << 24));
}

void reg_soft_reset()
{
	unsigned int val;

	val = reg_read_dc(0);
	reg_write_dc(0, val | (1 << 12));
	sc_delay(100);
	reg_write_dc(0, val);
}

void reg_framebuffer_addr(
    DCUltraL_t *dc
)
{
	reg_write_dc(dcregFrameBufferAddress0, dc->framebuffer.fb_phys_addr[0]);
	reg_write_dc(dcregFrameBufferUPlanarAddress0, dc->framebuffer.fb_phys_addr[1]);
	reg_write_dc(dcregFrameBufferVPlanarAddress0, dc->framebuffer.fb_phys_addr[2]);
}

void reg_overlay_addr(
    DCUltraL_t *dc
)
{
	reg_write_dc(dcregOverlayAddress0, dc->overlay.address[0]);
	reg_write_dc(dcregOverlayUPlanarAddress0, dc->overlay.address[1]);
	reg_write_dc(dcregOverlayVPlanarAddress0, dc->overlay.address[2]);
	reg_write_dc(dcregOverlayTL0, (dc->overlay.tlX & 0x7FFF) | ((dc->overlay.tlY & 0x7FFF) << 15) );
	reg_write_dc(dcregOverlayBR0, (dc->overlay.brX & 0x7FFF) | ((dc->overlay.brY & 0x7FFF) << 15) );
}

void reg_overlay_pos(
    DCUltraL_t *dc
)
{
	reg_write_dc(dcregOverlayTL0, (dc->overlay.tlX & 0x7FFF) | ((dc->overlay.tlY & 0x7FFF) << 15) );
	reg_write_dc(dcregOverlayBR0, (dc->overlay.brX & 0x7FFF) | ((dc->overlay.brY & 0x7FFF) << 15) );
}

void reg_framebuffer_set(
    Framebuffer_t *framebuffer,
    int output_en,
    int gamma_en
)
{
	unsigned int config = reg_read_dc(dcregFrameBufferConfig0);

	reg_write_dc(dcregFrameBufferAddress0, framebuffer->fb_phys_addr[0]);
	reg_write_dc(dcregFrameBufferUPlanarAddress0, framebuffer->fb_phys_addr[1]);
	reg_write_dc(dcregFrameBufferVPlanarAddress0, framebuffer->fb_phys_addr[2]);

	reg_write_dc(dcregFrameBufferStride0, framebuffer->fb_stride[0]);
	reg_write_dc(dcregFrameBufferUStride0, framebuffer->fb_stride[1]);
	reg_write_dc(dcregFrameBufferVStride0, framebuffer->fb_stride[2]);

	reg_write_dc(dcregFrameBufferColorKey0, framebuffer->colorKey);
	reg_write_dc(dcregFrameBufferColorKeyHigh0, framebuffer->colorKeyHigh);

	// dcregFrameBufferScaleConfig
	reg_write_dc(dcregFrameBufferScaleConfig0,
	    (framebuffer->filterTap & 0xF)
	    | ((framebuffer->horizontalFilterTap & 0xF) << 4) );

	reg_write_dc(dcregFrameBufferBGColor0, framebuffer->bgColor);

	/* Original size in pixel before rotation and scale. */
	// dcregFrameBufferSize
	reg_write_dc(dcregFrameBufferSize0,
	    (framebuffer->width & 0x7fff)
	    | ((framebuffer->height & 0x7fff) << 15) );

	reg_write_dc(dcregFrameBufferScaleFactorX0, framebuffer->scaleFactorX);
	reg_write_dc(dcregFrameBufferScaleFactorY0, framebuffer->scaleFactorY);

	// dcregFrameBufferClearValue
	reg_write_dc(dcregFrameBufferClearValue0, framebuffer->clearValue);

	reg_write_dc(dcregFrameBufferInitialOffset0,
	    (framebuffer->initialOffsetX & 0xffff)
	    | ((framebuffer->initialOffsetY & 0xffff) << 16) );

	// dcregFrameBufferConfig
	sc_always("framebuffer->fb_format=0x%x", framebuffer->fb_format);

	config &= ~(1 << 0 | 1 << 2 | 1 << 3 | 3 << 9 | 7 << 14 | 0x1f << 17 | 1 << 22 | 3 << 23 | 1 << 25 | 0x3f << 26);

	config |= (output_en & 1)
	    | ((gamma_en & 1) << 2)
	    | ((framebuffer->valid & 1) << 3)
	    | ((framebuffer->clearFB & 1) << 8)
	    | ((framebuffer->transparency & 3) << 9)
	    | ((framebuffer->rotAngle & 7) << 11)
	    | ((framebuffer->fb_yuv_standard & 7) << 14)
	    | ((framebuffer->tileMode & 0x1F) << 17)
	    | ((framebuffer->scale & 1) << 22)
	    | ((framebuffer->swizzle & 3) << 23)
	    | ((framebuffer->uvSwizzle & 1) << 25)
	    | ((framebuffer->fb_format & 0x3F) << 26) ;
	sc_always("config=0x%x", config);
	reg_write_dc(dcregFrameBufferConfig0, config);
}

/* Dither Operation */
void reg_dither_set(
    DCUltraL_t *dc
)
{
	unsigned int config = 0;

	config = config;
	if (dc->dither_enable) {
		reg_write_dc(0x01420, dc->dither_table_low);
		reg_write_dc(0x01428, dc->dither_table_high);

		config = (dc->dither_blue_channel & 0xF)
		    | ((dc->dither_green_channel & 0xF) << 8)
		    | ((dc->dither_red_channel & 0xF) << 16)
		    | 0x80000000;

		reg_write_dc(0x01410, dc->dither_enable << 31);
	} else {
		reg_write_dc(0x01410, 0);
		reg_write_dc(0x01420, 0);
		reg_write_dc(0x01428, 0);
	}
}

/* Display Operation */
void reg_panel_set(
    DCUltraL_t *dc
)
{
	unsigned int config;

	reg_test("444");

	config = dc->panel_de_en             |
	    ((dc->panel_de_polarity & 1) << 1) |
	    ((dc->panel_da_en & 1) << 4) |
	    ((dc->panel_da_polarity & 1) << 5) |
	    ((dc->panel_clock_en & 1) << 8) |
	    ((dc->panel_clock_polarity & 1) << 9);

	reg_write_dc(0x01418, config);

	config = (dc->panel_hline & 0x7fff)
	    | ((dc->panel_htotal & 0x7fff) << 16);

	reg_write_dc(0x01430, config);

	config = (dc->panel_vline & 0x7fff)
	    | ((dc->panel_vtotal & 0x7fff) << 16);

	reg_write_dc(0x01440, config);

	config = (dc->panel_hsync_start & 0x7FFF)
	    | ((dc->panel_hsync_end & 0x7FFF) << 15)
	    | 0x40000000   // pulse enabled
	    | ((dc->panel_hsync_polarity & 1) << 31);

	reg_write_dc(0x01438, config);

	config = (dc->panel_vsync_start & 0x7FFF)
	    | ((dc->panel_vsync_end & 0x7FFF) << 15)
	    | 0x40000000   // pulse enabled
	    | ((dc->panel_vsync_polarity & 1) << 31);

	reg_write_dc(0x01448, config);

}

/* Gamma Correctrion Operation */
void reg_gamma_set(
    unsigned int (*gamma)[3]
)
{
	unsigned int i, config;

	for (i = 0; i < GAMMA_TABLE_SIZE; i++) {
		reg_write_dc(0x01458, i & 0xFF);

		config = (gamma[i][2] & 0x3FF)
		    | ((gamma[i][1] & 0x3FF) << 10)
		    | ((gamma[i][0] & 0x3FF) << 20);

		reg_write_dc(0x01460, config);
	}
}

/* Cursor Operation */
void reg_cursor_set(
    DCUltraL_t *dc
)
{
	unsigned int config;
	config = (dc->cursor_format & 3)
	    | ((dc->display_controller & 1) << 4)
	    | ((dc->hot_spot_y & 0x1f) << 8)
	    | ((dc->hot_spot_x & 0x1f) << 16)
	    | ((dc->alpha_factor & 1) << 21)
	    | ((dc->flip_in_process & 1) << 31);

	reg_write_dc(0x01468, config);

	reg_write_dc(0x0146C, (uint32_t)dc->cursor_phy_addr);

	reg_write_dc(0x01474, dc->bg_color);
	reg_write_dc(0x01478, dc->fg_color);

}

void reg_cursor_disable()
{
	unsigned int config = 0;
	config = reg_read_dc(0x01468);
	config &= ~(0x3 << 0);
	\
	reg_write_dc(0x01468, config);
	return;
}

void reg_cursor_enable()
{
	unsigned int config = 0;
	config = reg_read_dc(0x01468);
	config &= ~(0x3 << 0);
	config |= 2;
	reg_write_dc(0x01468, config);
	return;
}

void reg_cursorlocation_set(
    DCUltraL_t *dc
)
{
	unsigned int data;

	data = (dc->loc_x & 0x7fff)
	    | ((dc->loc_y & 0x7fff) << 16);

	reg_write_dc(0x01470, data);
}

void reg_dpi_set(
    DCUltraL_t *dc
)
{
	unsigned int config;
	config = dc->output_dpi_format & 7;
	reg_write_dc(0x014B8, config);
	config = reg_read_dc(0x01f40);
	SC_SET_REG_BITS(config, dc->out_format, 4, 5);
	SC_SET_REG_BITS(config, dc->dvp_out_en, 16, 16);
	SC_SET_REG_BITS(config, dc->dsi_out_en, 17, 17);
	SC_SET_REG_BITS(config, 1, 20, 20);              // enable data fifo clear when one frame done
	SC_SET_REG_BITS(config, dc->rgb2yuv_en, 0, 0);
	reg_write_dc(0x01f40, config);
}

/* Inerrupt Operation */
void reg_interrupt_set(
    DCUltraL_t *dc
)
{
	unsigned int config;

	config = (dc->intr_disp0_en & 1)
	    | ((dc->intr_disp1_en & 1) << 4);

	reg_write_dc(dcregDisplayIntrEnable, config);
	sc_always("%x %x", reg_read_dc(dcregDisplayIntrEnable), config);
}

unsigned int reg_interrupt_get()
{
	unsigned int ret;

	ret = reg_read_dc(dcregDisplayIntr);

	return ret;
}

void reg_filter_index_set(
    DCUltraL_t *dc
)
{
	int i;

	if (!dc->framebuffer.scale)
		return;

	// HoriFilterKernel
	reg_write_dc(0x01838, 0);

	for (i = 0; i < 128; i++) {
		reg_write_dc(0x01A00, dc->horKernel[i]);
	}

	// VertiFilterKernel
	reg_write_dc(0x01A08, 0);

	for (i = 0; i < 128; i++) {
		reg_write_dc(0x01A10, dc->verKernel[i]);
	}
}

void reg_overlay_set(
    Overlay_t *overlay
)
{
	uint32_t config = 0;
	// dcregOverlayConfig
	config = (overlay->transparency & 3)
	    | ((overlay->rotAngle & 7) << 2)
	    | ((overlay->yuv_standard & 7) << 5)
	    | ((overlay->tileMode & 0x1F) << 8)
	    | ((overlay->swizzle & 3) << 13)
	    | ((overlay->uvSwizzle & 1) << 15)
	    | ((overlay->format & 0x3F) << 16)
	    | ((overlay->enable & 1) << 24)
	    | ((overlay->clearOverlay & 1) << 25);

	reg_write_dc(0x01540, config);

	// dcregOverlayAlphaBlendConfig
	/* Alpha blending mode. */
	config = (overlay->srcAlphaMode & 1)
	    | ((overlay->srcGlobalAlphaMode & 3) << 3)
	    | ((overlay->scrBlendingMode & 3) << 5) // 2
	    | ((overlay->srcAlphaFactor & 1) << 8)
	    | ((overlay->dstAlphaMode & 1) << 9)
	    | ((overlay->dstGlobalAlphaMode & 3) << 10)
	    | ((overlay->dstBlendingMode & 3) << 12) // 2
	    | ((overlay->dstAlphaFactor & 1) << 15); // 2

	reg_write_dc(0x01580, config);

	// dcregOverlayAddress
	reg_write_dc(0x015C0, overlay->address[0]);
	reg_write_dc(0x01840, overlay->address[1]); // Overlay UPlanar Address
	reg_write_dc(0x01880, overlay->address[2]); // Overlay VPlanar Address

	reg_write_dc(0x01600, overlay->stride[0]);
	reg_write_dc(0x018C0, overlay->stride[1]);
	reg_write_dc(0x01900, overlay->stride[2]);

	reg_write_dc(0x01640,
	    (overlay->tlX & 0x7FFF)
	    | ((overlay->tlY & 0x7FFF) << 15) );

	reg_write_dc(0x01680,
	    (overlay->brX & 0x7FFF)
	    | ((overlay->brY & 0x7FFF) << 15) );

	reg_write_dc(0x016C0, overlay->srcGlobalColor);
	reg_write_dc(0x01700, overlay->dstGlobalColor);

	reg_write_dc(0x01740, overlay->colorKey);
	reg_write_dc(0x01780, overlay->colorKeyHigh);

	// dcregOverlaySize
	reg_write_dc(0x017C0,
	    (overlay->width & 0x7FFF)
	    | ((overlay->height & 0x7FFF) << 15) );

	reg_write_dc(0x01940, overlay->clearValue);
}

void reg_version_get(
    unsigned int *chip_revision,
    unsigned int *chip_patch_revision,
    unsigned int *product_id,
    unsigned int *product_date
)
{
	if (chip_revision != NULL)
		*chip_revision = reg_read_dc(VIV_CHIP_REVISION_REG);

	if (chip_patch_revision != NULL)
		*chip_patch_revision = reg_read_dc(VIV_CHIP_PATCH_REVISION_REG);

	if (product_id != NULL)
		*product_id = reg_read_dc(VIV_CHIP_PRODUCT_ID_REG);

	if (product_date != NULL)
		*product_date = reg_read_dc(VIV_CHIP_DATE_REG);
}

void reg_disp_axi_enable(void)
{
	unsigned int val = 0;

	val = reg_read_dc(AQHiClockControl);
	val &= (~0x800);
	reg_write_dc(AQHiClockControl, val);
}

uint32_t reg_get_axi_total_reads(void)
{
	return reg_read_dc(gcTotalReads);
}

void reg_test(char *pre)
{
#if 0
	static int test = 0;
	unsigned int config;

	if (test++ % 2)
		config = 0x5420400;
	else
		config = 0x1420400;

	printf("%s:write 1430=0x%x\n", pre, config);
	reg_write_dc(0x01430, config);
	config = reg_read_dc(0x01430);
	printf("%s:read 1430=0x%x\n", pre, config);
#endif
}

