ifeq ($(PARAM_FILE), )
     PARAM_FILE:=../../../mpp/Makefile.param
     include $(PARAM_FILE)
endif

STATIC_LIBRARY = libsns_sc2310.a
SHARED_LIBRARY = libsns_sc2310.so


# Define files
#------------------------------------------
SRCDIRS   = . ../sc_gpio

INSTALL_HEADERS :=

OUTPUT_APPLICATION_LIB_DIR = ../libs

# Define flags
#------------------------------------------
CFLAGS  += -I../sc_gpio
CFLAGS  += -I../../../mpp/include

LDFLAGS +=

LOCAL_LIB = -lhal_gpio


# Include make rule file
#------------------------------------------
include ../usr_lib_rules.mk
