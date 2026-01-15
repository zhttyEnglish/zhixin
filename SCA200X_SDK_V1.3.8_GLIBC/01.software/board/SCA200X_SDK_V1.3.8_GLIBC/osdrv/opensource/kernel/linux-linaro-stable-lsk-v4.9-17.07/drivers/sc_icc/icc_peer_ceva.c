#include <linux/cpumask.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <asm/io.h>

#include "icc_priv.h"

struct icc_peer_ceva_priv {
	struct icc_peer *peer;
	uint32_t reg[2];
	uint32_t irq;
	void *trigger_base;
	void *clr_irq_base;
	void *ceva_cpm_base; //dsp internal reg base
	void *ceva_pc; //used to store ceva PC
};

static int icc_peer_ceva_notify(struct icc_peer *peer)
{
	struct icc_peer_ceva_priv *priv = peer->priv;

	/* write register to trigger irq on ceva */
	writel(0xffffffff, priv->trigger_base);

	return 0;
}

static void icc_peer_ceva_wakeup(struct icc_peer *peer)
{
	struct icc_peer_ceva_priv *priv = peer->priv;

	/* write register to clear irq from ceva */
	writel(0x0, priv->clr_irq_base);

	icc_wakeup_rx_thread(peer);
}

static struct icc_peer_ops icc_peer_ceva_ops = {
	.notify = icc_peer_ceva_notify,
	.wakeup = icc_peer_ceva_wakeup,
};

static char *dbg_gen_str[] = {
	"GVI",   /*bit 0*/
	"ERR IO access",/*bit 1*/
	"ERR from EPP", /*bit 2*/
	"ERR from EDP", /*bit 3*/
	"Resvd",  /*bit 4*/
	"Not support exclusive access", /*bit 5*/
	"EDP Read out of range", /*bit 6*/
	"EDP Write out of range", /*bit 7*/
	"AXIs0 Read out of range", /*bit 8*/
	"AXIs0 Write out of range", /*bit 9*/
	"AXIs1 Read out of range", /*bit 10*/
	"AXIs1 Write out of range", /*bit 11*/
	"AXIs2 Read out of range", /*bit 12*/
	"AXIs2 Write out of range", /*bit 13*/
	"Resvd", /*bit 14*/
	"Resvd", /*bit 15*/
	"Resvd", /*bit 16*/
	"DMA cross region", /*bit 17*/
	"LSU Read cross region", /*bit 18*/
	"LSU Write cross region", /*bit 19*/
	"DMA IDM cross", /*bit 20*/
	"LSU ld IDM cross", /*bit 21*/
	"Write buffer IDM cross", /*bit 22*/
	"Histogram no IDM", /*bit 23*/
	"Qman IDM cross", /*bit 24*/
	"Resvd", /*bit 25*/
	"Unmapped exception", /*bit 26*/
	"Stack violation", /*bit 27*/
	"MACs overflow", /*bit 28*/
	"Resvd", /*bit 29*/
	"AXIm0 error", /*bit 30*/
	"AXIm1 error" /*bit 31*/
};

static char *dbg_gen2_str[] = {
	"Ld blank access", /*bit 0*/
	"St blank access", /*bit 1*/
	"Resvd", /*bit 2*/
	"DMA blank access", /*bit 3*/
	"Qman blank access", /*bit 4*/
	"Exlusive restricted", /*bit 5*/
	"Histogram overflow", /*bit 6*/
	"VHIST overflow", /*bit 7*/
	"Watchdog min", /*bit 8*/
	"Watchdog max", /*bit 9*/
	"EPP timeout", /*bit 10*/
	"EDP timeout", /*bit 11*/
	"IOP timeout", /*bit 12*/
	"AXIm0 timeout", /*bit 13*/
	"AXIm1 timeout", /*bit 14*/
	"Resvd", /*bit 15*/
	"Divide by 0", /*bit 16*/
	"Undefined OP code", /*bit 17*/
	"Resvd", /*bit 18*/
	"P trans violation", /*bit 19*/
	"D trans violation", /*bit 20*/
	"Qman violation", /*bit 21*/
	"Resvd", /*bit 22*/
	"Resvd", /*bit 23*/
	"Resvd", /*bit 24*/
	"Resvd", /*bit 25*/
	"Resvd", /*bit 26*/
	"Resvd", /*bit 27*/
	"Resvd", /*bit 28*/
	"Resvd", /*bit 29*/
	"GVI non-critical", /*bit 30*/
	"GVI critical" /*bit 31*/
};

