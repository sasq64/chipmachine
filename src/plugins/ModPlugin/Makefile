include ../../../settings.mk

include $(UTILS)/config.mk
CFLAGS += -O2 -Wall -I$(UTILS)
CXXFLAGS += -std=c++0x

OBJDIR=obj/
TARGET=libmodplugin
TARGET_EXT=.a

#LINUX_CC = emcc
#LINUX_CXX = em++

DIRS := modplug
OBJS := ModPlugin.o
CFLAGS += -Wall -I$(UTILS) -Imodplug -I. -I../..

ifneq ($(EMSCRIPTEN),)
TARGETDIR=em/
endif

include $(UTILS)/build.mk
#Makefile.inc
