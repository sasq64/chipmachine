include ../../../settings.mk

OBJDIR=obj/
TARGET=libgmeplugin
EXT=.a

DIRS := gme
OBJS := GMEPlugin.o
CFLAGS := $(CFLAGS) -Wall -I$(UTILS) -I. -I../.. -DBLARGG_LITTLE_ENDIAN -DHAVE_ZLIB_H 

include $(UTILS)/Makefile.inc
