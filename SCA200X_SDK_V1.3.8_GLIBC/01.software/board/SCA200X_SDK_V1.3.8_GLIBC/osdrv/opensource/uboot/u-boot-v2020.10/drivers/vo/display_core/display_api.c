#include "vo/util.h"
#include "vo/display_core/display_type.h"
#include "vo/display_core/display_reg.h"
#include "vo/sc_display.h"
#include "vo/interface/display_interface.h"
#include "vo/display_hal/hal_vo.h"
//#include <asm/arch/sc_clk.h>
#include <linux/math64.h>

#define gcdPI                   3.14159265358979323846f

//#define gcoMATH_Sine(X)       (float)(sinf(X))   //hytest temp remove
#define gcoMATH_Sine(X)       (float)((X))

#define vivMATH_Add(X, Y)          (float)((X) + (Y))
#define vivMATH_Multiply(X, Y)     (float)((X) * (Y))
#define vivMATH_Divide(X, Y)       (float)((X) / (Y))
#define vivMATH_DivideFromUInteger(X, Y) (float)(X) / (float)(Y)

#define vivMATH_Int2Float(X)   (float)(X)

/* Alignment with a power of two value. */
#define gcmALIGN(n, align) \
(\
    ((n) + ((align) - 1)) & ~((align) - 1) \
)

/* FilterBlt information. */
#define gcvMAXKERNELSIZE        9
#define gcvSUBPIXELINDEXBITS    5

#define gcvSUBPIXELCOUNT \
    (1 << gcvSUBPIXELINDEXBITS)

#define gcvSUBPIXELLOADCOUNT \
    (gcvSUBPIXELCOUNT / 2 + 1)

#define gcvWEIGHTSTATECOUNT \
    (((gcvSUBPIXELLOADCOUNT * gcvMAXKERNELSIZE + 1) & ~1) / 2)

#define gcvKERNELTABLESIZE \
    (gcvSUBPIXELLOADCOUNT * gcvMAXKERNELSIZE * sizeof(unsigned short))

#define gcvKERNELSTATES \
    (gcmALIGN(gcvKERNELTABLESIZE + 4, 8))

#define CHANNLE_TABLE_NUM 8

static unsigned short ChannelTable[CHANNLE_TABLE_NUM][4] = {
	{4, 4, 4, 4}, /* XRGB444 */
	{4, 4, 4, 4}, /* ARGB444 */
	{1, 5, 5, 5}, /* XRGB1555 */
	{1, 5, 5, 5}, /* ARGB1555 */
	{0, 5, 6, 5}, /* RGB565 */
	{8, 8, 8, 8}, /* XRGB8888 */
	{8, 8, 8, 8}, /* ARGB8888 */
	{2, 10, 10, 10}, /* ARGB2101010 */
};

static const STRU_DC_CSC layer_csc_reg_tbl[SC_HAL_VO_LAYER_ID_MAX] = {
	{
		dcregVideoLayerYr, dcregVideoLayerUr, dcregVideoLayerVr, dcregVideoLayerCr,
		dcregVideoLayerYg, dcregVideoLayerUg, dcregVideoLayerVg, dcregVideoLayerCg,
		dcregVideoLayerYb, dcregVideoLayerUb, dcregVideoLayerVb, dcregVideoLayerCb,
		dcregVideoLayerYuvClipEn, dcregVideoLayerYClip, dcregVideoLayerUClip, dcregVideoLayerVClip
	},
	{
		dcregOverLayer0Yr, dcregOverLayer0Ur, dcregOverLayer0Vr, dcregOverLayer0Cr,
		dcregOverLayer0Yg, dcregOverLayer0Ug, dcregOverLayer0Vg, dcregOverLayer0Cg,
		dcregOverLayer0Yb, dcregOverLayer0Ub, dcregOverLayer0Vb, dcregOverLayer0Cb,
		dcregOverLayer0YuvClipEn, dcregOverLayer0YClip, dcregOverLayer0UClip, dcregOverLayer0VClip
	},
	{
		dcregOverLayer1Yr, dcregOverLayer1Ur, dcregOverLayer1Vr, dcregOverLayer1Cr,
		dcregOverLayer1Yg, dcregOverLayer1Ug, dcregOverLayer1Vg, dcregOverLayer1Cg,
		dcregOverLayer1Yb, dcregOverLayer1Ub, dcregOverLayer1Vb, dcregOverLayer1Cb,
		dcregOverLayer1YuvClipEn, dcregOverLayer1YClip, dcregOverLayer1UClip, dcregOverLayer1VClip
	}
};

static const STRU_DC_CSC_VALUE layer_csc_reg_value_tbl[SC_HAL_VO_CSC_DEFAULT] = {
	{
		0x04a8, 0xffff, 0x072b, 0xfffc225d,
		0x04a8, 0xff26, 0xfddd, 0x0001362a,
		0x04a8, 0x0874, 0x0001, 0xfffb7cd6,
		1,
		0x10, 0xeb,
		0x10, 0xf0,
		0x10, 0xf0
	},
	{
		0x0400, 0x0001, 0x0629, 0xFFFCECFA,
		0x0400, 0xFF45, 0xFE2A, 0x00014AA0,
		0x0400, 0x0744, 0xFFFF, 0xFFFC60C5,
		1,
		0x00, 0xff,
		0x00, 0xff,
		0x00, 0xff
	},
	{
		0x04a8, 0xfffe, 0x0662, 0xfffc875e,
		0x04a8, 0xfe6f, 0xfcbf, 0x0002206b,
		0x04a8, 0x0812, 0xFFFF, 0xfffbaf02,
		1,
		0x10, 0xeb,
		0x10, 0xf0,
		0x10, 0xf0
	},
	{
		0x0400, 0x0000, 0x057d, 0xfffd43d3,
		0x0400, 0xfea7, 0xfd35, 0x000213e3,
		0x0400, 0x06ef, 0xfffe, 0xfffc8b39,
		1,
		0x00, 0xff,
		0x00, 0xff,
		0x00, 0xff
	}
};

