#ifndef __SMARTCHIP_AHB_DMA_H__
#define __SMARTCHIP_AHB_DMA_H__

#define BASE_ADDR_DMA           0x45000000
#define BASE_ADDR_DMA_LOW_BIT   0x40b00030
#define AHB_DMA_MAX_LLP_ND      0x200

/* Refer to AHB DMA spec */
struct ahb_dma_llp {
	unsigned int src;
	unsigned int dst;
	unsigned int next_llp;
	unsigned int ctl_l;
	unsigned int ctl_h;
	unsigned int stat_src;
	unsigned int stat_dst;
};

int sc_ahb_dma_m2m(struct sc_nor_host *host, unsigned char *src, unsigned char *dst, unsigned int size,
    u_char *read_buf, unsigned int check_buf);

void sc_ahb_dma_init(struct sc_nor_host *host);

#endif
