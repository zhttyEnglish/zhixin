#include <linux/cpumask.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <asm/io.h>

#include "icc_priv.h"

struct icc_peer_cortex_m_priv {
	struct icc_peer *peer;
	uint32_t reg;
	uint32_t irq;
	void *vaddr;
};

static int icc_peer_cortex_m_notify(struct icc_peer *peer)
{
	struct icc_peer_cortex_m_priv *priv = peer->priv;

	/* write register to trigger irq on cortex M */
	writel(0xffffffff, priv->vaddr);

	return 0;
}

static void icc_peer_cortex_m_wakeup(struct icc_peer *peer)
{
	icc_wakeup_rx_thread(peer);
}

static struct icc_peer_ops icc_peer_cortex_m_ops = {
	.notify = icc_peer_cortex_m_notify,
	.wakeup = icc_peer_cortex_m_wakeup,
};

static irqreturn_t icc_peer_cortex_m_isr(int irq, void *dev_id)
{
	struct icc_peer *peer = (struct icc_peer *)dev_id;

	peer->peer_ops->wakeup(peer);

	return IRQ_HANDLED;
}

static int icc_debug_peer_show_cm(struct seq_file *s, void *unused)
{
	struct icc_peer *peer = s->private;
	struct icc_peer_cortex_m_priv *priv = peer->priv;

	seq_printf(s, "peer: %s, core id=%d, irq=%d, trigger address=0x%x mapped vaddr=%p\n",
	    peer->name, peer->core_id, priv->irq, priv->reg, priv->vaddr);
	return 0;
}

static int icc_debug_peer_open_cm(struct inode *inode, struct file *file)
{
	return single_open(file, icc_debug_peer_show_cm, inode->i_private);
}

static const struct file_operations debug_peer_cm_fops = {
	.open = icc_debug_peer_open_cm,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int icc_peer_cortex_m_create(struct icc_peer *peer, struct device_node *node)
{
	uint32_t address;
	int irq, ret;
	void *vaddr;
	struct platform_device *pdev;
	struct icc_peer_cortex_m_priv *priv;

	/* Get trigger address */
	ret = of_property_read_u32(node, "trigger-address", &address);
	if(ret < 0) {
		return ret;
	}

	vaddr = ioremap(address, 4);
	if(!vaddr) {
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

	/* request irq */
	ret = devm_request_irq(&pdev->dev, irq, icc_peer_cortex_m_isr, 0,
	        peer->name, peer);
	if(ret)
		return ret;

	/* bind irq to this cpu */
	ret = irq_set_affinity(irq, cpumask_of(icc_get_core_id()));
	if(ret)
		return ret;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if(!priv)
		return -ENOMEM;

	priv->peer  = peer;
	priv->reg   = (uint32_t)address;
	priv->vaddr = vaddr;
	priv->irq   = irq;

	peer->priv     = priv;
	peer->peer_ops = &icc_peer_cortex_m_ops;

	icc_debug("cortex m, wake irq=%d,trigger address=%p\n",  irq, vaddr);

	return 0;
}

static int icc_peer_cortex_m_get_coreid(struct device_node *node)
{
	int ret;
	u32 cpu_id;

	ret = of_property_read_u32(node, "cpu-id", &cpu_id);
	if(ret < 0) {
		return ret;
	}

	if(cpu_id > 3) {
		return -EINVAL;
	}

	switch(cpu_id) {
	case 0:
		return ICC_CORE_M_0;
	case 1:
		return ICC_CORE_M_1;
	case 2:
		return ICC_CORE_M_2;
	case 3:
		return ICC_CORE_M_3;
	default:
		return -1;
	}
}

struct icc_peer_table icc_peer_cortex_m_table = {
	.name = "cortex-m",
	.create = icc_peer_cortex_m_create,
	.get_coreid = icc_peer_cortex_m_get_coreid,
};

static int icc_peer_cortex_m_init(void)
{
	icc_register_peer_tbl(&icc_peer_cortex_m_table);
	return 0;
}

arch_initcall(icc_peer_cortex_m_init);
