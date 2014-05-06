MODULE_DIR = ../cpp-mods
include $(MODULE_DIR)/config.mk

OBJDIR := obj/
CFLAGS += -O2 -Wall -I$(MODULE_DIR)
SRCDIR := src/

#SDL_AUDIO := 1
include $(MODULE_DIR)/coreutils/module.mk
include $(MODULE_DIR)/audioplayer/module.mk
include $(MODULE_DIR)/grappix/module.mk

include src/plugins/ModPlugin/module.mk
include src/plugins/VicePlugin/module.mk
include src/plugins/GMEPlugin/module.mk
include src/plugins/SexyPSFPlugin/module.mk
include src/plugins/SC68Plugin/module.mk
include src/plugins/UADEPlugin/module.mk

TARGET := play
LOCAL_FILES += miniplay.cpp MusicPlayer.cpp
LIBS += -lz

CC=ccache clang -Qunused-arguments
CXX=ccache clang++ -Qunused-arguments

include $(MODULE_DIR)/build.mk
