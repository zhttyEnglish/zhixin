#include "vo/interface/display_interface.h"
#include "vo/interface/dsi_mipi/dsi_reg.h"
#include "vo/interface/dsi_mipi/dphy_reg.h"
#include "vo/interface/dsi_mipi/dsi_mipi_common.h"

static STRU_DSI_CFG g_dsi_cfg_default = {
	.dev_no         = 0,
	.lane           = {1, 1, 1, 1},
	.bits_per_pixel = 24,
	.sync_info      = {
		.hsa  = 6,
		.hbp  = 120,
		.hact = 1024,
		.hfp  = 120,

		.vsa  = 10,
		.vbp  = 23,
		.vact = 600,
		.vfp  = 12
	},
	.dphy_clk_mhz     = 320.0,
	.pll_freq_reg_2c0 = 0x32000000,
	.pll_freq_reg_38c = 0x2800
};

void dsi_reg_write(unsigned int addr, unsigned int data)
{
	write_reg32(DSI_HOST_REG_BASE + addr, data);
}

unsigned int dsi_reg_read(unsigned int addr)
{
	return read_reg32(DSI_HOST_REG_BASE + addr);
}

int dsi_set_timing(STRU_DSI_CFG *cfg)
{
	STRU_DSI_FREQ_INFO  Info = {0};
	int                 freq = (((int)(cfg->dphy_clk_mhz)) / 10) * 10;
	int                 ret = 0;
	unsigned int        dpi_bytes_per_pixel = 0;
	unsigned int        vid_num_lanes       = 0;

	unsigned int        vid_pkt_size        = 0;
	unsigned int        vid_num_chunks      = 0;
	unsigned int        vid_null_size       = 0;
	unsigned int        vid_hsa_time        = 0;
	unsigned int        vid_hbp_time        = 0;
	unsigned int        vid_hfp_time        = 0;

	unsigned int        vid_hact_time       = 0;
	unsigned int        vid_hline_time      = 0;
	unsigned int        vid_vsa_lines       = 0;
	unsigned int        vid_vbp_lines       = 0;
	unsigned int        vid_vfp_lines       = 0;
	unsigned int        vid_vactive_lines   = 0;
	unsigned int        reg_data            = 0;

	unsigned int        ta_sure             = 6;
	unsigned int        ta_get              = 20;
	unsigned int        ta_go               = 16;

	vid_num_lanes = 0;
	for (int i = 0; i < MAX_LANE_NUM; i++)
		if (cfg->lane[i] != 0)
			vid_num_lanes++;

	dpi_bytes_per_pixel = cfg->bits_per_pixel / 8;

	vid_num_chunks = 0;
	vid_null_size  = 0x0;
	vid_pkt_size   = cfg->sync_info.hact;
	vid_hsa_time   = cfg->sync_info.hsa;
	vid_hbp_time   = cfg->sync_info.hbp;
	vid_hfp_time   = cfg->sync_info.hfp;
	vid_hact_time  = vid_pkt_size * dpi_bytes_per_pixel / vid_num_lanes;
	vid_hline_time = vid_hsa_time + vid_hbp_time + vid_hact_time + vid_hfp_time;

	vid_vsa_lines  = cfg->sync_info.vsa;
	vid_vbp_lines  = cfg->sync_info.vbp;
	vid_vfp_lines  = cfg->sync_info.vfp;
	vid_vactive_lines = cfg->sync_info.vact;

	sc_always("dpi_bytes_per_pixel=%d, vid_num_lanes=%d, vid_null_size=%d, vid_pkt_size=%d, dphy_clk_mhz=%f, freq=%d",
	    dpi_bytes_per_pixel, vid_num_lanes, vid_null_size, vid_pkt_size, cfg->dphy_clk_mhz, freq);
	sc_always("vid_hsa_time=%d, vid_hbp_time=%d, vid_hfp_time=%d, vid_hact_time=%d, vid_hline_time=%d",
	    vid_hsa_time, vid_hbp_time, vid_hfp_time, vid_hact_time, vid_hline_time);
	sc_always("vid_vsa_lines=%d, vid_vbp_lines=%d, vid_vfp_lines=%d, vid_vactive_lines=%d",
	    vid_vsa_lines, vid_vbp_lines, vid_vfp_lines, vid_vactive_lines);

	if ((ret = dphy_freq_conf_get(freq, &Info)) != 0) {
		sc_err("unsupport dphy freq(%f, %d) in conf table!", cfg->dphy_clk_mhz, freq);
		return -1;
	}

	sc_always("set dsi reg");
	dsi_reg_write(DSI_HOST_REG_PWR_UP, 0x0);               // reset
	dsi_reg_write(DSI_HOST_REG_DPI_VCID, 0x0);             // [1:0] virtual channel id 0

	dsi_reg_write(DSI_HOST_REG_DPI_COLOR_CODING, 0x5);     // [3:0]color coding;    24-bit
	// [8]  loosely18_en     disable

	dsi_reg_write(DSI_HOST_REG_PCKHDL_CFG, 0x5);           //PCKHDL_CFG,[5:0]>{etop_tx_lp_en,crc_rx_en,
	//ecc_rx_en,bta_en,eotp_rx_en,eotp_tx_en}

	dsi_reg_write(DSI_HOST_REG_VID_PKT_SIZE,
	    vid_pkt_size);        //VID_PKT_SIZE,the number of pixels in a signle video packet

	dsi_reg_write(DSI_HOST_REG_VID_NUM_CHUNKS, vid_num_chunks);    //VID_NUM_CHUNKS
	//the number of chunks to use. The data in each chunk has
	//the size provided by VID_PKT_SIZE

	dsi_reg_write(DSI_HOST_REG_VID_NULL_SIZE, vid_null_size);      //VID_NULL_SIZE
	//configure the number of bytes inside a null packet
	//Setting to 0 disables null packets
	dsi_reg_write(DSI_HOST_REG_VID_HSA_TIME,   vid_hsa_time);      //VID_HSA_TIME

	dsi_reg_write(DSI_HOST_REG_VID_HBP_TIME,   vid_hbp_time);//VID_HBP_TIME

	dsi_reg_write(DSI_HOST_REG_VID_HLINE_TIME,   vid_hline_time);//VID_HLINE_TIME

	dsi_reg_write(DSI_HOST_REG_VID_VSA_LINES,   vid_vsa_lines);//VID_VSA_LINES
	//measured in number of horizontal lines

	dsi_reg_write(DSI_HOST_REG_VID_VBP_LINES,   vid_vbp_lines);//VID_VBP_LINES

	dsi_reg_write(DSI_HOST_REG_VID_VFP_LINES,   vid_vfp_lines);//VID_VFP_LINES

	dsi_reg_write(DSI_HOST_REG_VID_VACTIVE_LINES,   vid_vactive_lines);//VID_VACTIVE_LINES
	//the vertical active period measured in number of
	//horizontal lines

	dsi_reg_write(DSI_HOST_REG_PHY_TMR_CFG,   0x320068);//PHY_TMR_CFG
	dsi_reg_write(DSI_HOST_REG_PHY_TMR_LPCLK_CFG,   0x2e0080);    //PHY_TMR_LPCLK_CFG
	//[9:0]phy_clklp2hs_time, configures the maximum time that the
	//D-phy clock lane taske to go from low-power to hihg-speed
	//transimission measured in lane byte clock cycles
	//[25:16]phy_clklp2lp_time,configures the maximum time that the
	//D-PHY clock lane takes to go from high-speed to low-power
	//transimission measured in lane byte clokc cycles
	dsi_reg_write(DSI_HOST_REG_PHY_IF_CFG,   0x2800 | (vid_num_lanes - 1));           //PHY_IF_CFG
	//[1:0]n_lanes,0:lane 0
	//[15:8]phy_stop_wait_time, configures the minimus time PHY needs
	//to stay in StopState before requsting an high-speed transmission

	dsi_reg_write(DSI_HOST_REG_CLKMGR_CFG,       Info.lp_clk_div_factor);              //
	dsi_reg_write(DSI_HOST_REG_DPI_LP_CMD_TIM,   0x800080);              //
	dsi_reg_write(DSI_HOST_REG_VID_MODE_CFG,     0xbf00);              //
	dsi_reg_write(DSI_HOST_REG_CMD_MODE_CFG,     0x10F7F00);              //

	dsi_reg_write(DSI_HOST_REG_LPCLK_CTRL,     0x1);

	//ssc_reg,
	//ssc_en,reg_dither,ref_cyc_sel,por_sel,por_manual,lock_det_en,en_dither,en_3rd_sdm
	dsi_reg_write(DPHY_REG_PLL_DIG_PARA1,   0x410b016);
	dsi_reg_write(DPHY_REG_PLL_DIG_PARA2,   0x2468ac);
	//pcs0
	dsi_reg_write(DPHY_REG_PCS0_CLK_LANE_PARA0,   Info.clk_rst2enlptx); //cl_lane_para0
	dsi_reg_write(DPHY_REG_PCS0_CLK_LANE_PARA1,   Info.clk_init);       //cl_lane_para1
	dsi_reg_write(DPHY_REG_PCS0_CLK_LANE_PARA2,   Info.clk_wakeup);     //cl_lane_para2

	//clk_lane_para3
	reg_data = Info.clk_lane_bypass | (Info.clk_lpx << 8) | (Info.clk_prepare << 16) | (Info.clk_zero << 24);
	dsi_reg_write(DPHY_REG_PCS0_CLK_LANE_PARA3,   reg_data);
	reg_data = Info.clk_pre | (Info.clk_post << 8) | (Info.clk_trail << 16) | (Info.clk_exit << 24);
	dsi_reg_write(DPHY_REG_PCS0_CLK_LANE_PARA4,   reg_data);//cl_lane_para4
	dsi_reg_write(DPHY_REG_PCS0_DT_LANE0_PARA0,   0x193e8);//dt_lane0_para0
	dsi_reg_write(DPHY_REG_PCS0_DT_LANE0_PARA1,   Info.clk_lane_init);//dt_lane0_para1,initial time
	dsi_reg_write(DPHY_REG_PCS0_DT_LANE0_PARA2,   Info.initial_skew_ui);//dt_lane0_para2,dekew time
	dsi_reg_write(DPHY_REG_PCS0_DT_LANE0_PARA3,   0xc4);//dt_lane0_para3
	dsi_reg_write(DPHY_REG_PCS0_DT_LANE0_PARA4,   0xc803e8);//dt_lane0_para4

	reg_data = Info.hs_lane_bypass | (Info.hs_lpx << 8) | (Info.hs_prepare << 16) | (Info.hs_zero << 24);
	dsi_reg_write(DPHY_REG_PCS0_DT_LANE0_PARA5,   reg_data);//dt_lane0_para5

	reg_data = Info.hs_trail | (Info.hs_exit << 8);
	dsi_reg_write(DPHY_REG_PCS0_DT_LANE0_PARA6,   reg_data);//dt_lane0_para6
	dsi_reg_write(DPHY_REG_PCS0_DT_LANE0_PARA7,   (ta_go << 0) | (ta_sure << 8) | (ta_get << 16));//dt_lane0_para7
	dsi_reg_write(DPHY_REG_PCS0_DT_LANE1_PARA0,   0x193e8);//dt_lane1_para0
	dsi_reg_write(DPHY_REG_PCS0_DT_LANE1_PARA1,   Info.clk_lane_init);//dt_lane1_para1
	dsi_reg_write(DPHY_REG_PCS0_DT_LANE1_PARA2,   Info.initial_skew_ui);//dt_lane1_para2
	dsi_reg_write(DPHY_REG_PCS0_DT_LANE1_PARA3,   0xc4);//dt_lane1_para3

	reg_data = Info.hs_lane_bypass | (Info.hs_lpx << 8) | (Info.hs_prepare << 16) | (Info.hs_zero << 24);
	dsi_reg_write(DPHY_REG_PCS0_DT_LANE1_PARA4,   reg_data);//dt_lane1_para4
	reg_data = Info.hs_trail | (Info.hs_exit << 8);
	dsi_reg_write(DPHY_REG_PCS0_DT_LANE1_PARA5,   reg_data);//dt_lane1_para5
	dsi_reg_write(DPHY_REG_PCS0_FSFREQRANGE_PARA, 0x8);//freqrange
	//hc ctrl
	//[7:0],software enable
	//[8],enhstx_c
	//[9],hstxset0_c
	//[10],enlptx_c
	//[12:11],lptxdin_c
	dsi_reg_write(DPHY_REG_PCS0_HC_CTRL_PARA1,   0x0);//{hszero,hsprepare,lpx,bypass}
	dsi_reg_write(DPHY_REG_PCS0_HC_CTRL_PARA2,   0x0);//{hsxexit,hs_trail}
	dsi_reg_write(DPHY_REG_PCS0_HC_CTRL_PARA3,   0x0);//hs freq range
	//pcs1
	//{enulpstx_c,lptxdin_c,enhstx_c,hstxset0_c,enlptx_c,clkctrl_sel}
	dsi_reg_write(DPHY_REG_PCS1_CLK_LANE_PARA0,   Info.clk_rst2enlptx);//cl_lane_para0

	dsi_reg_write(DPHY_REG_PCS1_CLK_LANE_PARA1,   Info.clk_init);//cl_lane_para1
	dsi_reg_write(DPHY_REG_PCS1_CLK_LANE_PARA2,   Info.clk_wakeup);//cl_lane_para2

	reg_data = Info.clk_lane_bypass | (Info.clk_lpx << 8) | (Info.clk_prepare << 16) | (Info.clk_zero << 24);
	dsi_reg_write(DPHY_REG_PCS1_CLK_LANE_PARA3,   reg_data);//cl_lane_para3
	reg_data = Info.clk_pre | (Info.clk_post << 8) | (Info.clk_trail << 16) | (Info.clk_exit << 24);
	dsi_reg_write(DPHY_REG_PCS1_CLK_LANE_PARA4,   reg_data);//cl_lane_para4
	dsi_reg_write(DPHY_REG_PCS1_DT_LANE0_PARA0,   0x193e8);//dt_lane0_para0
	dsi_reg_write(DPHY_REG_PCS1_DT_LANE0_PARA1,   Info.clk_lane_init);//dt_lane0_para1
	dsi_reg_write(DPHY_REG_PCS1_DT_LANE0_PARA2,   Info.initial_skew_ui);//dt_lane0_para2
	dsi_reg_write(DPHY_REG_PCS1_DT_LANE0_PARA3,   0xc4);//dt_lane0_para3
	dsi_reg_write(DPHY_REG_PCS1_DT_LANE0_PARA4,   0xc803e8);//dt_lane0_para4

	reg_data = Info.hs_lane_bypass | (Info.hs_lpx << 8) | (Info.hs_prepare << 16) | (Info.hs_zero << 24);
	dsi_reg_write(DPHY_REG_PCS1_DT_LANE0_PARA5,   reg_data);//dt_lane0_para5
	reg_data = Info.hs_trail | (Info.hs_exit << 8);
	dsi_reg_write(DPHY_REG_PCS1_DT_LANE0_PARA6,   reg_data);//dt_lane0_para6,{hsexit,trail}
	dsi_reg_write(DPHY_REG_PCS1_DT_LANE0_PARA7,   0x641950);//dt_lane0_para7
	dsi_reg_write(DPHY_REG_PCS1_DT_LANE1_PARA0,   0x193e8);//dt_lane1_para0
	dsi_reg_write(DPHY_REG_PCS1_DT_LANE1_PARA1,   Info.clk_lane_init);//dt_lane1_para1
	dsi_reg_write(DPHY_REG_PCS1_DT_LANE1_PARA2,   Info.initial_skew_ui);//dt_lane1_para2
	dsi_reg_write(DPHY_REG_PCS1_DT_LANE1_PARA3,   0xc4);//dt_lane1_para3

	reg_data = Info.hs_lane_bypass | (Info.hs_lpx << 8) | (Info.hs_prepare << 16) | (Info.hs_zero << 24);
	dsi_reg_write(DPHY_REG_PCS1_DT_LANE1_PARA4,   reg_data);//dt_lane1_para4
	reg_data = Info.hs_trail | (Info.hs_exit << 8);
	dsi_reg_write(DPHY_REG_PCS1_DT_LANE1_PARA5,   reg_data);//dt_lane1_para5
	dsi_reg_write(DPHY_REG_PCS1_FSFREQRANGE_PARA,   0x8);//freqrange
	//hc sw ctrl
	dsi_reg_write(DPHY_REG_PCS1_HC_CTRL_PARA0,   0x0);
	dsi_reg_write(DPHY_REG_PCS1_HC_CTRL_PARA1,   0x0);//{hszero,hsprepare,lpx,bypass}
	dsi_reg_write(DPHY_REG_PCS1_HC_CTRL_PARA2,   0x0);
	dsi_reg_write(DPHY_REG_PCS1_HC_CTRL_PARA3,   0);
	//analog pll

	dsi_reg_write(DPHY_REG_PLL_DIG_PARA0,   cfg->pll_freq_reg_2c0);
	dsi_reg_write(DPHY_REG_MIPI_PLL,        cfg->pll_freq_reg_38c);

	//
	dsi_reg_write(DPHY_REG_MIPI_ANALOG_TX0_0,   0x0);
	dsi_reg_write(DPHY_REG_MIPI_ANALOG_TX0_1,   0x80000000);
	dsi_reg_write(DPHY_REG_MIPI_ANALOG_TX0_1,   0x0);
	dsi_reg_write(DPHY_REG_MIPI_ANALOG_TX0_2,   0x8000);
	//
	dsi_reg_write(DPHY_REG_MIPI_ANALOG_TX1_0,   0x0);
	dsi_reg_write(DPHY_REG_MIPI_ANALOG_TX1_1,   0x80000000);
	dsi_reg_write(DPHY_REG_MIPI_ANALOG_TX1_1,   0x0);
	dsi_reg_write(DPHY_REG_MIPI_ANALOG_TX1_2,   0x8000);

	dsi_reg_write(DSI_HOST_REG_PHY_CAL,   0);//start deskew calculate
	dsi_reg_write(DSI_HOST_REG_PWR_UP,  0x1);
	dsi_reg_write(DPHY_REG_PIXEL_DT_GEN_SEL,  0);

	dsi_reg_write(DSI_HOST_REG_PHY_RSTZ,  0xd);
	dsi_reg_write(DSI_HOST_REG_PHY_RSTZ,  0xf);

	dsi_reg_write(DSI_HOST_REG_BTA_TO_CNT,  0x10);

#if 0
	dsi_reg_write(0x34,   0x1);//MODE_CFG
	//#150us;
	sc_delay_us(150);

	uint8_t   test_data[100];
	dsi_read(MIPI_DATA_TYPE_GENERIC_SHORT_READ_1_PARA, 0x04, test_data, 0x10);
#endif

	sc_trace_line();
	return 0;
}

