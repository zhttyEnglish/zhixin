include cfg.mk

.PHONY: all build clean

all: build

build:
	@rm -rf release
	@$(MAKE) -C opensource
	@$(MAKE) -C tools
	@$(MAKE) -C rootfs_scripts

clean:
	@$(MAKE) -C opensource clean
	@$(MAKE) -C tools clean
	@$(MAKE) -C rootfs_scripts clean
	@rm -rf release
