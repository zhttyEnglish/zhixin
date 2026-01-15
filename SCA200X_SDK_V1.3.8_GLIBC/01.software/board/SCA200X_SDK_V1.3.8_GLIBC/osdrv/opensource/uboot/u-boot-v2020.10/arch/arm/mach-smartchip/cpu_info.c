/*
 * (C) Copyright 2007-2011
 * SmartChip Technology Co., Ltd. <www.sgitg.sgcc.com.cn>
 *
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>

#ifdef CONFIG_DISPLAY_CPUINFO
int print_cpuinfo(void)
{
#ifdef CONFIG_SMARTCHIP_SCA200V100
	puts("CPU:   SmartChip SCA200V100\n");
#elif defined CONFIG_SMARTCHIP_SCA200V200
	puts("CPU:   SmartChip SCA200V200\n");
#endif
	return 0;
}
#endif
