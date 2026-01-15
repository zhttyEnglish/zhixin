include ../../cfg.mk

uboot_dir := u-boot-v2020.10
build_dir := build
build_path := $(uboot_dir)/$(build_dir)

rel_img := boot.img
rel_img := $(patsubst %,$(build_path)/%,$(rel_img))

mk_arg := -C $(uboot_dir)
mk_arg += -j$(SC_MK_JOBS)
mk_arg += O=$(build_dir)
mk_arg += ARCH=$(SC_UBOOT_ARCH)
mk_arg += CROSS_COMPILE=$(SC_CROSS_COMPILE)

mkenvimage := $(uboot_dir)/$(build_dir)/tools/mkenvimage

## uboot env 大小为 32KB
uboot_env_size := 0x8000
uboot_env_in := uboot_env/env_emmc.txt
uboot_env_out := $(SC_UBOOT_REL_DIR)/uboot-env.emmc.img
uboot_env_out_null := $(SC_UBOOT_REL_DIR)/uboot-env.null.img

.PHONY: uboot install clean

uboot:
	@mkdir -p $(build_path)
	@echo "SC_UBOOT_CFG: $(SC_UBOOT_CFG)"
	@$(MAKE) $(mk_arg) $(SC_UBOOT_CFG)
	@$(MAKE) $(mk_arg)

install: uboot
	@mkdir -p $(SC_UBOOT_REL_DIR)
	@cp -v $(rel_img) $(SC_UBOOT_REL_DIR)
	@echo 0 > $(uboot_env_out_null)

uboot-env.img:
	@$(mkenvimage) -s $(uboot_env_size) -o $(uboot_env_out) $(uboot_env_in)

clean:
	@$(MAKE) -C $(uboot_dir) distclean
	@rm -rf $(build_path)

