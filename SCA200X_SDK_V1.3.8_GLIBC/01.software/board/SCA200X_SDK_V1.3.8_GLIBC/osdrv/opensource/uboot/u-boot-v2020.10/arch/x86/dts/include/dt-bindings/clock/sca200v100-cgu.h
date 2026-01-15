/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2021 SmartChip Electronics Co. Ltd.
 */

#ifndef _DT_BINDINGS_CLK_SMARTCHIP_SCA200V100_H
#define _DT_BINDINGS_CLK_SMARTCHIP_SCA200V100_H

/* gating and divider clock ID */

// coresight
#define CLK_CFG_CS      0
#define CLK_CORE_CS     1

// hdecomp
#define CLK_CFG_HDECOMP     2
#define CLK_CORE_HDECOMP    3

// topdmac
#define CLK_CFG_TOPDMAC     4
#define CLK_AXI_TOPDMAC     5
#define CLK_CORE_TOPDMAC    6

// emmc
#define CLK_CFG_EMMC        7
#define CLK_EMMC_CARDCLK    8
#define CLK_EMMC_FIXED_PIN      8
#define CLK_EMMC_SAMPLE_PIN     8
#define CLK_EMMC_DRV_PIN        8
// security
#define CLK_CFG_SEC     9
#define CLK_CFG_EFUSE       10

// rom
#define CLK_CFG_ROM     11

// spi debug
#define CLK_CFG_SPIDBG      12

// unknown
#define CLK_150_NOC_G0      13

// isp rsz ifc eis
#define CLK_CFG_ISP     14
#define CLK_ISP         15
#define CLK_AXI_VIF     16
#define CLK_HDR         17
#define CLK_CFG_SCALER      18
#define CLK_SCALER      19

// jpeg
#define CLK_CFG_JPEG        20
#define CLK_CORE_JPEG       21

// disp
#define CLK_CFG_DISP        22
#define CLK_CORE_DISP       23

// Vision Network
#define CLK_150_NOC_G1      24

// ceva top
#define CLK_CFG_CEVA        25
#define CLK_150_NOC_G2      26

// dla_top
#define CLK_CFG_DLA     27

// venc
#define CLK_CFG_VENC        28
#define CLK_AXI_VENC        29
#define CLK_BPU_VENC        30

// unknown
#define CLK_150_NOC_G3      31

// unknown
#define CLK_150_NOC_G4      32

// disp
#define CLK_PIXEL_DISP      33

// dvpctl
#define CLK_PATTERN_GEN     34

// dvp_pin_topology
#define CLK_SUB_1_1X_PIX    35

// disp vif
#define CLK_DVP_PIX_HLY     36
#define CLK_PIX_DVP_HLY     36

// vif dvp_pin_topology
#define CLK_SUB_1_2X_PIX    37

// vif
#define CLK_DVP_PIX     38

// mipi_dsi  disp
#define CLK_DSI_PIX     39

// ddr
#define CLK_CFG_DDR     40
#define CLK_DDR         41

// mipi_csi
#define CLK_CFG_MIPI        42
#define CLK_VIDEO_MIPI0     43
#define CLK_VIDEO_MIPI1     44
#define CLK_VIDEO_MIPI2     45
#define CLK_VIDEO_MIPI3     46
#define CLK_PCS_MIPI        47

// mipi_dsi
#define CLK_CFG_DSI     48

// vif
#define CLK_CFG_VIF     49

// dvpctl
#define CLK_CFG_DVPCTL      50

// usb20_0
#define CLK_CFG_USB2OTG0    51
#define CLK_PHY_USB2PHY0    52

// usb20_1
#define CLK_CFG_USB2OTG1    53
#define CLK_PHY_USB2PHY1    54

// USB20_2
#define CLK_CFG_USB2OTG2    55
#define CLK_PHY_USB2PHY2    56

// unknown
#define CLK_150_NOC_G5      57

// gmac
#define CLK_CFG_GMAC        58
#define CLK_CORE_GMAC       59
#define CLK_PHY_GMAC        60

