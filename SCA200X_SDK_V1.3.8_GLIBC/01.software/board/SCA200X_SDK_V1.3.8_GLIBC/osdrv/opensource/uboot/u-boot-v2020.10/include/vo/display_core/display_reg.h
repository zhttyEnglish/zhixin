
#ifndef _DISPLAY_REG_H_
#define _DISPLAY_REG_H_

#include "vo/sc_display.h"
#define DC_BASE_ADDR 0x010A0000

// Host Interface registers
#define AQHiClockControl 0x0000 // 0x0000 x1 R/W
#define AQHIIdle 0x0004 // 0x0001 x1 R
#define AQAxiConfig 0x00A4 // 0x0029 x1 R
#define AQAxiStatus 0x000C // 0x0003 x1 R
#define AQIntrAcknowledge 0x0010 // 0x0004 x1 R
#define AQIntrEnbl 0x0014 // 0x0005 x1 R/W
#define GCChipRev 0x0024 // 0x0009 x1 R
#define GCChipDate 0x0028 // 0x000A x1 R
#define gcTotalReads 0x0040 // 0x0010 x1 R
#define gcTotalCycles 0x0078 // 0x001E x1 R/W
#define gcregHIChipPatchRev 0x0098 // 0x0026 x1 R
#define gcProductId 0x00A8 // 0x002A x1 R

// General Configuration Register
#define dcregGeneralConfig0 0x148C // 0x052C x2 RW

// Frame Buffer Registers
#define dcregFrameBufferAddress0 0x1400 // 0x0500 x2 R/W
#define dcregFrameBufferStride0 0x1408 // 0x0502 x2 R/W
#define dcregFrameBufferColorKey0 0x1508 // 0x0542 x2 R/W
#define dcregFrameBufferColorKeyHigh0 0x1510 // 0x0544 x2 R/W
#define dcregFrameBufferConfig0 0x1518 // 0x0546 x2 somebitsR/W
#define dcregFrameBufferScaleConfig0 0x1520 // 0x0548 x2 R/W
#define dcregFrameBufferBGColor0 0x1528 // 0x054A x2 R/W
#define dcregFrameBufferUPlanarAddress0 0x1530 // 0x054C x2 R/W
#define dcregFrameBufferVPlanarAddress0 0x1538 // 0x054E x2 R/W
#define dcregFrameBufferUStride0 0x1800 // 0x0600 x2 R/W
#define dcregFrameBufferVStride0 0x1808 // 0x0602 x2 R/W
#define dcregFrameBufferSize0 0x1810 // 0x0604 x2 R/W
#define dcregFrameBufferScaleFactorX0 0x1828 // 0x060A x2 R/W
#define dcregFrameBufferScaleFactorY0 0x1830 // 0x060C x2 R/W
#define dcregFrameBufferClearValue0 0x1A18 // 0x0686 x2 R/W
#define dcregFrameBufferInitialOffset0 0x1A20 // 0x0688 x2 R/W

// Dither Registers
#define dcregDisplayDitherConfig0 0x1410 // 0x0504 x2 R/W
#define dcregDisplayDitherTableLow0 0x1420 // 0x0508 x2 R/W
#define dcregDisplayDitherTableHigh0 0x1428 // 0x050A x2 R/W

// Panel Configuration Registers
#define dcregPanelConfig0 0x1418 // 0x0506 x2 R/W
#define dcregHDisplay0 0x1430 // 0x050C x2 R/W
#define dcregHSync0 0x1438 // 0x050E x2 R/W
#define dcregVDisplay0 0x1440 // 0x0510 x2 R/W
#define dcregVSync0 0x1448 // 0x0512 x2 R/W
#define dcregDisplayCurrentLocation0 0x1450 // 0x0514 x2 R

// Gamma Correction Registers
#define dcregGammaIndex0 0x1458 // 0x0516 x2 W
#define dcregGammaData0 0x1460 // 0x0518 x2 W

// Cursor Registers
#define dcregCursorConfig 0x1468 // 0x051A x1 R/W(somebits Ronly)
#define dcregCursorAddress 0x146C // 0x051B x1 R/W
#define dcregCursorLocation 0x1470 // 0x051C x1 R/W
#define dcregCursorBackground 0x1474 // 0x051D x1 R/W
#define dcregCursorForeground 0x1478 // 0x051E x1 R/W

