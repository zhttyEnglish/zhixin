this-makefile := $(lastword $(MAKEFILE_LIST))
OSDRV_DIR := $(realpath $(dir $(this-makefile)))
SDK_DIR := $(realpath $(dir $(this-makefile))/..)
MPP_DIR := $(SDK_DIR)/mpp

include $(MPP_DIR)/cfg.mak

SC_MK_JOBS := $(shell nproc)

## option
## build app 32bit
SC_CONFIG_APP_32 := y

## CC
SC_CROSS_COMPILE := $(CONFIGSC_SC_CROSS:"%"=%)
SC_CFLAGS :=
SC_LDFLAGS :=
ifeq ($(SC_CONFIG_APP_32),y)
SC_LDFLAGS += -Wl,-rpath,/lib64:/usr/lib64
endif

CC := $(SC_CROSS_COMPILE)gcc
STRIP := $(SC_CROSS_COMPILE)strip

## CC 32
SC_CROSS_COMPILE_32 := $(CONFIGSC_SC_CROSS_32:"%"=%)
SC_CFLAGS_32 :=
SC_LDFLAGS_32 :=

CC_32 := $(SC_CROSS_COMPILE_32)gcc
STRIP_32 := $(SC_CROSS_COMPILE_32)strip

## uboot
# sca200v100
SC_UBOOT_CFG-$(CONFIGSC_BOARD1_SOM) := sca200v100_bd1_rel_emmc_defconfig
SC_UBOOT_CFG-$(CONFIGSC_BOARD2_DEV) := sca200v100_bd2_rel_emmc_defconfig
SC_UBOOT_CFG-$(CONFIGSC_BOARD3_DEV) := sca200v100_bd3_rel_emmc_defconfig
SC_UBOOT_CFG-$(CONFIGSC_BOARD4_DEV) := sca200v100_bd4_rel_emmc_defconfig
SC_UBOOT_CFG-$(CONFIGSC_SCA200V102_BD1_FAST_DEV) := sca200v102_bd1_fast_rel_emmc_defconfig
# sca200v200
SC_UBOOT_CFG-$(CONFIGSC_SCA200V200_BD1_DEV) := sca200v200_bd1_rel_emmc_defconfig
SC_UBOOT_CFG-$(CONFIGSC_SCA200V200_BD2_DEV) := sca200v200_bd2_rel_emmc_defconfig

SC_UBOOT_ARCH := arm
SC_UBOOT_CFG ?= $(SC_UBOOT_CFG-y)

SC_UBOOT_SRC_DIR ?= uboot
SC_UBOOT_REL_DIR ?= $(OSDRV_DIR)/release/out/uboot

## rtos
SC_RTOS_REL_DIR ?= $(OSDRV_DIR)/release/out/rtos

## kernel
SC_KERNEL_CFG-$(CONFIGSC_SCA200V100) := sca200v100_emmc_defconfig
SC_KERNEL_CFG-$(CONFIGSC_SCA200V200) := sca200v200_emmc_defconfig

SC_KERNEL_ARCH := arm64
SC_KERNEL_CFG ?= $(SC_KERNEL_CFG-y)

SC_KERNEL_SRC_DIR ?= kernel
SC_KERNEL_REL_DIR ?= $(OSDRV_DIR)/release/out/kernel

## kernel driver
SC_DRV_REL_DIR := $(OSDRV_DIR)/release/out/ko
SC_EXTDRV_REL_DIR := $(OSDRV_DIR)/release/out/ko/extdrv

## busybox
SC_BUSYBOX_CFG := smartchip_defconfig

SC_BUSYBOX_SRC_DIR ?= busybox
SC_BUSYBOX_REL_DIR ?= $(OSDRV_DIR)/release/out/busybox

## rootfs
SC_ROOTFS_REL_DIR := $(OSDRV_DIR)/release/out/rootfs

## tools
SC_TOOLS_BD_REL_DIR := $(OSDRV_DIR)/release/out/tools/board
SC_TOOLS_PC_REL_DIR := $(OSDRV_DIR)/release/out/tools/pc
