BIN_DIR  := bin

###############################################################################
CC=gcc
AR=ar
CPP=cpp
LD=ld
RANLIB=ranlib
NM=nm
STRIP=strip

TARGET := make_ext4fs img2simg simg2img
###############################################################################
all:
	@tar xf ext4_utils.tar.bz2
	@test -d $(BIN_DIR)|| mkdir -p $(BIN_DIR)
	@cd ext4_utils; \
		$(MAKE);\
		cp $(TARGET) ../$(BIN_DIR);

clean:
	rm -rf ext4_utils
	rm -rf $(BIN_DIR)

distclean: clean
###############################################################################
.PHONY: clean distclean all
###############################################################################
