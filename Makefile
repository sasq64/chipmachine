
include settings.mk

OBJDIR := obj/
CFLAGS := -g -Wall -I. -Iinclude -Iplugins/SexyPSFPlugin

CFLAGS := $(CFLAGS) -Inetlink/include -Isqlite3 -Iplugins/VicePlugin -Iplugins/ModPlugin

TARGET := player
MODULES := ziplib netlink/src
LIBS := -lsexypsf -lviceplayer -lmodplugin -lz
LDFLAGS := -Lplugins/SexyPSFPlugin -Lplugins/VicePlugin -Lplugins/ModPlugin
OBJS := player.o TelnetServer.o utils.o WebGetter.o URLPlayer.o Archive.o

WIN_CFLAGS := -static -Icurl/include -DWIN32
WIN_LIBS := -lwinmm -lcurldll -lws2_32
WIN_LDFLAGS := -Lcurl/lib -static
WIN_OBJS := AudioPlayerWindows.o
WIN_CC := gcc
WIN_CXX := g++

LINUX_CFLAGS := $(LINUX_CFLAGS) `curl-config --cflags`
LINUX_LIBS := -lasound `curl-config --libs`
LINUX_OBJS := AudioPlayerLinux.o

#GCC_VERSION := $(subst /platform-tools/,,$(dir $(shell which adb)))

all : vice sexypsf modplug netlink $(TARGET)$(EXT)

cleanall : clean
	make -f netlink.mk clean
	make -C plugins/VicePlugin clean
	make -C plugins/SexyPSFPlugin clean
	make -C plugins/ModPlugin clean


netlink :
	make -f netlink.mk

modplug :
	make -C plugins/ModPlugin

vice :
	make -C plugins/VicePlugin

sexypsf :
	make -C plugins/SexyPSFPlugin

include Makefile.inc