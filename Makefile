CPP_MODS = ../cpp-mods
include $(CPP_MODS)/config.mk

OBJDIR := obj/
CFLAGS += -g -O2 -Wall -I$(CPP_MODS) -Wno-switch
#CFLAGS += -O2
SRCDIR := src/
#LDFLAGS += -pg
USE_CCACHE := 1
USE_CLANG := 1

RUN_ARGS := -d

#SDL_AUDIO := 1

include $(CPP_MODS)/coreutils/module.mk
include $(CPP_MODS)/bbsutils/module.mk
include $(CPP_MODS)/webutils/module.mk
include $(CPP_MODS)/json/module.mk
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
include $(CPP_MODS)/musicplayer/plugins/MP3Plugin/module.mk

include $(CPP_MODS)/musicplayer/plugins/UADEPlugin/module.mk
#CFLAGS += -DNO_UADE

include $(CPP_MODS)/crypto/module.mk

#APP_DIR := /usr/share/chipmachine
#USER_DIR := chipmachine

TARGET := chipmachine
LOCAL_FILES += main.cpp ChipMachine.cpp ChipMachine_config.cpp ChipMachine_keys.cpp MusicDatabase.cpp PlaylistDatabase.cpp SearchIndex.cpp MusicPlayerList.cpp MusicPlayer.cpp TelnetInterface.cpp
LOCAL_FILES += renderable.cpp renderset.cpp state_machine.cpp RemoteLists.cpp SongFileIdentifier.cpp RemoteLoader.cpp
LIBS += -lz

include $(CPP_MODS)/build.mk

pkg:
	rm -rf debian-pkg/*
	mkdir -p debian-pkg/$(TARGET)/DEBIAN
	mkdir -p debian-pkg/$(TARGET)/usr/share/chipmachine
	mkdir -p debian-pkg/$(TARGET)/usr/bin
	cp extra/dpkg.control debian-pkg/$(TARGET)/DEBIAN/control
	cp MANUAL.md debian-pkg/$(TARGET)/usr/share/chipmachine
	cp -a data debian-pkg/$(TARGET)/usr/share/chipmachine
	rm -f debian-pkg/$(TARGET)/usr/share/chipmachine/data/*.dfield
	cp -a lua debian-pkg/$(TARGET)/usr/share/chipmachine
	rm -f debian-pkg/$(TARGET)/usr/share/chipmachine/lua/*~
	cp chipmachine debian-pkg/$(TARGET)/usr/bin
	$(PREFIX)strip debian-pkg/$(TARGET)/usr/bin/chipmachine
	#cp extra/cm debian-pkg/$(TARGET)/usr/bin/chipmachine
	chmod -R g-w debian-pkg
	(cd debian-pkg ; fakeroot dpkg-deb --build $(TARGET))
