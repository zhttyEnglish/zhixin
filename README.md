************************** ZX_A1 **************************

这是一份针对智芯AI电力网关软件及算法开发环境的简单介绍
   SDK
	|--SCA200X_SDK_Vxxx_GLIBC       # 发布的 sdk 源码(u-boot, kernel, mpp)
    |--toolchain                    # gcc toolchain
	|--dipai_20211204				# 智芯编写的抓流送算法部分的应用
	|--ubuntu_img					# 烧录固件
	|--zh_狄拍（上海）科技有限公司		# 文档
	|--update.tar.gz				# 更新包
	
> SCA200X_SDK_Vxxx_GLIBC 是软件开发包.
  这里面有针对SOC的各个硬件单元的封装库，基础环境，示例代码等.
  软件SDK的开发可以参考《SCA200x 开发环境用户指南》,这个文件在单独外发的Documents目录下，描述了软件SDK的目录结构，环境搭建，示例代码等。
  
  SCA200X_SDK_V1.3.8_GLIBC/01.software/board/SCA200X_SDK_V1.3.8_GLIBC/mpp/sample 目录下有我们的应用，联想算法和智芯demo.
  
  zhixin  				#我们的app 主要功能 从智芯的应用接受算法的结果，分析后上报给融合终端。
  aging_test			#老化测试
  alg_version			#读写算法版本
  net_led				#控制网络灯
  sn_oprate				#读写sn
  version_oprate		#读写版本号
  lenovo_alg			#联想算法 
  
  编译方法：在 sample 目录下执行 make APP_32BIT=y   
  联想算法需要在 lenovo_alg 目录下使用 make APP_32BIT=y 单独编译
  
  kernel 和 boot 编译方法参考 zh_狄拍（上海）科技有限公司/01.software/board/OSDRV/《SCA200x 开发环境用户指南》

> toolchain 是gcc 编译工具链
  gcc-linaro-7.5.0-2019.12--xxx是软件编译工具链。
  
> dipai_20211204 是智芯编写的应用，在 dipai_20211204 目录下执行 make 编译

> ubuntu_img 烧录的固件 包含 boot.img--2G kernel.common.img linux.ext4.sparse.img smartchip-sca200v100-bd2.dtb
	cmd.txt 中有烧录命令  烧录工具见 SCA200X_SDK_V1.3.8_GLIBC/01.software/pc/BurnTool
	烧录方法可参考 zh_狄拍（上海）科技有限公司/01.software/pc/BurnTool 烧写使用指南.pdf
	
	其中 linux.ext4.sparse.img 是盒子的rootfs, 可以用 simg2img 转换成raw image格式, 使用 mount 挂载之后修改，再使用 sudo make_ext4fs -l 4096M -s linux.ext4.sparse.img rootfs 制作成新的固件
	
> update.tar.gz 是升级更新包，目前只支持本地升级，通过盒子的web升级，也可将更新包复制到盒子的 /userdata/ 下再执行update.sh
	更新包中 update_bd2.sh 是选择更新区域的脚本，可按照需求选择boot、kernel或者app
	update_clean.sh 是更新app的具体操作脚本，将包里的文件按照目录进行替换，可以按照需求进行编写。