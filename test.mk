
include settings.mk

OBJDIR := testobj/
SRCDIR := src/
CFLAGS := -g -Wall  -DUNIT_TEST
INCLUDES := netlink/include sqlite3 plugins/VicePlugin plugins/ModPlugin plugins/GMEPlugin plugins/UADEPlugin plugins/SexyPSFPlugin

TARGET := test
MODULES := ziplib netlink/src
LIBS := -lsexypsfplugin -lviceplugin -lmodplugin -lgmeplugin -lz
LDFLAGS := -Wl,-Map -Wl,mapfile -Lplugins/SexyPSFPlugin -Lplugins/VicePlugin -Lplugins/ModPlugin -Lplugins/GMEPlugin
LDFLAGS := -Wl,-Map -Wl,mapfile -Lsrc/plugins/SexyPSFPlugin -Lsrc/plugins/VicePlugin -Lsrc/plugins/ModPlugin -Lsrc/plugins/GMEPlugin -Lsrc/plugins/UADEPlugin
OBJS := catch.o TelnetServer.o TextScreen.o WebGetter.o URLPlayer.o Archive.o utils.o log.o SongDb.o SearchIndex.o sqlite3/sqlite3.o

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


XDEPENDS := modplug vice sexypsf gmeplugin
PI_XDEPENDS := uadeplugin
LINUX_XDEPENDS = uadeplugin

all : start_rule

run :
	./test

cleanall : clean
	make -C src/plugins/VicePlugin clean
	make -C src/plugins/SexyPSFPlugin clean
	make -C src/plugins/ModPlugin clean
	make -C src/plugins/GMEPlugin
	make -C src/plugins/UADEPlugin

modplug :
	make -C src/plugins/ModPlugin

vice :
	make -C src/plugins/VicePlugin

sexypsf :
	make -C src/plugins/SexyPSFPlugin

gmeplugin :
	make -C src/plugins/GMEPlugin

uadeplugin :
	make -C src/plugins/UADEPlugin

include Makefile.inc
