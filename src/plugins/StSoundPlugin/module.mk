ifeq ($(MODPLUGIN_INCLUDED),)
MODPLUGIN_INCLUDED = 1
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

modplug_DIRS := $(THIS_DIR)modplug
modplug_FILES := $(THIS_DIR)ModPlugin.cpp
modplug_INCLUDES := $(THIS_DIR)modplug $(THIS_DIR)../.. $(MODULE_DIR)

INCLUDES += $(THIS_DIR)

MODULES += modplug

endif
