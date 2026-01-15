comma   := ,
###
# Name of target with a '.' as filename prefix. foo/bar.o => foo/.bar.o
dot-target = $(dir $@).$(notdir $@)

###
# The temporary file to save gcc -MMD generated dependencies must not
# contain a comma
depfile = $(subst $(comma),_,$(dot-target).d)
dep-cflag = -Wp,-MMD,$(depfile),-MT,$@

# target source
OBJS  := $(SMP_SRCS:%.c=%.o)
CXXOBJS := $(SMP_CXXSRCS:%.cpp=%.o)

existing-targets := $(wildcard $(sort $(OBJS) $(COMM_OBJ)))
existing-dep := $(foreach f,$(existing-targets),$(dir $(f)).$(notdir $(f)).d)

CFLAGS += $(COMM_INC)

SAMPLE_LIBS := $(MPI_LIBS)
SAMPLE_LIBS += $(SENSOR_LIBS)
SAMPLE_LIBS += $(AUDIO_LIBA)
SAMPLE_LIBS += $(HAL_LIBS)

SAMPLE_LD_ARG := $(CFLAGS) $(LIBS_LD_CFLAGS)
SAMPLE_LD_ARG += $(LDFLAGS)
SAMPLE_LD_ARG += $(COMM_OBJ) $(OBJS) $(CXXOBJS)
SAMPLE_LD_ARG += $(SENSOR_LDFLAGS)
SAMPLE_LD_ARG += $(MPI_LDFLAGS)
SAMPLE_LD_ARG += -Wl,--start-group $(SAMPLE_LIBS) -Wl,--end-group
SAMPLE_LD_ARG += $(EPRI_LD_CFLAGS)
SAMPLE_LD_ARG += -lpthread -lm -lstdc++ -lsqlite3

TARGET_PATH ?= .

.PHONY : clean all

all: $(TARGET)

$(TARGET):$(COMM_OBJ) $(OBJS) $(CXXOBJS) $(SAMPLE_LIBS) $(SENSOR_LIBSO)
	$(ECHO) "    LD    $@"
	$(Q)$(CC) $(SAMPLE_LD_ARG) -o $(TARGET_PATH)/$@

%.o:%.c
	$(ECHO) "    CC    $@"
	$(Q)$(CC) $(dep-cflag) $(CFLAGS) -c $< -o $@

%.o:%.cpp
	$(ECHO) "    CXX   $@"
	$(Q)$(CXX) $(CFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -f $(TARGET_PATH)/$(TARGET)
	@rm -f $(CXXOBJS)
	@rm -f $(OBJS)
	@rm -f $(COMM_OBJ)
	@rm -f $(existing-dep)

cleanstream:
	@rm -f *.h264
	@rm -f *.h265
	@rm -f *.jpg
	@rm -f *.mjp
	@rm -f *.mp4

ifneq ($(MAKECMDGOALS),clean)
sinclude $(existing-dep)
endif
