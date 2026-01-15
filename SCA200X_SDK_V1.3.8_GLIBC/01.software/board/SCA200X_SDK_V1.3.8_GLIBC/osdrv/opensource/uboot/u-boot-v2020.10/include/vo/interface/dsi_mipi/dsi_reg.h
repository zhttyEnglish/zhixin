/*****************************************************************************
Copyright: 2016-2021, SmartChip. Co., Ltd.
File name: dsi_reg.h
Description: define the dphy register map
Author: SmartChip Software Team
Version: v1.0
Date:2021-05-18
History:2021-05-18 : first release sdk
*****************************************************************************/
#ifndef _DSI_REG_H_
#define _DSI_REG_H_
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define DSI_HOST_REG_BASE     0x01120000

//This register contains the version of the DSI host controller.
#define DSI_HOST_REG_VERSION                0x00
//This register controls the power up of the controller.
#define DSI_HOST_REG_PWR_UP                 0x04
//This register configures the factor for internal dividers to divide lanebyteclk for timeout...
#define DSI_HOST_REG_CLKMGR_CFG             0x08
//This register configures the Virtual Channel ID for DPI traffic.
#define DSI_HOST_REG_DPI_VCID               0x0c
//This register configures DPI color coding.
#define DSI_HOST_REG_DPI_COLOR_CODING       0x10
//This register configures the polarity of DPI signals.
#define DSI_HOST_REG_DPI_CFG_POL            0x14
//This register configures the timing for low-power commands sent while in video mode.
#define DSI_HOST_REG_DPI_LP_CMD_TIM         0x18
//This register configures Virtual Channel ID for DBI traffic.
#define DSI_HOST_REG_DBI_VCID               0x1c
//This register configures the bit width of pixels for DBI.
#define DSI_HOST_REG_DBI_CFG                0x20
//This register configures whether DWC_mipi_dsi_host is to partition DBI traffic automatically.
#define DSI_HOST_REG_DBI_PARTITIONING_EN    0x24
//This register configures the command size and the size for automatic partitioning of DBI...
#define DSI_HOST_REG_DBI_CMDSIZE            0x28
//This register configures how EoTp, BTA, CRC and ECC are to be used, to meet peripherals...
#define DSI_HOST_REG_PCKHDL_CFG             0x2c
//This register configures the Virtual Channel ID of READ responses to store and return to Generic...
#define DSI_HOST_REG_GEN_VCID               0x30
//This register configures the mode of operation between Video or Command Mode. (Commands can still...
#define DSI_HOST_REG_MODE_CFG               0x34
//This register configures several aspects of Video mode operation, the transmission mode, switching...
#define DSI_HOST_REG_VID_MODE_CFG           0x38
//This register configures the video packet size.
#define DSI_HOST_REG_VID_PKT_SIZE           0x3c
//This register configures the number of chunks to use. The data in each chunk has the size provided...
#define DSI_HOST_REG_VID_NUM_CHUNKS         0x40
//This register configures the size of null packets.
#define DSI_HOST_REG_VID_NULL_SIZE          0x44
//This register configures the video HSA time.
#define DSI_HOST_REG_VID_HSA_TIME           0x48
//This register configures the video HBP time.Synopsys, Inc. 195 SolvNet
#define DSI_HOST_REG_VID_HBP_TIME           0x4c
//This register configures the overall time for each video line.
#define DSI_HOST_REG_VID_HLINE_TIME         0x50
//This register configures the VSA period.
#define DSI_HOST_REG_VID_VSA_LINES          0x54
//This register configures the VBP period.
#define DSI_HOST_REG_VID_VBP_LINES          0x58
//This register configures the VFP period.
#define DSI_HOST_REG_VID_VFP_LINES          0x5c
//This register configures the vertical resolution of video.
#define DSI_HOST_REG_VID_VACTIVE_LINES      0x60
//This register configures the size of eDPI packets.
#define DSI_HOST_REG_EDPI_CMD_SIZE          0x64
//This register configures several aspect of command mode operation, tearing effect, acknowledge for...
#define DSI_HOST_REG_CMD_MODE_CFG           0x68
//his register sets the header for new packets sent using the Generic interface.
#define DSI_HOST_REG_GEN_HDR                0x6c
//This register sets the payload for packets sent using the Generic interface and, when read returns...
#define DSI_HOST_REG_GEN_PLD_DATA           0x70
//This register configures contains information about the status of FIFOs related to DBI and Generic...
#define DSI_HOST_REG_CMD_PKT_STATUS         0x74
//This register configures counters that trigger timeout errors. These are used to warn the system...
#define DSI_HOST_REG_TO_CNT_CFG             0x78
//This register configures the Peripheral Response timeout after high-speed Read operations.
#define DSI_HOST_REG_HS_RD_TO_CNT           0x7c
//This register configures the Peripheral Response timeout after low-power Read operations.
#define DSI_HOST_REG_LP_RD_TO_CNT           0x80
//This register configures the Peripheral Response timeout after high-speed Write operations.
#define DSI_HOST_REG_HS_WR_TO_CNT           0x84
//This register configures the Peripheral Response timeout after low-power Write operations.
#define DSI_HOST_REG_LP_WR_TO_CNT           0x88
//This register configures the Peripheral Response timeout after Bus Turnaround completion.
#define DSI_HOST_REG_BTA_TO_CNT             0x8c
//This register stores 3D control information for VSS packets in video mode.
#define DSI_HOST_REG_SDF_3D                 0x90
//This register configures the possibility for using non continuous clock in the clock lane.
#define DSI_HOST_REG_LPCLK_CTRL             0x94
//This register sets the time that DWC_mipi_dsi_host assumes in calculations for the clock lane to...
#define DSI_HOST_REG_PHY_TMR_LPCLK_CFG      0x98
//This register sets the time that DWC_mipi_dsi_host assumes in calculations for the data lanes to...
#define DSI_HOST_REG_PHY_TMR_CFG            0x9c
//This register controls resets and the PLL of the D-PHY.
#define DSI_HOST_REG_PHY_RSTZ               0xa0
//This register configures the number of active lanes and the minimum time to remain in stop...
#define DSI_HOST_REG_PHY_IF_CFG             0xa4
//This register configures entering and leaving ULPS in the D?PHY.
#define DSI_HOST_REG_PHY_ULPS_CTRL          0xa8
//This register configures the pins that activate triggers in the D-PHY.
#define DSI_HOST_REG_PHY_TX_TRIGGERS        0xac
//This register contains information about the status of the D?PHY.
#define DSI_HOST_REG_PHY_STATUS             0xb0
//This register controls clock and clear pins of the D-PHY vendor specific interface.
#define DSI_HOST_REG_PHY_TST_CTRL0          0xb4
//This register controls data and enable pins of the D-PHY vendor specific interface.
#define DSI_HOST_REG_PHY_TST_CTRL1          0xb8
//This register contains the status of interrupt sources from acknowledge reports and the D-PHY.
#define DSI_HOST_REG_INT_ST0                0xbc
//This register contains the status of interrupt sources related to timeouts, ECC, CRC, packet size,...
#define DSI_HOST_REG_INT_ST1                0xc0
//This register configures masks for the sources of interrupts that affect the INT_ST0 register. Write...
#define DSI_HOST_REG_INT_MSK0               0xc4
//This register configures masks for the sources of interrupts that affect the INT_ST1 register.
#define DSI_HOST_REG_INT_MSK1               0xc8
//This register controls the skew calibration of D-PHY.
#define DSI_HOST_REG_PHY_CAL                0xcc
//This register forces that affect the INT_ST0 register.
#define DSI_HOST_REG_INT_FORCE0             0xd8
//This register forces interrupts that affect the INT_ST1 register.
#define DSI_HOST_REG_INT_FORCE1             0xdc
//This register configures automatic ULPS control.
#define DSI_HOST_REG_AUTO_ULPS_MODE         0xe0
//This register configures the delay (in lanebyteclk) to wait before entering ULPS.
#define DSI_HOST_REG_AUTO_ULPS_ENTRY_DELAY  0xe4
//This register configures the DPHY wakeup time (in pclk).
#define DSI_HOST_REG_AUTO_ULPS_WAKEUP_TIME  0xe8
//This register configures Display Stream Compression
#define DSI_HOST_REG_DSC_PARAMETER          0xf0
//This register configures times related to PHY to perform some operations in lane byte clock...
#define DSI_HOST_REG_PHY_TMR_RD_CFG         0xf4
//This register configures the minimum time required by PHY between ulpsactivenot and ulpsexitreq...
#define DSI_HOST_REG_AUTO_ULPS_MIN_TIME     0xf8
//Select the PHY interface
#define DSI_HOST_REG_PHY_MODE               0xfc
//This register controls dpi shadow feature
#define DSI_HOST_REG_VID_SHADOW_CTRL        0x100
//This register holds the value that controller is using for DPI_VCID.
#define DSI_HOST_REG_DPI_VCID_ACT           0x10c
//This register holds the value that controller is using for DPI_COLOR_CODING.
#define DSI_HOST_REG_DPI_COLOR_CODING_ACT   0x110
//This register holds the value that controller is using for DPI_LP_CMD_TIM.
#define DSI_HOST_REG_DPI_LP_CMD_TIM_ACT     0x118
//This register configures the tearing effect by Hardware operations.
#define DSI_HOST_REG_EDPI_TE_HW_CFG         0x11c
//This register holds the value that controller is using for VID_MODE_CFG.
#define DSI_HOST_REG_VID_MODE_CFG_ACT       0x138
//This register holds the value that controller is using for VID_PKT_SIZE.
#define DSI_HOST_REG_VID_PKT_SIZE_ACT       0x13c
//This register holds the value that controller is using for VID_NUM_CHUNKS.
#define DSI_HOST_REG_VID_NUM_CHUNKS_ACT     0x140
//This register holds the value that controller is using for VID_NULL_SIZE.
#define DSI_HOST_REG_VID_NULL_SIZE_ACT      0x144
//This register holds the value that controller is using for VID_HSA_TIME.
#define DSI_HOST_REG_VID_HSA_TIME_ACT       0x148
//This register holds the value that controller is using for VID_HBP_TIME.
#define DSI_HOST_REG_VID_HBP_TIME_ACT       0x14c
//This register holds the value that controller is using for VID_HLINE_TIME.
#define DSI_HOST_REG_VID_HLINE_TIME_ACT     0x150
//This register holds the value that controller is using for VID_VSA_LINES.
#define DSI_HOST_REG_VID_VSA_LINES_ACT      0x154
//This register holds the value that controller is using for VID_VBP_LINES.
#define DSI_HOST_REG_VID_VBP_LINES_ACT      0x158
//This register holds the value that controller is using for VID_VFP_LINES.
#define DSI_HOST_REG_VID_VFP_LINES_ACT      0x15c
//This register holds the value that controller is using for VID_VACTIVE_LINES.
#define DSI_HOST_REG_VID_VACTIVE_LINES_ACT  0x160
//This register configures contains information about the status of FIFOs related to DPI and eDPI...
#define DSI_HOST_REG_VID_PKT_STATUS         0x168
//This register holds the value that controller is using for SDF_3D.
#define DSI_HOST_REG_SDF_3D_ACT             0x190
//DSC encoder COREID.
#define DSI_HOST_REG_DSC_ENC_COREID         0x200
//Vesa DSC Version.
#define DSI_HOST_REG_DSC_ENC_VERSION        0x204
//DSC encoder flatness determination.
#define DSI_HOST_REG_DSC_ENC_FLATNESS_DET_THRES 0x208
//This register is use to delay DPI events in order to be compliant with DSC encoder latency.
#define DSI_HOST_REG_DSC_ENC_DELAY          0x20c
//Compressed line size in units of 1/16th of a bit.
#define DSI_HOST_REG_DSC_ENC_COMPRESSED_LINE_SIZE 0x210
//Calculation of the number of lines in excess.
#define DSI_HOST_REG_DSC_ENC_LINES_IN_EXCESS 0x214
//Adjustment needed to calculate end of last line of a slice.
#define DSI_HOST_REG_DSC_ENC_RBUF_ADDR_LAST_LINE_A  0x218
//This register is to enable DSC encoder.
#define DSI_HOST_REG_DSC_MODE               0x21c
//This register contains the status of interrupt sources from DSC encoder.
#define DSI_HOST_REG_DSC_ENC_INT_ST         0x220
//This register configures masks for the sources of interrupts that affect the INT_ST_DSC register....
#define DSI_HOST_REG_DSC_ENC_INT_MSK        0x224
//This register forces interrupts that affect the INT_ST_DSC register.
#define DSI_HOST_REG_DSC_ENC_INT_FORCE      0x228
//Selection of the DSC FIFO whose word count is reported in DSC_FIFO_STATUS register.
#define DSI_HOST_REG_DSC_FIFO_STATUS_SELECT 0x22c
//Status of FIFOs related to DSC encoder.
#define DSI_HOST_REG_DSC_FIFO_STATUS        0x230
//Status of FIFOs related to the second DSC encoder.
#define DSI_HOST_REG_DSC_FIFO_STATUS2       0x234
//Word count of FIFOs related to the DSC encoder.
#define DSI_HOST_REG_DSC_FIFO_WORD_COUNT    0x238
//Word count of FIFOs related to the second DSC encoder.
#define DSI_HOST_REG_DSC_FIFO_WORD_COUNT2   0x23c
//This register holds the bytes in the range [0,3] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_0_3        0x260
//This register holds the bytes in the range [4,7] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_4_7        0x264
//This register holds the bytes in the range [8,11] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_8_11       0x268
//This register holds the bytes in the range [12,15] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_12_15      0x26c
//This register holds the bytes in the range [16,19] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_16_19      0x270
//This register holds the bytes in the range [20,23] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_20_23      0x274
//This register holds the bytes in the range [24,27] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_24_27      0x278
//This register holds the bytes in the range [28,31] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_28_31      0x27c
//This register holds the bytes in the range [32,35] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_32_35      0x280
//This register holds the bytes in the range [36,39] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_36_39      0x284
//This register holds the bytes in the range [40,43] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_40_43      0x288
//This register holds the bytes in the range [44,47] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_44_47      0x28c
//This register holds the bytes in the range [48,51] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_48_51      0x290
//This register holds the bytes in the range [52,55] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_52_55      0x294
//This register holds the bytes in the range [56,59] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_56_59      0x298
//This register holds the bytes in the range [60,63] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_60_63      0x29c
//This register holds the bytes in the range [64,67] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_64_67      0x2a0
//This register holds the bytes in the range [68,71] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_68_71      0x2a4
//This register holds the bytes in the range [72,75] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_72_75      0x2a8
//This register holds the bytes in the range [76,79] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_76_79      0x2ac
//This register holds the bytes in the range [80,83] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_80_83      0x2b0
//This register holds the bytes in the range [84,87] of the Picture Parameter Set (PPS).
#define DSI_HOST_REG_DSC_ENC_PPS_84_87      0x2b4

#endif

