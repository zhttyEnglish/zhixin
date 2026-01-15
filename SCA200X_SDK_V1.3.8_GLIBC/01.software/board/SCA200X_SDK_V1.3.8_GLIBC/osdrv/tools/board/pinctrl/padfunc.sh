#!/bin/sh

check_chip()
{
	if grep "smartchip,smartx-sca200v100" /proc/device-tree/compatible ; then
		return 0;
	fi

	echo "err: not sca200v100"
	exit 1
}

check_chip

let reg_base=0x01073000
echo "pinnum   pad_func"

let pinnum=0
while [[ $pinnum != 193 ]];  do

    let 'reg_num = pinnum / 3 * 4'
    let 'reg_part = pinnum % 3 * 10'

    let 'real_addr = reg_base + reg_num'
    let reg_data=`devmem $real_addr`

    let 'pad_func = (reg_data >> reg_part) & 7'

    echo " " $pinnum "      " $pad_func

    let pinnum++
done
