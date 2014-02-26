ifeq ($(SC68PLUGIN_INCLUDED),)
SC68PLUGIN_INCLUDED = 1
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

sc68_FILES := $(THIS_DIR)SC68Plugin.cpp
sc68_INCLUDES := $(THIS_DIR) $(THIS_DIR)libsc68/sc68 $(THIS_DIR)file68 $(THIS_DIR)file68/sc68 $(THIS_DIR)unice68 $(THIS_DIR)libsc68 $(MODULE_DIR)
sc68_DIRS := $(THIS_DIR)file68/src $(THIS_DIR)libsc68 $(THIS_DIR)libsc68/emu68 $(THIS_DIR)libsc68/io68 $(THIS_DIR)libsc68/sc68 $(THIS_DIR)unice68
sc68_CFLAGS := -DHAVE_CONFIG_H -Wall

INCLUDES += $(THIS_DIR)

MODULES += sc68

endif
