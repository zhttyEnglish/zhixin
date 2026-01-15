/*****************************************************************************
Copyright: 2016-2021, SmartChip. Co., Ltd.
File name: dphy_reg.h
Description: define the dphy register map
Author: SmartChip Software Team
Version: v1.0
Date:2021-05-18
History:2021-05-18 : first release sdk
*****************************************************************************/
#ifndef _DPHY_REG_H_
#define _DPHY_REG_H_

//PLL_SYNTH_DIG
#define DPHY_REG_PLL_DIG_PARA0              0x2c0
#define DPHY_REG_PLL_DIG_PARA1              0x2c4
#define DPHY_REG_PLL_DIG_PARA2              0x2c8

//PCS0
#define DPHY_REG_PCS0_CLK_LANE_PARA0             0x2cc
#define DPHY_REG_PCS0_CLK_LANE_PARA1             0x2d0
#define DPHY_REG_PCS0_CLK_LANE_PARA2             0x2d4
#define DPHY_REG_PCS0_CLK_LANE_PARA3             0x2d8
#define DPHY_REG_PCS0_CLK_LANE_PARA4             0x2dc

#define DPHY_REG_PCS0_DT_LANE0_PARA0             0x2e0
#define DPHY_REG_PCS0_DT_LANE0_PARA1             0x2e4
#define DPHY_REG_PCS0_DT_LANE0_PARA2             0x2e8
#define DPHY_REG_PCS0_DT_LANE0_PARA3             0x2ec
#define DPHY_REG_PCS0_DT_LANE0_PARA4             0x2f0
#define DPHY_REG_PCS0_DT_LANE0_PARA5             0x2f4
#define DPHY_REG_PCS0_DT_LANE0_PARA6             0x2f8
#define DPHY_REG_PCS0_DT_LANE0_PARA7             0x2fc

#define DPHY_REG_PCS0_DT_LANE1_PARA0             0x300
#define DPHY_REG_PCS0_DT_LANE1_PARA1             0x304
#define DPHY_REG_PCS0_DT_LANE1_PARA2             0x308
#define DPHY_REG_PCS0_DT_LANE1_PARA3             0x30c
#define DPHY_REG_PCS0_DT_LANE1_PARA4             0x310
#define DPHY_REG_PCS0_DT_LANE1_PARA5             0x314

#define DPHY_REG_PCS0_FSFREQRANGE_PARA           0x318

#define DPHY_REG_PCS0_HC_CTRL_PARA0              0x31c
#define DPHY_REG_PCS0_HC_CTRL_PARA1              0x320
#define DPHY_REG_PCS0_HC_CTRL_PARA2              0x324
#define DPHY_REG_PCS0_HC_CTRL_PARA3              0x328
//PCS1
#define DPHY_REG_PCS1_CLK_LANE_PARA0             0x32c
#define DPHY_REG_PCS1_CLK_LANE_PARA1             0x330
#define DPHY_REG_PCS1_CLK_LANE_PARA2             0x334
#define DPHY_REG_PCS1_CLK_LANE_PARA3             0x338
#define DPHY_REG_PCS1_CLK_LANE_PARA4             0x33c

#define DPHY_REG_PCS1_DT_LANE0_PARA0             0x340
#define DPHY_REG_PCS1_DT_LANE0_PARA1             0x344
#define DPHY_REG_PCS1_DT_LANE0_PARA2             0x348
#define DPHY_REG_PCS1_DT_LANE0_PARA3             0x34c
#define DPHY_REG_PCS1_DT_LANE0_PARA4             0x350
#define DPHY_REG_PCS1_DT_LANE0_PARA5             0x354
#define DPHY_REG_PCS1_DT_LANE0_PARA6             0x358
#define DPHY_REG_PCS1_DT_LANE0_PARA7             0x35c

#define DPHY_REG_PCS1_DT_LANE1_PARA0             0x360
#define DPHY_REG_PCS1_DT_LANE1_PARA1             0x364
#define DPHY_REG_PCS1_DT_LANE1_PARA2             0x368
#define DPHY_REG_PCS1_DT_LANE1_PARA3             0x36c
#define DPHY_REG_PCS1_DT_LANE1_PARA4             0x370
#define DPHY_REG_PCS1_DT_LANE1_PARA5             0x374

#define DPHY_REG_PCS1_FSFREQRANGE_PARA           0x378

#define DPHY_REG_PCS1_HC_CTRL_PARA0              0x37c
#define DPHY_REG_PCS1_HC_CTRL_PARA1              0x380
#define DPHY_REG_PCS1_HC_CTRL_PARA2              0x384
#define DPHY_REG_PCS1_HC_CTRL_PARA3              0x388

//mipi_pll
#define DPHY_REG_MIPI_PLL                        0x38c

//mipi_tx_top
#define DPHY_REG_MIPI_ANALOG_TX0_0               0x390    // phy_reg52
#define DPHY_REG_MIPI_ANALOG_TX0_1               0x394    // phy_reg53
#define DPHY_REG_MIPI_ANALOG_TX0_2               0x398    // phy_reg54

#define DPHY_REG_MIPI_ANALOG_TX1_0               0x39c    // phy_reg55
#define DPHY_REG_MIPI_ANALOG_TX1_1               0x3a0    // phy_reg56
#define DPHY_REG_MIPI_ANALOG_TX1_2               0x3a4    // phy_reg57

//pixel_dt_gen
#define DPHY_REG_PIXEL_DT_GEN_VTOTAL             0x3a8
#define DPHY_REG_PIXEL_DT_GEN_VSA                0x3ac
#define DPHY_REG_PIXEL_DT_GEN_VBP                0x3b0
#define DPHY_REG_PIXEL_DT_GEN_VACT               0x3b4
#define DPHY_REG_PIXEL_DT_GEN_HTOTAL             0x3b8
#define DPHY_REG_PIXEL_DT_GEN_HSA                0x3bc
#define DPHY_REG_PIXEL_DT_GEN_HBP                0x3c0
#define DPHY_REG_PIXEL_DT_GEN_HACT               0x3c4
#define DPHY_REG_PIXEL_DT_GEN_FORM               0x3c8
#define DPHY_REG_PIXEL_DT_GEN_EN                 0x3cc
#define DPHY_REG_PIXEL_DT_GEN_INI_TIME           0x3d0
#define DPHY_REG_PIXEL_DT_GEN_SEL                0x3d4

#endif

