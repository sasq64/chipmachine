include ../../../settings.mk

OBJDIR=obj/
TARGET=libsc68plugin
EXT=.a

MODULES := file68/src libsc68 libsc68/emu68 libsc68/io68 libsc68/sc68 unice68
OBJS := SC68Plugin.o
CFLAGS := $(CFLAGS) -DHAVE_CONFIG_H -Wall -I. -I../..
INCLUDES := . libsc68/sc68 file68 file68/sc68

include ../../../Makefile.inc