static const STRU_DC_CSC output_csc_reg_tbl = {
	dcregOutputYr, dcregOutputUr, dcregOutputVr, dcregOutputCr,
	dcregOutputYg, dcregOutputUg, dcregOutputVg, dcregOutputCg,
	dcregOutputYb, dcregOutputUb, dcregOutputVb, dcregOutputCb,
	dcregOutputYuvClipEn, dcregOutputYClip, dcregOutputUClip, dcregOutputVClip
};

static const STRU_DC_CSC_VALUE output_csc_reg_value_tbl[SC_HAL_VO_CSC_DEFAULT] = {
	{
		0x00bb, 0x0275, 0x003f, 0x004200,
		0xff99, 0xfea6, 0x01c2, 0x020200,
		0x01c2, 0xfe67, 0xffd7, 0x020200,
		1,
		0x10, 0xea,
		0x10, 0xf0,
		0x10, 0xf0
	},
	{
		0x00da, 0x02dc, 0x004a, 0x000200,
		0xff88, 0xfe6d, 0x020b, 0x020200,
		0x020b, 0xfe25, 0xffd0, 0x020200,
		1,
		0x00, 0xff,
		0x00, 0xff,
		0x00, 0xff
	},
	{
		0x0107, 0x0204, 0x0064, 0x004200,
		0xff68, 0xfed6, 0x01c2, 0x020200,
		0x01c2, 0xfe87, 0xffb7, 0x020200,
		1,
		0x10, 0xea,
		0x10, 0xf0,
		0x10, 0xf0
	},
	{
		0x0132, 0x0259, 0x0075, 0x000200,
		0xff50, 0xfea5, 0x020b, 0x020200,
		0x020b, 0xfe4a, 0xffab, 0x020200,
		1,
		0x00, 0xff,
		0x00, 0xff,
		0x00, 0xff
	}
};

#define MAX_DISPLAY_OBJ_NUM 2
static disp_obj_t *g_display_obj[MAX_DISPLAY_OBJ_NUM] = {NULL};
static int register_display_obj(disp_obj_t *obj)
{
	//first to check if have been register
	int i = 0;
	for(i = 0; i < MAX_DISPLAY_OBJ_NUM; i++) {
		if(g_display_obj[i] == obj) {
			sc_err("obj %p have register", obj);
			return -1;
		} else if(g_display_obj[i] == NULL) {
			g_display_obj[i] = obj;
			sc_always("find a slot %d to register", i);
			return 0;
		}
	}
	sc_err("no slot for display obj");
	return -1;
}

#if 0
static int unregister_display_obj(disp_obj_t *obj)
{
	//first to check if have been register
	int i = 0;
	for(i = 0; i < MAX_DISPLAY_OBJ_NUM; i++) {
		if(g_display_obj[i] == obj) {
			sc_err("obj %p have register, unregister it", obj);
			return 0;
		}
	}
	sc_err("the obj %p not register, can not unregister", obj);
	return -1;
}
#endif

/* Get bit width of each channel.
 */
static int getChannelWidth(
    InputFormat_e fb_format,
    unsigned short *a,
    unsigned short *r,
    unsigned short *g,
    unsigned short *b
)
{
	if (fb_format == Input_ARGB2101010)
		fb_format = 7;

	if (fb_format >= CHANNLE_TABLE_NUM)
		return dcSTATUS_INVALID_ARGUMENTS;

	if (a != NULL)
		*a = ChannelTable[fb_format][0];

	if (r != NULL)
		*r = ChannelTable[fb_format][1];

	if (g != NULL)
		*g = ChannelTable[fb_format][2];

	if (b != NULL)
		*b = ChannelTable[fb_format][3];

	return dcSTATUS_OK;
}

#if 0
/* Get bits per pixel of format.
 */
static int getBpp(
    InputFormat_e format
)
{
	int val;

	switch (format) {
	case Input_A8:
	case Input_R8:
	case Input_INDEX8:
	case Input_NV12:
	case Input_NV16:
		// 8-bit colors.
		val = 8;
		break;

	case Input_XRGB4444:
	case Input_ARGB4444:
	case Input_XRGB1555:
	case Input_ARGB1555:
	case Input_RGB565:
	case Input_UYVY:
	case Input_YUY2:
	case Input_YV12:
	case Input_RG16:
		// 16-bit colors.
		val = 16;
		break;

	case Input_INDEX1:
		val = 1;
		break;

	case Input_INDEX2:
		val = 2;
		break;

	case Input_INDEX4:
		val = 4;
		break;

	default:
		// Default to 32-bit colors.
		val = 32;
		break;
	}

	return val;
}

