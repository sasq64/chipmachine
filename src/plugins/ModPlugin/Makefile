include ../../../settings.mk

include $(UTILS)/config.mk

OBJDIR=obj/
TARGET=libmodplugin
TARGET_EXT=.a

#LINUX_CC = emcc
#LINUX_CXX = em++

MODULES := modplug
OBJS := ModPlugin.o
CFLAGS += -Wall -I$(UTILS) -Imodplug -I. -I../..

include $(UTILS)/build.mk
#Makefile.inc
