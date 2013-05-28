OBJDIR := obj/
CFLAGS := -g -Wall -I. -Iinclude -Iplugins/SexyPSFPlugin

CFLAGS := $(CFLAGS) -Inetlink/include -Isqlite3 -Iplugins/VicePlugin -Iplugins/ModPlugin

CXXFLAGS := -std=c++0x
TARGET := player
MODULES := ziplib netlink/src
LIBS := -lsexypsf -lviceplayer -lmodplugin -lz
LDFLAGS := -Lplugins/SexyPSFPlugin -Lplugins/VicePlugin -Lplugins/ModPlugin
OBJS := player.o TelnetServer.o utils.o WebGetter.o URLPlayer.o Archive.o sqlite3/sqlite3.o

WIN_CFLAGS := -static -Icurl/include -DWIN32
WIN_LIBS := -lwinmm -lcurldll -lws2_32
WIN_LDFLAGS := -Lcurl/lib -static
WIN_OBJS := AudioPlayerWindows.o
WIN_CC := gcc
WIN_CXX := g++

LINUX_CC := gcc
LINUX_CXX := g++
LINUX_CFLAGS := `curl-config --cflags` -Doverride=""
LINUX_LIBS := -lasound `curl-config --libs`
LINUX_OBJS := AudioPlayerLinux.o

#GCC_VERSION := $(subst /platform-tools/,,$(dir $(shell which adb)))

all : vice sexypsf netlink $(TARGET)$(EXT)

netlink :
	make -f netlink.mk

vice :
	make -C plugins/VICEPlugin

sexypsf :
	make -C plugins/SexyPSFPlugin

include Makefile.inc