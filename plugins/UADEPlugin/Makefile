include ../../settings.mk

OBJDIR=obj/
TARGET=libuade
EXT := .a

SRCDIR := uade-2.13/src/
UADE =
FE = frontends/common/

OBJS := \
	$(FE)amifilemagic.o \
	$(FE)eagleplayer.o \
	$(FE)effects.o \
	$(FE)md5.o \
	$(FE)songdb.o \
	$(FE)support.o \
	$(FE)uadeconf.o \
	$(FE)uadecontrol.o \
	$(FE)vplist.o \
	$(UADE)unixatomic.o \
	$(UADE)audio.o \
	$(UADE)cfgfile.o \
	$(UADE)cia.o \
	$(UADE)compiler.o \
	$(UADE)cpudefs.o \
	$(UADE)cpuemu.o \
	$(UADE)cpustbl.o \
	$(UADE)custom.o \
	$(UADE)debug.o \
	$(UADE)fpp.o \
	$(UADE)main.o \
	$(UADE)md-support.o \
	$(UADE)memory.o \
	$(UADE)missing.o \
	$(UADE)newcpu.o \
	$(UADE)readcpu.o \
	$(UADE)sd-sound.o \
	$(UADE)simpleipc.o \
	$(UADE)simplesupport.o \
	$(UADE)sinctable.o \
	$(UADE)text_scope.o \
	$(UADE)uade.o \
	$(UADE)uadeipc.o \
	$(UADE)uademain.o 

CFLAGS := \
	-I$(SRCDIR)include \
	-I$(SRCDIR)frontends/include \
	-I$(SRCDIR)frontends/include/uade \
	-I$(SRCDIR)$(FE)

include ../../Makefile.inc