ifeq ($(UADEPLUGIN_INCLUDED),)
UADEPLUGIN_INCLUDED = 1
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
FE = $(THIS_DIR)/uade/src/frontends/common/

uade_FILES := $(THIS_DIR)/UADEPlugin.cpp \
	$(FE)unixatomic.c \
	$(FE)uadeipc.c \
	$(FE)amifilemagic.c \
	$(FE)eagleplayer.c \
	$(FE)unixwalkdir.c \
	$(FE)effects.c \
	$(FE)uadecontrol.c \
	$(FE)uadeconf.c \
	$(FE)uadestate.c \
	$(FE)uadeutils.c \
	$(FE)md5.c \
	$(FE)ossupport.c \
	$(FE)rmc.c \
	$(FE)songdb.c \
	$(FE)songinfo.c \
	$(FE)vparray.c \
	$(FE)support.c \
	$(FE)fifo.c \
	$(THIS_DIR)/uade/compat/strlrep.c \
	$(THIS_DIR)/bencodetools/bencode.c

uade_INCLUDES := $(MODULE_DIR) $(THIS_DIR) $(THIS_DIR)/../.. $(THIS_DIR)/uade/src/include $(THIS_DIR)/uade/src/frontends/include $(FE)
INCLUDES += $(THIS_DIR)/..
MODULES += uade

endif
