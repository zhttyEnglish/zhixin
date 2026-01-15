#------------------------------------------
# Define flags
#------------------------------------------
CFLAGS    += -g -rdynamic -funwind-tables -ffunction-sections -Wall

CXXFLAGS  += -g -Wall

ifneq ($(SHARED_LIBRARY),)
CFLAGS   += -fpic
CXXFLAGS += -fpic
endif

RM := rm


#------------------------------------------
# Define file filter
#------------------------------------------
SRCEXTS = .c .cpp

SOURCES = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))

SRC_C   = $(filter-out %.cpp,$(SOURCES))
SRC_CXX = $(filter-out %.c,$(SOURCES))
OBJS    = $(addsuffix .o, $(basename $(SOURCES)))
DEPS    = $(OBJS:.o=.d)


# Define compile and link variables.
#------------------------------------------
DEPEND.c    = $(CC) -MM $(CFLAGS) -E -MQ $(patsubst %.c, %.o, $<) $<
DEPEND.cxx  = $(CC) -MM $(CXXFLAGS) -E -MQ $(patsubst %.cpp, %.o, $<) $<
COMPILE.c   = $(CC)  $(CFLAGS) -c
COMPILE.cxx = $(CXX) $(CXXFLAGS) -c
LINK.c      = $(CC)  $(CFLAGS) $(LDFLAGS)
LINK.cxx    = $(CXX) $(CXXFLAGS) $(LDFLAGS)


# Define targets
#------------------------------------------
.PHONY: all install_header install clean distclean help lib_install lib_stage


# Delete the default suffixes
.SUFFIXES:

all:  install_header $(PROGRAM) $(STATIC_LIBRARY) $(SHARED_LIBRARY) install


# Rules for creating dependency files (.d).
#------------------------------------------
%.d:%.c
	$(DEPEND.c) >> $@

%.d:%.cpp
	$(DEPEND.cxx) >> $@


# Rules for generating object files (.o).
#----------------------------------------
%.o:%.c
	$(COMPILE.c) $< -o $@

%.o:%.cpp
	$(COMPILE.cxx) $< -o $@


.PHONY: install_header install

install_header:
ifneq ("$(INSTALL_HEADERS)","")
	@cp -u $(INSTALL_HEADERS) $(OUTPUT_APPLICATION_HEADER_DIR)
endif

lib_stage:
	@cp -u $(SHARED_LIBRARY) $(STAGE_DIR)/$(install_sub_dir)

install: $(PROGRAM) $(STATIC_LIBRARY) $(SHARED_LIBRARY)
ifneq ("$(STATIC_LIBRARY)","")
	@cp -u $(STATIC_LIBRARY) $(OUTPUT_APPLICATION_LIB_DIR)
endif
ifneq ("$(SHARED_LIBRARY)","")
	@cp -u $(SHARED_LIBRARY) $(OUTPUT_APPLICATION_LIB_DIR)
endif
ifneq ("$(PROGRAM)","")
	@cp -u $(PROGRAM) $(OUTPUT_APPLICATION_BIN_DIR)
endif


#-------------------------------------
# Rules for generating the executable.
#-------------------------------------
LD_LIBS_START += -Wl,--whole-archive

LD_LIBS_END   += -Wl,--no-whole-archive

ifneq ($(PROGRAM),)
$(PROGRAM):$(OBJS)
ifeq ($(SRC_CXX),)              # Only C program
ifeq ($(LINK_SCRIPT),)
	$(LINK.c) $(LD_LIBS_START) $(OBJS) $(LIBS) -o $@ $(LOCAL_LIB) $(LD_LIBS_END)
else                            # C++, or C and C++ program
	$(AR) -r $(PROGRAM).a $(OBJS)
	$(LINK.c) $(LD_LIBS_START) $(PROGRAM).a $(LIBS) -T $(LINK_SCRIPT) -o $@ $(LOCAL_LIB) $(LD_LIBS_END)
