#!/bin/sh

report_error()
{
	echo "******* Error: There's something wrong, please check! *****"
	exit 1
}

insko()
{
	local mmz_start=$1
	local mmz_size="$2M"

	insmod /komod/sc_osal.ko anony=1 mmz=anonymous,0,$mmz_start,$mmz_size || report_error
	insmod /komod/sc_procfs.ko
	insmod /komod/sc_mpp_drv.ko
	insmod /komod/sc_vb.ko
	insmod /komod/sc_sys.ko
	insmod /komod/sc_mpp_proc_ctrl.ko
	insmod /komod/sc_scaler.ko
	insmod /komod/sc_ifc.ko
	insmod /komod/sc_npu.ko
	insmod /komod/sc_gdc.ko
	insmod /komod/sc_rcu.ko
}

rmko()
{
	rmmod sc_rcu
	rmmod sc_gdc
	rmmod sc_npu
	rmmod sc_ifc
	rmmod sc_scaler
	rmmod sc_mpp_proc_ctrl
	rmmod sc_sys
	rmmod sc_vb
	rmmod sc_mpp_drv
	rmmod sc_procfs
	rmmod sc_osal
}

mpp_init()
{
	# Enable the following scripts if you run IPC demo
	hal &
	mpi &
	sleep 3
}

mpp_exit()
{
	killall hal
	killall mpi
	sleep 1
}

calc_mmz_sub()
{
	local mem_start=$1
	local mem_size=$2
	local osmem_size=$3
	local mmz_start=0
	local mmz_size=0

	mmz_start=$(( mem_start/1024/1024 + osmem_size ))
	mmz_start=$(printf "0x%x00000" $mmz_start)

	mmz_size=$((mem_size - osmem_size))
	echo "mem_start: $mem_start, mem_size: $mem_size M"
	echo "osm_start: $mem_start, osm_size: $osmem_size M"
	echo "mmz_start: $mmz_start, mmz_size: $mmz_size M"

	# out
	G_MMZ_START=$mmz_start
	G_MMZ_SIZE=$mmz_size
}

calc_mmz()
{
	if [ -z $G_OSMEM_SIZE ]; then
		echo "[error] os mem size is null"
		exit 1;
	fi

	if [ -z $G_MEM_SIZE ] ; then
		echo "[error] total mem size is null"
		exit 1;
	fi

	if [ $G_OSMEM_SIZE -ge $G_MEM_SIZE ] ; then
		echo "[err] os mem size:$G_OSMEM_SIZE, over total mem size:$G_MEM_SIZE"
		exit;
	fi

	calc_mmz_sub $G_MEM_START $G_MEM_SIZE $G_OSMEM_SIZE

}

load_usage()
{
	echo "Usage:  ./loadsca200v100 [-option]"
	echo "options:"
	echo "    -i                       insert modules"
	echo "    -r                       remove modules"
	echo "    -a                       remove modules first, then insert modules"
	echo "    -h                       help information"
	echo -e "for example: ./loadsca200v100 -i\n"
}

parse_arg()
{
	while [ $# -gt 0 ]; do
		arg=$1
		shift

		case $arg in
			"-i")
				G_ARG_INSMOD=1;
				;;
			"-r")
				G_ARG_RMMOD=1;
				;;
			"-a")
				G_ARG_INSMOD=1;
				G_ARG_RMMOD=1;
				;;
			"-h")
				load_usage;
				;;
		esac
	done
}

####################Variables Definition##########################

#DDR start:0x20000000
G_MEM_START=0x20000000         # phy mem start

# TOTAL_MEM, OS_MEM: uboot bootargs -> linux cmdline -> init env
G_MEM_SIZE=$TOTAL_MEM          # total mem. unit is M byte.
G_OSMEM_SIZE=$OS_MEM           # os mem. unit is M byte.

## clac out var
G_MMZ_START=0          # mmz start addr
G_MMZ_SIZE=0           # mmz size

G_ARG_INSMOD=0
G_ARG_RMMOD=0
##################################################################

if [ $# -lt 1 ]; then
	load_usage;
	exit 0;
fi

parse_arg "$@"
calc_mmz

if [ $G_ARG_RMMOD -eq 1 ]; then
	mpp_exit
	rmko;
fi

if [ $G_ARG_INSMOD -eq 1 ]; then
	insko $G_MMZ_START $G_MMZ_SIZE
	mpp_init
fi

