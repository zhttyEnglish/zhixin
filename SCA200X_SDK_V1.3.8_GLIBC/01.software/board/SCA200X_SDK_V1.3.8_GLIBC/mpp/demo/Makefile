
CC=arm-linux-gnueabihf-gcc
CPP=arm-linux-gnueabihf-g++

# target source
SRC  := $(wildcard *.c)
OBJ  := $(SRC:%.c=%.o)

CPPSRC  := $(wildcard *.cpp)
CPPOBJ  := $(CPPSRC:%.cpp=%.o)

INC_FLAGS = -I . -I ./inc

INC_FLAGS += -I /home/dp/samba/zhixin/sca200x_ubuntu_sdk/SCA200X_SDK_V1.3.8_GLIBC/01.software/board/SCA200X_SDK_V1.3.8_GLIBC/mpp/include

LDFLAGS += -L ./lib -lsc_rtsplive  -lliveMedia  -lBasicUsageEnvironment -lgroupsock -lUsageEnvironment -lstdc++ -lpthread
LDFLAGS += -L ./lib -lsc_vdec 
LDFLAGS += -L ./lib 
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
	$(CPP) -c $< -o $@  $(INC_FLAGS) $(CXXFLAGS)

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJ) $(CPPOBJ)