// cci
#define CLK_CCI         61

// sram
#define CLK_SRAM        62

// unknow
#define CLK_600_NOC     63

// clk sensor
#define CLK_SENSOR0     64
#define CLK_SENSOR1     65
#define CLK_SENSOR2     66
#define CLK_SENSOR3     67

// peripheral
#define CLK_CORE_PERIP      68

// audio_codec
#define CLK_AUDIO_300M      69
#define CLK_AUDIO_ADC       70

// i2s_mst
#define CLK_PERIP_I2SM      71

// sdcard
#define CLK_SDC_FIXED       72
#define CLK_SDC_SAMPLE      73
#define CLK_SDC_DRV     74

// qspi
#define CLK_CORE_QSPI       75

#define CLK_MAX_GATING      76

#define CLK_ID_MASK     0xffff
#define CLK_INFO_MASK       0xffff0000

/* Selector ID without gating */
#define CLK_EMMC_FREQ       1000
#define CLK_EMMC_CLK        1001
#define CLK_EMMC_DRV        1002
#define CLK_AUDIO       1003

/* PLL clock ID */
#define CLK_PLL_PIXEL       1004

/* Gating value*/
#define OFF         0x00000000
#define ON          0x00010000
#define CLK_GATING_SHIFT    16

/* Selector value */
#define CLK_CORE_HDECOMP_CLK666M            0x00000000
#define CLK_CORE_HDECOMP_CLK500M            0x00020000
#define CLK_CORE_HDECOMP_CLK400M            0x00030000
#define CLK_CORE_HDECOMP_CLK_BACKUP         0x00040000
#define CLK_CORE_HDECOMP_CLK_BACKUP_DIV2    0x00050000
#define CLK_CORE_HDECOMP_CLK_BACKUP_DIV4    0x00060000
#define CLK_CORE_HDECOMP_CLK_BACKUP_DIV8    0x00070000

#define CLK_CORE_TOPDMAC_CLK500M            0x00010000
#define CLK_CORE_TOPDMAC_CLK400M            0x00020000

#define CLK_CORE_TOPDMAC_CLK_BACKUP         0x00040000
#define CLK_CORE_TOPDMAC_CLK_BACKUP_DIV2    0x00050000
#define CLK_CORE_TOPDMAC_CLK_BACKUP_DIV4    0x00060000
#define CLK_CORE_TOPDMAC_CLK_BACKUP_DIV8    0x00070000

#define CLK_EMMC_SAMPLE_PIN_CLK_EMMC_SAMPLE 0x00000000
#define CLK_EMMC_SAMPLE_PIN_CLK_EMMC_DIV    0x00010000

#define CLK_EMMC_DRV_PIN_CLK_EMMC_DRV       0x00000000
#define CLK_EMMC_DRV_PIN_CLK_EMMC_DIV       0x00010000

#define CLK_ISP_CLKPIXEL                    0x00000000
#define CLK_ISP_CLK666M                     0x00010000
#define CLK_ISP_CLK500M                     0x00030000
#define CLK_ISP_CLK400M                     0x00050000
#define CLK_ISP_CLK300M                     0x00070000
#define CLK_ISP_CLK250M                     0x00080000
#define CLK_ISP_CLK200M                     0x00090000
#define CLK_ISP_CLK150M                     0x000a0000
#define CLK_ISP_CLK75M                      0x000b0000
#define CLK_ISP_CLK_BACKUP                  0x000c0000
#define CLK_ISP_CLK_BACKUP_DIV2             0x000d0000
#define CLK_ISP_CLK_BACKUP_DIV4             0x000e0000
#define CLK_ISP_CLK_BACKUP_DIV8             0x000f0000

