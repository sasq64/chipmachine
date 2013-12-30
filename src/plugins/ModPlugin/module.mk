ifeq ($(MODPLUGIN_INCLUDED),)
MODPLUGIN_INCLUDED = 1
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

MODULES += $(THIS_DIR)modplug
FILES += $(THIS_DIR)ModPlugin.cpp
INCLUDES += $(THIS_DIR)modplug $(THIS_DIR) $(THIS_DIR)../..

endif
