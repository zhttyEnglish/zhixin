include ../Makefile.param

###########SMP SRCS & INC ############
SMP_SRCS := $(wildcard $(ARM_ARCH)/*.c)
SMP_SRCS += $(wildcard *.c)
SMP_SRCS += $(wildcard ../svp/common/*.c)


SMP_SRCS += $(wildcard ../svp/npu/sample_npu_software/*.c)

SMP_CXXSRCS += $(wildcard ../svp/npu/sample_npu_software/*.cpp)


CFLAGS += -I../svp/common
CXXFLAGS += -Wno-psabi

ifeq ($(MPI_SO_FLAG), y)
  ## use libxx.so
  MPI_LDFLAGS += -lnpu
  MPI_LDFLAGS += -lrcu
else
  ## use libxx.a
  MPI_LIBS += $(REL_LIB)/libnpu.a
  MPI_LIBS += $(REL_LIB)/librcu.a
endif
MPI_LIBS += ../svp/npu/libcjson.a

SMP_INC :=

TARGET := sample_vio_main

include ../$(ARM_ARCH)_$(OSTYPE).mak
