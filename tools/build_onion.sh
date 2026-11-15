#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 4 ]]; then
  echo "usage: $0 TOOLCHAIN_ROOT SDL_SOURCE OUTPUT_DIR TOOLCHAIN_ARCHIVE" >&2
  exit 2
fi

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
toolchain_root="$(realpath "$1")"
sdl_source="$(realpath "$2")"
output_dir="$(mkdir -p "$3" && realpath "$3")"
toolchain_archive="$(realpath "$4")"
mini="$toolchain_root/mini"
prebuilt="$toolchain_root/prebuilt"
sysroot="$mini/arm-buildroot-linux-gnueabihf/sysroot"
compiler="$mini/bin/arm-linux-gnueabihf-g++"
sdl_commit="0631abc8e8916db6f9bc7e2afd0c22913d092a29"
expected_toolchain_sha256="8addff71be4b015a4e1aef51e43635e50978d558a1675f5b1664124e8437d071"
toolchain_sha256="$(sha256sum "$toolchain_archive" | awk '{print $1}')"
[[ "$toolchain_sha256" == "$expected_toolchain_sha256" ]] || {
  echo "unexpected Miyoo toolchain archive SHA-256: $toolchain_sha256" >&2
  exit 1
}
export GWENT_TOOLCHAIN_SHA256="$toolchain_sha256"
if [[ "$toolchain_root" != /opt ]]; then
  echo "the Miyoo v1.0 compiler wrapper requires the SDK to be extracted under /opt" >&2
  exit 1
fi

for path in "$compiler" "$sysroot/usr/include/SDL2/SDL.h" \
  "$sdl_source/prebuilt/640x480/libSDL2-2.0.so.0" \
  "$sdl_source/prebuilt/640x480/libEGL.so" \
  "$sdl_source/prebuilt/640x480/libGLESv2.so"; do
  [[ -e "$path" ]] || { echo "missing Miyoo build dependency: $path" >&2; exit 1; }
done
actual_sdl_commit="$(git -C "$sdl_source" rev-parse HEAD)"
[[ "$actual_sdl_commit" == "$sdl_commit" ]] || {
  echo "expected steward-fu/sdl2 $sdl_commit, got $actual_sdl_commit" >&2
  exit 1
}

links="$output_dir/toolchain-links"
mkdir -p "$links"
ln -sf "$prebuilt/arm-linux-gnueabihf/bin/ld.bfd" "$links/ld"
ln -sf "$prebuilt/arm-linux-gnueabihf/bin/ld.bfd" "$links/arm-linux-gnueabihf-ld"
ln -sf "$prebuilt/arm-linux-gnueabihf/bin/ld.bfd" "$links/arm-linux-gnueabihf-ld.bfd"
export PATH="$links:$prebuilt/arm-linux-gnueabihf/bin:$prebuilt/bin:$mini/bin:$PATH"

# The pinned Mini backend omitted KEY_HOME even though the Miyoo MENU key
# reports that Linux keycode. Keep the change as a small auditable patch and
# compile the library that will actually ship in the port.
sdl_event_source="$sdl_source/sdl2/src/video/mini/SDL_event_mini.c"
if ! grep -Fq 'mymap[KEY_HOME] = SDLK_HOME;' "$sdl_event_source"; then
  git -C "$sdl_source" apply --check "$repo_root/patches/miyoo-sdl-keyhome.patch"
  git -C "$sdl_source" apply "$repo_root/patches/miyoo-sdl-keyhome.patch"
fi

# Normalize text scripts/configuration when the checkout was created on
# Windows; preserve binary SDK/SDL files and their contents.
find "$sdl_source/sdl2" -type f \( -name '*.sh' -o -name '*.ac' -o -name '*.m4' -o -name '*.in' \
  -o -name config.sub -o -name config.guess \) -exec sed -i 's/\r$//' {} +
find "$sdl_source/sdl2/build-scripts" -type f -exec sed -i 's/\r$//' {} +

ln -sfn "$sdl_source/prebuilt/640x480/libEGL.so" "$sdl_source/sdl2/libEGL.so"
ln -sfn "$sdl_source/prebuilt/640x480/libGLESv2.so" "$sdl_source/sdl2/libGLESv2.so"
make -C "$sdl_source" cfg
make -C "$sdl_source" sdl2
sdl_library="$sdl_source/sdl2/build/.libs/libSDL2-2.0.so.0"
[[ -f "$sdl_library" ]] || { echo "Miyoo SDL build did not produce $sdl_library" >&2; exit 1; }

build_dir="$output_dir/build"
if [[ -L "$build_dir" ]]; then
  echo "refusing to use a symlink as the generated build directory: $build_dir" >&2
  exit 1
fi
cmake -E remove_directory "$build_dir"
cmake -S "$repo_root" -B "$build_dir" \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=arm \
  -DCMAKE_SYSROOT="$sysroot" \
  -DCMAKE_CXX_COMPILER="$compiler" \
  -DCMAKE_CXX_FLAGS="-B$links/ -ffile-prefix-map=$repo_root=. -ffile-prefix-map=$sdl_source=third_party/miyoo-sdl2" \
  -DCMAKE_SKIP_BUILD_RPATH=ON \
  -DCMAKE_AR="$prebuilt/arm-linux-gnueabihf/bin/ar" \
  -DCMAKE_RANLIB="$prebuilt/arm-linux-gnueabihf/bin/ranlib" \
  -DCMAKE_NM="$prebuilt/arm-linux-gnueabihf/bin/nm" \
  -DCMAKE_OBJCOPY="$prebuilt/arm-linux-gnueabihf/bin/objcopy" \
  -DCMAKE_OBJDUMP="$prebuilt/arm-linux-gnueabihf/bin/objdump" \
  -DCMAKE_READELF="$prebuilt/arm-linux-gnueabihf/bin/readelf" \
  -DCMAKE_STRIP="$prebuilt/arm-linux-gnueabihf/bin/strip" \
  -DPython3_EXECUTABLE="$(command -v python3)" \
  -DGWENT_WITH_SDL=ON \
  -DGWENT_BUILD_TESTS=OFF \
  -DGWENT_SDL2_INCLUDE_DIR="$sysroot/usr/include/SDL2" \
  -DGWENT_SDL2_LIBRARY="$sdl_library"
cmake --build "$build_dir" --parallel "${JOBS:-2}"

json_lib_dir="$sysroot/usr/lib"
libstdcxx="$json_lib_dir/libstdc++.so.6.0.25"
libgcc="$json_lib_dir/libgcc_s.so.1"
[[ -f "$libstdcxx" && -f "$libgcc" ]] || {
  echo "missing GCC 8 runtime libraries in $json_lib_dir" >&2
  exit 1
}
bash "$repo_root/tools/package_onion.sh" "$build_dir" "$sdl_source" \
  "$json_lib_dir" "$libstdcxx" "$libgcc" "$output_dir"

readelf_bin="$prebuilt/bin/arm-linux-gnueabihf-readelf"
if [[ ! -x "$readelf_bin" ]]; then readelf_bin=arm-linux-gnueabihf-readelf; fi
"$readelf_bin" -h "$build_dir/gwent" | grep -q 'Machine:.*ARM'
"$readelf_bin" -h "$build_dir/gwent" | grep -q 'hard-float ABI'
printf 'Validated ARM hard-float executable: %s\n' "$build_dir/gwent"
