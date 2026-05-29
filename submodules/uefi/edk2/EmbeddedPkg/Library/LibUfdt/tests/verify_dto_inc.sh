#!/bin/bash

PROG_NAME=`basename $0`

function usage() {
  echo "Usage:"
  echo "  $PROG_NAME <Base DTS> <Overlay DTS> <Output DTS>"
}

function on_exit() {
  rm -rf "$TEMP_DIR"
}

#
# Start
#

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

# Finds '/dts-v1/; and /plugin/;' then replace them with '/* REMOVED */'
OVERLAY_DTS_DIR=`dirname "$OVERLAY_DTS"`
OVERLAY_DTS_NAME=`basename "$OVERLAY_DTS"`
OVERLAY_DT_WO_HEADER_DTS="$TEMP_DIR/$OVERLAY_DTS_NAME"
sed "s/\\(\\/dts-v1\\/\\s*;\\|\\/plugin\\/\\s*;\\)/\\/\\* REMOVED \\*\\//g" \
  "$OVERLAY_DTS" > "$OVERLAY_DT_WO_HEADER_DTS"

# Appends /include/ ...;
BASE_DTS_DIR=`dirname "$BASE_DTS"`
BASE_DTS_NAME=`basename "$BASE_DTS"`
BASE_DT_WITH_INC_DTS="$TEMP_DIR/$BASE_DTS_NAME"
cp "$BASE_DTS" "$BASE_DT_WITH_INC_DTS"
echo "/include/ \"$OVERLAY_DT_WO_HEADER_DTS\"" >> "$BASE_DT_WITH_INC_DTS"

# Simulate device tree overlay
MERGED_DTB="$BASE_DT_WITH_INC_DTS.dtb"
dtc -@ -i "$BASE_DTS_DIR" -i "$OVERLAY_DTS_DIR" -O dtb -o "$MERGED_DTB" "$BASE_DT_WITH_INC_DTS"

# Dump
dtc -s -O dts -o "$OUT_DTS" "$MERGED_DTB"
