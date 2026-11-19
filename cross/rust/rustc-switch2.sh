#!/usr/bin/env bash
# The rustc that meson runs for aarch64-switch2-horizon.
here="$(cd "$(dirname "$0")" && pwd)"
: "${DEVKITPRO:=/opt/devkitpro}"
export RUSTC_BOOTSTRAP=1 RUST_TARGET_PATH="$here"

args=()
while [ $# -gt 0 ]; do
	case "$1" in
	-C) if [[ "$2" == linker=* ]]; then shift 2; continue; fi ;;
	-Clinker=*) shift; continue ;;
	esac
	args+=("$1"); shift
done

exec rustc -Zunstable-options --sysroot "$here/sysroot" --target aarch64-switch2-horizon -C force-unwind-tables=no \
	-L "$DEVKITPRO/devkitA64/aarch64-none-elf/lib" -L "$DEVKITPRO/libounce/lib" \
	"${args[@]}"
