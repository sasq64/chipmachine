
############################## NORMALLY DONT TOUCH ANYTHING BELOW ###########################

TOP_MAKEFILE = $(firstword $(MAKEFILE_LIST))

ifeq ($(OS),Windows_NT)
	MODULES := $(MODULES) $(WIN_MODULES)
	OBJS := $(OBJS) $(WIN_OBJS)
	CFLAGS := $(CFLAGS) $(WIN_CFLAGS) -DWIN32
	LIBS := $(LIBS) $(WIN_LIBS)
	LDFLAGS := $(LDFLAGS) $(WIN_LDFLAGS)
	ifdef WIN_CC
	    CC := $(WIN_CC)
	endif
	ifdef WIN_CXX
		CXX := $(WIN_CXX)
	endif
	ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
    endif
else
    UNAME_S := $(shell uname -s)
    UNAME_N := $(shell uname -n)
    ifeq ($(UNAME_S),Linux)
	    MODULES := $(MODULES) $(LINUX_MODULES)
	    OBJS := $(OBJS) $(LINUX_OBJS)
	    CFLAGS := $(CFLAGS) $(LINUX_CFLAGS) -DLINUX
		LIBS := $(LIBS) $(LINUX_LIBS)
		LDFLAGS := $(LDFLAGS) $(LINUX_LDFLAGS)
	    ifdef LINUX_CC
	    CC := $(LINUX_CC)
	    endif
	    ifdef LINUX_CXX
	    CXX := $(LINUX_CXX)
	    endif
    endif

    ifeq ($(UNAME_N),raspberrypi)
	    MODULES := $(MODULES) $(PI_MODULES)
	    OBJS := $(OBJS) $(PI_OBJS)
	    CFLAGS := $(CFLAGS) $(PI_CFLAGS) -DRASPBERRYPI
		LIBS := $(LIBS) $(PI_LIBS)
		LDFLAGS := $(LDFLAGS) $(PI_LDFLAGS)    
    endif

    ifeq ($(UNAME_S),Darwin)
    endif

    UNAME_P := $(shell uname -p)
    ifeq ($(UNAME_P),x86_64)
    	#GLFW := glfw64
    endif

    ifneq ($(filter %86,$(UNAME_P)),)
        #CCFLAGS += -D IA32
    endif

    ifneq ($(filter arm%,$(UNAME_P)),)
        #CCFLAGS += -D ARM
    endif
endif



ifndef CC
CC := $(PREFIX)gcc$(GCC_VERSION)
endif

ifndef CXX
CXX := $(PREFIX)g++$(GCC_VERSION)
endif

LD := $(CXX)

AR := ar
AS := $(PREFIX)as
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump

export PATH := $(XPATH):$(PATH)


SRC_MODULES := $(addprefix $(SRCDIR),$(MODULES))
CFLAGS += $(addprefix -I,$(SRC_MODULES))
CXXFLAGS := $(CXXFLAGS) $(CFLAGS)


