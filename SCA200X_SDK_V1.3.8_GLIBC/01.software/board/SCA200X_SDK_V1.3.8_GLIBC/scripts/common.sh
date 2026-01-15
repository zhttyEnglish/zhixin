#!/bin/bash

#set -e

# $1: command
# $2: total
# $3: command to calc totals
#
# usage:
#  run_command_progress_float <cmd> <total> [cmd to calc total]
#
run_command_progress_float()
{
	local ucmd=$1
	local utotal=$2
	local utotal_cmd=$3

	local readonly RCP_RANGE=50
	local rcp_tmp=0

	## 行数
	local rcp_lines=0
	local rcp_nextpos=1
	local rcp_total=0

	# @bar_percent 为 0-100
	local bar_percent=0
	# @bar_pound_cnt 为 "#" 个数
	local bar_pound_cnt=0
	# @bar_pound 为 "####"
	local bar_pound=
	# @bar_base 为 "[    ]------...|"
	local bar_base=

	echo "run_command_progress_float: '$ucmd'"

	## 1. 计算 rcp_total
	if [ -n "$utotal_cmd" ] ; then
		echo -n "Initializing progress bar ..."
		rcp_total=`eval $utotal_cmd`;
		echo -n "\r"
		[ -z "$rcp_total" ] && rcp_total=1
	else
		[ -n "$utotal" ] && rcp_total=$utotal
	fi

	[ -z "$rcp_total" ] && rcp_total=1
	[ $rcp_total -le 0 ] && rcp_total=1

	## 2. 生成 "[    ]--------------...|"
	bar_base="[    ]"
	while [ $rcp_tmp -lt $RCP_RANGE ]; do
		bar_base="$bar_base-"
		((++rcp_tmp))
	done
	bar_base="${bar_base}|"
	printf "\r$bar_base\r"

	## 3. 生成 "[ 50%]##------------...|"
	set +e
	eval $ucmd | while read line; do
		((++rcp_lines))

		if [ $rcp_lines -ge $rcp_nextpos ] ; then
			## 3.1 计算 bar_percent,
			((bar_percent = (rcp_lines * 101 - 1) / rcp_total))
			((bar_pound_cnt = ( rcp_lines * ( RCP_RANGE + 1 ) - 1 ) / rcp_total))
			((rcp_nextpos = ((bar_percent + 1) * rcp_total ) / 100))
			[ $rcp_nextpos -gt $rcp_total ] && rcp_nextpos=$rcp_total

			## 3.2 生成 "[ 50%]##------------...|"
			rcp_tmp=0
			bar_pound=""
			while [ $rcp_tmp -lt $bar_pound_cnt ] ; do
				bar_pound="$bar_pound#"
				((++rcp_tmp))
			done
			printf "\r$bar_base\r[%3d%%]$bar_pound\r" $bar_percent
		fi
	done
	set -e

	echo ""
}

# $1: tarfile
# $2: dst dir. 可选
run_cmd_untar()
{
	local tarfile=$1
	local dstdir=$2

	local tar_args=

	if [ -n "$dstdir" ] ; then
		tar_args="-C $dstdir"
	fi

	run_command_progress_float "tar -xvf $tarfile $tar_args" 0 "tar -tf $tarfile| wc -l"
}
