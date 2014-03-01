ifeq ($(GMEPLUGIN_INCLUDED),)
GMEPLUGIN_INCLUDED = 1
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

gme_DIRS := $(THIS_DIR)/gme
gme_FILES := $(THIS_DIR)/GMEPlugin.cpp
gme_INCLUDES := $(THIS_DIR) $(THIS_DIR)/../.. $(MODULE_DIR)
gme_CFLAGS := -DBLARGG_LITTLE_ENDIAN -DHAVE_ZLIB_H 

INCLUDES += $(THIS_DIR)/..

MODULES += gme

endif
