ifeq ($(SEXYPSFPLUGIN_INCLUDED),)
SEXYPSFPLUGIN_INCLUDED = 1
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

sexy_DIRS := $(THIS_DIR)/sexypsf
sexy_FILES := $(THIS_DIR)/SexyPSFPlugin.cpp $(THIS_DIR)/sexypsf/spu/spu.c
sexy_INCLUDES := $(THIS_DIR) $(THIS_DIR)/../.. $(MODULE_DIR)
sexy_CFLAGS := -DPSS_STYLE=1

INCLUDES += $(THIS_DIR)

MODULES += sexy

endif
