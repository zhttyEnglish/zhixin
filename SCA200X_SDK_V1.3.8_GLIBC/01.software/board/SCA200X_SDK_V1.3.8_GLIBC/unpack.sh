#!/bin/bash

source scripts/common.sh

run_cmd_untar package/mpp.tgz
run_cmd_untar package/osdrv.tgz
run_cmd_untar package/drv.tgz

run_cmd_untar osdrv/opensource/uboot/u-boot-v2020.10.tgz                     osdrv/opensource/uboot
run_cmd_untar osdrv/opensource/kernel/linux-linaro-stable-lsk-v4.9-17.07.tgz osdrv/opensource/kernel
run_cmd_untar osdrv/opensource/busybox/busybox-1.25.0.tgz                    osdrv/opensource/busybox