int dsi_init(void)
{
	return dsi_set_timing(&g_dsi_cfg_default);
}

int dsi_exit_cmd_mode(void)
{
	sc_trace_line();
	dsi_reg_write(DSI_HOST_REG_MODE_CFG,  0x0);
	sc_delay(1);
	return 0;
}

/* note this command should execute in video mode */
int dsi_short_cmd_without_pra(uint8_t data_type)
{
	dsi_reg_write(DSI_HOST_REG_GEN_HDR,   data_type);
	return 0 ;
}

/* note this command should execute in video mode */
int dsi_short_cmd_1pra(uint8_t data_type, uint8_t para)
{
	dsi_reg_write(DSI_HOST_REG_GEN_HDR,    (((uint32_t)para) << 8) |  data_type );
	return 0;
}

/* note this command should execute in video mode */
int dsi_short_cmd_2pra(uint8_t data_type, uint8_t para1, uint8_t para2)
{
	dsi_reg_write(DSI_HOST_REG_GEN_HDR,    (((uint32_t)para2) << 16) | (((uint32_t)para1) << 8) |  data_type );
	return 0;
}

int read_panel_id()
{
	uint8_t test_data[4] = {0};
	if(dsi_read(0x14, 0x04, test_data, 0x3) == 0) {
		int id = (test_data[3] << 24) | (test_data[2] << 16) | (test_data[1] << 8) | test_data[0];
		sc_always("panel_id : 0x%x \n", id);
		return id;
	}
	return -1;
}

