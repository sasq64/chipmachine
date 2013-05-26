OBJDIR := obj/
CFLAGS := -g -Wall -I. -Iinclude -Iplugins/ModPlugin/modplug/libmodplug -Iplugins/SexyPSFPlugin


CFLAGS := $(CFLAGS) -Inetlink/include \
	-Iplugins/VICEPlugin \
	-Iplugins/VICEPlugin/common \
    -Iplugins/VICEPlugin/vice \
    -Iplugins/VICEPlugin/vice/drive \
    -Iplugins/VICEPlugin/vice/lib/p64 \
    -Iplugins/VICEPlugin/vice/c64 \

    # -Ivice/c64 \
    # -Ivice/c64/cart \
    # -Ivice/c64dtv \
    # -Ivice/imagecontents \
    # -Ivice/monitor \
    # -Ivice/raster \
    # -Ivice/sid \
    # -Ivice/tape \
    # -Ivice/userport \
    # -Ivice/vdrive \
    # -Ivice/vicii \
    # -Ivice/rtc


CXXFLAGS := -std=c++0x
TARGET := player
MODULES := plugins/ModPlugin/modplug ziplib netlink/src
LIBS := -lsexypsf -lviceplayer -lz
LDFLAGS := -Lplugins/SexyPSFPlugin -Lplugins/VICEPlugin
OBJS := player.o server.o utils.o WebGetter.o URLPlayer.o Archive.o VicePlayer.o

WIN_CFLAGS := -static -Icurl/include -DWIN32 -Doverride=""
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

all : $(TARGET)$(EXT) vice sexypsf netlink

netlink :
	make -f netlink.mk

vice :
	make -C plugins/VICEPlugin

sexypsf :
	make -C plugins/SexyPSFPlugin

include Makefile.inc