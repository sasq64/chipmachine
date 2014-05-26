MODULE_DIR = ../cpp-mods
include $(MODULE_DIR)/config.mk

OBJDIR := obj/
CFLAGS += -g -O2 -Wall -I$(MODULE_DIR) -Wno-switch
SRCDIR := src/

USE_CCACHE := 1
USE_CLANG := 1

#SDL_AUDIO := 1

include $(MODULE_DIR)/coreutils/module.mk
include $(MODULE_DIR)/bbsutils/module.mk
include $(MODULE_DIR)/webutils/module.mk
include $(MODULE_DIR)/sqlite3/module.mk
include $(MODULE_DIR)/audioplayer/module.mk
include $(MODULE_DIR)/lua/module.mk
include $(MODULE_DIR)/grappix/module.mk
include $(MODULE_DIR)/fft/module.mk

include $(MODULE_DIR)/musicplayer/plugins/ModPlugin/module.mk
include $(MODULE_DIR)/musicplayer/plugins/VicePlugin/module.mk
include $(MODULE_DIR)/musicplayer/plugins/GMEPlugin/module.mk
include $(MODULE_DIR)/musicplayer/plugins/SexyPSFPlugin/module.mk
include $(MODULE_DIR)/musicplayer/plugins/SC68Plugin/module.mk
include $(MODULE_DIR)/musicplayer/plugins/StSoundPlugin/module.mk
include $(MODULE_DIR)/musicplayer/plugins/AdPlugin/module.mk
include $(MODULE_DIR)/musicplayer/plugins/UADEPlugin/module.mk

include $(MODULE_DIR)/crypto/module.mk

TARGET := chipmachine
LOCAL_FILES += main.cpp MusicDatabase.cpp SearchIndex.cpp MusicPlayerList.cpp MusicPlayer.cpp TelnetInterface.cpp
LIBS += -lz

include $(MODULE_DIR)/build.mk
