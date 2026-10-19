#!/usr/bin/env bash
# Builds one game's reconstructed sources under src/<game>/ with a given
# MinGW g++, so we don't hand-maintain a growing file list as the directory
# tree grows to mirror the recovered original layout (manifest/source_layout.tsv).
# The files are compiled in parallel into build/<game>/ (only the ones that changed) and then linked.
#
# The Lua sources of ScummVM (common/lua) and the glue of tools/luashim are built in; the ScummVM tree is
# $SCUMMVM_DIR (default /c/dev/scummvm).
#
# SDL=1 builds the standalone player with the real SDL2 (third_party/SDL2, -DUSE_REAL_SDL) into build/<game>-sdl/ and src/<game>/<game>_sdl.exe.
#
# Usage: tools/build.sh <game> [g++ path] [extra g++ args...]
#   tools/build.sh deponia1
#   tools/build.sh deponia1 /c/mingw32/bin/g++.exe
set -euo pipefail

game="${1:?usage: build.sh <game> [g++ path] [extra args...]}"
gxx="${2:-/c/mingw64/bin/g++.exe}"
shift $(( $# >= 2 ? 2 : 1 ))
extra_args=("$@")

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
src_dir="$repo_root/src/$game"
variant=""
if [ "${SDL:-0}" = "1" ]; then variant="-sdl"; fi
out_exe="$src_dir/${game}${variant/-/_}.exe"
obj_dir="$repo_root/build/$game$variant"
scummvm="${SCUMMVM_DIR:-/c/dev/scummvm}"
tp="$repo_root/third_party"
tplib="$repo_root/build/third_party/64"

if [ ! -d "$src_dir" ]; then
    echo "no such source dir: $src_dir" >&2
    exit 1
fi

# (the Lua files of ScummVM that need its file system and persistence are left out, as in tools/tbuild.py)
lua_exclude="scummvm_file.cpp lua_persist.cpp lua_persistence_util.cpp lua_unpersist.cpp liolib.cpp loslib.cpp loadlib.cpp double_serialization.cpp"

mapfile -t sources < <(find "$src_dir" -name '*.cpp' | sort)
if [ -d "$scummvm/common/lua" ]; then
    for f in "$scummvm"/common/lua/*.cpp; do
        case " $lua_exclude " in *" $(basename "$f") "*) continue ;; esac
        sources+=("$f")
    done
    sources+=("$repo_root/tools/luashim/luashim.cpp")
fi
echo "Building ${#sources[@]} source files with $gxx ..." >&2

# the third-party libraries (third_party/, THIRD_PARTY.md) as static libraries
if [ "${SDL:-0}" = "1" ] && [ ! -f "$tplib/libSDL2.a" ]; then
    python "$repo_root/tools/build_third_party.py" sdl2 >&2
fi
if [ ! -f "$tplib/libwebp.a" ] || [ ! -f "$tplib/libfreetype.a" ] || [ ! -f "$tplib/libharfbuzz.a" ] || [ ! -f "$tplib/libvpx.a" ] || [ ! -f "$tplib/libvorbis.a" ]; then
    python "$repo_root/tools/build_third_party.py" >&2
fi

mkdir -p "$obj_dir"
objects=()
jobs=()
for src in "${sources[@]}"; do
    case "$src" in
        "$src_dir"/*) rel="${src#$src_dir/}" ;;
        *) rel="ext/$(basename "$src")" ;;
    esac
    obj="$obj_dir/${rel%.cpp}.o"
    objects+=("$obj")
    if [ ! -f "$obj" ] || [ "$src" -nt "$obj" ] || [ -n "$(find "$src_dir" -name '*.h' -newer "$obj" -print -quit)" ]; then
        jobs+=("$src|$obj")
    fi
done

compile() {
    local src="${1%%|*}" obj="${1##*|}" flags=(-std=c++17 -Wall -Wextra)
    mkdir -p "$(dirname "$obj")"
    case "$src" in "$SRC_DIR"/*) ;; *) flags=(-std=c++17 -w) ;; esac
    "$GXX" "${flags[@]}" -I "$SRC_DIR" -I "$SCUMMVM" -I "$TP/libwebp/src" -I "$TP/opusfile/include" -I "$TP/opus/include" -I "$TP/ogg/include" -I "$TP/freetype/include" -I "$TP/harfbuzz/src" -I "$TP/libvpx" -I "$TP/vorbis/include" -I "$TPLIB/gen" $EXTRA_FLAGS -c "$src" -o "$obj"
}
export -f compile
export GXX="$gxx" SRC_DIR="$src_dir" SCUMMVM="$scummvm" TP="$tp" TPLIB="$tplib"
# (an array cannot be exported to the compile function that xargs runs: the flags are a string)
EXTRA_FLAGS="${extra_args[*]:-}"
if [ "${SDL:-0}" = "1" ]; then EXTRA_FLAGS="$EXTRA_FLAGS -DUSE_REAL_SDL -I $tp/SDL2/include"; fi
export EXTRA_FLAGS

if [ ${#jobs[@]} -gt 0 ]; then
    echo "Compiling ${#jobs[@]} of ${#sources[@]} files ..." >&2
    printf '%s\n' "${jobs[@]}" | xargs -P "$(nproc 2>/dev/null || echo 4)" -I{} bash -c 'compile "$1"' _ {}
fi

sdl_libs=""
if [ "${SDL:-0}" = "1" ]; then
    sdl_libs="$tplib/libSDL2.a -lm -ldinput8 -ldxguid -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lsetupapi -lversion -luuid -ladvapi32 -lkernel32"
fi
"$gxx" -o "$out_exe" "${objects[@]}" "$tplib/libopusfile.a" "$tplib/libopus.a" "$tplib/libvorbis.a" "$tplib/libogg.a" "$tplib/libwebp.a" "$tplib/libharfbuzz.a" "$tplib/libfreetype.a" "$tplib/libvpx.a" $sdl_libs "${extra_args[@]}"
echo "Built $out_exe" >&2