#define CLK_AXI_VIF_CLKPIXEL                0x00000000
#define CLK_AXI_VIF_CLK666M                 0x00010000
#define CLK_AXI_VIF_CLK500M                 0x00030000
#define CLK_AXI_VIF_CLK400M                 0x00050000
#define CLK_AXI_VIF_CLK300M                 0x00070000
#define CLK_AXI_VIF_CLK250M                 0x00080000
#define CLK_AXI_VIF_CLK200M                 0x00090000
#define CLK_AXI_VIF_CLK150M                 0x000a0000
#define CLK_AXI_VIF_CLK75M                  0x000b0000
#define CLK_AXI_VIF_CLK_BACKUP              0x000c0000
#define CLK_AXI_VIF_CLK_BACKUP_DIV2         0x000d0000
#define CLK_AXI_VIF_CLK_BACKUP_DIV4         0x000e0000
#define CLK_AXI_VIF_CLK_BACKUP_DIV8         0x000f0000

#define CLK_HDR_CLKPIXEL                    0x00000000
#define CLK_HDR_CLK666M                     0x00010000
#define CLK_HDR_CLK500M                     0x00030000
#define CLK_HDR_CLK400M                     0x00050000
#define CLK_HDR_CLK300M                     0x00070000
#define CLK_HDR_CLK250M                     0x00080000
#define CLK_HDR_CLK200M                     0x00090000
#define CLK_HDR_CLK150M                     0x000a0000
#define CLK_HDR_CLK75M                      0x000b0000
#define CLK_HDR_CLK_BACKUP                  0x000c0000
#define CLK_HDR_CLK_BACKUP_DIV2             0x000d0000
#define CLK_HDR_CLK_BACKUP_DIV4             0x000e0000
#define CLK_HDR_CLK_BACKUP_DIV8             0x000f0000

#define CLK_SCALER_
#define CLK_SCALER_CLKPIXEL                 0x00000000
#define CLK_SCALER_CLK666M                  0x00010000
#define CLK_SCALER_CLK500M                  0x00030000
#define CLK_SCALER_CLK400M                  0x00050000
#define CLK_SCALER_CLK300M                  0x00070000
#define CLK_SCALER_CLK250M                  0x00080000
#define CLK_SCALER_CLK200M                  0x00090000
#define CLK_SCALER_CLK150M                  0x000a0000
#define CLK_SCALER_CLK75M                   0x000b0000
#define CLK_SCALER_CLK_BACKUP               0x000c0000
#define CLK_SCALER_CLK_BACKUP_DIV2          0x000d0000
#define CLK_SCALER_CLK_BACKUP_DIV4          0x000e0000
#define CLK_SCALER_CLK_BACKUP_DIV8          0x000f0000

#define CLK_CORE_JPEG_CLKPIXEL              0x00000000
#define CLK_CORE_JPEG_CLK666M               0x00010000
#define CLK_CORE_JPEG_CLK500M               0x00030000
#define CLK_CORE_JPEG_CLK400M               0x00050000
#define CLK_CORE_JPEG_CLK300M               0x00070000
#define CLK_CORE_JPEG_CLK250M               0x00080000
#define CLK_CORE_JPEG_CLK200M               0x00090000
#define CLK_CORE_JPEG_CLK150M               0x000a0000
#define CLK_CORE_JPEG_CLK75M                0x000b0000
#define CLK_CORE_JPEG_CLK_BACKUP            0x000c0000
#define CLK_CORE_JPEG_CLK_BACKUP_DIV2       0x000d0000
#define CLK_CORE_JPEG_CLK_BACKUP_DIV4       0x000e0000
#define CLK_CORE_JPEG_CLK_BACKUP_DIV8       0x000f0000

#define CLK_CORE_DISP_CLKPIXEL              0x00000000
#define CLK_CORE_DISP_CLK666M               0x00010000
#define CLK_CORE_DISP_CLK500M               0x00030000
#define CLK_CORE_DISP_CLK400M               0x00050000
#define CLK_CORE_DISP_CLK300M               0x00070000
#define CLK_CORE_DISP_CLK250M               0x00080000
#define CLK_CORE_DISP_CLK200M               0x00090000
#define CLK_CORE_DISP_CLK150M               0x000a0000
#define CLK_CORE_DISP_CLK75M                0x000b0000
#define CLK_CORE_DISP_CLK_BACKUP            0x000c0000
#define CLK_CORE_DISP_CLK_BACKUP_DIV2       0x000d0000
#define CLK_CORE_DISP_CLK_BACKUP_DIV4       0x000e0000
#define CLK_CORE_DISP_CLK_BACKUP_DIV8       0x000f0000

