#!/bin/bash

install_tool()
{
	local tools=$1
	local tarf=${tools}.tar.xz
	local topdir=/opt/toolchain/7.5.0
	local tooldir=$topdir/$tools
	local bindir=$tooldir/bin

	tarx()
	{
		if [ -d $tooldir ] ; then
			echo "Delete exist directory $tooldir"
			rm -rf $tooldir
		fi

		echo "Extract cross tools ..."
		mkdir -p $topdir
		tar -xf $tarf -C $topdir
	}

	setpath()
	{
		sed -i "/\/$tools\//d" /etc/profile

		cat >> /etc/profile <<- EOF

			# $(date)
			export PATH="$bindir:\$PATH"
			EOF
	}

	tarx
	setpath
}

install_tool gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu
install_tool gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf
