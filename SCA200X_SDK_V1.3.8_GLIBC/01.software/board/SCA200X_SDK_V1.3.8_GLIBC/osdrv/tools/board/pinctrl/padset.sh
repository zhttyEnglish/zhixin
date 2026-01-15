#!/bin/sh

## 使用寄存器改写 pin 的功能
## 只改写了 PB_SW寄存器, 没有改写 PAD_FUNC_SEL 寄存器
## 请使用 pinctrl.c 的输出程序

do_set()
{
    let reg_base=0x01073000
    echo "pinnum   pad_func"

    let pinnum=$1
    let config=$2

    let 'reg_num  = pinnum/3*4'
    let 'reg_part = pinnum%3*10'

    let 'real_addr = reg_base + reg_num'
    let reg_data=`devmem $real_addr`

    let 'mask =~(0x3ff <<reg_part)'
    let 'config &= 0x3ff'
    let 'config <<= reg_part'

    let 'pad_func=(reg_data & mask) | config'
    devmem $real_addr 32 $pad_func

    printf "pinnum=%d, addr=%#x, val=%#x --> %#x\n"  $pinnum $real_addr $reg_data $pad_func
}

check_chip()
{
	if grep "smartchip,smartx-sca200v100" /proc/device-tree/compatible ; then
		return 0;
	fi

	echo "err: not sca200v100"
	exit 1
}

if [ $# -lt 2 ] ; then
    echo "usage: $0 <pin_num> <config>"
    exit 1
fi

check_chip
do_set $1 $2
