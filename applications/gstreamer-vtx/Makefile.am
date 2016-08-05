SUBDIRS= m4 libayemu .

plugin_LTLIBRARIES = libvtxplugin.la

libvtxplugin_la_SOURCES = gstvtx.cc

libvtxplugin_la_CFLAGS = $(GST_CFLAGS)
libvtxplugin_la_CXXFLAGS = $(GST_CFLAGS) $(GST_PLUGINS_BASE_CFLAGS)
libvtxplugin_la_LIBADD = $(top_builddir)/libayemu/libayemu.la $(GST_PLUGINS_BASE_LIBS) $(GST_LIBS) -lstdc++
libvtxplugin_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

noinst_HEADERS = gstvtx.h

EXTRA_DIST = play.sh autogen.sh autoregen.sh gst-autogen.sh
