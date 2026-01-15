// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

/*
 * Misc boot support
 */
#include <common.h>
#include <command.h>
#include <cpu_func.h>

#ifdef CONFIG_CMD_BOOTS

extern void release_secondary_cpu(unsigned long cpuid, unsigned long sprs_el3, unsigned long scr_el3, unsigned long entry_point);

static int do_boots(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	ulong	addr, cpuid;

	if (argc < 3)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "cpu1")) {
		cpuid = 1;
	}
	else if (!strcmp(argv[1], "cpu2")) {
		cpuid = 2;
	}
	else if (!strcmp(argv[1], "cpu3")) {
		cpuid = 3;
	}
	else {
		return CMD_RET_USAGE;
	}

	addr = simple_strtoul(argv[2], NULL, 16);
	printf ("## Boot %s at 0x%08lX ...\n", argv[1], addr);

	flush_dcache_all();
	release_secondary_cpu(cpuid, 0x3cd, 0x630, addr);

	return 0;
}

/* -------------------------------------------------------------------- */

U_BOOT_CMD(
	boots, CONFIG_SYS_MAXARGS, 0,	do_boots,
	"boot secondary 'cpu' at address 'addr'",
	"boots cpu addr\n"
	"    - boot secondary 'cpu' at address 'addr'\n"
	"    - cpu could be cpu1 cpu2 cpu3\n"
);

#endif

