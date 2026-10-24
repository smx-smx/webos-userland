#!/bin/sh
# Generate a stub symbol map from a real shared library.
# Functions are aliased to "dummy", data objects to "dummy_data".
#
# usage: tools/gen-lds.sh path/to/libfoo.so > src/<platform>/foo.lds
set -e
NM="${NM:-nm}"
"$NM" -D --defined-only "$1" | awk '
    $3 ~ /^(_init|_fini|_edata|_end|__end__|__bss_start|__bss_start__|__bss_end__|_bss_end__)$/ { next }
    $2 ~ /^[TtWi]$/ { printf "\"%s\" = \"dummy\";\n", $3; next }
    $2 ~ /^[BbDdRrGgSsVv]$/ { printf "\"%s\" = \"dummy_data\";\n", $3 }
' | LC_ALL=C sort -f -t'"' -k2,2
