ifeq ($(MODPLUGIN_INCLUDED),)
MODPLUGIN_INCLUDED = 1

THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

MODULES += $(THIS_DIR)/modplug
FILES += $(THIS_DIR)/ModPlugin.cpp

CFLAGS += -I$(THIS_DIR)/modplug -I$(THIS_DIR) -I$(realpath $(THIS_DIR)/../..)

endif