#define CLK_PIXEL_DISP_CLKPIXEL             0x00000000
#define CLK_PIXEL_DISP_CLKPIXEL_DIV2        0x00010000

#define CLK_VIDEO_MIPI0_CLKPIXEL            0x00000000
#define CLK_VIDEO_MIPI0_CLK500M             0x00020000
#define CLK_VIDEO_MIPI0_CLK400M             0x00030000
#define CLK_VIDEO_MIPI0_CLK300M             0x00040000
#define CLK_VIDEO_MIPI0_CLK200M             0x00050000
#define CLK_VIDEO_MIPI0_CLK150M             0x00060000
#define CLK_VIDEO_MIPI0_CLK125M             0x00070000
#define CLK_VIDEO_MIPI0_CLK100M             0x00080000
#define CLK_VIDEO_MIPI0_CLK75M              0x00090000
#define CLK_VIDEO_MIPI0_CLK50M              0x000a0000
#define CLK_VIDEO_MIPI0_CLK25M              0x000b0000
#define CLK_VIDEO_MIPI0_CLK_BACKUP          0x000c0000
#define CLK_VIDEO_MIPI0_CLK_BACKUP_DIV2     0x000d0000
#define CLK_VIDEO_MIPI0_CLK_BACKUP_DIV4     0x000e0000
#define CLK_VIDEO_MIPI0_CLK_BACKUP_DIV8     0x000f0000

#define CLK_VIDEO_MIPI1_CLKPIXEL            0x00000000
#define CLK_VIDEO_MIPI1_CLK500M             0x00020000
#define CLK_VIDEO_MIPI1_CLK400M             0x00030000
#define CLK_VIDEO_MIPI1_CLK300M             0x00040000
#define CLK_VIDEO_MIPI1_CLK200M             0x00050000
#define CLK_VIDEO_MIPI1_CLK150M             0x00060000
#define CLK_VIDEO_MIPI1_CLK125M             0x00070000
#define CLK_VIDEO_MIPI1_CLK100M             0x00080000
#define CLK_VIDEO_MIPI1_CLK75M              0x00090000
#define CLK_VIDEO_MIPI1_CLK50M              0x000a0000
#define CLK_VIDEO_MIPI1_CLK25M              0x000b0000
#define CLK_VIDEO_MIPI1_CLK_BACKUP          0x000c0000
#define CLK_VIDEO_MIPI1_CLK_BACKUP_DIV2     0x000d0000
#define CLK_VIDEO_MIPI1_CLK_BACKUP_DIV4     0x000e0000
#define CLK_VIDEO_MIPI1_CLK_BACKUP_DIV8     0x000f0000

#define CLK_VIDEO_MIPI2_CLKPIXEL            0x00000000
#define CLK_VIDEO_MIPI2_CLK500M             0x00020000
#define CLK_VIDEO_MIPI2_CLK400M             0x00030000
#define CLK_VIDEO_MIPI2_CLK300M             0x00040000
#define CLK_VIDEO_MIPI2_CLK200M             0x00050000
#define CLK_VIDEO_MIPI2_CLK150M             0x00060000
#define CLK_VIDEO_MIPI2_CLK125M             0x00070000
#define CLK_VIDEO_MIPI2_CLK100M             0x00080000
#define CLK_VIDEO_MIPI2_CLK75M              0x00090000
#define CLK_VIDEO_MIPI2_CLK50M              0x000a0000
#define CLK_VIDEO_MIPI2_CLK25M              0x000b0000
#define CLK_VIDEO_MIPI2_CLK_BACKUP          0x000c0000
#define CLK_VIDEO_MIPI2_CLK_BACKUP_DIV2     0x000d0000
#define CLK_VIDEO_MIPI2_CLK_BACKUP_DIV4     0x000e0000
#define CLK_VIDEO_MIPI2_CLK_BACKUP_DIV8     0x000f0000

