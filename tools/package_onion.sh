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

game_dir="$stage/Roms/PORTS/Games/GwentClassic"
shortcut_dir="$stage/Roms/PORTS/Shortcuts/Cards"
mkdir -p "$game_dir" "$shortcut_dir"
mkdir -p "$game_dir/assets/gwent-classic"
install -m 755 "$build_dir/gwent" "$game_dir/gwent"
install -m 755 "$(dirname "$0")/../packaging/onion/launch.sh" "$game_dir/launch.sh"
install -m 755 "$(dirname "$0")/../packaging/onion/GwentClassic.notfound" "$shortcut_dir/GwentClassic.notfound"
cp -a assets/gwent-classic/img assets/gwent-classic/sfx assets/gwent-classic/svg "$game_dir/assets/gwent-classic/"

copy_shared() {
  local source="$1" name="$2"
  if [[ ! -e "$source" ]]; then echo "missing runtime library: $source" >&2; exit 1; fi
  install -m 644 "$source" "$game_dir/$name"
}

copy_shared "$sdl_source/prebuilt/640x480/libSDL2-2.0.so.0" libSDL2-2.0.so.0
copy_shared "$sdl_source/prebuilt/640x480/libEGL.so" libEGL.so
copy_shared "$sdl_source/prebuilt/640x480/libGLESv2.so" libGLESv2.so
copy_shared "$(find "$json_lib_dir" -name 'libjson-c.so.5*' -type f -print -quit)" libjson-c.so.5
copy_shared "$libstdcxx_file" libstdc++.so.6
copy_shared "$libgcc_file" libgcc_s.so.1

install -m 644 LICENSE "$game_dir/LICENSE.GwentClassic.txt"
install -m 644 LICENSES.md "$game_dir/LICENSES.GwentClassic.md"
install -m 644 "$sdl_source/LICENSE" "$game_dir/LICENSE.SDL2-port.txt"
install -m 644 "$sdl_source/sdl2/LICENSE.txt" "$game_dir/LICENSE.SDL2-core.txt"
install -m 644 "$sdl_source/swiftshader/LICENSE.txt" "$game_dir/LICENSE.SwiftShader.txt"
install -m 644 "$sdl_source/sdl2/dependency/json-c-0.15.tar.gz" "$game_dir/json-c-0.15-source.tar.gz"
install -m 644 packaging/onion/THIRD_PARTY.txt "$game_dir/THIRD_PARTY.txt"
install -m 644 assets/gwent-classic/ASSET_ORIGIN.md "$game_dir/assets/gwent-classic/ASSET_ORIGIN.md"
printf '%s\n' 'steward-fu/sdl2 commit 0631abc8e8916db6f9bc7e2afd0c22913d092a29' > "$game_dir/BUILD-SDL.txt"

archive="$output_dir/GwentClassic-OnionOS.zip"
rm -f "$archive"
(cd "$stage" && zip -qr "$archive" Roms)
sha256sum "$archive" | tee "$archive.sha256"
