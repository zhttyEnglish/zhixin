#
# Makefile for Invensense MPU6509 device.
#

obj-$(CONFIG_INV_MPU6509_IIO) += inv-mpu6509.o
inv-mpu6509-objs := inv_mpu_core.o inv_mpu_ring.o inv_mpu_trigger.o inv_mpu_ioctl.o

ifdef CONFIG_INV_MPU6509_I2C
	inv-mpu6509-objs += designware_i2c.o
else
ifdef CONFIG_INV_MPU6509_SPI
	inv-mpu6509-objs += designware_spi.o
endif
endif

obj-$(CONFIG_INV_MPU6509_I2C) += inv-mpu6509-i2c.o
inv-mpu6509-i2c-objs := inv_mpu_i2c.o

obj-$(CONFIG_INV_MPU6509_SPI) += inv-mpu6509-spi.o
inv-mpu6509-spi-objs := inv_mpu_spi.o