#define CLK_VIDEO_MIPI3_CLKPIXEL            0x00000000
#define CLK_VIDEO_MIPI3_CLK500M             0x00020000
#define CLK_VIDEO_MIPI3_CLK400M             0x00030000
#define CLK_VIDEO_MIPI3_CLK300M             0x00040000
#define CLK_VIDEO_MIPI3_CLK200M             0x00050000
#define CLK_VIDEO_MIPI3_CLK150M             0x00060000
#define CLK_VIDEO_MIPI3_CLK125M             0x00070000
#define CLK_VIDEO_MIPI3_CLK100M             0x00080000
#define CLK_VIDEO_MIPI3_CLK75M              0x00090000
#define CLK_VIDEO_MIPI3_CLK50M              0x000a0000
#define CLK_VIDEO_MIPI3_CLK25M              0x000b0000
#define CLK_VIDEO_MIPI3_CLK_BACKUP          0x000c0000
#define CLK_VIDEO_MIPI3_CLK_BACKUP_DIV2     0x000d0000
#define CLK_VIDEO_MIPI3_CLK_BACKUP_DIV4     0x000e0000
#define CLK_VIDEO_MIPI3_CLK_BACKUP_DIV8     0x000f0000

#define CLK_PCS_MIPI_CLK200M                0x00000000
#define CLK_PCS_MIPI_CLK333M                0x00010000

#define CLK_DSI_PIX_CLKPIXEL                0x00000000
#define CLK_DSI_PIX_CLKPIXEL_DIV2           0x00010000

#define CLK_CFG_GMAC_CLK150M                0x00000000
#define CLK_CFG_GMAC_CLK150M_DIV2           0x00010000
#define CLK_CFG_GMAC_CLK150M_DIV4           0x00020000
#define CLK_CFG_GMAC_CLK150M_DIV8           0x00030000

#define CLK_PHY_GMAC_CLK50M                 0x00000000
#define CLK_PHY_GMAC_CLK25M                 0x00010000

#define CLK_SENSOR0_CLK_SENSOR_1            0x00000000
#define CLK_SENSOR0_CLK_SENSOR_1_DIV2       0x00010000
#define CLK_SENSOR0_CLK_SENSOR_2            0x00020000
#define CLK_SENSOR0_CLK24M                  0x00030000

#define CLK_SENSOR1_CLK_SENSOR_1            0x00000000
#define CLK_SENSOR1_CLK_SENSOR_1_DIV2       0x00010000
#define CLK_SENSOR1_CLK_SENSOR_2            0x00020000
#define CLK_SENSOR1_CLK24M                  0x00030000

#define CLK_SENSOR2_CLK_SENSOR_1            0x00000000
#define CLK_SENSOR2_CLK_SENSOR_1_DIV2       0x00010000
#define CLK_SENSOR2_CLK_SENSOR_2            0x00020000
#define CLK_SENSOR2_CLK24M                  0x00030000

#define CLK_SENSOR3_CLK_SENSOR_1            0x00000000
#define CLK_SENSOR3_CLK_SENSOR_1_DIV2       0x00010000
#define CLK_SENSOR3_CLK_SENSOR_2            0x00020000
#define CLK_SENSOR3_CLK24M                  0x00030000

#define CLK_CORE_QSPI_CLK400M               0x00010000

#define I2S_MST0_SCLK_CLK_PERIP_I2SM        0x00000000
#define I2S_MST0_SCLK_CLK_PERIP_I2SM_DIV2   0x00010000
#define I2S_MST0_SCLK_CLK_PERIP_I2SM_DIV4   0x00020000
#define I2S_MST0_SCLK_CLK_PERIP_I2SM_DIV8   0x00030000
#define I2S_MST0_SCLK_CLK_PERIP_I2SM_DIV16  0x00040000
#define I2S_MST0_SCLK_CLK_PERIP_I2SM_DIV32  0x00050000
#define I2S_MST0_SCLK_CLK_PERIP_I2SM_DIV64  0x00060000
#define I2S_MST0_SCLK_CLK_PERIP_I2SM_DIV128 0x00070000

