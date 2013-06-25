
include settings.mk

OBJDIR := obj/
CFLAGS := -g -Wall -I. -Iinclude -Iplugins/SexyPSFPlugin
CFLAGS := $(CFLAGS) -Inetlink/include -Isqlite3 -Iplugins/VicePlugin -Iplugins/ModPlugin -Iplugins/GMEPlugin -Iplugins/UADEPlugin

TARGET := player
MODULES := ziplib netlink/src
LIBS := -lsexypsfplugin -lviceplugin -lmodplugin -lgmeplugin -lz
# -liconv
LDFLAGS := -Wl,-Map -Wl,mapfile -Lplugins/SexyPSFPlugin -Lplugins/VicePlugin -Lplugins/ModPlugin -Lplugins/GMEPlugin -Lplugins/UADEPlugin
OBJS := player.o TelnetServer.o TextScreen.o SongDb.o SearchIndex.o WebGetter.o URLPlayer.o Archive.o utils.o log.o sqlite3/sqlite3.o

WIN_CFLAGS := $(WIN_CFLAGS) -static -Icurl/include -DWIN32
WIN_LIBS := -lwinmm -lcurldll -lws2_32 -liconv
WIN_LDFLAGS := -Lcurl/lib -static
WIN_OBJS := AudioPlayerWindows.o
WIN_CC := gcc
WIN_CXX := g++

LINUX_CFLAGS := $(LINUX_CFLAGS) `curl-config --cflags`
LINUX_LIBS := -luade -lasound `curl-config --libs`
LINUX_OBJS := AudioPlayerLinux.o

PI_OBJS := lcd.o
PI_LIBS := -luade -lwiringPi

#GCC_VERSION := $(subst /platform-tools/,,$(dir $(shell which adb)))

XDEPENDS := netlink modplug vice sexypsf gmeplugin
PI_XDEPENDS := uadeplugin
LINUX_XDEPENDS = uadeplugin

all : start_rule

run :
	./player

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

gmeplugin :
	make -C plugins/GMEPlugin

uadeplugin :
	make -C plugins/UADEPlugin

include Makefile.inc

#XDEPENDS := plugins/ModPlugin/libmodplugin.a plugins/GMEPlugin/libgmeplugin.a plugins/SexyPSFPlugin/libsexypsfplugin.a plugins/VicePlugin/libviceplugin.a
#all : vice sexypsf gmeplugin modplug netlink $(TARGET)$(EXT)
#all : $(TARGET)$(EXT)
#
#run :
#	./player
#
#cleanall : clean
#	make -f netlink.mk clean
#	make -C plugins/VicePlugin clean
#	make -C plugins/SexyPSFPlugin clean
#	make -C plugins/ModPlugin clean
#
#
#netlink :
#	make -f netlink.mk
#
#plugins/ModPlugin/libmodplugin.a :
#	make -C plugins/ModPlugin
#
#plugins/VicePlugin/libviceplugin.a :
#	make -C plugins/VicePlugin
#
#plugins/SexyPSFPlugin/libsexypsfplugin.a :
#	make -C plugins/SexyPSFPlugin
#
#plugins/GMEPlugin/libgmeplugin.a :
#	make -C plugins/GMEPlugin
#