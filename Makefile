MODULE_DIR = ../utils
include $(MODULE_DIR)/config.mk

OBJDIR := obj/
CFLAGS := -g -Wall -I$(MODULE_DIR) -DLINUX
CXXFLAGS := -std=c++0x
SRCDIR := src/

include $(MODULE_DIR)/coreutils/module.mk
include $(MODULE_DIR)/bbsutils/module.mk
include $(MODULE_DIR)/sqlite3/module.mk
include $(MODULE_DIR)/audioplayer/module.mk

include src/plugins/ModPlugin/module.mk
include src/plugins/VicePlugin/module.mk
include src/plugins/GMEPlugin/module.mk
include src/plugins/SexyPSFPlugin/module.mk
include src/plugins/UADEPlugin/module.mk
include src/plugins/SC68Plugin/module.mk

TARGET := player
LOCAL_FILES += play.cpp
LIBS += -lz
# TelnetInterface.cpp SongDb.cpp SearchIndex.cpp URLPlayer.cpp

CC=ccache clang -Qunused-arguments
CXX=ccache clang++ -Qunused-arguments
AR=llvm-ar
include $(MODULE_DIR)/build.mk
