include ../cfg.mk

sub_dirs := $(SC_UBOOT_SRC_DIR)
sub_dirs += $(SC_KERNEL_SRC_DIR)
sub_dirs += $(SC_BUSYBOX_SRC_DIR)

sub_build := $(patsubst %,%_build,$(sub_dirs))
sub_clean := $(patsubst %,%_clean,$(sub_dirs))


.PHONY: all clean

all: $(sub_build)
clean: $(sub_clean)

%_build:
	@cd $(patsubst %_build,%,$@); $(MAKE) install;

%_clean:
	@cd $(patsubst %_clean,%,$@); $(MAKE) clean;

