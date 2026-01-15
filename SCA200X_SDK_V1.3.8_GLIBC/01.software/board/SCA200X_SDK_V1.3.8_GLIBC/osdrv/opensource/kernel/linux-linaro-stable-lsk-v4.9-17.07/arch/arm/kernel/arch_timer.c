/*
 *  linux/arch/arm/kernel/arch_timer.c
 *
 *  Copyright (C) 2011 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>

#include <asm/delay.h>

#include <clocksource/arm_arch_timer.h>
#include <asm/arch_timer.h>


static unsigned long arch_timer_read_counter_long(void)
{
	return arch_timer_read_counter();
}

static struct delay_timer arch_delay_timer;

static void __init arch_timer_delay_timer_register(void)
{
	/* Use the architected timer for the delay loop. */
	arch_delay_timer.read_current_timer = arch_timer_read_counter_long;
	arch_delay_timer.freq = arch_timer_get_rate();
	register_current_timer_delay(&arch_delay_timer);
}

int __init arch_timer_arch_init(void)
{
	u32 arch_timer_rate = arch_timer_get_rate();

	if (arch_timer_rate == 0)
		return -ENXIO;

	arch_timer_delay_timer_register();

	return 0;
}

#include <linux/syscalls.h>

/*
 *  for there is no mmio interface for read the cntpct, add the syscall
 */
SYSCALL_DEFINE1(gettimeofcntpct, u64 __user *, u_cntpct)
{
	if (u_cntpct) {
		u64 tmp = arch_counter_get_cntpct();
		if (copy_to_user(u_cntpct, &tmp, sizeof(tmp)))
			return -EFAULT;
	}

	printk(KERN_CRIT "calling gettimeofcntpct \n");

	return 0;
}


