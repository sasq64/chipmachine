CPP_MODS = ../cpp-mods
include $(CPP_MODS)/config.mk

OBJDIR := obj/
CFLAGS += -O2 -Wall -I$(CPP_MODS)
#SRCDIR := miniplay/

#SDL_AUDIO := 1
include $(CPP_MODS)/coreutils/module.mk
include $(CPP_MODS)/audioplayer/module.mk
#include $(CPP_MODS)/grappix/module.mk

include $(CPP_MODS)/musicplayer/plugins/ModPlugin/module.mk
include $(CPP_MODS)/musicplayer/plugins/VicePlugin/module.mk
include $(CPP_MODS)/musicplayer/plugins/GMEPlugin/module.mk
include $(CPP_MODS)/musicplayer/plugins/SexyPSFPlugin/module.mk
include $(CPP_MODS)/musicplayer/plugins/SC68Plugin/module.mk
include $(CPP_MODS)/musicplayer/plugins/UADEPlugin/module.mk

include $(CPP_MODS)/crypto/module.mk

TARGET := play
LOCAL_FILES += miniplay.cpp
LIBS += -lz

CC=ccache clang -Qunused-arguments
CXX=ccache clang++ -Qunused-arguments

include $(CPP_MODS)/build.mk
