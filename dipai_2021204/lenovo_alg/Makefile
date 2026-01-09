
CC=arm-linux-gnueabihf-gcc
CPP=arm-linux-gnueabihf-g++

# target source
SRC  := $(wildcard *.c)
OBJ  := $(SRC:%.c=%.o)

CPPSRC  := $(wildcard *.cpp)
CPPOBJ  := $(CPPSRC:%.cpp=%.o)

INC_FLAGS = -I . -I ./include

INC_FLAGS += -I /home/stf/samba_share/workspace/nfs/sc_sdk/SCA200X_SDK_V1.3.9_GLIBC/mpp/include
INC_FLAGS += -I /home/stf/samba_share/workspace/nfs/sc_sdk/SCA200X_SDK_V1.3.9_GLIBC/mpp/sample/common
INC_FLAGS += -I  /home/stf/samba_share/workspace/nfs/opencv_release32/include/


LDFLAGS = -L  /home/stf/samba_share/workspace/nfs/opencv_release32/lib/  -lopencv_imgproc -lopencv_imgcodecs -lopencv_core 
LDFLAGS += -L . -lsample_npu -lcjson -lutils -lssl -lcrypto
LDFLAGS += -L /home/stf/samba_share/workspace/nfs/sc_sdk/SCA200X_SDK_V1.3.9_GLIBC/mpp/lib32 -lnpu -lrcu -lmpi
LDFLAGS +=-lpthread -lm  -lstdc++ -lm -lpthread -fopenmp -O2

TARGET := test_ll

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


