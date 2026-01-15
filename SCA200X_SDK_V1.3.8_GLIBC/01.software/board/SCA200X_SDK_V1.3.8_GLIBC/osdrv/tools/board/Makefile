include ../../cfg.mk

sub_dirs-y :=
sub_dirs-  :=

sub_dirs-y += gpio
sub_dirs-y += i2c
sub_dirs-y += spi
sub_dirs-y += pinctrl
sub_dirs-y += pwm
sub_dirs-y += watchdog
sub_dirs-  += gdb

sub_build := $(patsubst %,%_build,$(sub_dirs-y))
sub_install := $(patsubst %,%_install,$(sub_dirs-y))
sub_clean := $(patsubst %,%_clean,$(sub_dirs-y)$(sub_dirs-))

all: install

build: $(sub_build)
install: $(sub_install)
clean: $(sub_clean)

%_build:
	@cd $(patsubst %_build,%,$@); $(MAKE);

%_install: %_build
	@mkdir -p $(SC_TOOLS_BD_REL_DIR)
	@cd $(patsubst %_install,%,$@); $(MAKE) install DSTDIR=$(SC_TOOLS_BD_REL_DIR);

%_clean:
	@cd $(patsubst %_clean,%,$@); $(MAKE) clean;

