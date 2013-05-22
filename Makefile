OBJDIR := obj/
CFLAGS := -g -Wall -I. -Iinclude -Iplugins/ModPlugin/modplug/libmodplug -Iplugins/SexyPSFPlugin
CXXFLAGS := -std=c++0x
TARGET := player
MODULES := plugins/ModPlugin/modplug ziplib
LIBS := -lsexypsf -lz
LDFLAGS := -Lplugins/SexyPSFPlugin
OBJS := player.o utils.o

WIN_CFLAGS := -static -Icurl-7.30.0/include -DWIN32 -Doverride=""
WIN_LIBS := -lwinmm -lcurldll
WIN_LDFLAGS := -Lcurl-7.30.0/lib -static
WIN_OBJS := AudioPlayerWindows.o

LINUX_CC := gcc
LINUX_CXX := g++
LINUX_CFLAGS := `curl-config --cflags` -Doverride=""
LINUX_LIBS := -lasound `curl-config --libs`
LINUX_OBJS := AudioPlayerLinux.o

include Makefile.inc