// When YUV, calculate each planar's size, and bits per pixel.
// When RGB, calculate bits per pixel.
static void calcBufferSizeBpp(
    InputFormat_e format,
    unsigned int imageWidth,
    unsigned int imageHeight,
    unsigned int *nPlane,
    unsigned int width[],
    unsigned int height[],
    unsigned int bpp[]
)
{
	int i;

	for (i = 0; i < 3; i++) {
		width[i] = 0;
		height[i] = 0;
		bpp[i] = 0;
	}

	switch (format) {
	case Input_XRGB4444:
	case Input_ARGB4444:
	case Input_XRGB1555:
	case Input_ARGB1555:
	case Input_RGB565:
		*nPlane = 1;
		width[0] = imageWidth;
		height[0] = imageHeight;
		bpp[0] = 16;
		break;

	case Input_XRGB8888:
	case Input_ARGB8888:
	case Input_ARGB2101010:
		*nPlane = 1;
		width[0] = imageWidth;
		height[0] = imageHeight;
		bpp[0] = 32;
		break;

	case Input_YUY2:
	case Input_UYVY:
		*nPlane = 1;
		width[0] = imageWidth;
		height[0] = imageHeight;
		bpp[0] = 16;
		break;

	case Input_YV12:
		*nPlane = 3;
		width[0] = imageWidth;
		height[0] = imageHeight;
		bpp[0] = 8;
		width[1] = width[2] = imageWidth / 2;
		height[1] = height[2] = imageHeight / 2;
		bpp[1] = bpp[2] = 8;
		break;

	case Input_NV12:
		*nPlane = 2;
		width[0] = imageWidth;
		height[0] = imageHeight;
		bpp[0] = 8;
		width[1] = imageWidth / 2;
		height[1] = imageHeight / 2;
		bpp[1] = 2 * 8;
		break;

	case Input_NV16:
		*nPlane = 2;
		width[0] = imageWidth;
		height[0] = imageHeight;
		bpp[0] = 8;
		width[1] = imageWidth / 2;
		height[1] = imageHeight;
		bpp[1] = 2 * 8;
		break;

	case Input_NV12_10BIT:
		*nPlane = 2;
		width[0] = imageWidth;
		height[0] = imageHeight;
		bpp[0] = 10;
		width[1] = imageWidth / 2;
		height[1] = imageHeight / 2;
		bpp[1] = 20;
		break;

	case Input_NV16_10BIT:
		*nPlane = 2;
		width[0] = imageWidth;
		height[0] = imageHeight;
		bpp[0] = 1 * 10;
		width[1] = imageWidth / 2;
		height[1] = imageHeight;
		bpp[1] = 1 * 20;
		break;

	case Input_A8:
		*nPlane = 1;
		width[0] = imageWidth;
		height[0] = imageHeight;
		bpp[0] = 8;
		break;

	case Input_P010:
		*nPlane = 2;
		width[0] = imageWidth;
		height[0] = imageHeight;
		bpp[0] = 16;
		width[1] = imageWidth / 2;
		height[1] = imageHeight / 2;
		bpp[1] = 32;
		break;

	default:
		width[0] = imageWidth;
		height[0] = imageHeight;

		/* Other format. */
		bpp[0] = getBpp(format);
		*nPlane = 1;
		break;
	}

}
#endif

static unsigned int getStretchFactor(
    unsigned int srcSize,
    unsigned int destSize
)
{
	unsigned int stretchFactor = 0;

	if ((srcSize > 1) && (destSize > 1)) {
		stretchFactor = ((srcSize - 1) << 16) / (destSize - 1);
	}

	return stretchFactor;
}

static float sincFilter(
    float x,
    int radius
)
{
	float pit, pitd, f1, f2, result;
	float fRadius = vivMATH_Int2Float(radius);

	if (x == 0.0f) {
		result = 1.0f;
	} else if ((x < -fRadius) || (x > fRadius)) {
		result = 0.0f;
	} else {
		pit  = vivMATH_Multiply(gcdPI, x);
		pitd = vivMATH_Divide(pit, fRadius);

		f1 = vivMATH_Divide(gcoMATH_Sine(pit), pit);
		f2 = vivMATH_Divide(gcoMATH_Sine(pitd), pitd);

		result = vivMATH_Multiply(f1, f2);
	}

	return result;
}

/* Calculate weight array for sync filter.
 */
