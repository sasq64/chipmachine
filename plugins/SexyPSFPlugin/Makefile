OBJDIR=obj/
TARGET=libsexypsf
EXT := .a

OBJS := sexypsf/PsxBios.o sexypsf/PsxCounters.o sexypsf/PsxDma.o \
       sexypsf/Spu.o sexypsf/PsxHw.o sexypsf/PsxMem.o sexypsf/Misc.o \
       sexypsf/R3000A.o sexypsf/PsxInterpreter.o \
       sexypsf/PsxHLE.o sexypsf/spu/spu.o 

#LOCAL_SRC_FILES += $(MY_SOURCES:$(LOCAL_PATH)%=%)

CFLAGS := -Isexypsf -DPSS_STYLE=1
LDLIBS := -lz

include ../../Makefile.inc