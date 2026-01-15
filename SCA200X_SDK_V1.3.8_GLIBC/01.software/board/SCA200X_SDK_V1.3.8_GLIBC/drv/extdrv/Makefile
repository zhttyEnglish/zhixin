include ../../osdrv/cfg.mk
SC_DRV_REL_DIR := $(SC_DRV_REL_DIR)/extdrv

kernel_dir := ../../osdrv/opensource/kernel/linux-linaro-stable-lsk-v4.9-17.07
ifneq ($(MAKECMDGOALS),clean)
  kernel_dir := $(kernel_dir)/build
endif

KERNEL_MODULE_DIR := $(CURDIR)
MAKEFLAGS += --no-print-directory

mk_arg := -C $(kernel_dir)
mk_arg += -j$(SC_MK_JOBS)
mk_arg += ARCH=$(SC_KERNEL_ARCH)
mk_arg += CROSS_COMPILE=$(SC_CROSS_COMPILE)

module-y :=
module-y += sc_gpio

clean-y := $(module-y)
clean-y += $(module-)
clean-y := $(patsubst %,%_clean,$(clean-y))

.PHONY: all clean install
.PHONY: moudle $(module-y)
.PHONY: $(clean-y)
.PHONY: prepare prepare_dir

all: install

module: $(module-y)

$(module-y):prepare
	@$(MAKE) $(mk_arg) M=$(KERNEL_MODULE_DIR)/$@ modules
	@if [ -d $@/apptest ] ; then \
		$(MAKE) -C $@/apptest ;\
	fi

clean: $(clean-y)

$(clean-y):
	@$(MAKE) $(mk_arg) M=$(KERNEL_MODULE_DIR)/$(patsubst %_clean,%,$@) clean
	@if [ -d $(patsubst %_clean,%,$@)/apptest ] ; then \
		$(MAKE) -C $(patsubst %_clean,%,$@)/apptest clean;\
	fi

prepare: prepare_dir

prepare_dir:
	mkdir -p $(SC_DRV_REL_DIR)

install: module
	@echo "install..."
	@$(foreach kmod, $(module-y), cp $(kmod)/*.ko $(SC_DRV_REL_DIR);)

help:
	@echo "target:"
	@echo "  [all]  : all"
	@echo "  install: install"
	@echo "  clean  : clean"
	@echo "  <dir>  : make subdir"
