
CC=arm-linux-gnueabihf-gcc
CPP=arm-linux-gnueabihf-g++

# target source
SRC  := $(wildcard *.c) $(wildcard lenovo_alg/*.c)
OBJ  := $(SRC:%.c=%.o)

CPPSRC  := $(wildcard *.cpp) $(wildcard lenovo_alg/*.cpp)
CPPOBJ  := $(CPPSRC:%.cpp=%.o)

INC_FLAGS = -I . -I ./inc -I opencv_release32/include -I ./lenovo_alg

INC_FLAGS += -I /home/dp/samba/zhixin/sca200x_ubuntu_sdk/SCA200X_SDK_V1.3.8_GLIBC/01.software/board/SCA200X_SDK_V1.3.8_GLIBC/mpp/include
INC_FLAGS += -I /home/dp/samba/zhixin/sca200x_ubuntu_sdk/SCA200X_SDK_V1.3.8_GLIBC/01.software/board/SCA200X_SDK_V1.3.8_GLIBC/mpp/include

LDFLAGS += -L ./lib -lsc_rtsplive  -lliveMedia  -lBasicUsageEnvironment -lgroupsock -lUsageEnvironment -lstdc++ -lsc_vdec 
LDFLAGS += -L ./lenovo_alg  -lsample_npu -lcjson -lutils -lssl -lcrypto
LDFLAGS += -L  ./opencv_release32/lib/  -lopencv_imgproc -lopencv_imgcodecs -lopencv_core 
LDFLAGS += -L /home/dp/samba/zhixin/sca200x_ubuntu_sdk/SCA200X_SDK_V1.3.8_GLIBC/01.software/board/SCA200X_SDK_V1.3.8_GLIBC/mpp/lib32 -lnpu -lrcu -lmpi

LDFLAGS +=-lpthread -lm  -lstdc++ -lm -lpthread  -O2


TARGET := test_main

.PHONY : clean all

all: $(TARGET)

$(TARGET):$(OBJ) $(CPPOBJ)
	$(CC) -o $(TARGET) $(OBJ) $(CPPOBJ) $(LDFLAGS)

$(OBJ): $ %.o: %.c
	$(CC) -c $< -o $@  $(INC_FLAGS) $(CFLAGS)

$(CPPOBJ): $ %.o: %.cpp
	$(CPP) -c $< -o $@  $(INC_FLAGS) $(CXXFLAGS) -Wno-psabi

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJ) $(CPPOBJ)


