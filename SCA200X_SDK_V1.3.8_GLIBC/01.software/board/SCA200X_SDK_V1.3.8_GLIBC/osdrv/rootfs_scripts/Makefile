## sdk/osdrv/cfg.mk
include ../cfg.mk

ROOTFS_LIB_DIR := $(OSDRV_DIR)/release/out/rootfs
ROOTFS_DIR := $(OSDRV_DIR)/release/out/rootfs_all
PROJ_PREBUILD_DIR := ./prebuild

TOOL_EXT4 := $(SC_TOOLS_PC_REL_DIR)/bin/make_ext4fs
ROOTFS_OUT_IMG_SPARSE := $(OSDRV_DIR)/release/out/rootfs.64M.ext4.sparse.img

define cmd_rm_svn
	cd $(1); \
		find . -name '.svn' -o -name '.git' |\
		xargs --no-run-if-empty \
		rm -rf
endef

define cmd_strip
	cd $(1); \
		find . -type f -exec file {} \; | \
		grep 'ELF 32-bit.*' | \
		grep -v 'no section header' | \
		cut -d : -f 1 | \
		xargs --no-run-if-empty \
		$(STRIP_32) \
			--strip-debug --strip-unneeded --remove-section=.comment --remove-section=.note
	cd $(1); \
		find .  -type f -exec file {} \; | \
		grep 'ELF 64-bit.*' | \
		grep -v 'no section header' | \
		cut -d : -f 1 | \
		xargs --no-run-if-empty \
		$(STRIP) \
			--strip-debug --strip-unneeded --remove-section=.comment --remove-section=.note
endef

.PHONY:all
all:
	@rm -rf $(ROOTFS_DIR)
	@rm -rf $(ROOTFS_LIB_DIR)
	@tar -xf rootfs.tgz -C $(dir $(ROOTFS_LIB_DIR))
	@cp -ar $(ROOTFS_LIB_DIR) $(ROOTFS_DIR)
	@cp -ar $(SC_BUSYBOX_REL_DIR)/* $(ROOTFS_DIR)
	@:
	@cp -ar $(MPP_DIR)/ko/* $(ROOTFS_DIR)/komod
	@cp -ar $(MPP_DIR)/bin/* $(ROOTFS_DIR)/usr/bin
	@mkdir  $(ROOTFS_DIR)/local
	@cp -r  $(MPP_DIR)/sensor/tuning $(ROOTFS_DIR)/local
	@cp -ar $(MPP_DIR)/lib64/*   $(ROOTFS_DIR)/usr/lib
	@rm -rf $(ROOTFS_DIR)/usr/lib/*.a
	@cp -ar $(MPP_DIR)/lib32/*   $(ROOTFS_DIR)/usr/lib32
	@rm -rf $(ROOTFS_DIR)/usr/lib32/*.a
	@cp -ar $(SC_TOOLS_BD_REL_DIR)/*.out $(ROOTFS_DIR)/usr/bin
	@:
	@$(call cmd_rm_svn,$(ROOTFS_DIR))
	@-$(call cmd_strip,$(ROOTFS_DIR))
	@$(TOOL_EXT4) -l 64M -s $(ROOTFS_OUT_IMG_SPARSE) $(ROOTFS_DIR)
	@echo "#################################################"
	@echo "$(ROOTFS_OUT_IMG_SPARSE) is ok"

.PHONY:clean
clean:
	@rm -rf $(ROOTFS_DIR)
	@rm -rf $(ROOTFS_LIB_DIR)
	@rm -rf $(ROOTFS_OUT_IMG_SPARSE)


