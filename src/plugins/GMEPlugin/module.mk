include $(UTILS)/Makefile.inc
ifeq ($(GMEPLUGIN_INCLUDED),)
GMEPLUGIN_INCLUDED = 1
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

MODULES += $(THIS_DIR)/gme
FILES += $(THIS_DIR)/GMEPlugin.cpp
CFLAGS += $(CFLAGS) -I$(THIS_DIR) -I$(realpath $(THIS_DIR)/../..) -DBLARGG_LITTLE_ENDIAN -DHAVE_ZLIB_H 

endif