endif
else                            # C++, or C and C++ program
ifeq ($(LINK_SCRIPT),)
	$(LINK.cxx) $(OBJS) $(LIBS) -o $@ $(LOCAL_LIB)
else
	$(LINK.cxx) $(OBJS) $(LIBS) -T $(LINK_SCRIPT) -o $@ $(LOCAL_LIB)
endif
endif
endif

INSTALL_HEADERS  ?=
INSTALL_HEAD_DIR ?= $(TARGET_STAGING_INC)


ifneq ($(STATIC_LIBRARY),)
$(STATIC_LIBRARY): $(OBJS)
ifeq ("$(notdir $(OBJS))", "")
	@echo "using prebuild in sdk ...";
else
	$(AR) -r $@ $(OBJS) $(LIBS);
endif
endif

ifneq ($(SHARED_LIBRARY),)
$(SHARED_LIBRARY):$(OBJS)
ifeq ("$(notdir $(OBJS))", "")
	@echo "using prebuild in sdk ...";
else
	$(CC) -shared $(OBJS) $(LIBS) -o $@;
endif
endif


# if target != clean && target != install_header
ifneq ($(MAKECMDGOALS), clean)
ifneq ($(MAKECMDGOALS), install_header)
ifneq ($(DEPS),)
sinclude $(DEPS)
endif
endif
endif

clean_headers:= $(foreach headers, $(INSTALL_HEADERS), $(patsubst %, $(OUTPUT_APPLICATION_HEADER_DIR)/%, $(notdir $(headers))))
clean:
ifeq ("$(notdir $(OBJS))", "")
	@echo "do not clean in customer build";
else
	rm -rf $(OBJS) $(PROGRAM) $(STATIC_LIBRARY) $(SHARED_LIBRARY) $(DEPS)

ifneq ($(PROGRAM),)
	@$(RM) -f $(OUTPUT_APPLICATION_BIN_DIR)/$(PROGRAM)
endif
ifneq ("$(SHARED_LIBRARY)","")
	@$(RM) -rf $(OUTPUT_APPLICATION_LIB_DIR)/$(SHARED_LIBRARY)
endif
ifneq ("$(STATIC_LIBRARY)","")
	@$(RM) -rf $(OUTPUT_APPLICATION_LIB_DIR)/$(STATIC_LIBRARY)
endif
ifneq ("$(INSTALL_HEADERS)","")
	@$(RM) -rf $(clean_headers)
endif

endif

distclean: clean


# Show help.
help:
	@echo 'Usage: make [TARGET]'
	@echo 'TARGETS:'
	@echo '  all       (=make) compile and link.'
	@echo '  clean     clean objects and the executable file.'
	@echo '  show      show variables (for debug use only).'
	@echo '  help      print this message.'


# Show variables (for debug use only.)
show:
	@echo 'OUTPUT_BIN_DIR :' $(OUTPUT_APPLICATION_BIN_DIR)
	@echo 'PROGRAM        :' $(PROGRAM)
	@echo 'OUTPUT_LIB_DIR :' $(OUTPUT_APPLICATION_LIB_DIR)
	@echo 'STATIC_LIBRARY :' $(STATIC_LIBRARY)
	@echo 'SHARED_LIBRARY :' $(SHARED_LIBRARY)
	@echo 'SRCDIRS        :' $(SRCDIRS)
	@echo 'SOURCES        :' $(SOURCES)
	@echo 'SRC_CXX        :' $(SRC_CXX)
	@echo 'SRC_C          :' $(SRC_C)
	@echo 'OBJS           :' $(OBJS)
	@echo 'DEPS           :' $(DEPS)
	@echo 'DEPEND.c       :' $(DEPEND.c)
	@echo 'DEPEND.cxx     :' $(DEPEND.cxx)
	@echo 'COMPILE.c      :' $(COMPILE.c)
	@echo 'COMPILE.cxx    :' $(COMPILE.cxx)
	@echo 'LINK.c         :' $(LINK.c)
	@echo 'LINK.cxx       :' $(LINK.cxx)