static char *uop_str[] = {
	"System level undefined opcode", /*bit 0*/
	"LS0 undefined opcode", /*bit 1*/
	"LS1 undefined opcode", /*bit 2*/
	"Scalar0 undefined opcode", /*bit 3*/
	"Resvd", /*bit 4*/
	"Resvd", /*bit 5*/
	"Resvd", /*bit 6*/
	"PCU undefined opcode", /*bit 7*/
	"VPU0 undefined opcode", /*bit 8*/
	"VPU1 undefined opcode", /*bit 9*/
	"Memory alignment violation", /*bit 10*/
	"LVPU undefined opcode" /*bit 11*/
};

static void ceva_dbg_interpreter(unsigned int dbg_gen, unsigned int dbg_gen2)
{
	int i = 0;
	if(dbg_gen) {
		for(i = 0; i < 32; i++) {
			if(dbg_gen & (1 << i)) {
				printk("DBG_GEN Bit %d: %s\n", i, dbg_gen_str[i]);
			}
		}
	}

	if(dbg_gen2) {
		for(i = 0; i < 32; i++) {
			if(dbg_gen2 & (1 << i)) {
				printk("DBG_GEN2 Bit %d: %s\n", i, dbg_gen2_str[i]);
			}
		}
	}
}
static void ceva_uop_interpreter(unsigned int uop_sts)
{
	int i = 0;
	if(uop_sts) {
		for(i = 0; i < 12; i++) {
			if(uop_sts & (1 << i)) {
				printk("Bit %d: %s\n", i, uop_str[i]);
			}
		}
	}
}
static void ceva_qman_interpreter(unsigned int qman)
{
	if(qman) {
		if(qman & 0xFF) {
			printk("Queue empty violation, val = 0x%x\n", qman & 0xFF);
		}

		if(qman & (1 << 16)) {
			printk("Frame number violation\n");
		}
	}
}

static irqreturn_t icc_peer_ceva_isr(int irq, void *dev_id)
{
	struct icc_peer *peer = (struct icc_peer *)dev_id;
	struct icc_peer_ceva_priv *priv = peer->priv;
	unsigned int status = 0;
	unsigned int val = 0, val1 = 0;

	//check int status
	status = readl(priv->trigger_base + 0x40); //trigger: 0x010c4008, status: 0x010c0048
	if(status & (1 << 10)) { //icc
		peer->peer_ops->wakeup(peer);
	}

	if(status & (1 << 9)) { //int 1
		printk("Recv CEVA int1 irq.\n");
		writel(0, priv->trigger_base + 0x20); //int1 0x010c4028
	}

	if(status & 0x1FF) { //uop and gvi exceptions
		//send an irq to ceva to get pc and sp, but it's not the real crime scene
		writel(1, priv->trigger_base + 0xC); //nmi mask 0x010c4014
		writel(1, priv->trigger_base + 0x8); //nmi 0x010c4010

		//capture dbg registers
		val = readl(priv->ceva_cpm_base + 0xD14);
		val1 = readl(priv->ceva_cpm_base + 0xD24);
		printk("CEVA DBG_GEN: 0x%x, DBG_GEN2: 0x%x\n", val, val1);
		writel(0, priv->ceva_cpm_base + 0xD14); //clear irq
		writel(0, priv->ceva_cpm_base + 0xD24); //clear irq
		ceva_dbg_interpreter(val, val1);

		//uop regs
		val = readl(priv->ceva_cpm_base + 0xC58);
		val1 = readl(priv->ceva_cpm_base + 0xC5C);
		printk("CEVA UOP_STS: 0x%x, UOP_PAR: 0x%x\n", val, val1);
		ceva_uop_interpreter(val);

		//qman status
		val = readl(priv->ceva_cpm_base + 0x1198);
		printk("CEVA QMAN_IRQ_STATUS: 0x%x\n", val);
		writel(val, priv->ceva_cpm_base + 0x1198); //clear irq
		ceva_qman_interpreter(val);

		//print ceva pc
		printk("Probably CEVA PC at: 0x%x\n", readl(priv->ceva_pc));
	}

	return IRQ_HANDLED;
}

static int icc_debug_peer_show_ceva(struct seq_file *s, void *unused)
{
	struct icc_peer *peer = s->private;
	struct icc_peer_ceva_priv *priv = peer->priv;

	seq_printf(s,
		"peer: %s, core id=%d, irq=%d, "
		"trigger reg=(0x%x, %p), "
		"clr irq reg=(0x%x, %p)\n",
	    peer->name, peer->core_id, priv->irq,
	    priv->reg[0], priv->trigger_base,
	    priv->reg[1], priv->clr_irq_base);
	return 0;
}

