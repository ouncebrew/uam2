#!/bin/sh
set -e
objcopy="$1"; ar="$2"; obj="$3"; out="$4"
shift 4
tmp="$out.o"
"$objcopy" "$@" "$obj" "$tmp"
rm -f "$out"
"$ar" rcs "$out" "$tmp"
rm -f "$tmp"
