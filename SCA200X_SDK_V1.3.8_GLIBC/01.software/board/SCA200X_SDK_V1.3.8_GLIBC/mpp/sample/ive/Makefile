#sample Makefile

include ../Makefile.param
SMP_INC := -I sample
CFLAGS += $(SMP_INC)

SMP_SRCS := $(wildcard *.c)
SMP_SRCS += $(wildcard ./sample/*.c)
TARGET := sample_ive_main

include ../$(ARM_ARCH)_$(OSTYPE).mak