MO := $(patsubst %.cpp,%.o, $(wildcard $(addsuffix /*.cpp, $(SRC_MODULES))))
MO += $(patsubst %.cxx,%.o, $(wildcard $(addsuffix /*.cxx, $(SRC_MODULES))))
MO += $(patsubst %.cc,%.o, $(wildcard $(addsuffix /*.cc, $(SRC_MODULES))))
MO += $(patsubst %.c,%.o, $(wildcard $(addsuffix /*.c, $(SRC_MODULES))))
MO += $(patsubst %.s,%.o, $(wildcard $(addsuffix /*.s, $(SRC_MODULES))))

MODULEOBJS := $(subst $(SRCDIR),,$(MO))

OBJFILES := $(addprefix $(OBJDIR), $(subst ../,,$(OBJS)) $(MODULEOBJS))

start_rule : $(XDEPENDS) $(TARGETDIR) $(OBJFILES) $(TARGETDIR)$(TARGET)$(EXT)

-include $(OBJFILES:.o=.d)

# Implicit rules

$(OBJDIR)%.o : $(SRCDIR)%.c
	@mkdir -p $(@D)
	$(CC) -c -MMD $(CFLAGS) $< -o $@

$(OBJDIR)%.o : $(SRCDIR)%.cpp
	@mkdir -p $(@D)
	$(CXX) -c -MMD $(CXXFLAGS) $< -o $@

$(OBJDIR)%.o : $(SRCDIR)%.cc
	@mkdir -p $(@D)
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(OBJDIR)%.o : $(SRCDIR)%.cxx
	@mkdir -p $(@D)
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(OBJDIR)%.o : $(SRCDIR)%.S
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) $< -o $@

$(OBJDIR)%.o : $(SRCDIR)%.s
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) $< -o $@

$(OBJDIR)%.d : $(SRCDIR)%.c
	@mkdir -p $(@D)
	@$(CC) -MM -MG  -MT '$(OBJDIR)$*.o' $(CFLAGS) $< > $@

$(OBJDIR)%.d : $(SRCDIR)%.cpp
	@mkdir -p $(@D)
	@$(CXX) -MM -MG -MT '$(OBJDIR)$*.o' $(CXXFLAGS) $< > $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

$(OBJDIR)icon.o : icon.rc
	windres $< -O coff -o $@
	
$(OBJDIR) :
	mkdir -p $(OBJDIR)

$(TARGETDIR) :
	mkdir -p $(TARGETDIR)

$(TARGETDIR)$(TARGET).a : $(TARGETDIR) $(OBJFILES) $(DEPS)
	$(AR) r $(TARGETDIR)$(TARGET).a $(OBJFILES)
	 
$(TARGETDIR)$(TARGET).elf : $(OBJFILES) $(DEPS)
	$(LD) $(LDFLAGS) $(OBJFILES) $(LIBS) -o $(TARGETDIR)$(TARGET).elf

$(TARGETDIR)$(TARGET).exe : $(OBJFILES) $(DEPS)
	$(LD) $(LDFLAGS) $(OBJFILES) $(LIBS) -o $(TARGETDIR)$(TARGET).exe

$(TARGETDIR)$(TARGET) : $(OBJFILES) $(DEPS)
	$(LD) -o $(TARGETDIR)$(TARGET) $(LDFLAGS) $(OBJFILES) $(LIBS) 

$(TARGETDIR)$(TARGET).bin : $(TARGETDIR)$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGETDIR)$(TARGET).so : $(OBJFILES) $(DEPS)
	$(LD) $(LDFLAGS) -Wl,-soname,$(TARGET).so -shared -o $(TARGETDIR)$(TARGET).so $(OBJFILES) $(LIBS)

$(TARGETDIR)$(TARGET).dll : $(OBJFILES) $(DEPS)
	$(LD) $(LDFLAGS) -shared -o $(TARGETDIR)$(TARGET).dll $(OBJFILES) $(LIBS)

clean :
	rm -rf $(OBJDIR) $(TARGET)$(EXT)
	
cleandep :
	rm -f $(OBJFILES:.o=.d)

superclean:
	rm -rf $(OBJDIR) $(TARGET)$(EXT) $(addsuffix /*~, $(MODULES)) *.elf *~


#
##
### Nintendo DS Rules
##
#

%.gbfs :
	gbfs $@ data/*


$(TARGETDIR)$(TARGET).ds.gba : $(TARGETDIR)$(TARGET).nds
	dsbuild $< -o $@.tmp
	padbin 256 $@.tmp
	cat $@.tmp $(TARGETDIR)$(TARGET).gbfs > $(TARGETDIR)$(TARGET).ds.gba


# $(TARGETDIR)$(TARGET).nds : $(TARGETDIR)$(TARGET).bin $(TARGETDIR)$(TARGET)7.bin
#	ndstool  -9 $(TARGETDIR)$(TARGET).bin -7 $(TARGETDIR)$(TARGET)7.bin -c $@

#ifdef ARM7BIN
# NDSTFLAGS += -7 $(ARM7BIN)
#endif

$(TARGETDIR)$(TARGET).nds : $(TARGETDIR)$(TARGET).bin
	ndstool $(NDSTFLAGS) -9 $(TARGETDIR)$(TARGET).bin -c $@
	

toflash : $(TARGETDIR)$(TARGET).nds
	mount /mnt/cflash
	cp $(TARGETDIR)$(TARGET).nds /mnt/cflash/_boot_mp.nds
	umount /mnt/cflash

togbfs : $(TARGETDIR)$(TARGET).nds
	cp $(TARGETDIR)$(TARGET).nds $(TARGETDIR)$(TARGET)_temp.nds
	padbin 1024 $(TARGETDIR)$(TARGET)_temp.nds
	cat $(TARGETDIR)$(TARGET)_temp.nds $(TARGETDIR)$(TARGET).gbfs > $(TARGETDIR)$(TARGET).nds

#
##
### PSP Rules
##
#

$(TARGETDIR)$(TARGET).pbp : $(TARGETDIR)$(TARGET).elf
	psp-fixup-imports $<
	mksfo $(PSP_APPNAME) PARAM.SFO
	psp-strip $< -o $<.strip
	pack-pbp $@ PARAM.SFO NULL  NULL NULL NULL NULL $<.strip NULL
	rm $<.strip