static void calculateSyncTable(
    unsigned char KernelSize,
    unsigned int SrcSize,
    unsigned int DstSize,
    unsigned int *kernelStates
)
{
	float fScale;
	int kernelHalf;
	float fSubpixelStep;
	float fSubpixelOffset;
	unsigned int subpixelPos;
	int kernelPos;
	int padding;
	unsigned short *kernelArray;

	fScale = vivMATH_DivideFromUInteger(DstSize, SrcSize);
	/* Adjust the factor for magnification. */
	if (fScale > 1.0f) {
		fScale = 1.0f;
	}

	/* Calculate the kernel half. */
	kernelHalf = (int) (KernelSize >> 1);

	/* Calculate the subpixel step. */
	fSubpixelStep = vivMATH_Divide(1.0f,
	        vivMATH_Int2Float(gcvSUBPIXELCOUNT));

	/* Init the subpixel offset. */
	fSubpixelOffset = 0.5f;

	/* Determine kernel padding size. */
	padding = (gcvMAXKERNELSIZE - KernelSize) / 2;

	/* Set initial kernel array pointer. */
	kernelArray = (unsigned short *) (kernelStates + 1);

	/* Loop through each subpixel. */
	for (subpixelPos = 0; subpixelPos < gcvSUBPIXELLOADCOUNT; subpixelPos++) {
		/* Define a temporary set of weights. */
		float fSubpixelSet[gcvMAXKERNELSIZE];

		/* Init the sum of all weights for the current subpixel. */
		float fWeightSum = 0.0f;
		unsigned short weightSum = 0;
		short adjustCount, adjustFrom;
		short adjustment;

		/* Compute weights. */
		for (kernelPos = 0; kernelPos < gcvMAXKERNELSIZE; kernelPos++) {
			/* Determine the current index. */
			int index = kernelPos - padding;

			/* Pad with zeros. */
			if ((index < 0) || (index >= KernelSize)) {
				fSubpixelSet[kernelPos] = 0.0f;
			} else {
				if (KernelSize == 1) {
					fSubpixelSet[kernelPos] = 1.0f;
				} else {
					/* Compute the x position for filter function. */
					float fX =
					    vivMATH_Multiply(
					        vivMATH_Add(
					            vivMATH_Int2Float(index - kernelHalf),
					            fSubpixelOffset),
					        fScale);

					/* Compute the weight. */
					fSubpixelSet[kernelPos] = sincFilter(fX, kernelHalf);
				}

				/* Update the sum of weights. */
				fWeightSum = vivMATH_Add(fWeightSum,
				        fSubpixelSet[kernelPos]);
			}
		}

		/* Adjust weights so that the sum will be 1.0. */
		for (kernelPos = 0; kernelPos < gcvMAXKERNELSIZE; kernelPos++) {
			/* Normalize the current weight. */
			float fWeight = vivMATH_Divide(fSubpixelSet[kernelPos],
			        fWeightSum);

			/* Convert the weight to fixed point and store in the table. */
			if (fWeight == 0.0f) {
				kernelArray[kernelPos] = 0x0000;
			} else if (fWeight >= 1.0f) {
				kernelArray[kernelPos] = 0x4000;
			} else if (fWeight <= -1.0f) {
				kernelArray[kernelPos] = 0xC000;
			} else {
				kernelArray[kernelPos] = (short)
				    vivMATH_Multiply(fWeight, 16384.0f);
			}

			weightSum += kernelArray[kernelPos];
		}

		/* Adjust the fixed point coefficients. */
		adjustCount = 0x4000 - weightSum;
		if (adjustCount < 0) {
			adjustCount = -adjustCount;
			adjustment = -1;
		} else {
			adjustment = 1;
		}

		adjustFrom = (gcvMAXKERNELSIZE - adjustCount) / 2;

		for (kernelPos = 0; kernelPos < adjustCount; kernelPos++) {
			kernelArray[adjustFrom + kernelPos] += adjustment;
		}

		kernelArray += gcvMAXKERNELSIZE;

		/* Advance to the next subpixel. */
		fSubpixelOffset = vivMATH_Add(fSubpixelOffset, -fSubpixelStep);
	}

}

