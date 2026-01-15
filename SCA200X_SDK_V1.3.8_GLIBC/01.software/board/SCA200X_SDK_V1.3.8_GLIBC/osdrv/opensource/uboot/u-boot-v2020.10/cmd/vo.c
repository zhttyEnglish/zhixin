#include <common.h>
#include <command.h>
#include <hexdump.h>
#include <mapmem.h>
#include <vo/vo.h>

int do_start_vo(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	if (argc < 2)
		return CMD_RET_USAGE;

	argc--;
	argv++;

	if (!strcmp(argv[0], "startdev")) {
		uint32_t dev_id         = 0;
		uint32_t intf_type      = 0;
		uint32_t sub_intf_type  = 0;
		uint32_t sync_type      = 0;

		if (argc != 5)
			return CMD_RET_USAGE;

		dev_id        = simple_strtoul(argv[1], NULL, 10);
		intf_type     = simple_strtoul(argv[2], NULL, 10);
		sub_intf_type = simple_strtoul(argv[3], NULL, 10);
		sync_type     = simple_strtoul(argv[4], NULL, 10);

		//        printf("startdev: dev_id=%d, intf_type=%d, sub_intf_type=%d, sync_type=%d\n",
		//                        dev_id, intf_type, sub_intf_type, sync_type);
		sc_vo_dev_enable(dev_id, intf_type, sub_intf_type, sync_type);
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[0], "stopdev")) {
		uint32_t dev_id         = 0;

		if (argc != 2)
			return CMD_RET_USAGE;

		dev_id        = simple_strtoul(argv[1], NULL, 10);
		printf("stopdev: dev_id=%d\n", dev_id);
		sc_vo_dev_disable(dev_id);
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[0], "startlayer")) {
		uint32_t layer_id = 0;
		uint32_t addr     = 0;
		uint32_t stride   = 0;
		uint32_t x        = 0;
		uint32_t y        = 0;
		uint32_t w        = 0;
		uint32_t h        = 0;
		uint32_t csc      = 0;

		if (argc != 9)
			return CMD_RET_USAGE;

		layer_id = simple_strtoul(argv[1], NULL, 10);
		addr     = simple_strtoul(argv[2], NULL, 16);
		stride   = simple_strtoul(argv[3], NULL, 10);
		x        = simple_strtoul(argv[4], NULL, 10);
		y        = simple_strtoul(argv[5], NULL, 10);
		w        = simple_strtoul(argv[6], NULL, 10);
		h        = simple_strtoul(argv[7], NULL, 10);
		csc      = simple_strtoul(argv[8], NULL, 10);

		//        printf("startlayer: layer_id=%d addr=0x%x stride=%d x=%d y=%d w=%d h=%d\n",
		//                    layer_id, addr, stride, x, y, w, h);
		sc_vo_layer_enable(layer_id, addr, stride, x, y, w, h, csc);
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[0], "stoplayer")) {
		uint32_t layer_id       = 0;

		if (argc != 2)
			return CMD_RET_USAGE;

		layer_id = simple_strtoul(argv[1], NULL, 10);
		printf("stoplayer: layer_id=%d\n", layer_id);
		sc_vo_layer_disable(layer_id);
		return CMD_RET_SUCCESS;
	}

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
    vo, 10,  0,  do_start_vo,
    "video output",
    "\n"
    "startdev   <dev intftype subintf sync>            - start vo device\n"
    "stopdev    <dev>                                  - stop vo device\n"
    "startlayer <layer addr stride x y w h csc>        - start vo layer\n"
    "stoplayer  <layer>                                - stop vo layer\n\n"
    "description:\n"
    "dev:                0 - vo dev 0\n"
    "intftype:\n"
    "                    0 - none\n"
    "                    1 - dvp\n"
    "                    2 - dp\n"
    "                    3 - mipi_tx\n"
    "subintf:\n"
    "  -for dvp:         0 - none\n"
    "                    1 - vga\n"
    "                    2 - hdmi\n"
    "  -for mipi_tx:     0 - none\n"
    "                    1 - EK79007\n"
    "sync:\n"
    "  -for EK79007:     0 - 1024*600@60\n"
    "\n"
    "csc:                0 - bt709 limit\n"
    "                    1 - bt709 full\n"
    "                    2 - bt601 limit\n"
    "                    3 - bt601 full\n"
);

