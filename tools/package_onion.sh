#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 6 ]]; then
  echo "usage: $0 BUILD_DIR SDL_SOURCE JSON_C_LIBRARY_DIR LIBSTDCXX_FILE LIBGCC_FILE OUTPUT_DIR" >&2
  exit 2
fi

build_dir="$(realpath "$1")"
sdl_source="$(realpath "$2")"
json_lib_dir="$(realpath "$3")"
libstdcxx_file="$(realpath "$4")"
libgcc_file="$(realpath "$5")"
output_dir="$(mkdir -p "$6" && realpath "$6")"
stage="$(mktemp -d)"
trap 'rm -rf "$stage"' EXIT

expected_sdl_commit="0631abc8e8916db6f9bc7e2afd0c22913d092a29"
actual_sdl_commit="$(git -C "$sdl_source" rev-parse HEAD)"
if [[ "$actual_sdl_commit" != "$expected_sdl_commit" ]]; then
  echo "expected steward-fu/sdl2 $expected_sdl_commit, got $actual_sdl_commit" >&2
  exit 1
fi

game_dir="$stage/Roms/PORTS/Games/GwentClassic"
shortcut_dir="$stage/Roms/PORTS/Shortcuts/Cards"
mkdir -p "$game_dir" "$shortcut_dir"
mkdir -p "$game_dir/assets/gwent-classic"
install -m 755 "$build_dir/gwent" "$game_dir/gwent"
install -m 755 "$(dirname "$0")/../packaging/onion/launch.sh" "$game_dir/launch.sh"
install -m 755 "$(dirname "$0")/../packaging/onion/GwentClassic.notfound" "$shortcut_dir/GwentClassic.notfound"
cp -a assets/gwent-classic/img assets/gwent-classic/sfx assets/gwent-classic/svg assets/gwent-classic/music "$game_dir/assets/gwent-classic/"

copy_shared() {
  local source="$1" name="$2"
  if [[ ! -e "$source" ]]; then echo "missing runtime library: $source" >&2; exit 1; fi
  install -m 644 "$source" "$game_dir/$name"
}

copy_text() {
  local source="$1" name="$2"
  tr -d '\r' < "$source" > "$game_dir/$name"
  chmod 644 "$game_dir/$name"
}

copy_shared "$sdl_source/sdl2/build/.libs/libSDL2-2.0.so.0" libSDL2-2.0.so.0
copy_shared "$sdl_source/prebuilt/640x480/libEGL.so" libEGL.so
copy_shared "$sdl_source/prebuilt/640x480/libGLESv2.so" libGLESv2.so
copy_shared "$(find "$json_lib_dir" -name 'libjson-c.so.5*' -type f -print -quit)" libjson-c.so.5
copy_shared "$libstdcxx_file" libstdc++.so.6
copy_shared "$libgcc_file" libgcc_s.so.1

copy_text LICENSE LICENSE.GwentClassic.txt
copy_text LICENSES.md LICENSES.GwentClassic.md
copy_text third_party/gcc-runtime/COPYING3 LICENSE.GCC-runtime-GPL-3.0.txt
copy_text third_party/gcc-runtime/COPYING.RUNTIME LICENSE.GCC-runtime-exception-3.1.txt
copy_text "$sdl_source/LICENSE" LICENSE.SDL2-port.txt
copy_text "$sdl_source/sdl2/LICENSE.txt" LICENSE.SDL2-core.txt
copy_text "$sdl_source/swiftshader/LICENSE.txt" LICENSE.SwiftShader.txt
install -m 644 "$(dirname "$0")/../patches/miyoo-sdl-keyhome.patch" "$game_dir/SDL-MIYOO-KEYHOME.patch"
install -m 644 "$sdl_source/sdl2/dependency/json-c-0.15.tar.gz" "$game_dir/json-c-0.15-source.tar.gz"
copy_text packaging/onion/THIRD_PARTY.txt THIRD_PARTY.txt
copy_text assets/gwent-classic/ASSET_ORIGIN.md assets/gwent-classic/ASSET_ORIGIN.md
printf 'steward-fu/sdl2 commit %s\nMiyoo key mapping patch: SDL-MIYOO-KEYHOME.patch\n' "$actual_sdl_commit" > "$game_dir/BUILD-SDL.txt"
{
  printf 'SDL2 source: steward-fu/sdl2 %s\n' "$actual_sdl_commit"
  printf 'Miyoo toolchain: steward-fu mini_toolchain-v1.0\n'
  printf 'Miyoo toolchain archive SHA-256: %s\n' "${GWENT_TOOLCHAIN_SHA256:-not recorded}"
  printf 'Target: Miyoo Mini Plus, 32-bit ARM hard-float, OnionOS\n'
  printf 'GCC runtime: libstdc++ and libgcc_s from GCC 8.2.1\n'
} > "$game_dir/BUILD-INFO.txt"
if [[ -f "$sdl_source/COPYING.RUNTIME" ]]; then
  install -m 644 "$sdl_source/COPYING.RUNTIME" "$game_dir/LICENSE.GCC-runtime.txt"
fi

# Normalize mode bits so archive metadata is stable across build hosts.
find "$stage/Roms" -type d -exec chmod 755 {} +
find "$stage/Roms" -type f -exec chmod 644 {} +
chmod 755 "$game_dir/gwent" "$game_dir/launch.sh" "$shortcut_dir/GwentClassic.notfound"

archive="$output_dir/GwentClassic-OnionOS.zip"
rm -f "$archive"
(cd "$stage" && find Roms -exec touch -h -d '1980-01-01 00:00:00 UTC' {} + && find Roms -print | LC_ALL=C sort | zip -X -q "$archive" -@)
sha256sum "$archive" | tee "$archive.sha256"
