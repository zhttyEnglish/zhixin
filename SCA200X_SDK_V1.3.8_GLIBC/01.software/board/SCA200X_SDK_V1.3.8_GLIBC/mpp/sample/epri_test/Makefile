include ../Makefile.param

SMP_SRCS := $(wildcard *.c) $(wildcard */*.c) $(wildcard */*/*.c)
SMP_CXXSRCS:= $(wildcard *.cpp) $(wildcard */*.cpp) $(wildcard */*/*.cpp) $(wildcard */*/*/*.cpp)
SMP_INC := -I openssl
SMP_INC += -I opencv_include/
SMP_INC += -I test1_issue/
SMP_INC += -I test5_audio_alarm/
SMP_INC += -I test5_audio_alarm/inc/ffmpeg
SMP_INC += -I test6_track/
SMP_INC += -I test6_track/alg
SMP_INC += -I test6_track/alg/iou
SMP_INC += -I test6_track/ptz
SMP_INC += -I test7_vi/
SMP_INC += -I test8_distance/
SMP_INC += -I test9_filter/
SMP_INC += -I inc

#SMP_FFMPEGLIB := -L test5_audio_alarm/ffmpeg -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswscale 
CFLAGS += $(SMP_INC)
#CFLAGS += $(SMP_FFMPEGLIB)

TARGET := sample_epri_test_main

include ../$(ARM_ARCH)_$(OSTYPE).mak
