ifeq ($(PARAM_FILE), )
   PARAM_FILE:=../Makefile.param
   include $(PARAM_FILE)
endif

sub_dirs :=
sub_dirs += audio
sub_dirs += vio
sub_dirs += vdec
sub_dirs += venc
sub_dirs += vpss
sub_dirs += region
sub_dirs += vgs
sub_dirs += svp
#sub_dirs += snap
sub_dirs += vo
sub_dirs += ive
#sub_dirs += gdc
sub_dirs += desktop_vo

sub_dirs += zhixin
sub_dirs += sn_oprate
sub_dirs += version_oprate
sub_dirs += net_led
sub_dirs += alg_version
sub_dirs += aging_test

#eprt test.
ifeq ($(CONFIGSC_EPRI_TEST_ALL), y)
	sub_dirs += epri_test
endif


sub_build := $(patsubst %,%_build,$(sub_dirs))
sub_clean := $(patsubst %,%_clean,$(sub_dirs))

## sample/smp_linux.mak 使用
## 用于存放 sample 的可执行程序
export TARGET_PATH := $(CURDIR)/out

all: $(sub_build)
clean: $(sub_clean)

%_build: prepare_dir
	@$(MAKE) -C $(patsubst %_build,%,$@);

%_clean: clean_dir
	@$(MAKE) -C $(patsubst %_clean,%,$@) clean;

.PHONE: prepare_dir clean_dir
prepare_dir:
	@mkdir -p $(TARGET_PATH)

clean_dir:
	@rm -rf $(TARGET_PATH)
