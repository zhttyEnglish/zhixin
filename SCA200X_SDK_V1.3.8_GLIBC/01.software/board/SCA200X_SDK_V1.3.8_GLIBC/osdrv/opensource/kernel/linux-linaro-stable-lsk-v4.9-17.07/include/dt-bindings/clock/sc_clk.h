#ifndef __SC_CLK_H__
#define __SC_CLK_H__
#if 0
enum {
	SC_CLK_MOD_CORE = 0xffff0000,
	SC_CLK_MOD_ISP,
	SC_CLK_MOD_DISP,
	SC_CLK_MOD_HEVC_CORE,
	SC_CLK_MOD_HEVC_BPU,
	SC_CLK_MOD_H264,
	SC_CLK_MOD_JPEG,
	SC_CLK_MOD_MIPI,
	SC_CLK_MOD_VIF,
	SC_CLK_MOD_DMAC_CEVA,
	SC_CLK_MOD_DMAC_TOP,
	SC_CLK_MOD_M7,
	SC_CLK_MOD_BB,
	SC_CLK_MOD_BB_LDPC,
	SC_CLK_MOD_TYPEC,
	SC_CLK_MOD_GMAC,
	SC_CLK_MOD_PIX,
	SC_CLK_MOD_AU_PLL_FOR_SENSOR,
};

enum {
	SC_CLK_PLL2 = 0xffff0000,
	SC_CLK_25M,
	SC_CLK_50M,
	SC_CLK_100M,
	SC_CLK_125M,
	SC_CLK_150M,
	SC_CLK_200M,
	SC_CLK_250M,
	SC_CLK_300M,
	SC_CLK_330M,
	SC_CLK_333M,
	SC_CLK_360M,
	SC_CLK_400M,
	SC_CLK_450M,
	SC_CLK_500M,
	SC_CLK_600M,
	SC_CLK_666M,
	SC_CLK_PIXEL,
	SC_CLK_AU_PLL_FOR_SENSOR,
};
#else
#define SC_CLK_MOD_CORE        0xffff0000
#define SC_CLK_MOD_ISP         0xffff0001
#define SC_CLK_MOD_DISP        0xffff0002
#define SC_CLK_MOD_HEVC_CORE   0xffff0003
#define SC_CLK_MOD_HEVC_BPU    0xffff0004
#define SC_CLK_MOD_H264        0xffff0005
#define SC_CLK_MOD_JPEG        0xffff0006
#define SC_CLK_MOD_MIPI        0xffff0007
#define SC_CLK_MOD_VIF         0xffff0008
#define SC_CLK_MOD_DMAC_CEVA   0xffff0009
#define SC_CLK_MOD_DMAC_TOP    0xffff000a
#define SC_CLK_MOD_M7          0xffff000b
#define SC_CLK_MOD_BB          0xffff000c
#define SC_CLK_MOD_BB_LDPC     0xffff000d
#define SC_CLK_MOD_TYPEC       0xffff000e
#define SC_CLK_MOD_GMAC        0xffff000f
#define SC_CLK_MOD_PIX         0xffff0010
#define SC_CLK_MOD_AU_PLL_FOR_SENSOR 0xffff0011

#define SC_CLK_PLL2   0xffff0000
#define SC_CLK_25M    0xffff0001
#define SC_CLK_50M    0xffff0002
#define SC_CLK_100M   0xffff0003
#define SC_CLK_125M   0xffff0004
#define SC_CLK_150M   0xffff0005
#define SC_CLK_200M   0xffff0006
#define SC_CLK_250M   0xffff0007
#define SC_CLK_300M   0xffff0008
#define SC_CLK_330M   0xffff0009
#define SC_CLK_333M   0xffff000a
#define SC_CLK_360M   0xffff000b
#define SC_CLK_400M   0xffff000c
#define SC_CLK_450M   0xffff000d
#define SC_CLK_500M   0xffff000e
#define SC_CLK_600M   0xffff000f
#define SC_CLK_666M   0xffff0010
#define SC_CLK_PIXEL  0xffff0011
#define SC_CLK_AU_PLL_FOR_SENSOR 0xffff0012

#endif
#endif
