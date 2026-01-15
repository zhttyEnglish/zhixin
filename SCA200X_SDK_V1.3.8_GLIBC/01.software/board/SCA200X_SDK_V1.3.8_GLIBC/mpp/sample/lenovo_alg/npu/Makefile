# mpp/sample/svp/Makefile.param
include ../Makefile.param
###########SMP SRCS & INC ############
SMP_SRCS := $(wildcard *.c)
SMP_SRCS += $(wildcard ./sample/*.c)
SMP_SRCS += $(wildcard ../common/*.c)
SMP_SRCS += $(wildcard ./sample_npu_software/*.c)

SMP_CXXSRCS := $(wildcard ../common/*.cpp)
SMP_CXXSRCS += $(wildcard sample_npu_software/*.cpp)

TARGET := lenovo_alg

CFLAGS += -I../common
CFLAGS += -Isample
CFLAGS += -Isample_npu_software

CFLAGS += -O2

CXXFLAGS += -Wno-psabi

LDFLAGS += -lpthread -lrt -lm
LDFLAGS += -Wall -g3 -O3

###opencv
CFLAGS   += -I./opencv_release32/include
CXXFLAGS += -I./opencv_release32/include

LDFLAGS += -L./opencv_release32/lib
LDFLAGS += -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc

###openssl32
CFLAGS   += -I./openssl_32/include
CXXFLAGS += -I./openssl_32/include

LDFLAGS += -L./openssl_32/lib
LDFLAGS += -lssl -lcrypto


include ../../$(ARM_ARCH)_$(OSTYPE).mak

# ###########################################################
# # --- Build static library: libsample_npu.a ---
# ###########################################################

# # 自动从 SMP_SRCS 里筛选 sample_npu.c
# LIB_NPU_SRCS := $(filter %sample_npu.c, $(SMP_SRCS))
# LIB_NPU_OBJS := $(LIB_NPU_SRCS:.c=.o)

# libsample_npu.a: $(LIB_NPU_OBJS)
# 	@echo "AR  libsample_npu.a"
# 	$(AR) rcs $@ $^

# ###########################################################
# # --- Build everything ---
# ###########################################################

# all: $(TARGET) libsample_npu.a