#define I2S_MST1_SCLK_CLK_PERIP_I2SM        0x00000000
#define I2S_MST1_SCLK_CLK_PERIP_I2SM_DIV2   0x00010000
#define I2S_MST1_SCLK_CLK_PERIP_I2SM_DIV4   0x00020000
#define I2S_MST1_SCLK_CLK_PERIP_I2SM_DIV8   0x00030000
#define I2S_MST1_SCLK_CLK_PERIP_I2SM_DIV16  0x00040000
#define I2S_MST1_SCLK_CLK_PERIP_I2SM_DIV32  0x00050000
#define I2S_MST1_SCLK_CLK_PERIP_I2SM_DIV64  0x00060000
#define I2S_MST1_SCLK_CLK_PERIP_I2SM_DIV128 0x00070000

#define I2S_MST2_SCLK_CLK_PERIP_I2SM        0x00000000
#define I2S_MST2_SCLK_CLK_PERIP_I2SM_DIV2   0x00010000
#define I2S_MST2_SCLK_CLK_PERIP_I2SM_DIV4   0x00020000
#define I2S_MST2_SCLK_CLK_PERIP_I2SM_DIV8   0x00030000
#define I2S_MST2_SCLK_CLK_PERIP_I2SM_DIV16  0x00040000
#define I2S_MST2_SCLK_CLK_PERIP_I2SM_DIV32  0x00050000
#define I2S_MST2_SCLK_CLK_PERIP_I2SM_DIV64  0x00060000
#define I2S_MST2_SCLK_CLK_PERIP_I2SM_DIV128 0x00070000

#define CLK_AXI_VENC_CLKPIXEL               0x00000000
#define CLK_AXI_VENC_CLK666M                0x00010000
#define CLK_AXI_VENC_CLK500M                0x00030000
#define CLK_AXI_VENC_CLK400M                0x00050000
#define CLK_AXI_VENC_CLK300M                0x00070000
#define CLK_AXI_VENC_CLK250M                0x00080000
#define CLK_AXI_VENC_CLK200M                0x00090000
#define CLK_AXI_VENC_CLK150M                0x000a0000
#define CLK_AXI_VENC_CLK75M                 0x000b0000
#define CLK_AXI_VENC_CLK_BACKUP             0x000c0000
#define CLK_AXI_VENC_CLK_BACKUP_DIV2        0x000d0000
#define CLK_AXI_VENC_CLK_BACKUP_DIV4        0x000e0000
#define CLK_AXI_VENC_CLK_BACKUP_DIV8        0x000f0000

#define CLK_BPU_VENC_CLKPIXEL               0x00000000
#define CLK_BPU_VENC_CLK666M                0x00010000
#define CLK_BPU_VENC_CLK500M                0x00030000
#define CLK_BPU_VENC_CLK400M                0x00050000
#define CLK_BPU_VENC_CLK300M                0x00070000
#define CLK_BPU_VENC_CLK250M                0x00080000
#define CLK_BPU_VENC_CLK200M                0x00090000
#define CLK_BPU_VENC_CLK150M                0x000a0000
#define CLK_BPU_VENC_CLK75M                 0x000b0000
#define CLK_BPU_VENC_CLK_BACKUP             0x000c0000
#define CLK_BPU_VENC_CLK_BACKUP_DIV2        0x000d0000
#define CLK_BPU_VENC_CLK_BACKUP_DIV4        0x000e0000
#define CLK_BPU_VENC_CLK_BACKUP_DIV8        0x000f0000

#define CLK_PERIP_I2SM_CLKAUDIO             0x00000000
#define CLK_PERIP_I2SM_CLKAUDIO_DIV2        0x00010000
#define CLK_PERIP_I2SM_CLKAUDIO_DIV3        0x00020000
#define CLK_PERIP_I2SM_CLKAUDIO_DIV4        0x00030000

#define CLK_SELECT_SHIFT    16

#endif
