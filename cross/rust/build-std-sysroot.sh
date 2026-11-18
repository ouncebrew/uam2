#!/usr/bin/env bash
# Builds the Rust std sysroot for aarch64-switch2-horizon into cross/rust/sysroot.
set -e

here="$(cd "$(dirname "$0")" && pwd)"
: "${DEVKITPRO:=/opt/devkitpro}"
tgt=aarch64-switch2-horizon
sysroot="$here/sysroot"
work="${TMPDIR:-/tmp}/uam2rt-std"

export RUST_TARGET_PATH="$here" RUSTC_BOOTSTRAP=1
export RUSTFLAGS="-Zunstable-options -L $DEVKITPRO/devkitA64/aarch64-none-elf/lib -L $DEVKITPRO/libounce/lib"
export CARGO_TARGET_DIR="$work/target"

rm -rf "$work" && mkdir -p "$work/src"
printf '#![no_std]\n' > "$work/src/lib.rs"
cat > "$work/Cargo.toml" <<'TOML'
[package]
name = "uam2rt-std"
version = "0.0.0"
edition = "2021"

[lib]
crate-type = ["rlib"]

[profile.release]
panic = "abort"
opt-level = "s"
codegen-units = 1
TOML

(cd "$work" && cargo build --release --target "$tgt" \
	-Zbuild-std=core,alloc,std,panic_abort -Zbuild-std-features=)

rust_sysroot="$(rustc --print sysroot)"
rm -rf "$sysroot"
mkdir -p "$sysroot/lib/rustlib/$tgt/lib"
cp "$work/target/$tgt/release/deps/"*.rlib "$work/target/$tgt/release/deps/"*.rmeta \
	"$sysroot/lib/rustlib/$tgt/lib/"
for d in "$rust_sysroot/lib/rustlib/"*/; do
	b="$(basename "$d")"
	[ "$b" = "$tgt" ] || [ -e "$sysroot/lib/rustlib/$b" ] || ln -s "$d" "$sysroot/lib/rustlib/$b"
done
rustc -V > "$sysroot/rustc-version"

printf 'pub fn n() -> usize { vec![1, 2, 3].len() }\n' > "$work/t.rs"
rustc -Zunstable-options --target "$tgt" --sysroot "$sysroot" --crate-type=rlib -C panic=abort \
	"$work/t.rs" -o "$work/t.rlib"
echo "sysroot ready: $sysroot"
