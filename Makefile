CPP_MODS = ../cpp-mods
include $(CPP_MODS)/config.mk

OBJDIR := obj/
CFLAGS += -g -O2 -Wall -I$(CPP_MODS) -Wno-switch
SRCDIR := src/
#LDFLAGS += -pg
USE_CCACHE := 1
USE_CLANG := 1

#SDL_AUDIO := 1

include $(CPP_MODS)/coreutils/module.mk
include $(CPP_MODS)/bbsutils/module.mk
include $(CPP_MODS)/webutils/module.mk
include $(CPP_MODS)/sqlite3/module.mk
include $(CPP_MODS)/audioplayer/module.mk
include $(CPP_MODS)/lua/module.mk
include $(CPP_MODS)/grappix/module.mk
include $(CPP_MODS)/fft/module.mk
include $(CPP_MODS)/archive/module.mk

include $(CPP_MODS)/musicplayer/plugins/ModPlugin/module.mk
include $(CPP_MODS)/musicplayer/plugins/OpenMPTPlugin/module.mk
include $(CPP_MODS)/musicplayer/plugins/HTPlugin/module.mk
include $(CPP_MODS)/musicplayer/plugins/HEPlugin/module.mk
include $(CPP_MODS)/musicplayer/plugins/GSFPlugin/module.mk
include $(CPP_MODS)/musicplayer/plugins/NDSPlugin/module.mk
include $(CPP_MODS)/musicplayer/plugins/USFPlugin/module.mk
include $(CPP_MODS)/musicplayer/plugins/VicePlugin/module.mk
include $(CPP_MODS)/musicplayer/plugins/GMEPlugin/module.mk
include $(CPP_MODS)/musicplayer/plugins/SexyPSFPlugin/module.mk
include $(CPP_MODS)/musicplayer/plugins/SC68Plugin/module.mk
include $(CPP_MODS)/musicplayer/plugins/StSoundPlugin/module.mk
include $(CPP_MODS)/musicplayer/plugins/AdPlugin/module.mk
include $(CPP_MODS)/musicplayer/plugins/UADEPlugin/module.mk

include $(CPP_MODS)/crypto/module.mk


TARGET := chipmachine
LOCAL_FILES += main.cpp ChipMachine.cpp MusicDatabase.cpp SearchIndex.cpp MusicPlayerList.cpp MusicPlayer.cpp TelnetInterface.cpp
LIBS += -lz

include $(CPP_MODS)/build.mk

pkg:
	rm -rf debian-pkg/*
	mkdir -p debian-pkg/$(TARGET)/DEBIAN
	mkdir -p debian-pkg/$(TARGET)/opt/chipmachine
	mkdir -p debian-pkg/$(TARGET)/usr/bin
	cp extra/dpkg.control debian-pkg/$(TARGET)/DEBIAN/control
	cp MANUAL.md debian-pkg/$(TARGET)/opt/chipmachine
	cp -a data debian-pkg/$(TARGET)/opt/chipmachine
	rm -f debian-pkg/$(TARGET)/opt/chipmachine/data/*.dfield
	cp -a lua debian-pkg/$(TARGET)/opt/chipmachine
	rm -f debian-pkg/$(TARGET)/opt/chipmachine/lua/*~ debian-pkg/$(TARGET)/opt/chipmachine/lua/*.lua
	cp chipmachine debian-pkg/$(TARGET)/opt/chipmachine
	arm-linux-gnueabihf-strip debian-pkg/$(TARGET)/opt/chipmachine/chipmachine
	cp extra/cm debian-pkg/$(TARGET)/usr/bin/chipmachine
	(cd debian-pkg ; dpkg-deb --build $(TARGET))
