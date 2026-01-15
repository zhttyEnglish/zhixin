#sample Makefile

include ../Makefile.param
SMP_INC :=  -I opencv/include
CFLAGS += $(SMP_INC)

LDFLAGS   += -L opencv/lib -lopencv_core -lopencv_imgproc -lopencv_imgcodecs  -lopencv_highgui


SMP_SRCS := $(wildcard *.cpp)
#SMP_SRCS += $(wildcard ./sample/*.c)
TARGET := sample_gdc_main

include ../$(ARM_ARCH)_$(OSTYPE).mak
