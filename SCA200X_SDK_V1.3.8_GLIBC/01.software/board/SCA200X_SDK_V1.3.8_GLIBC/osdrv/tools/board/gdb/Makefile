include ../../../cfg.mk

TOOLS_TOP_DIR := $(shell pwd)

NAME_NCURSES := ncurses-6.0
TAR_NCURSES := ncurses-6.0.tar.gz
INSTALL_DIR := install

NAME_GDB := gdb-7.9.1
TAR_GDB := gdb-7.9.1.tar.gz

BUILD_32 ?= n
ifeq ($(BUILD_32),y)
  OSDRV_CROSS ?= $(SC_CROSS_COMPILE_32:%-=%)
  OSDRV_CROSS_CFLAGS ?= -Wall $(SC_CFLAGS_32)
  OSDRV_CROSS_LDFLAGS ?= $(SC_LDFLAGS_32)
else
  OSDRV_CROSS ?= $(SC_CROSS_COMPILE:%-=%)
  OSDRV_CROSS_CFLAGS ?= -Wall $(SC_CFLAGS)
  OSDRV_CROSS_LDFLAGS ?= $(SC_LDFLAGS)
endif

.PHONY: all
all: gdb

.PHONY: prepare
prepare:
	@rm $(TOOLS_TOP_DIR)/$(NAME_NCURSES) -rf;
	@tar -xf $(TAR_NCURSES);
	@rm $(TOOLS_TOP_DIR)/$(NAME_GDB) -rf;
	@tar -xf $(TAR_GDB);

.PHONY: ncurses
ncurses: prepare
	@cd $(TOOLS_TOP_DIR)/$(NAME_NCURSES)/; \
		./configure --host=$(OSDRV_CROSS) \
		CFLAGS="$(OSDRV_CROSS_CFLAGS)" \
		CPPFLAGS="-P" \
		LDFLAGS="$(OSDRV_CROSS_LDFLAGS)" \
		--prefix=$(TOOLS_TOP_DIR)/$(INSTALL_DIR)/;
	@cd $(TOOLS_TOP_DIR)/$(NAME_NCURSES)/; \
		$(MAKE) -j > /dev/null; \
		$(MAKE) install;

.PHONY: gdb
gdb: ncurses
	@cd $(TOOLS_TOP_DIR)/$(NAME_GDB)/; \
		./configure \
		CFLAGS="$(OSDRV_CROSS_CFLAGS) -lm" \
		LDFLAGS="$(OSDRV_CROSS_LDFLAGS) -L$(TOOLS_TOP_DIR)/$(INSTALL_DIR)/lib" \
		--host=$(OSDRV_CROSS) \
		--disable-tui \
		--prefix=$(TOOLS_TOP_DIR)/$(INSTALL_DIR);
	@cd $(TOOLS_TOP_DIR)/$(NAME_GDB)/; \
		gdb_cv_prfpregset_t_broken=no \
		$(MAKE) -j > /dev/null; \
		$(MAKE) install;

.PHONY: clean
clean: distclean

.PHONY: clean1
clean1:
	@$(MAKE) -C $(TOOLS_TOP_DIR)/$(NAME_GDB) clean
	@$(MAKE) -C $(TOOLS_TOP_DIR)/$(NAME_NCURSES) clean

.PHONY: distclean
distclean:
	@rm $(TOOLS_TOP_DIR)/$(NAME_GDB) -rf;
	@rm $(TOOLS_TOP_DIR)/$(NAME_NCURSES) -rf;
	@rm $(TOOLS_TOP_DIR)/$(INSTALL_DIR) -rf;

help:
	@echo "  make            #for aarch64"
	@echo "  make BUILD_32=y #for arm 32"
