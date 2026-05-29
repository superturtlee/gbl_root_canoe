#!/bin/bash

alert() {
  echo "$*" >&2
}

die() {
  echo "ERROR: $@"
  exit 1
}

command_exists () {
  type "$1" &> /dev/null;
}

remove_local_fixups() {
  sed '/__local_fixups__/ {s/^\s*__local_fixups__\s*//; :again;N; s/{[^{}]*};//; /^$/ !b again; d}' $1
}

remove_overlay_stuff() {
  # remove __symbols__, phandle, "linux,phandle" and __local_fixups__
  sed "/__symbols__/,/[}];/d" $1 | sed "/\(^[ \t]*phandle\)/d" | sed "/\(^[ \t]*linux,phandle\)/d" | sed '/^\s*$/d' | remove_local_fixups
}

dts_diff () {
  diff -u <(cat "$1" | remove_overlay_stuff) <(cat "$2" | remove_overlay_stuff)
}
