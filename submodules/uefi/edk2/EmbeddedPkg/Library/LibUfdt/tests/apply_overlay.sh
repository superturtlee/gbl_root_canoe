#!/bin/bash

PROG_NAME=`basename $0`

function usage() {
  echo "Usage:"
  echo "  $PROG_NAME (--fdt|--ufdt) <Base DTS> <Overlay DTS> <Output DTS>"
}

function on_exit() {
  rm -rf "$TEMP_DIR"
}

#
# Start
#

# Setup OVERLAY
if [ "$1" == "--fdt" ]; then
  shift
  OVERLAY="fdt_apply_overlay"
elif [ "$1" == "--ufdt" ]; then
  shift
  OVERLAY="ufdt_apply_overlay"
else
  usage
  exit 1
fi

if [[ $# -lt 3 ]]; then
  usage
  exit 1
fi

BASE_DTS=$1
OVERLAY_DTS=$2
OUT_DTS=$3

TEMP_DIR=`mktemp -d`
# The script will exit directly if any command fails.
set -e
trap on_exit EXIT

# Compile the *-base.dts to make *-base.dtb
BASE_DTS_NAME=`basename "$BASE_DTS"`
BASE_DTB="$TEMP_DIR/${BASE_DTS_NAME}-base.dtb"
dtc -@ -qq -O dtb -o "$BASE_DTB" "$BASE_DTS"

# Compile the *-overlay.dts to make *-overlay.dtb
OVERLAY_DTS_NAME=`basename "$OVERLAY_DTS"`
OVERLAY_DTB="$TEMP_DIR/${OVERLAY_DTS_NAME}-overlay.dtb"
dtc -@ -qq -O dtb -o "$OVERLAY_DTB" "$OVERLAY_DTS"

# Run ufdt_apply_overlay to combine *-base.dtb and *-overlay.dtb
# into *-merged.dtb
MERGED_DTB="$TEMP_DIR/${BASE_DTS_NAME}-merged.dtb"
"$OVERLAY" "$BASE_DTB" "$OVERLAY_DTB" "$MERGED_DTB"

# Dump
dtc -s -O dts -o "$OUT_DTS" "$MERGED_DTB"
