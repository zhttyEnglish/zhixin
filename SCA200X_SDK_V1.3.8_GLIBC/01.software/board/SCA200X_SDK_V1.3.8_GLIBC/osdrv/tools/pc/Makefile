## osdrv/cfg.mk
## osdrv/tools/pc/Makefile
include ../../cfg.mk

.PHONY: all clean

all:
	@$(MAKE) -C ext4_utils
	@mkdir -p $(SC_TOOLS_PC_REL_DIR)
	@cp -ar ext4_utils/bin/ $(SC_TOOLS_PC_REL_DIR)

clean:
	@$(MAKE) -C ext4_utils clean

