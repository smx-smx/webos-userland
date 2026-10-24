#!/bin/sh
# Run a test program with the environment procmgr gives to NC4 apps.
# usage: run.sh <program> [args...]
cd "$(dirname "$0")"
export LD_LIBRARY_PATH=/mnt/swue/lib:/usr/local/lib:/lib:/usr/lib:/usr/lib/gconv:/mnt/addon/lib:/mnt/vendor:/mnt/lg/res/lglib:/mnt/lg/res/lgres:/mnt/lg/local/lib
export LD_PRELOAD=/mnt/lg/res/lglib/liblibm.so:/mnt/vendor/libgal.so:/mnt/vendor/libmtal.so
export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-0.10
export HOME=/home
prog=$1; shift
exec "./$prog" "$@"
