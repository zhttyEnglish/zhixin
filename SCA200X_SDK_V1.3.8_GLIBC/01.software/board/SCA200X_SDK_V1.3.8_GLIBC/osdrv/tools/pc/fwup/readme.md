# README

## 概述

1. 拷贝固件组件到 images 目录下，默认固件为：

* boot.img: u-boot
* kernel.img: 内核
* rootfs.512M.ext4.sparse.img: rootfs

2. 生成可升级的固件包：

```sh
./gen_fwup.sh
```

## firmware.its 详解

its 用于 dtc 打包固件：


```c
	description = "boot";
	arch = "arm";
	compression = "none";

	data = /incbin/("../images/boot.img");
	type = "standalone";
	load = <0x00000000>;
	entry = <0x1>;
	hash@1 {
		algo = "sha1";
	};
```

description: 节点名字，无限制，比如 "boot", "boot_sec", "com1".
arch: 默认为 "arm"即可。

compression: 表示文件的压缩类型
 * none: 表明文件是没有压缩的
 * gzip: 表明文件是gzip 压缩文件
data: 节点文件路径。
type: 节点类型。
 * standalone: 表示 u-boot, u-boot.env 二进制数据等. 如 boot.img, env.img. rootfs.ext4.raw.img
 * kernel: 表示内核。kernel.img
 * ramdisk/filesystem：表示: 表示 ext4 sparse image 镜像文件. rootfs.ext4.sparse.img

load: mmc 偏移，该节点内容写到mmc "entry" 区的 load 位置。
  --> mmc 块地址。 一块为 512Byte
entry: mmc 区。
  --> 0：mmc user 区。 除 boot.img 外的文件。
  --> 1：mmc boot0 区。用于放 boot.img.

3. 升级过程指示灯.

  使用 BGA636 K20: PIN131 FUNC0 GPIO1_17 接 LED。
  升级开始 GPIO1_17 置1, 高电平.
  升级完成 GPIO1_17 闪烁.

4. u-boot 下手动升级

  setenv ipaddr 192.168.100.126; setenv serverip 192.168.100.140;setenv ethaddr a0:b0:c1:02:03:04
  tftpboot 28000000  smartchip_fw.bin && fwup dram 28000000


