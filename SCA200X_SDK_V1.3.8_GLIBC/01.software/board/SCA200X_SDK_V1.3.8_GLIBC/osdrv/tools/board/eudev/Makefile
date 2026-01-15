EUDEV_SRC := eudev-3.2.11
EUDEV_TGZ := $(EUDEV_SRC).tar.gz

INS_DIR   := $(CURDIR)/_install
BD_DIR    := _build

CROSS_COMPILE :=aarch64-linux-gnu-

HOST := $(CROSS_COMPILE:%-=%)
LDFLAGS :=-Wl,-rpath,/lib64:/usr/lib64

N := $(shell nproc)

info:
	@echo "xx: $(HOST)"
	@echo "xx: $(LDFLAGS)"

all:

.PHONY: unzip
unzip:
	@wget 'https://github.com/eudev-project/eudev/releases/download/v3.2.11/eudev-3.2.11.tar.gz'
	@rm -rf $(EUDEV_SRC)
	@tar xzf $(EUDEV_TGZ)

.PHONY: config
config:
	@rm -rf $(BD_DIR)
	@mkdir $(BD_DIR)
	@cd $(BD_DIR); ../$(EUDEV_SRC)/configure \
		LDFLAGS=$(LDFLAGS) \
		--host=$(HOST) \
		--prefix= \
		--sbindir=/sbin \
		--libexecdir=/lib \
		--with-rootlibdir=/lib \
		--disable-manpages \
		--disable-introspection \
		--disable-dependency-tracking \
		--disable-static \
		--enable-shared \
		--disable-kmod \
		--disable-blkid \
		--enable-split-usr \
		--enable-rule-generator \
		--disable-hwdb \
		--disable-selinux

.PHONY: build
build:
	@rm -rf $(INS_DIR)
	@mkdir -p $(INS_DIR)
	@$(MAKE) -j$(N) -C $(BD_DIR)
	@$(MAKE) -j$(N) -C $(BD_DIR) install DESTDIR=$(INS_DIR)

.PHONY: all
all:
	@$(MAKE) unzip
	@$(MAKE) config
	@$(MAKE) build

.PHONY: clean
clean:
	@rm -rf $(EUDEV_SRC)
	@rm -rf $(BD_DIR)
	@rm -rf $(INS_DIR)
