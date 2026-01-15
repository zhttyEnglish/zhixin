include ../../cfg.mk

busybox_dir := busybox-1.25.0
build_dir := build
build_path := $(busybox_dir)/$(build_dir)

mk_arg := -C $(busybox_dir)
mk_arg += -j$(SC_MK_JOBS)
mk_arg += O="$(build_dir)"
mk_arg += CROSS_COMPILE="$(SC_CROSS_COMPILE)"
mk_arg += EXTRA_LDFLAGS="$(SC_LDFLAGS)"

.PHONY: busybox install clean

busybox:
	@mkdir -p $(build_path)
	@$(MAKE) $(mk_arg) $(SC_BUSYBOX_CFG)
	@$(MAKE) $(mk_arg) busybox

install: busybox
	@mkdir -p $(SC_BUSYBOX_REL_DIR)
	@fakeroot $(MAKE) $(mk_arg) CONFIG_PREFIX=$(SC_BUSYBOX_REL_DIR) install

clean:
	@$(MAKE) -C $(busybox_dir) doc-clean distclean
	@rm -rf $(build_path)
info:
	echo $(mk_arg)
