/*****************************************************************************
Copyright: 2016-2021, SmartChip. Co., Ltd.
File name: dsi_mipi_common.h
Description: define the common struct
Author: SmartChip Software Team
Version: v1.0
Date:2021-10-26
History:2021-10-26 : first release sdk
*****************************************************************************/
#ifndef _DSI_MIPI_COMMON_H_
#define _DSI_MIPI_COMMON_H_

#define   MAX_LANE_NUM           4

enum {
	MIPI_DATA_TYPE_GENERIC_SHORT_WRITE_0_PARA     = 0x03,
	MIPI_DATA_TYPE_GENERIC_SHORT_WRITE_1_PARA     = 0x13,
	MIPI_DATA_TYPE_GENERIC_SHORT_WRITE_2_PARA     = 0x23,

	MIPI_DATA_TYPE_GENERIC_SHORT_READ_0_PARA      = 0x04,
	MIPI_DATA_TYPE_GENERIC_SHORT_READ_1_PARA      = 0x14,
	MIPI_DATA_TYPE_GENERIC_SHORT_READ_2_PARA      = 0x24,

	MIPI_DATA_TYPE_DCS_SHORT_WRITE_0_PARA         = 0x05,
	MIPI_DATA_TYPE_DCS_SHORT_WRITE_1_PARA         = 0x15,

	MIPI_DATA_TYPE_DCS_SHORT_READ_0_PARA          = 0x06,

	MIPI_DATA_TYPE_SET_MAX_READ_SIZE              = 0x37,

	MIPI_DATA_TYPE_NULL_PACKET                    = 0x09,
	MIPI_DATA_TYPE_BLANK_PACKET                   = 0x19,
	MIPI_DATA_TYPE_GENERIC_LONG_WRITE             = 0x29,
	MIPI_DATA_TYPE_DCS_LONG_WRITE                 = 0x39,

	MIPI_DATA_TYPE_PACKED_PIXEL_RGB565            = 0x0E,
	MIPI_DATA_TYPE_PACKED_PIXEL_RGB666            = 0x1E,
	MIPI_DATA_TYPE_LOOSELY_PACKED_PIXEL_RGB666    = 0x2E,
	MIPI_DATA_TYPE_PACKED_PIXEL_RGB888            = 0x3E
};

typedef struct {
	unsigned int  hsa;
	unsigned int  hbp;
	unsigned int  hact;
	unsigned int  hfp;

	unsigned int  vsa;
	unsigned int  vbp;
	unsigned int  vact;
	unsigned int  vfp;
} STRU_DSI_SYNC_INFO;

typedef struct {
	unsigned int         dev_no;
	unsigned int         lane[MAX_LANE_NUM];
	int                  bits_per_pixel;
	STRU_DSI_SYNC_INFO   sync_info;
	float                dphy_clk_mhz;
	unsigned int         pll_freq_reg_2c0;
	unsigned int         pll_freq_reg_38c;
} STRU_DSI_CFG;

typedef struct {
	unsigned int clk_init;
	unsigned int clk_wakeup;
	unsigned int clk_lane_bypass;
	unsigned int clk_lpx;
	unsigned int clk_prepare;
	unsigned int clk_zero;
	unsigned int clk_trail;
	unsigned int clk_exit;
	unsigned int clk_pre;
	unsigned int clk_post;

	unsigned int clk_lane_init;
	unsigned int clk_rst2enlptx;
	unsigned int hs_lane_bypass;
	unsigned int hs_lpx;
	unsigned int hs_prepare;
	unsigned int hs_zero;
	unsigned int hs_trail;
	unsigned int hs_exit;

	unsigned int pll_freq_reg_2c0;
	unsigned int pll_freq_reg_38c;

	unsigned int lp_clk_div_factor;
	unsigned int initial_skew_ui;
	unsigned int periodic_skew_ui;
} STRU_DSI_FREQ_INFO;

int dphy_freq_conf_get(int freqMhz, STRU_DSI_FREQ_INFO *info);
int dsi_set_timing(STRU_DSI_CFG *cfg);
int dsi_init(void);
int dsi_exit_cmd_mode(void);

void dsi_reg_write(unsigned int addr, unsigned int data);
int dsi_read(uint8_t data_type, uint8_t addr, uint8_t *data, uint32_t len);
int dsi_long_cmd(uint8_t data_type, uint8_t *para, uint16_t len);
int dsi_short_cmd_without_pra(uint8_t data_type);
int dsi_short_cmd_1pra(uint8_t data_type, uint8_t para);
int dsi_short_cmd_2pra(uint8_t data_type, uint8_t para1, uint8_t para2);

#endif

