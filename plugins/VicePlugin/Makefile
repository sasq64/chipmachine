include ../../settings.mk

OBJDIR=obj/
TARGET=libviceplugin
EXT=.a

MODULES := vice/resid

OBJS := \
	VicePlugin.o \
	common/archdep.o \
    common/console.o \
    common/joy.o \
    common/signals.o \
    common/vsyncarch.o \
    common/gui/uicmdline.o \
    common/gui/vsidui.o \
    common/null/fullscreen.o \
    common/null/ui.o \
    common/null/uiapi.o \
    common/null/keyboard.o \
    vice/alarm.o \
    vice/clkguard.o \
    vice/charset.o \
    vice/cmdline.o \
    vice/crc32.o \
    vice/debug.o \
    vice/datasette.o \
    vice/dma.o \
    vice/event.o \
    vice/findpath.o \
    vice/fliplist.o \
    vice/init.o \
    vice/initcmdline.o \
    vice/interrupt.o \
    vice/ioutil.o \
    vice/kbdbuf.o \
    vice/keyboard.o \
    vice/joystick.o \
    vice/lib.o \
    vice/lightpen.o \
    vice/log.o \
    vice/machine.o \
    vice/machine-bus.o \
    vice/network.o \
    vice/palette.o \
    vice/ram.o \
    vice/romset.o \
    vice/resources.o \
    vice/screenshot.o \
    vice/snapshot.o \
    vice/sound.o \
    vice/sysfile.o \
    vice/translate.o \
    vice/traps.o \
    vice/util.o \
    vice/vsync.o \
    vice/zfile.o \
    vice/zipcode.o \
    vice/c64/c64-cmdline-options.o \
    vice/c64/c64embedded.o \
    vice/c64/c64export.o \
    vice/c64/c64gluelogic.o \
    vice/c64/c64io.o \
    vice/c64/c64keyboard.o \
    vice/c64/c64meminit.o \
    vice/c64/c64memlimit.o \
    vice/c64/c64memrom.o \
    vice/c64/c64memsnapshot.o \
    vice/c64/c64model.o \
    vice/c64/c64pla.o \
    vice/c64/c64rom.o \
    vice/c64/c64romset.o \
    vice/c64/c64sound.o \
    vice/c64/c64video.o \
    vice/c64/patchrom.o \
    vice/c64/psid.o \
    vice/c64/reloc65.o \
    vice/c64/vsid.o \
    vice/c64/vsid-resources.o \
    vice/c64/vsid-snapshot.o \
    vice/c64/vsidcia1.o \
    vice/c64/vsidcia2.o \
    vice/c64/vsidmem.o \
    vice/c64/vsidstubs.o \
    vice/core/ciacore.o \
    vice/core/ciatimer.o \
    vice/core/viacore.o \
    vice/raster/raster.o \
    vice/raster/raster-cache.o \
    vice/raster/raster-canvas.o \
    vice/raster/raster-changes.o \
    vice/raster/raster-cmdline-options.o \
    vice/raster/raster-line.o \
    vice/raster/raster-line-changes.o \
    vice/raster/raster-line-changes-sprite.o \
    vice/raster/raster-modes.o \
    vice/raster/raster-resources.o \
    vice/raster/raster-sprite.o \
    vice/raster/raster-sprite-cache.o \
    vice/raster/raster-sprite-status.o \
    vice/sid/fastsid.o \
    vice/sid/sid.o \
    vice/sid/sid-cmdline-options.o \
    vice/sid/sid-resources.o \
    vice/sid/sid-snapshot.o \
    vice/sid/resid.o \
    vice/sounddrv/soundaiff.o \
    vice/sounddrv/sounddump.o \
    vice/sounddrv/soundfs.o \
    vice/sounddrv/soundiff.o \
    vice/sounddrv/soundmovie.o \
    vice/sounddrv/soundvoc.o \
    vice/sounddrv/soundwav.o \
    vice/vicii/vicii.o \
    vice/vicii/vicii-badline.o \
    vice/vicii/vicii-color.o \
    vice/vicii/vicii-cmdline-options.o \
    vice/vicii/vicii-fetch.o \
    vice/vicii/vicii-irq.o \
    vice/vicii/vicii-mem.o \
    vice/vicii/vicii-phi1.o \
    vice/vicii/vicii-resources.o \
    vice/vicii/vicii-snapshot.o \
    vice/vicii/vicii-sprites.o \
    vice/vicii/vicii-stubs.o \
    vice/vicii/vicii-timing.o \
    overrides/attach.o \
    overrides/autostart.o \
    overrides/maincpu.o \
    overrides/mouse.o \
    overrides/monitor/monitor.o \
    overrides/monitor/monitor_network.o \
    overrides/serial.o \
    overrides/sounddrv/sounddummy.o \
    overrides/vicii/vicii-draw.o \
    overrides/video/video.o

INCLUDES := \
	-Icommon \
	-Icommon/readline \
    -Ivice \
    -Ivice/c64 \
    -Ivice/c64/cart \
    -Ivice/c64dtv \
    -Ivice/drive \
    -Ivice/imagecontents \
    -Ivice/lib/p64 \
    -Ivice/monitor \
    -Ivice/raster \
    -Ivice/sid \
    -Ivice/tape \
    -Ivice/userport \
    -Ivice/vdrive \
    -Ivice/vicii \
    -Ivice/rtc

CFLAGS := $(CFLAGS) -Wall -I. $(INCLUDES)

#LINUX_CFLAGS := -Doverride=""

include ../../Makefile.inc
