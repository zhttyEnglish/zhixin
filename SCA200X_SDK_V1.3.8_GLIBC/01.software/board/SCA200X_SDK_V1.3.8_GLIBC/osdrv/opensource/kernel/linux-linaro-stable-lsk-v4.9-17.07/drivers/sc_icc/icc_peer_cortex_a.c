#include <linux/cpumask.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/debugfs.h>

#include "icc_priv.h"

extern void gic_send_sgi(unsigned int cpu_id, unsigned int irq);

struct icc_peer_cortex_a_priv {
	struct icc_peer *peer;
};

enum ipi_msg_type {
	IPI_WAKEUP,
	IPI_TIMER,
	IPI_RESCHEDULE,
	IPI_CALL_FUNC,
	IPI_CPU_STOP,
	IPI_IRQ_WORK,
	IPI_COMPLETION,
	IPI_CPU_BACKTRACE,
	/*
	 * SGI8-15 can be reserved by secure firmware, and thus may
	 * not be usable by the kernel. Please keep the above limited
	 * to at most 8 entries.
	 */
};

int icc_peer_cortex_a_notify(struct icc_peer *peer)
{
	int cpu_id;

	switch(peer->core_id) {
	case ICC_CORE_A_0:
		cpu_id = 0;
		break;
	case ICC_CORE_A_1:
		cpu_id = 1;
		break;
	case ICC_CORE_A_2:
		cpu_id = 2;
		break;
	case ICC_CORE_A_3:
		cpu_id = 3;
		break;
	default:
		return -EINVAL;
	}

	icc_debug("cpu %d", cpu_id);

	gic_send_sgi(cpu_id, IPI_IRQ_WORK);

	return 0;
}

static struct icc_peer_ops icc_peer_cortex_a_ops = {
	.notify = icc_peer_cortex_a_notify,
	.wakeup = icc_wakeup_rx_thread,
};

static int icc_debug_peer_show_ca(struct seq_file *s, void *unused)
{
	struct icc_peer *peer = s->private;

	seq_printf(s, "peer: %s, core id=%d, ipi=%d\n", peer->name, peer->core_id, IPI_IRQ_WORK);
	return 0;
}

static int icc_debug_peer_open_ca(struct inode *inode, struct file *file)
{
	return single_open(file, icc_debug_peer_show_ca, inode->i_private);
}

static const struct file_operations debug_peer_ca_fops = {
	.open = icc_debug_peer_open_ca,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int icc_peer_cortex_a_create(struct icc_peer *peer, struct device_node *node)
{
	u32 irq;
	int ret;
	struct icc_core *icc = peer->icc;

	struct icc_peer_cortex_a_priv *priv;

	ret = of_property_read_u32(node, "trigger-irq", &irq);
	if(ret < 0) {
		return ret;
	}

	if(irq != IPI_IRQ_WORK) {
		return -EINVAL;
	}

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if(!priv)
		return -ENOMEM;

	priv->peer     = peer;
	peer->priv     = priv;
	peer->peer_ops = &icc_peer_cortex_a_ops;
	peer->dbg_file = debugfs_create_file(peer->name, 0644,
	        icc->dbg_dir, peer,
	        &debug_peer_ca_fops);

	return 0;
}

static int icc_peer_cortex_a_get_coreid(struct device_node *node)
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
		return ICC_CORE_A_0;
	case 1:
		return ICC_CORE_A_1;
	case 2:
		return ICC_CORE_A_2;
	case 3:
		return ICC_CORE_A_3;
	default:
		return -1;
	}
}

struct icc_peer_table icc_peer_cortex_a_table = {
	.name = "cortex-a",
	.create = icc_peer_cortex_a_create,
	.get_coreid = icc_peer_cortex_a_get_coreid,
};

static int icc_peer_cortex_a_init(void)
{
	icc_register_peer_tbl(&icc_peer_cortex_a_table);
	return 0;
}

arch_initcall(icc_peer_cortex_a_init);
