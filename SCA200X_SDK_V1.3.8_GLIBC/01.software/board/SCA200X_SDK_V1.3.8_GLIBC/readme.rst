sdk 目录结构
============

未解包
------

目录 ::

    sdk
    |-- clean.sh
    |-- image/                             # 提前编译好的文件
    |   |-- boot.img
    |   |-- kernel.img
    |   `-- smartchip-sca200*.dtb
    |-- package/
    |   |-- drv.tgz
    |   |-- mpp.tgz
    |   `-- osdrv.tgz
    |-- readme.rst
    |-- scripts/
    |   `-- common.sh
    `-- unpack.sh


mpp 解包
--------

目录 ::

    mpp/
    |-- Makefile.param                 # makefile 配置文件
    |-- bin/                           # mpp server 程序
    |-- cfg.mak                        # makefile 配置文件
    |-- include/                       # mpp 库头文件
    |-- ko/                            # 提前编译的内核驱动
    |-- lib32/                         # 32位 mpp 库
    |-- lib64/                         # 64位 mpp 库
    |-- sample/                        # mpp 库的使用示例
    |-- sensor/                        # sensor 库源码
    `-- tools/                         # mpp 库的使用示例

osdrv 解包
----------

目录 ::

    osdrv/
    |-- Makefile
    |-- cfg.mk
    |-- opensource/                   # u-boot, linux kernel, busybox
    |-- rootfs_scripts/               # rootfs libc, rootfs 起动脚本
    `-- tools/                        # pc 及 板子上的工具

编译
====

cd sdk
./unpack.sh
make -C mpp/sample
make -C osdrv
make -C drv

Makefile 配置文件
-----------------

mpp/cfg.mak
osdrv/cfg.mk

mpp 示例编译
------------

# 修改SENSOR?_TYPE
vim mpp/sample/Makefile.param
make -C mpp/sample

u-boot
------

默认进入命令行的密码为: sss
