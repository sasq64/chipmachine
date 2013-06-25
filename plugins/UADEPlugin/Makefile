include ../../settings.mk

OBJDIR=obj/
TARGET=libuade
EXT := .a

UADE = uade/src/
FE = $(UADE)frontends/common/


OBJS := $(FE)unixatomic.o \
	$(FE)uadeipc.o \
	$(FE)amifilemagic.o \
	$(FE)eagleplayer.o \
	$(FE)unixwalkdir.o \
	$(FE)effects.o \
	$(FE)uadecontrol.o \
	$(FE)uadeconf.o \
	$(FE)uadestate.o \
	$(FE)uadeutils.o \
	$(FE)md5.o \
	$(FE)ossupport.o \
	$(FE)rmc.o \
	$(FE)songdb.o \
	$(FE)songinfo.o \
	$(FE)vparray.o \
	$(FE)support.o \
	$(FE)fifo.o \
	uade/compat/strlrep.o \
	UADEPlugin.o


	#$(FE)amifilemagic.o \
	#$(FE)eagleplayer.o \
	#$(FE)effects.o \
	#$(FE)md5.o \
	#$(FE)songdb.o \
	#$(FE)support.o \
	#$(FE)uadeconf.o \
	#$(FE)uadecontrol.o \
	#$(FE)vplist.o \
	#$(FE)songinfo.o \
	#$(UADE)unixatomic.o \
	#$(UADE)uadeipc.o \
	#$(UADE)ossupport.o \
	#libuade.o \
	#UADEPlugin.o


	#uadesimple libuade.o audio.o unixatomic.o uadeipc.o amifilemagic.o eagleplayer.o unixwalkdir.o effects.o 
	#uadecontrol.o uadeconf.o md5.o ossupport.o songdb.o songinfo.o vplist.o support.o 

CFLAGS := \
	-I/usr/local/include \
	-I$(UADE)include \
	-I$(UADE)frontends/include \
	-I$(FE) \
	-I.

include ../../Makefile.inc