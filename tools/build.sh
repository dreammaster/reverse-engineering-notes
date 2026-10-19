#!/usr/bin/env bash
# Builds one game's reconstructed sources under src/<game>/ with a given
# MinGW g++, so we don't hand-maintain a growing file list as the directory
# tree grows to mirror the recovered original layout (manifest/source_layout.tsv).
# The files are compiled in parallel into build/<game>/ (only the ones that changed) and then linked.
#
# The Lua sources of ScummVM (common/lua) and the glue of tools/luashim are built in; the ScummVM tree is
# $SCUMMVM_DIR (default /c/dev/scummvm).
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
out_exe="$src_dir/${game}.exe"
obj_dir="$repo_root/build/$game"
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
if [ ! -f "$tplib/libwebp.a" ] || [ ! -f "$tplib/libfreetype.a" ] || [ ! -f "$tplib/libharfbuzz.a" ] || [ ! -f "$tplib/libvpx.a" ]; then
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
    "$GXX" "${flags[@]}" -I "$SRC_DIR" -I "$SCUMMVM" -I "$TP/libwebp/src" -I "$TP/opusfile/include" -I "$TP/opus/include" -I "$TP/ogg/include" -I "$TP/freetype/include" -I "$TP/harfbuzz/src" -I "$TP/libvpx" -I "$TPLIB/gen" "${EXTRA[@]}" -c "$src" -o "$obj"
}
export -f compile
export GXX="$gxx" SRC_DIR="$src_dir" SCUMMVM="$scummvm" TP="$tp" TPLIB="$tplib"
EXTRA=("${extra_args[@]}")
export EXTRA

if [ ${#jobs[@]} -gt 0 ]; then
    echo "Compiling ${#jobs[@]} of ${#sources[@]} files ..." >&2
    printf '%s\n' "${jobs[@]}" | xargs -P "$(nproc 2>/dev/null || echo 4)" -I{} bash -c 'compile "$1"' _ {}
fi

"$gxx" -o "$out_exe" "${objects[@]}" "$tplib/libopusfile.a" "$tplib/libopus.a" "$tplib/libogg.a" "$tplib/libwebp.a" "$tplib/libharfbuzz.a" "$tplib/libfreetype.a" "$tplib/libvpx.a" "${extra_args[@]}"
echo "Built $out_exe" >&2
