include ../Makefile.param

###########SMP SRCS & INC ############
SMP_SRCS := $(wildcard $(ARM_ARCH)/*.c)
SMP_SRCS += $(wildcard *.c)
SMP_INC := -I$(CURDIR)

CFLAGS += $(SMP_INC)
TARGET := sample_vo_main

include ../$(ARM_ARCH)_$(OSTYPE).mak
