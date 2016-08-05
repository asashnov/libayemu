#!/bin/sh

set -e

DST=/home/alex/.gstreamer-0.10/plugins
mkdir -p $DST
mv .libs/libvtxplugin.so $DST  || echo "passed"

#GST_DEBUG=vtx:2,filesrc:5
GST_DEBUG=5
export GST_DEBUG

gst-launch -t -v -m filesrc location=samples/spf21_00.vtx ! vtx ! audioresample ! osssink
