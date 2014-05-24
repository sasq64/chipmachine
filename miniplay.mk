MODULE_DIR = ../cpp-mods
include $(MODULE_DIR)/config.mk

OBJDIR := obj/
CFLAGS += -O2 -Wall -I$(MODULE_DIR)
SRCDIR := src/

#SDL_AUDIO := 1
include $(MODULE_DIR)/coreutils/module.mk
include $(MODULE_DIR)/audioplayer/module.mk
include $(MODULE_DIR)/grappix/module.mk

include $(MODULE_DIR)/musicplayer/plugins/ModPlugin/module.mk
include $(MODULE_DIR)/musicplayer/plugins/VicePlugin/module.mk
include $(MODULE_DIR)/musicplayer/plugins/GMEPlugin/module.mk
include $(MODULE_DIR)/musicplayer/plugins/SexyPSFPlugin/module.mk
include $(MODULE_DIR)/musicplayer/plugins/SC68Plugin/module.mk
include $(MODULE_DIR)/musicplayer/plugins/UADEPlugin/module.mk

include $(MODULE_DIR)/crypto/module.mk

TARGET := play
LOCAL_FILES += miniplay.cpp
LIBS += -lz

CC=ccache clang -Qunused-arguments
CXX=ccache clang++ -Qunused-arguments

include $(MODULE_DIR)/build.mk
