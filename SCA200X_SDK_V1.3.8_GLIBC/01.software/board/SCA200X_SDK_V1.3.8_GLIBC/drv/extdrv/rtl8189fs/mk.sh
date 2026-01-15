#!/bin/bash

## fix SDK_PATH to your path
#SDK_PATH=/home/proj/sdk_v1.3
SDK_PATH=$(realpath ../../../)

KSRCK=$SDK_PATH/osdrv/opensource/kernel
KSRC=$KSRCK/linux-linaro-stable-lsk-v4.9-17.07/build/

make -C $KSRCK
make -j ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- KSRC=$KSRC
