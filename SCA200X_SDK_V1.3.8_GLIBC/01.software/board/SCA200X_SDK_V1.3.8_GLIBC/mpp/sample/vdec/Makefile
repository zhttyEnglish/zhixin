include ../Makefile.param

###########SMP SRCS & INC ############
SMP_SRCS := $(wildcard $(ARM_ARCH)/*.c)
SMP_SRCS += $(wildcard *.c)
SMP_INC := -I.

TARGET := sample_vdec_main

include ../$(ARM_ARCH)_$(OSTYPE).mak
