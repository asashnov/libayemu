libdir = $(INPUT_PLUGIN_DIR)

lib_LTLIBRARIES = libvtx.la

libvtx_la_LDFLAGS = -module -avoid-version

COMMON_SRC = xmms-vtx.c xmms-vtx.h about.c info.c config.c

libvtx_la_SOURCES = $(COMMON_SRC)

COMMON_CFLAGS = @XMMS_CFLAGS@

AM_CFLAGS = $(COMMON_CFLAGS)

EXTRA_DIST = 				\
    music_sample/csoon.vtx   		\
    music_sample/dizzy4_2.vtx  		\
    music_sample/dizzy6.vtx  		\
    music_sample/sil00.vtx  		\
    music_sample/sil02.vtx		\
    music_sample/dizzy3.vtx  		\
    music_sample/dizzy5.vtx    		\
    music_sample/dv3-f.vtx   		\
    music_sample/sil01.vtx  		\
    music_sample/turican2.vtx

local_install: libvtx.la
	$(INSTALL) -d -m 755 $(HOME)/.xmms/Plugins
	$(LIBTOOL) $(INSTALL) -m 755 libvtx.la $(HOME)/.xmms/Plugins