// DPI Configuration Registers
#define dcregDpiConfig0 0x14B8 // 0x052E x2 R/W

// Interrupt & Gating Registers
#define dcregDisplayIntr 0x147C // 0x051F x1 R
#define dcregDisplayIntrEnable 0x1480 // 0x0520 x1 R/W
#define dcregCursorModuleClockGatingControl 0x1484 // 0x0521 x1 R/W
#define dcregModuleClockGatingControl0 0x1A28 // 0x068A x2 R/W

// Debug Registers
#define dcregDebugCounterSelect0 0x14D0 // 0x0534 x2 R/W
#define dcregDebugCounterValue0 0x14D8 // 0x0536 x2 R

// Filter and Index Registers
#define dcregIndexColorTableIndex0 0x1818 // 0x0606 x2 W
#define dcregIndexColorTableData0 0x1820 // 0x0608 x2 W
#define dcregHoriFilterKernelIndex0 0x1838 // 0x060E x2 W
#define dcregHoriFilterKernel0 0x1A00 // 0x0680 x2 W
#define dcregVertiFilterKernelIndex0 0x1A08 // 0x0682 x2 W
#define dcregVertiFilterKernel0 0x1A10 // 0x0684 x2 W

// Overlay Registers
#define dcregOverlayConfig0 0x1540 // 0x0550 x16 somebitsR/W
#define dcregOverlayAlphaBlendConfig0 0x1580 // 0x0560 x16 R/W
#define dcregOverlayAddress0 0x15C0 // 0x0570 x16 R/W
#define dcregOverlayStride0 0x1600 // 0x0580 x16 R/W
#define dcregOverlayTL0 0x1640 // 0x0590 x16 R/W
#define dcregOverlayBR0 0x1680 // 0x05A0 x16 R/W
#define dcregOverlaySrcGlobalColor0 0x16C0 // 0x05B0 x16 R/W
#define dcregOverlayDstGlobalColor0 0x1700 // 0x05C0 x16 R/W
#define dcregOverlayColorKey0 0x1740 // 0x05D0 x16 R/W
#define dcregOverlayColorKeyHigh0 0x1780 // 0x05E0 x16 R/W
#define dcregOverlaySize0 0x17C0 // 0x05F0 x16 R/W
#define dcregOverlayUPlanarAddress0 0x1840 // 0x0610 x16 R/W
#define dcregOverlayVPlanarAddress0 0x1880 // 0x0620 x16 R/W
#define dcregOverlayUStride0 0x18C0 // 0x0630 x16 R/W
#define dcregOverlayVStride0 0x1900 // 0x0640 x16 R/W
#define dcregOverlayClearValue0 0x1940 // 0x0650 x16 R/W
#define dcregOverlayIndexColorTableIndex0 0x1980 // 0x0660 x16 W
#define dcregOverlayIndexColorTableData0 0x19C0 // 0x0670 x16 W

// video layer csc
#define dcregVideoLayerYr          0x1E40
#define dcregVideoLayerUr          0x1E48
#define dcregVideoLayerVr          0x1E50
#define dcregVideoLayerCr          0x1E58

#define dcregVideoLayerYg          0x1E60
#define dcregVideoLayerUg          0x1E68
#define dcregVideoLayerVg          0x1E70
#define dcregVideoLayerCg          0x1E78

#define dcregVideoLayerYb          0x1E80
#define dcregVideoLayerUb          0x1E88
#define dcregVideoLayerVb          0x1E90
#define dcregVideoLayerCb          0x1E98

#define dcregVideoLayerYuvClipEn   0x1EA0
#define dcregVideoLayerYClip       0x1EA8
#define dcregVideoLayerUClip       0x1EB0
#define dcregVideoLayerVClip       0x1EB8

// overlay0 layer csc
#define dcregOverLayer0Yr          0x1A40
#define dcregOverLayer0Ur          0x1A80
#define dcregOverLayer0Vr          0x1AC0
#define dcregOverLayer0Cr          0x1B00

#define dcregOverLayer0Yg          0x1B40
#define dcregOverLayer0Ug          0x1B80
#define dcregOverLayer0Vg          0x1BC0
#define dcregOverLayer0Cg          0x1C00

