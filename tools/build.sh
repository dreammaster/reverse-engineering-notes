#!/usr/bin/env bash
# Builds one game's reconstructed sources under src/<game>/ with a given
# MinGW g++, so we don't hand-maintain a growing file list as the directory
# tree grows to mirror the recovered original layout (manifest/source_layout.tsv).
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

if [ ! -d "$src_dir" ]; then
    echo "no such source dir: $src_dir" >&2
    exit 1
fi

mapfile -t sources < <(find "$src_dir" -name '*.cpp' | sort)
echo "Building ${#sources[@]} source files with $gxx ..." >&2

"$gxx" -std=c++17 -Wall -Wextra -I "$src_dir" -o "$out_exe" "${sources[@]}" "${extra_args[@]}"
echo "Built $out_exe" >&2
