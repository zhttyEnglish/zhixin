include ../../../cfg.mk

CFLAGS += -Wall

build: $(OBJS)
	@$(CC) $(SC_LDFLAGS) $(OBJS) -o $(TARGET)

%.o:%.c
	@$(CC) -c $(SC_CFLAGS) $(CFLAGS) $^

install: build
ifneq ($(DSTDIR),)
	@cp $(TARGET) $(DSTDIR)
endif

clean:
	@rm -f $(TARGET) *.o