static int icc_debug_peer_open_ceva(struct inode *inode, struct file *file)
{
	return single_open(file, icc_debug_peer_show_ceva, inode->i_private);
}

static const struct file_operations debug_peer_ceva_fops = {
	.open = icc_debug_peer_open_ceva,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int icc_peer_ceva_create(struct icc_peer *peer, struct device_node *node)
{
	uint32_t address[3];
	int irq, ret;
	void *vaddr1, *vaddr2, *vaddr3, *vaddr4;
	struct platform_device *pdev;
	struct icc_peer_ceva_priv *priv;
	struct icc_core *icc = peer->icc;

	/* Get trigger address */
	ret = of_property_read_u32(node, "trigger-address", &address[0]);
	if(ret < 0) {
		return ret;
	}

	vaddr1 = ioremap(address[0], 0x100);
	if(!vaddr1) {
		return -ENOMEM;
	}

	icc_debug("%s %d trigger vaddr=%x\n", __func__, __LINE__, vaddr1);

	/* Get trigger address */
	ret = of_property_read_u32(node, "clr-irq-address", &address[1]);
	if(ret < 0) {
		return ret;
	}

	vaddr2 = ioremap(address[1], 4);
	if(!vaddr2) {
		return -ENOMEM;
	}

	icc_debug("%s %d clr irq vaddr=%x\n", __func__, __LINE__, vaddr2);

	ret = of_property_read_u32(node, "ceva-cpm-base", &address[2]);
	if(ret < 0) {
		address[2] = 0x01c00000;
	}
	vaddr3 = ioremap(address[2], 0x2000);
	if(!vaddr3) {
		return -ENOMEM;
	}

	icc_debug("%s %d ceva cpm vaddr=%x\n", __func__, __LINE__, vaddr3);

	vaddr4 = ioremap(0x010c1014, 4);
	if(!vaddr4) {
		return -ENOMEM;
	}

	/* Register respinse irq */
	pdev = of_platform_device_create(node, peer->name, NULL);
	if(!pdev) {
		return -EINVAL;
	}

	irq = platform_get_irq(pdev, 0);
	if(irq < 0) {
		return irq;
	}

	icc_debug("%s %d irq=%d\n", __func__, __LINE__, irq);

	/* request irq */
	ret = devm_request_irq(&pdev->dev, irq, icc_peer_ceva_isr, 0, peer->name, peer);
	if(ret) {
		/*do not return error, becasue ceva peers share the irqs, print log*/
		printk("ignored request_irq fail=%d coreid=%d %s\n", irq, peer->core_id, peer->name);
		ret = 0;
	}

	/* bind irq to this cpu */
	ret = irq_set_affinity(irq, cpumask_of(icc_get_core_id()));
	if(ret)
		return ret;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if(!priv)
		return -ENOMEM;

	priv->peer  = peer;
	priv->reg[0]   = (uint32_t)address[0];
	priv->reg[1]   = (uint32_t)address[1];
	priv->trigger_base = vaddr1;
	priv->clr_irq_base = vaddr2;
	priv->ceva_cpm_base = vaddr3;
	priv->ceva_pc = vaddr4;
	priv->irq   = irq;
	peer->priv     = priv;
	peer->peer_ops = &icc_peer_ceva_ops;
	peer->dbg_file = debugfs_create_file(peer->name, 0644,
	        icc->dbg_dir, peer,
	        &debug_peer_ceva_fops);

	icc_debug("ceva, wake irq=%d,trigger address=%p, clr irq address=%p\n",
	    irq, vaddr1, vaddr2);

	return 0;
}

static int icc_peer_ceva_get_coreid(struct device_node *node)
{
	int ret, core_id;

	ret = of_property_read_u32(node, "cpu-id", &core_id);
	if(ret < 0) {
		return ret;
	}

	if(core_id > 3) {
		return -EINVAL;
	}

	switch(core_id) {
	case 0:
		return ICC_CORE_CEVA_0;
	case 1:
		return ICC_CORE_CEVA_1;
	case 2:
		return ICC_CORE_CEVA_2;
	case 3:
		return ICC_CORE_CEVA_3;
	default:
		return -1;
	}
}

struct icc_peer_table icc_peer_ceva_table = {
	.name = "ceva",
	.create = icc_peer_ceva_create,
	.get_coreid = icc_peer_ceva_get_coreid,
};

static int icc_peer_ceva_init(void)
{
	icc_register_peer_tbl(&icc_peer_ceva_table);
	return 0;
}

arch_initcall(icc_peer_ceva_init);
