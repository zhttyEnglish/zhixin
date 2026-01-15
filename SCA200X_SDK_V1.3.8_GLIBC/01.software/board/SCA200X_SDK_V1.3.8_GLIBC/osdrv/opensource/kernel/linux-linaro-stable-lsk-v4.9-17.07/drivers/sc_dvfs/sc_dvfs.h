#ifndef __SC_DVFS_H__
#define __SC_DVFS_H__
#include <linux/of.h>
#include <asm/io.h>

enum {
	SC_DVFS_DEV_ARMLINUX,
	SC_DVFS_DEV_ARMRTOS,
	SC_DVFS_DEV_ARMM,
	SC_DVFS_DEV_CEVA,
	DVFS_PROCESSORS,
};

struct sc_dvfs_processor {
	unsigned char *name;
	int valid;
	int (*get_load)(struct sc_dvfs_processor *processor);
	int (*get_freq)(struct sc_dvfs_processor *processor);
	int (*get_volt)(struct sc_dvfs_processor *processor);
	/*
	 * freq, 10MHz
	 */
	int (*set_freq)(struct sc_dvfs_processor *processor, unsigned int freq);
	int (*set_volt)(struct sc_dvfs_processor *processor);
	void *priv;
};

int sc_dvfs_init_ceva(struct device_node *node, struct sc_dvfs_processor *processor);

#endif
