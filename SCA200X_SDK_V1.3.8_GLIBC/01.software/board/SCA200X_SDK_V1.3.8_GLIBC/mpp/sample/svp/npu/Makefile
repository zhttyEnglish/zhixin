# mpp/sample/svp/Makefile.param
include ../Makefile.param

###########SMP SRCS & INC ############
SMP_SRCS := $(wildcard *.c)
SMP_SRCS += $(wildcard ./sample/*.c)
SMP_SRCS += $(wildcard ../common/*.c)
SMP_SRCS += $(wildcard ./sample_npu_software/*.c)

SMP_CXXSRCS := $(wildcard ../common/*.cpp)
SMP_CXXSRCS += $(wildcard sample_npu_software/*.cpp)

TARGET := sample_npu_main

CFLAGS += -I../common
CFLAGS += -Isample
CFLAGS += -Isample_npu_software
CFLAGS += -O2

CXXFLAGS += -Wno-psabi

LDFLAGS += -lpthread -lrt -lm
LDFLAGS += -Wall -g3 -O3

include ../../$(ARM_ARCH)_$(OSTYPE).mak

