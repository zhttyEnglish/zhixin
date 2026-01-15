#include <linux/init.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <asm/mcpm.h>
#include <asm/mach/map.h>

#define CEVA_MCCI_REG 0x62C00000

//DMA registers
#define DW_AXI_DMAC_BASE (0x60620000)

#define ADDR_DMAC_CFGREG                         0x010
#define ADDR_DMAC_INTSTATUS          0x030
#define ADDR_DMAC_COMMONREG_INSTATUS_ENABLE      0x040
#define ADDR_DMAC_COMMONREG_INTSIGNAL_ENABLE     0x048

#define ADDR_DMAC_CHEN                           0x18
#define ADDR_CH1_CTL_L                           0x118
#define ADDR_CH1_CTL_H                           0x11c
#define ADDR_CH1_INTSTATUS                       0x188
#define ADDR_CH1_INTCLEAR                        0x198

#define ADDR_CH1_INTSTATUS_ENABLE_L              0x180
#define ADDR_CH1_INTSTATUS_ENABLE_H              0x184
#define ADDR_CH1_INTSIGNAL_ENABLE_L              0x190
#define ADDR_CH1_INTSIGNAL_ENABLE_H              0x194
#define ADDR_CH1_CFG_L                           0x120
#define ADDR_CH1_CFG_H                           0x124
#define ADDR_CH1_SAR_L                           0x100
#define ADDR_CH1_SAR_H                           0x104
#define ADDR_CH1_DAR_L                           0x108
#define ADDR_CH1_DAR_H                           0x10c
#define ADDR_CH1_BLOCK_TS                        0x110
#define ADDR_CH1_IDREG_L                         0x150
#define ADDR_CH1_LLP_L                           0x128
#define ADDR_CH1_LLP_H                           0x12c

int g_dst_addr = 0x80000000;
volatile int test_flag = -1;

static void dma_one_block(unsigned int sar, unsigned int dar, unsigned int byte_num, unsigned burst_len,
    unsigned int burst_size) /*len = 15, size = 4*/
{
	void __iomem *dmac_base = 0;
	unsigned int done = 0;
	unsigned int ch1_ctl_l;
	unsigned int ch1_ctl_h;
	unsigned int ch1_cfg_h;
	unsigned int block_num;
#ifdef DMA_ORIG
	block_num = (byte_num / (1 << burst_size)) - 1 ;
	ch1_ctl_l = (burst_size << 8) + (burst_size << 11) + (burst_size << 14) + (burst_size << 18);
	ch1_ctl_h = (1 << 6) + (burst_len << 7) + (1 << 15) + (burst_len << 16);
	ch1_cfg_h = (15 << 23) + (15 << 27);
#else
	block_num = (byte_num / (1 << burst_size)) - 1 ;
	ch1_ctl_l = (burst_size << 8) + (burst_size << 11) + (0 << 14) + (0 << 18);
	ch1_ctl_h = (0 << 6) + (burst_len << 7) + (0 << 15) + (burst_len << 16);
	ch1_cfg_h = (15 << 23) + (15 << 27);

#endif
	dmac_base = ioremap(DW_AXI_DMAC_BASE, 0x1000);

	iowrite32(0x00000003, (dmac_base + ADDR_DMAC_CFGREG));
	iowrite32(0x000001ff, (dmac_base + ADDR_DMAC_COMMONREG_INSTATUS_ENABLE));
	iowrite32(0x000001a0, (dmac_base + ADDR_DMAC_COMMONREG_INTSIGNAL_ENABLE));

	iowrite32(0xFFFFFFFF, (dmac_base + ADDR_CH1_INTSTATUS_ENABLE_L));
	iowrite32(0xFFFFFFFF, (dmac_base + ADDR_CH1_INTSTATUS_ENABLE_H));
	iowrite32(0xFFFFFFFF, (dmac_base + ADDR_CH1_INTSIGNAL_ENABLE_L));
	iowrite32(0xFFFFFFFF, (dmac_base + ADDR_CH1_INTSIGNAL_ENABLE_H));
	iowrite32(0x00000000, (dmac_base + ADDR_CH1_CFG_L));
	iowrite32(ch1_cfg_h, (dmac_base + ADDR_CH1_CFG_H)); //0x78180000, 7f92
	iowrite32((ch1_ctl_l | (0xff << 22)), (dmac_base + ADDR_CH1_CTL_L)); //29:26,25:22->AWCACHE/ARCACHE, all '1'.
	iowrite32(ch1_ctl_h, (dmac_base + ADDR_CH1_CTL_H));

	//transfer test_flag to 0x80000000 to see if CCI works properly.
	iowrite32(sar, (dmac_base + ADDR_CH1_SAR_L));
	iowrite32(0x00000000, (dmac_base + ADDR_CH1_SAR_H));
	iowrite32(dar, (dmac_base + ADDR_CH1_DAR_L));
	iowrite32(0x00000000, (dmac_base + ADDR_CH1_DAR_H));
	iowrite32(block_num, (dmac_base + ADDR_CH1_BLOCK_TS)); //block size
	iowrite32(0x00000000, (dmac_base + ADDR_CH1_IDREG_L));
	iowrite32(0x00000000, (dmac_base + ADDR_CH1_LLP_L));
	iowrite32(0x00000000, (dmac_base + ADDR_CH1_LLP_H));

	//debug info
	pr_info("reg cfg 0x%x\n", readl(dmac_base + ADDR_DMAC_CFGREG));
	pr_info("reg comm reg instatus 0x%x\n", readl(dmac_base + ADDR_DMAC_COMMONREG_INSTATUS_ENABLE));
	pr_info("reg comm reg insignal 0x%x\n", readl(dmac_base + ADDR_DMAC_COMMONREG_INTSIGNAL_ENABLE));
	pr_info("reg ch1 cfgl 0x%x\n", readl(dmac_base + ADDR_CH1_CFG_L));
	pr_info("reg ch1 cfgh 0x%x\n", readl(dmac_base + ADDR_CH1_CFG_H));
	pr_info("reg ch1 ctll 0x%x\n", readl(dmac_base + ADDR_CH1_CTL_L));
	pr_info("reg ch1 ctlh 0x%x\n", readl(dmac_base + ADDR_CH1_CTL_H));

	pr_info("reg ch1 SARL 0x%x\n", readl(dmac_base + ADDR_CH1_SAR_L));
	pr_info("reg ch1 SARH 0x%x\n", readl(dmac_base + ADDR_CH1_SAR_H));
	pr_info("reg ch1 DARL 0x%x\n", readl(dmac_base + ADDR_CH1_DAR_L));
	pr_info("reg ch1 DARH 0x%x\n", readl(dmac_base + ADDR_CH1_DAR_H));
	pr_info("reg ch1 BLOCKTS 0x%x\n", readl(dmac_base + ADDR_CH1_BLOCK_TS));

	test_flag = 0x12345678;

	iowrite32(0x00000101, (dmac_base + ADDR_DMAC_CHEN)); //channel 1 enable
#if 1 //use interrupt instead
	do {
		done = readl(dmac_base + ADDR_CH1_INTSTATUS) & 0x00000002;
		pr_info("ch1 status = %x, int status %x\n", done, readl(dmac_base + ADDR_DMAC_INTSTATUS));
	} while(done != 0x00000002);

	pr_info("dma transfer done!\n");
#endif
}