#define dcregOverLayer0Yb          0x1C40
#define dcregOverLayer0Ub          0x1C80
#define dcregOverLayer0Vb          0x1CC0
#define dcregOverLayer0Cb          0x1D00

#define dcregOverLayer0YuvClipEn   0x1D40
#define dcregOverLayer0YClip       0x1D80
#define dcregOverLayer0UClip       0x1DC0
#define dcregOverLayer0VClip       0x1E00

// overlay1 layer csc
#define dcregOverLayer1Yr          0x1A44
#define dcregOverLayer1Ur          0x1A84
#define dcregOverLayer1Vr          0x1AC4
#define dcregOverLayer1Cr          0x1B04

#define dcregOverLayer1Yg          0x1B44
#define dcregOverLayer1Ug          0x1B84
#define dcregOverLayer1Vg          0x1BC4
#define dcregOverLayer1Cg          0x1C04

#define dcregOverLayer1Yb          0x1C44
#define dcregOverLayer1Ub          0x1C84
#define dcregOverLayer1Vb          0x1CC4
#define dcregOverLayer1Cb          0x1D04

#define dcregOverLayer1YuvClipEn   0x1D44
#define dcregOverLayer1YClip       0x1D84
#define dcregOverLayer1UClip       0x1DC4
#define dcregOverLayer1VClip       0x1E04

// output csc
#define dcregOutputYr              0x1EC0
#define dcregOutputUr              0x1EC8
#define dcregOutputVr              0x1ED0
#define dcregOutputCr              0x1ED8

#define dcregOutputYg              0x1EE0
#define dcregOutputUg              0x1EE8
#define dcregOutputVg              0x1EF0
#define dcregOutputCg              0x1EF8

#define dcregOutputYb              0x1F00
#define dcregOutputUb              0x1F08
#define dcregOutputVb              0x1F10
#define dcregOutputCb              0x1F18

#define dcregOutputYuvClipEn       0x1F20
#define dcregOutputYClip           0x1F28
#define dcregOutputUClip           0x1F30
#define dcregOutputVClip           0x1F38

/* Read-only Registers */
#define VIV_CHIP_REVISION_REG                                 0x0024
#define VIV_CHIP_DATE_REG                                     0x0028
#define VIV_CHIP_PATCH_REVISION_REG                           0x0098
#define VIV_CHIP_PRODUCT_ID_REG                               0x00A8

/*******************************************************************************
** Register access.
*/
unsigned int reg_read_dc(
    unsigned int addr
);

void reg_write_dc(
    unsigned int addr,
    unsigned int data
);

/*******************************************************************************
** Function operations
*/
void reg_config_en(void);
void reg_display_start(void);
void reg_display_overlay_start(void);
void reg_display_pause(void);
void reg_display_overlay_pause(void);

void reg_soft_reset(void);

void reg_framebuffer_addr(
    DCUltraL_t *dc
);
void reg_overlay_addr(
    DCUltraL_t *dc
);
void reg_overlay_pos(
    DCUltraL_t *dc
);

void reg_framebuffer_set(
    Framebuffer_t *framebuffer,
    int output_en,
    int gamma_en
);

/* Dither Operation */
void reg_dither_set(
    DCUltraL_t *dc
);

/* Display Operation */
void reg_panel_set(
    DCUltraL_t *dc
);

/* Gamma Correctrion Operation */
void reg_gamma_set(
    unsigned int (*gamma)[3]
);

/* Cursor Operation */
void reg_cursor_set(
    DCUltraL_t *dc
);

void reg_cursor_disable(void);
void reg_cursor_enable(void);

void reg_cursorlocation_set(
    DCUltraL_t *dc
);

void reg_dpi_set(
    DCUltraL_t *dc
);

/* Inerrupt Operation */
void reg_interrupt_set(
    DCUltraL_t *dc
);

unsigned int reg_interrupt_get(void);

void reg_filter_index_set(
    DCUltraL_t *dc
);

void reg_overlay_set(
    Overlay_t *overlay
);

void reg_version_get(
    unsigned int *chip_revision,
    unsigned int *chip_patch_revision,
    unsigned int *product_id,
    unsigned int *product_date
);

void reg_disp_axi_enable(void);
uint32_t reg_get_axi_total_reads(void);
void reg_test(char *pre);

#endif
