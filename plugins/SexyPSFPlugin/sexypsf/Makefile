#
# sexyPSF Makefile for xmms
#

all: sexypsf

VERSION = 0.4.8
# CPU.  ix86 or ppc, or LSBFIRST for non-x86 LSB-first platforms or MSBFIRST

CPU = ix86
CC = gcc 
RM = rm -f

LIBS = -shared -lz
OPTIMIZE = -O2 -finline-functions -ffast-math
FLAGS = -DPSS_STYLE=1 -DSPSFVERSION="\"${VERSION}\"" -fPIC

OBJS =	PsxBios.o PsxCounters.o PsxDma.o Spu.o PsxHw.o PsxMem.o Misc.o	\
	R3000A.o PsxInterpreter.o PsxHLE.o spu/spu.o

OBJS+=	xmms/xmms.o
FLAGS+=	`gtk-config --cflags`

ifeq (${CPU}, ppc)
	FLAGS+=-DMSB_FIRST
endif	

ifeq (${CPU}, MSBFIRST)
	FLAGS+=-DMSB_FIRST
endif

CFLAGS += -Wall -I. ${FLAGS}

sexypsf: ${OBJS}
	${CC} ${LDFLAGS} ${OBJS} -o libsexypsf.so ${LIBS}

install: sexypsf
	mv libsexypsf.so ${HOME}/.xmms/Plugins

installglobal: sexypsf
	mv libsexypsf.so `xmms-config --input-plugin-dir`

.PHONY: clean sexypsf

clean:
	${RM} ${OBJS} libsexypsf.so

../%.o: ../%.c
	${CC} ${CFLAGS} -c -o $@ $<

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $<
