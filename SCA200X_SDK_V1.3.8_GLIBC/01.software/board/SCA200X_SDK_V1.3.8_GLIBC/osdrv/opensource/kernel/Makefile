include ../../cfg.mk

kernel_dir := linux-linaro-stable-lsk-v4.9-17.07
build_dir := build
build_path := $(kernel_dir)/$(build_dir)
dtb_dir := arch/arm64/boot/dts/sc
dtb_img := $(build_path)/$(dtb_dir)/*.dtb

rel_img := kernel.img
rel_img := $(patsubst %,$(build_path)/%,$(rel_img))

mk_arg := -C $(kernel_dir)
mk_arg += -j$(SC_MK_JOBS)
mk_arg += O=$(build_dir)
mk_arg += ARCH=$(SC_KERNEL_ARCH)
mk_arg += CROSS_COMPILE=$(SC_CROSS_COMPILE)

mod_tmp_dir := $(CURDIR)/$(build_path)/install

.PHONY: kernel install clean

kernel:
	@mkdir -p $(build_path)
	@$(MAKE) $(mk_arg) $(SC_KERNEL_CFG)
	@$(MAKE) $(mk_arg) kernel-img
	@$(MAKE) $(mk_arg) dtbs
	@$(MAKE) $(mk_arg) modules
	@$(MAKE) $(mk_arg) INSTALL_MOD_PATH=$(mod_tmp_dir) modules_install

install: kernel
	@mkdir -p $(SC_KERNEL_REL_DIR)
	@mkdir -p $(SC_DRV_REL_DIR)
	@cp -v $(rel_img) $(SC_KERNEL_REL_DIR)
	@cp -v $(dtb_img) $(SC_KERNEL_REL_DIR)
	@find $(mod_tmp_dir) -name "*.ko"|xargs -i --no-run-if-empty cp {} $(SC_DRV_REL_DIR)

clean:
	@$(MAKE) -C $(kernel_dir) ARCH=$(SC_KERNEL_ARCH) distclean
	@rm -rf $(mod_tmp_dir)
	@rm -rf $(build_path)