/* note this command should execute in video mode */
int dsi_read(uint8_t data_type, uint8_t addr, uint8_t *data, uint32_t len)
{
	uint32_t  reg = 0;
	uint32_t  cnt = 0;
	uint32_t  align_len = SC_ALIGN4(len);
	uint32_t  left_len = len;
	uint32_t  copy_len;
	int       ret = 0;
	int       mode_cfg_bak = dsi_reg_read(DSI_HOST_REG_MODE_CFG);

	/* enter cmd mode */
	dsi_reg_write(DSI_HOST_REG_MODE_CFG,  0x1);

	/* set read size */
	dsi_reg_write(DSI_HOST_REG_GEN_HDR,  (align_len << 8) | 0x37);
	sc_delay(1);

	dsi_reg_write(DSI_HOST_REG_GEN_HDR,   (((uint32_t)addr) << 8) | data_type);
	reg = dsi_reg_read(DSI_HOST_REG_CMD_PKT_STATUS);
	sc_debug("wait ready reg=0x%x.", reg);
	while(reg & 0x50) {
		if (cnt++ >= 100) {
			sc_err("read timeout");
			ret = -1;
			goto End;
		}

		reg = dsi_reg_read(DSI_HOST_REG_CMD_PKT_STATUS);
		sc_debug("wait ready reg=0x%x.", reg);
		sc_delay(1);
	}

	for (int i = 0; i < align_len / 4; i++) {
		reg = dsi_reg_read(DSI_HOST_REG_GEN_PLD_DATA);
		sc_always("reg = 0x%x", reg);
		if (left_len >= 4)
			copy_len = 4;
		else
			copy_len = left_len;

		for (int j = 0; j < copy_len; j++) {
			*data = (reg >> (j * 8)) & 0xff;
			sc_debug("data: 0x%x", *data);
			data++;
		}

		left_len -= copy_len;
	}

End:
	/* enter video mode for dsi to recieve dphy's stop signal */
	dsi_reg_write(DSI_HOST_REG_MODE_CFG,  0x0);
	sc_delay(1);
	dsi_reg_write(DSI_HOST_REG_PHY_RSTZ,   0xd);
	sc_delay(1);
	dsi_reg_write(DSI_HOST_REG_PHY_RSTZ,   0xf);
	sc_delay(1);
	dsi_reg_write(DSI_HOST_REG_MODE_CFG,  mode_cfg_bak);
	sc_always("reset dphy and restore mode:%d", mode_cfg_bak);
	return ret;
}

/* note this command should execute in video mode */
int dsi_long_cmd(uint8_t data_type, uint8_t *para, uint16_t len)
{
	uint32_t   offset = 0;
	uint32_t   data = 0;
	uint32_t   mod;

	while(offset < len) {
		mod = offset % 4;

		if (mod == 0)
			data = *(para + offset);
		else {
			data |= (((uint32_t)(*(para + offset))) << (mod * 8));
			if (mod == 3) {
				sc_debug("data: 0x%x", data);
				dsi_reg_write(DSI_HOST_REG_GEN_PLD_DATA, data );
			}
		}
		offset++;
	}

	if (len % 4) {
		sc_debug("data1: 0x%x", data);
		dsi_reg_write(DSI_HOST_REG_GEN_PLD_DATA, data );
	}

	dsi_reg_write(DSI_HOST_REG_GEN_HDR,    ((len & 0xff00) << 16) | ((len & 0xff) << 8) |  data_type );
	sc_debug("0x6c: 0x%x", ((len & 0xff00) << 16) | ((len & 0xff) << 8) |  data_type);
	return 0;
}

