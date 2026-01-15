#!/bin/sh
## 基于寄存器的 gpio 显示统计
## 只用于 sca200v100

get_gpio_addr()
{
	local gpionum=$1

	let 'groupno = gpionum >> 5'
	let 'partno = (gpionum % 32 ) >> 3'
	let 'pinno=( (gpionum%32) % 8 )'
	let 'addr_gpio = BASE_ADDR_GPIO + groupno * 0x20000'

	let 'offset_out_v    =  0x00 + partno * 0xc'
	let 'offset_direction=  0x04 + partno * 0xc'
	let 'offset_in_v     =  0x50 + partno * 0x4'

	let 'addr_dir = addr_gpio + offset_direction'
	let 'addr_vin = addr_gpio + offset_in_v'
	let 'addr_vout = addr_gpio + offset_out_v'

	GPIO_PIN_NO=$pinno
	GPIO_ADDR_DIR=$addr_dir
	GPIO_ADDR_VIN=$addr_vin
	GPIO_ADDR_VOUT=$addr_vout
}

get_gpio_val()
{
	local gpionum=$1

	get_gpio_addr $gpionum
	let dir_tmp=`devmem $GPIO_ADDR_DIR`

	let 'gpio_direction = (dir_tmp >> GPIO_PIN_NO) & 1'
	if [ $gpio_direction -eq 0 ] ; then
		## in
		GPIO_DIR="in"
		let 'addr_value = GPIO_ADDR_VIN'
	else
		## out
		GPIO_DIR="out"
		let 'addr_value = GPIO_ADDR_VOUT'
	fi

	let value_tmp=`devmem $addr_value`
	let 'GPIO_VAL = (value_tmp >> GPIO_PIN_NO) & 1'
}

get_gpio_name()
{
	local gpionum=$1

	let 'groupno = gpionum >> 5'
	let 'pinno = (gpionum%32)'

	GPIO_NAME="gpio${groupno}_${pinno}"
}

#$1: gpionum
#$2: b_echo_head
get_gpio_addr_one()
{
	local gpionum=$1
	local b_echo_head=$2

	get_gpio_addr $gpionum
	get_gpio_name $gpionum

	if [ $b_echo_head -eq 1 ] ; then
		printf "gpionum \t name \t direction \t value_in \t value_out\n"
	fi
	printf "%d \t %s \t 0x%08x \t 0x%08x \t 0x%08x\n" \
		$gpionum $GPIO_NAME \
		$GPIO_ADDR_DIR $GPIO_ADDR_VIN $GPIO_ADDR_VOUT
}

#$1: gpionum
#$2: b_echo_head
get_gpio_val_one()
{
	local gpionum=$1
	local b_echo_head=$2

	get_gpio_val $gpionum
	get_gpio_name $gpionum

	if [ $b_echo_head -eq 1 ] ; then
		printf "gpionum \t name \t direction \t value\n"
	fi
	printf "%d \t %s \t %s \t %d\n" \
		$gpionum $GPIO_NAME $GPIO_DIR $GPIO_VAL
}

get_gpio_addr_all()
{
	local gpionum=0
	local b_echo_head=1

	while [[ $gpionum != 128 ]] ; do
		get_gpio_addr_one $gpionum $b_echo_head
		let gpionum++
		b_echo_head=0
	done
}

get_gpio_val_all()
{
	local gpionum=0
	local b_echo_head=1

	while [[ $gpionum != 128 ]] ; do
		get_gpio_val_one $gpionum $b_echo_head
		let gpionum++
		b_echo_head=0
	done
}

#$1: gpio num | all
do_val()
{
	local num=$1

	case $num in
		all)
			get_gpio_val_all
			;;
		*)
			get_gpio_val $num
			;;
	esac
}

#$1: gpio num | all
do_addr()
{
	local num=$1

	case $num in
		all)
			get_gpio_addr_all
			;;
		*)
			get_gpio_addr $num
			;;
	esac
}

check_chip()
{
	if grep "smartchip,smartx-sca200v100" /proc/device-tree/compatible ; then
		return 0;
	fi

	echo "err: not sca200v100"
	exit 1
}

usage()
{
	echo "$0 <cmd> [arg]:"
	echo "$0 val <gpio num> : get one gpio's val"
	echo "$0 val all        : get all gpio's val"
	echo "$0 addr <gpio num>: get one gpio's register addr"
	echo "$0 addr all       : get all gpio's register addr"
}

let BASE_ADDR_GPIO=0x08400000

if [ $# -lt 2 ] ; then
	usage
	exit 0
fi

cmd=$1
num=$2

check_chip
case $cmd in
	addr)
		do_addr $num
		;;
	val)
		do_val $num
		;;
	*)
		usage
		exit 0
		;;
esac

