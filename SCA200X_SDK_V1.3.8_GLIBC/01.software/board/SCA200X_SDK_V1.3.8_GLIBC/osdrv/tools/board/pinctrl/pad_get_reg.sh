#!/bin/sh

## desc: 显示 pin 所对应的寄存器地址
## $1: pinnum
## return: none
get_reg_addr()
{
    let reg_base=0x01073000

    let pinnum=$1

    let 'reg_num  = pinnum/3*4'
    let 'reg_shift = pinnum%3*10'

    let 'real_addr = reg_base + reg_num'
    let 'mask =~(0x3ff <<reg_shift)'

    printf "pinnum=%d, addr=%#x, shift=%d, mask=%#x\n"  $pinnum $real_addr $reg_shift $mask
}

check_chip()
{
	if grep "smartchip,smartx-sca200v100" /proc/device-tree/compatible ; then
		return 0;
	fi

	echo "err: not sca200v100"
	exit 1
}

if [ $# -lt 1 ] ; then
    echo "usage: $0 <pin_num>"
    exit 1
fi

check_chip
get_reg_addr $1
