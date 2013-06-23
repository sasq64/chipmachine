include ../../settings.mk

OBJDIR=obj/
TARGET=libuade
EXT := .a

UADE = uade-2.13/src/
FE = $(UADE)frontends/common/

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
	$(FE)songinfo.o \
	$(UADE)unixatomic.o \
	$(UADE)uadeipc.o \
	$(UADE)ossupport.o \
	libuade.o \
	UADEPlugin.o


	#uadesimple libuade.o audio.o unixatomic.o uadeipc.o amifilemagic.o eagleplayer.o unixwalkdir.o effects.o 
	#uadecontrol.o uadeconf.o md5.o ossupport.o songdb.o songinfo.o vplist.o support.o 

CFLAGS := \
	-I$(UADE)include \
	-I$(UADE)frontends/include \
	-I$(FE)

include ../../Makefile.inc