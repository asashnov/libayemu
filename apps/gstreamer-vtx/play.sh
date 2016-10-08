#!/bin/sh

set -e

test -f /usr/bin/gst-launch || \
  { echo "gst-launch not found. try apt-get install gstreamer-tools" ; exit 1; }

DST=$HOME/.gstreamer-0.10/plugins

test -d $DST || mkdir -p $DST

cp -f .libs/libvtxplugin.so $DST

#GST_DEBUG=vtx:2,filesrc:5

GST_DEBUG=5

export GST_DEBUG

gst-launch -t -v -m filesrc location=samples/spf21_00.vtx \
  ! vtx ! audioresample \
  ! alsasink > play.log 2>&1