static int dsp_print(void)
{
	void __iomem *print_lock = ioremap(0x80050000, 0x100);
	void __iomem *print_buf = ioremap(0x80730000, 0x100);

	pr_info("CEVA\n");
	if (strlen(print_buf) > 0) {
		while(readl(print_lock) == 1);
		writel(1, print_lock);
		pr_info("%s\n", print_buf);
		memset(print_buf, 0x0, sizeof(print_buf));
		writel(0, print_lock);
	}

	return 0;
}

void __init ceva_test_stub(void)
{
	void __iomem *page_table = NULL;
	void __iomem *ceva_rst = NULL;
	void __iomem *tmp = NULL;
	unsigned int i = 0;

	//pr_info("test in platsmp.c: PA: %x\n", virt_to_phys(&test_flag));

	//Test DMA
	//Cache read
	//dma_one_block((virt_to_phys(&test_flag) & 0xffffff00), 0x80000000, 0x100, 15, 4);

	//Cache write
	//dma_one_block(0x80000000, (virt_to_phys(&test_flag) & 0xfffffff0), 8, 15, 2);

#if 1   //Test SMMU+CCI+CEVA
	//Note: SMMU init has been done in ROM code, so here we only init page table and let CEVA run.

	//init page table which base is at 0x80100000
	page_table = ioremap_cache(0x80100000, 0x4000);
	if (!page_table) {
		pr_info("Map mem error!\n");
		return;
	}
	//Initialize the page table, swap 0x80900000 with 0x80A00000
	for(i = 0; i < 4096; i++) {
		//11C0E for Normal sharable outer/inner write-alloc cachable, 0x2C02 for Device Non-sharable memory
		//Cache coherency require the memory to be sharable, while exclusive access require the memory to be non-sharable
		if(i == 0x800)
			writel((i << 20) | 0x2C02, (unsigned int *)(page_table + i * 4));
		else if(i == 0x809)
			writel((0x80A << 20) | 0x11C0E, (unsigned int *)(page_table + i * 4));
		else if(i == 0x80A)
			writel((0x809 << 20) | 0x11C0E, (unsigned int *)(page_table + i * 4));
		else
			writel((i << 20) | 0x11C0E, (unsigned int *)(page_table + i * 4));
	}
	pr_info("entry %p is: %x\n", page_table + 0x2000, readl(page_table + 0x2000));
	pr_info("entry %p is: %x\n", page_table + 0x2004, readl(page_table + 0x2004));
	pr_info("entry %p is: %x\n", page_table + 0x2008, readl(page_table + 0x2008));
	pr_info("entry %p is: %x\n", page_table + 0x200c, readl(page_table + 0x200c));

	//set test flag
	tmp = ioremap_cache(0x80908000, 0x100);
	writel(0xdeadbeef, tmp);

	//Let CEVA core 0/1 run
	ceva_rst = ioremap(0x64571000, 0x1000);
	writel(1, ceva_rst + 4);  //core 0 rst
	writel(1, ceva_rst + 8);
	writel(1, ceva_rst + 0xC);
	writel(1, ceva_rst + 0x10);
	writel(1, ceva_rst + 0x14); //core 1 rst
	writel(1, ceva_rst + 0x18);
	writel(1, ceva_rst + 0x1C);
	writel(1, ceva_rst + 0x20);

	while(1)
		dsp_print();

#endif
	while(1);
}
//EXPORT_SYMBOL(ceva_test_stub);