int dc_cfg_qos(disp_obj_t *obj)
{
	if(obj->overlay_id == 0) {
		sc_always("");
		reg_write_dc(0x1a38, 0xff);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////
/////////////////////////// APIs /////////////////////////////////////////////////

void dc_display_reset(disp_obj_t *obj)
{
	sc_func_enter();
	//  unsigned int val=0;
	if(obj->overlay_id == 0) {
		sc_always("reset the display");
		SC_SET_REG_BITS(__REG32__(CGU_REG_BASE + 0x4018), 0, 9, 10);  // cclk pclk sw_reset
		SC_SET_REG_BITS(__REG32__(CGU_REG_BASE + 0x4018), 0x03, 9, 10); // cclk pclk sw_reset
		sc_delay(1);

		//     reg_soft_reset();
		reg_config_en();
		obj->dc_inst.intr_disp0_en = 1;
		obj->dc_inst.intr_disp1_en = 0;
		//reg_interrupt_set(&obj->dc_inst);

		/* enable AXI debug*/
		reg_disp_axi_enable();
	} else {
		sc_always("reset the display overlay, do nothing");
	}
}
void dc_display_start(disp_obj_t *obj)
{
	sc_func_enter();
	if(obj->overlay_id == 0) {
		reg_display_start();
	} else {
		reg_display_overlay_start();
	}
}
void dc_display_pause(disp_obj_t *obj)
{
	sc_func_enter();
	if(obj->overlay_id == 0) {
		reg_display_pause();
	} else {
		reg_display_overlay_pause();
	}
}
void dc_set_frameaddr(
    DCUltraL_t *dc,
    unsigned int address
)
{
	int i;

	dc->framebuffer.fb_phys_addr[0] = address;
	for (i = 1; i < dc->framebuffer.nPlanes; i++) {
		dc->framebuffer.fb_phys_addr[i] =
		    dc->framebuffer.fb_phys_addr[i - 1] + dc->framebuffer.fb_stride[i - 1] * dc->framebuffer.planeHeight[i - 1];
	}

	reg_framebuffer_addr(dc);
}

void dc_set_frameaddr2(
    DCUltraL_t *dc,
    display_buffer_t *buffer
)
{
	unsigned int address0 = (unsigned int)buffer->pannel[0].buffer_pa;
	unsigned int address1 = (unsigned int)buffer->pannel[1].buffer_pa;
	unsigned int address2 = (unsigned int)buffer->pannel[2].buffer_pa;
	int x = buffer->x;
	int y = buffer->y;
	if(dc->overlay_id == 0) {
		dc->framebuffer.fb_phys_addr[0] = address0 + dc->zoom_addr_offset[0];
		dc->framebuffer.fb_phys_addr[1] = address1 + dc->zoom_addr_offset[1];
		dc->framebuffer.fb_phys_addr[2] = address2 + dc->zoom_addr_offset[2];
		reg_framebuffer_addr(dc);
	} else {
		dc->overlay.address[0] = address0;
		dc->overlay.address[1] = address1;
		dc->overlay.address[2] = address2;
		dc->overlay.tlX = x;
		dc->overlay.tlY = y;
		dc->overlay.brX = x + dc->overlay.width;
		dc->overlay.brY = y + dc->overlay.height;
		reg_overlay_addr(dc);
	}
}

void dc_set_overlay_pos(
    DCUltraL_t *dc,
    int x, int y
)
{
	if(dc->overlay_id == 1) {
		dc->overlay.tlX = x;
		dc->overlay.tlY = y;
		dc->overlay.brX = x + dc->overlay.width;
		dc->overlay.brY = y + dc->overlay.height;
		reg_overlay_pos(dc);
	}
}

void dc_set_framebuffer(
    DCUltraL_t *dc
)
{
	//    unsigned int width[3], height[3];
	//    unsigned int bpp[3];
	//    unsigned int nPlane;
	//    unsigned int i;
	unsigned int scaleFactorX;
	unsigned int scaleFactorY;

	unsigned int *kernel_p = sc_malloc(128 * sizeof(int));
	if (dc->framebuffer.scale) {
		/* Generate factorX and factorY. */
		scaleFactorX = getStretchFactor(dc->framebuffer.width, dc->panel_hline);
		scaleFactorY = getStretchFactor(dc->framebuffer.height, dc->panel_vline);

		sc_info("scale:%x,%x\n", scaleFactorX, scaleFactorY);

		dc->framebuffer.scaleFactorX = scaleFactorX;
		dc->framebuffer.scaleFactorY = scaleFactorY;

		calculateSyncTable(dc->framebuffer.horizontalFilterTap, dc->framebuffer.width, dc->panel_hline, kernel_p);
		memcpy(&dc->horKernel, (kernel_p + 1), 312);
		calculateSyncTable(dc->framebuffer.filterTap, dc->framebuffer.height, dc->panel_vline, kernel_p);
		memcpy(&dc->verKernel, (kernel_p + 1), 312);
	}

	reg_framebuffer_set (
	    &(dc->framebuffer),
	    dc->output_enable,
	    dc->gamma_enable
	);
	//log_info("after reg_framebuffer_set\n");

	reg_filter_index_set(dc);
	sc_free(kernel_p);
}

void dc_set_zoom(
    DCUltraL_t *dc, float zoom
)
{
	//  unsigned int i;
	unsigned int scaleFactorX;
	unsigned int scaleFactorY;
	int w = (float)dc->framebuffer.width / zoom;
	int h = (float)dc->framebuffer.height / zoom;

	unsigned int *kernel_p = sc_malloc(128 * sizeof(int));
	if (dc->framebuffer.scale) {
		/* Generate factorX and factorY. */
		scaleFactorX = getStretchFactor(w, dc->panel_hline);
		scaleFactorY = getStretchFactor(h, dc->panel_vline);

		sc_info("scale:%x,%x\n", scaleFactorX, scaleFactorY);

		dc->framebuffer.scaleFactorX = scaleFactorX;
		dc->framebuffer.scaleFactorY = scaleFactorY;

		calculateSyncTable(dc->framebuffer.horizontalFilterTap, w, dc->panel_hline, kernel_p);
		memcpy(&dc->horKernel, (kernel_p + 1), 312);
		calculateSyncTable(dc->framebuffer.filterTap, h, dc->panel_vline, kernel_p);
		memcpy(&dc->verKernel, (kernel_p + 1), 312);
	}
	// dcregFrameBufferScaleConfig
	reg_write_dc(dcregFrameBufferScaleConfig0,
	    (dc->framebuffer.filterTap & 0xF)
	    | ((dc->framebuffer.horizontalFilterTap & 0xF) << 4) );

	/* Original size in pixel before rotation and scale. */
	// dcregFrameBufferSize
	reg_write_dc(dcregFrameBufferSize0, (w & 0x7fff) | ((h & 0x7fff) << 15));

	reg_write_dc(dcregFrameBufferScaleFactorX0, dc->framebuffer.scaleFactorX);
	reg_write_dc(dcregFrameBufferScaleFactorY0, dc->framebuffer.scaleFactorY);

	reg_filter_index_set(dc);
	sc_free(kernel_p);
}

int dc_set_output_csc(int csc_type)
{
	unsigned int reg_val = 0;
	const STRU_DC_CSC  *csc_reg;
	const STRU_DC_CSC_VALUE  *csc_val;

	if (csc_type > SC_HAL_VO_CSC_DEFAULT)
		return -1;
	else if (csc_type == SC_HAL_VO_CSC_DEFAULT)
		csc_type = SC_HAL_VO_CSC_BT709_FULL;

	csc_reg = &output_csc_reg_tbl;
	csc_val = &output_csc_reg_value_tbl[csc_type];

	sc_always("csc_type=%d", csc_type);

	reg_write_dc(csc_reg->Yr, csc_val->Yr);
	reg_write_dc(csc_reg->Ur, csc_val->Ur);
	reg_write_dc(csc_reg->Vr, csc_val->Vr);
	reg_write_dc(csc_reg->Cr, csc_val->Cr);

	reg_write_dc(csc_reg->Yg, csc_val->Yg);
	reg_write_dc(csc_reg->Ug, csc_val->Ug);
	reg_write_dc(csc_reg->Vg, csc_val->Vg);
	reg_write_dc(csc_reg->Cg, csc_val->Cg);

	reg_write_dc(csc_reg->Yb, csc_val->Yb);
	reg_write_dc(csc_reg->Ub, csc_val->Ub);
	reg_write_dc(csc_reg->Vb, csc_val->Vb);
	reg_write_dc(csc_reg->Cb, csc_val->Cb);

	reg_write_dc(csc_reg->YuvClipEn, csc_val->YuvClipEn);
	reg_val = reg_read_dc(csc_reg->YClip);
	SC_SET_REG_BITS(reg_val, csc_val->YClipLow, 0, 7);
	SC_SET_REG_BITS(reg_val, csc_val->YClipHigh, 16, 23);
	reg_write_dc(csc_reg->YClip, reg_val);

	reg_val = reg_read_dc(csc_reg->UClip);
	SC_SET_REG_BITS(reg_val, csc_val->UClipLow, 0, 7);
	SC_SET_REG_BITS(reg_val, csc_val->UClipHigh, 16, 23);
	reg_write_dc(csc_reg->UClip, reg_val);

	reg_val = reg_read_dc(csc_reg->VClip);
	SC_SET_REG_BITS(reg_val, csc_val->VClipLow, 0, 7);
	SC_SET_REG_BITS(reg_val, csc_val->VClipHigh, 16, 23);
	reg_write_dc(csc_reg->VClip, reg_val);

	return 0;
}

int dc_set_layer_csc(ENUM_SC_HAL_VO_LAYER_ID LayerId, ENUM_SC_HAL_VO_CSC csc_type)
{
	unsigned int reg_val = 0;
	const STRU_DC_CSC  *csc_reg;
	const STRU_DC_CSC_VALUE  *csc_val;

	if (LayerId >= SC_HAL_VO_LAYER_ID_MAX)
		return -1;

	if (csc_type > SC_HAL_VO_CSC_DEFAULT)
		return -1;
	else if (csc_type == SC_HAL_VO_CSC_DEFAULT)
		csc_type = SC_HAL_VO_CSC_BT709_FULL;

	sc_always("layerid = %d, csc_type=%d", LayerId, csc_type);

	csc_reg = &layer_csc_reg_tbl[LayerId];
	csc_val = &layer_csc_reg_value_tbl[csc_type];

	reg_write_dc(csc_reg->Yr, csc_val->Yr);
	reg_write_dc(csc_reg->Ur, csc_val->Ur);
	reg_write_dc(csc_reg->Vr, csc_val->Vr);
	reg_write_dc(csc_reg->Cr, csc_val->Cr);

	reg_write_dc(csc_reg->Yg, csc_val->Yg);
	reg_write_dc(csc_reg->Ug, csc_val->Ug);
	reg_write_dc(csc_reg->Vg, csc_val->Vg);
	reg_write_dc(csc_reg->Cg, csc_val->Cg);

	reg_write_dc(csc_reg->Yb, csc_val->Yb);
	reg_write_dc(csc_reg->Ub, csc_val->Ub);
	reg_write_dc(csc_reg->Vb, csc_val->Vb);
	reg_write_dc(csc_reg->Cb, csc_val->Cb);

	reg_write_dc(csc_reg->YuvClipEn, csc_val->YuvClipEn);
	reg_val = reg_read_dc(csc_reg->YClip);
	SC_SET_REG_BITS(reg_val, csc_val->YClipLow, 0, 7);
	SC_SET_REG_BITS(reg_val, csc_val->YClipHigh, 16, 23);
	reg_write_dc(csc_reg->YClip, reg_val);

	reg_val = reg_read_dc(csc_reg->UClip);
	SC_SET_REG_BITS(reg_val, csc_val->UClipLow, 0, 7);
	SC_SET_REG_BITS(reg_val, csc_val->UClipHigh, 16, 23);
	reg_write_dc(csc_reg->UClip, reg_val);

	reg_val = reg_read_dc(csc_reg->VClip);
	SC_SET_REG_BITS(reg_val, csc_val->VClipLow, 0, 7);
	SC_SET_REG_BITS(reg_val, csc_val->VClipHigh, 16, 23);
	reg_write_dc(csc_reg->VClip, reg_val);

	return 0;
}

int  dc_set_panel(
    DCUltraL_t *dc,
    int de_en,
    int da_en,
    int clock_en,
    int de_polarity,
    int da_polarity,
    int clock_polarity,
    unsigned int hsync_polarity,
    unsigned int vsync_polarity,
    unsigned int hline,
    unsigned int vline,
    float fps
)
{
	int index = 0;
	disp_obj_t *obj = (disp_obj_t *)dc->priv;
	int size = obj->res_infor.res_count;
	dc->panel_de_en = de_en;
	dc->panel_da_en = da_en;
	dc->panel_clock_en = clock_en;
	dc->panel_de_polarity = de_polarity;
	dc->panel_da_polarity = da_polarity;
	dc->panel_clock_polarity = clock_polarity;

	dc->panel_hline = hline;
	dc->panel_hsync_polarity = hsync_polarity;

	dc->panel_vline = vline;
	dc->panel_vsync_polarity = vsync_polarity;

	for(index = 0; index < size; index++) {
		if((obj->res_infor.res[index].h_valid_pixels == hline) && \
		    (obj->res_infor.res[index].v_valid_lines == vline) && \
		    (fps <= (obj->res_infor.res[index].fps + 0.2)) && (fps > (obj->res_infor.res[index].fps - 0.2))) {
			break;
		}
	}
	if(index >= size) {
		sc_err("can not find  a timing for this res, (%d %d %d)", hline, vline, (int)(fps * 10000));
		sc_err("so we find a 1920 1080 60 as default");
		for(index = 0; index < size; index++) {
			if((obj->res_infor.res[index].h_valid_pixels == 1920) && \
			    (obj->res_infor.res[index].v_valid_lines == 1080) && \
			    (60 <= (obj->res_infor.res[index].fps + 0.2)) && (60 > (obj->res_infor.res[index].fps - 0.2))) {
				break;
			}
		}
	}

	dc->panel_htotal = obj->res_infor.res[index].h_total_pixels;
	dc->panel_hsync_start = obj->res_infor.res[index].start_hsync;
	dc->panel_hsync_end = obj->res_infor.res[index].end_hsync;

	dc->panel_vtotal = obj->res_infor.res[index].v_total_lines;
	dc->panel_vsync_start = obj->res_infor.res[index].start_vsync;
	dc->panel_vsync_end = obj->res_infor.res[index].end_vsync;
	sc_always("index=%d %d %d %d %d %d %d %d %d %d", index, dc->panel_hline, dc->panel_htotal, dc->panel_hsync_start,
	    dc->panel_hsync_end, \
	    dc->panel_vline, dc->panel_vtotal, dc->panel_vsync_start, dc->panel_vsync_end, (int)(fps * 10000));
	reg_panel_set(dc);
	return index;
}

int  dc_set_panel2(
    DCUltraL_t *dc,
    int de_en,
    int da_en,
    int clock_en,
    int de_polarity,
    int da_polarity,
    int clock_polarity,
    unsigned int hsync_polarity,
    unsigned int vsync_polarity,
    disp_res_infor_t  *res
)
{
	dc->panel_de_en = de_en;
	dc->panel_da_en = da_en;
	dc->panel_clock_en = clock_en;
	dc->panel_de_polarity = de_polarity;
	dc->panel_da_polarity = da_polarity;
	dc->panel_clock_polarity = clock_polarity;

	dc->panel_hline = res->h_valid_pixels;
	dc->panel_hsync_polarity = hsync_polarity;

	dc->panel_vline = res->v_valid_lines;
	dc->panel_vsync_polarity = vsync_polarity;

	dc->panel_htotal = res->h_total_pixels;
	dc->panel_hsync_start = res->start_hsync;
	dc->panel_hsync_end = res->end_hsync;

	dc->panel_vtotal = res->v_total_lines;
	dc->panel_vsync_start = res->start_vsync;
	dc->panel_vsync_end = res->end_vsync;
	sc_always("%d %d %d %d %d %d %d %d %d", dc->panel_hline, dc->panel_htotal, dc->panel_hsync_start, dc->panel_hsync_end, \
	    dc->panel_vline, dc->panel_vtotal, dc->panel_vsync_start, dc->panel_vsync_end, (int)(res->fps * 10000));
	reg_panel_set(dc);
	return 0;
}

float dc_power_on(DCUltraL_t *dc)
{

	disp_obj_t *obj = (disp_obj_t *)dc->priv;
	float fre = 0;
	if(obj->interlace_mod == SC_SYSTEM_INTERLACE_MOD_NULL) {
		fre = (dc->panel_htotal * dc->panel_vtotal * dc->fps) / 1000.0 / 1000.0;
	} else {
		fre = ((dc->panel_htotal * dc->panel_vtotal * dc->fps / 2) + (dc->panel_htotal * (dc->panel_vtotal + 1) * dc->fps / 2))
		    / 1000.0 / 1000.0;
	}
#if 0
	uint32_t tmp = *((volatile uint32_t *)0x60634018);
	tmp &= ~(1 << 2);
	tmp |= 0x03;
	tmp |= (1 << 12);
	(*(volatile uint32_t *)0x60634018) = tmp;

	int clk = sc_clk_get(SC_CLK_MOD_DISP);
	sc_clk_set(SC_CLK_MOD_DISP, clk);

	tmp = *((volatile uint32_t *)0x60630000);
	tmp &= ~(1 << 15);
	*((volatile uint32_t *)0x60630000) = tmp;

	tmp = *((volatile uint32_t *)0x60630000);
	tmp |= (1 << 17);
	*((volatile uint32_t *)0x60630000) = tmp;

	sc_debug("0x60630000=0x%x 0x60634018=0x%x", *((volatile uint32_t *)0x60630000), *((volatile uint32_t *)0x60634018));
	sc_hal_set_pix_clk(fre);
#endif
	sc_always("fre2: %f",  fre);
	//sca200v100_cgu_init();
	sc_hal_set_pix_clk(fre * 2);
	return fre;
}

/* Set output.
 * enable:   Enable output or not.
 * format:   DPI data format.
 *           DPI_D16CFG1/DPI_D16CFG2/DPI_D16CFG3/DPI_D18CFG1/DPI_D18CFG2/DPI_D24/DPI_D30
 */
void dc_set_output(
    DCUltraL_t *dc,
    int enable,
    OutputFormat_e dpi_format
)
{
	// 1, When output is disabled, all pixels will be black. This allows panel to
	// have correct timing but without any pixels.
	dc->output_enable = enable;

	dc->output_dpi_format = dpi_format;
	// 2, Set DPI interface data format.
	reg_dpi_set(dc);
}

void dc_set_dither(
    DCUltraL_t *dc,
    int enable,
    InputFormat_e fb_format,
    unsigned int low,
    unsigned int high
)
{
	unsigned short rw, gw, bw;
	int ret;

	// When enabled, R8G8B8 mode show better on panels with fewer bits per pixel.
	dc->dither_enable = enable;

	ret = getChannelWidth(fb_format, NULL, &rw, &gw, &bw);

	if (ret != dcSTATUS_OK) {
		dc->dither_red_channel = 0;
		dc->dither_green_channel = 0;
		dc->dither_blue_channel = 0;
	} else {
		dc->dither_red_channel = rw;
		dc->dither_green_channel = gw;
		dc->dither_blue_channel = bw;
	}

	dc->dither_table_low = low;
	dc->dither_table_high = high;

	reg_dither_set(dc);
}

void dc_clear_dither(
    DCUltraL_t *dc
)
{
	dc->dither_enable = SET_DISABLE;

	reg_dither_set(dc);
}

void dc_set_gamma(
    DCUltraL_t *dc,
    int enable,
    unsigned int (*gamma_tab)[3]
)
{
	dc->gamma_enable = enable;

	if (enable)
		reg_gamma_set(gamma_tab);
}

void dc_cursor_disable(DCUltraL_t *dc)
{
	reg_cursor_disable();
}
void dc_cursor_enable(DCUltraL_t *dc)
{
	reg_cursor_enable();
}

#if 0
static uint32_t to_big_end(uint32_t val)
{
	uint32_t val1, val2, val3, val4;
	val1 = (val & 0xff) << 24;
	val2 = ((val & (0xff << 8)) >> 8) << 16;
	val3 = ((val & (0xff << 16)) >> 16) << 8;
	val4 = (val & (0xff << 24)) >> 24;
	return val1 | val2 | val3 | val4;
}
#endif

/* Move hardware cursor location on the screen.
 */
void dc_move_cursor(
    DCUltraL_t *dc,
    unsigned int x,
    unsigned int y
)
{
	dc->loc_x = x;
	dc->loc_y = y;
	reg_cursorlocation_set(dc);
}

void dc_get_cursor(
    DCUltraL_t *dc,
    unsigned int *x,
    unsigned int *y
)
{
	*x = dc->loc_x;
	*y = dc->loc_y;
}

void dc_set_overlay(disp_obj_t *obj)
{
	reg_overlay_set(&obj->dc_inst.overlay);
}

void dc_init(disp_obj_t *obj, DisNotify notify)
{

	int ret = 0;
	obj->dis_notify = notify;
	sc_always("dc init ,register irq process notify");
	ret = register_display_obj(obj);
	if(ret) {
		sc_err("can not register obj, create more than 2 layer");
		return;
	}

	if(obj->overlay_id == 0) {
		sc_always("dc init overlay id ==0 ,register irq funciton");
		// sc_irq_reg(DISPLAY_ENG_IRQ_NUM, dc_irq, NULL);
		/* Reset framebuffer to clear all framebuffer states.
		 * This function must be called before start DC(before other framebuffer setting).
		 * After reset framebuffer, set other framebuffer setting.
		 */
		//reg_test("111");
		reg_soft_reset();
		//reg_test("222");

		reg_config_en();

		obj->dc_inst.intr_disp0_en = 1;
		obj->dc_inst.intr_disp1_en = 0;

		//reg_interrupt_set(&obj->dc_inst);

		//reg_test("333");

	} else {
		sc_always("not back layer, not ops irq overlay_id=%d", obj->overlay_id);
	}
	//we malloc cursor mem
	sc_always("dc init ,register irq process notify exit");
}

void dc_deinit(disp_obj_t *obj)
{
	unsigned int idle;
	DCUltraL_t *dc = &obj->dc_inst;
	register_display_obj(obj);
	if(obj->overlay_id == 0) {
		/* reset dpi */
		reg_write_dc(0X1518, (1 << 4));
		/* disable overlay */
		reg_write_dc(0X1540, (0 << 24));
		/* disable cursor output. */
		reg_write_dc(0X1468, 0);
		/* wait hardware enter idle state */
		do {
			idle = reg_read_dc(0X4);

			if (idle == 0) {
				sc_delay(100);
			}
		} while ((idle & (1 << 16)) == 0);
		if (dc->irq) {

			/* disable interrupts */
			dc->intr_disp0_en = 0;
			dc->intr_disp1_en = 0;
			//reg_interrupt_set(dc);
		}
	}
}

void dc_reg_check(DCUltraL_t *dc)
{
	int i;
	unsigned int val;

	// frame buffer registers
	for (i = 0x500; i <= 0x502; i += 2) {
		val = reg_read_dc(i << 2);
		sc_always("%04x:%08x\n", i << 2, val);
	}

	for (i = 0x542; i <= 0x54e; i += 2) {
		val = reg_read_dc(i << 2);
		sc_always("%04x:%08x\n", i << 2, val);
	}

	for (i = 0x600; i <= 0x604; i += 2) {
		val = reg_read_dc(i << 2);
		sc_always("%04x:%08x\n", i << 2, val);
	}

	for (i = 0x60a; i <= 0x60c; i += 2) {
		val = reg_read_dc(i << 2);
		sc_always("%04x:%08x\n", i << 2, val);
	}

	for (i = 0x686; i <= 0x688; i += 2) {
		val = reg_read_dc(i << 2);
		sc_always("%04x:%08x\n", i << 2, val);
	}

	// Panel Configuration Registers
	val = reg_read_dc(0x506 << 2);
	sc_always("%04x:%08x\n", 0x506 << 2, val);
	for (i = 0x50c; i <= 0x514; i += 2) {
		val = reg_read_dc(i << 2);
		sc_always("%04x:%08x\n", i << 2, val);
	}

	// DPI Configuration Registers
	val = reg_read_dc(0x52e << 2);
	sc_always("%04x:%08x\n", 0x52e << 2, val);
}
