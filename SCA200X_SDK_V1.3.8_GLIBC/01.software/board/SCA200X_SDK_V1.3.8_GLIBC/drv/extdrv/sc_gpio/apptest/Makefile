include ../../../../osdrv/cfg.mk

CFLAGS += $(SC_CFLAGS)
LDFLAGS := $(SC_LDFLAGS)

CFLAGS_32 += $(SC_CFLAGS_32)
LDFLAGS_32 := $(SC_LDFLAGS_32)

all:
	$(CC) $(CFLAGS) $(LDFLAGS) -o gpioTest.out gpioTest.c
	$(CC_32) $(CFLAGS_32) $(LDFLAGS_32) -o gpioTest.32.out gpioTest.c

clean:
	@rm -f *.out
