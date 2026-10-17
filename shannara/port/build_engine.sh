#!/bin/bash
# Builds port/build/engine_tool.exe: the hand-written runtime plus the transpiled game logic (port/gen, produced by
# reversing/scripts/transpile_handlers.py - see port/README.md). Run from the repository's shannara/ directory.
set -e
mkdir -p port/build/gen port/build/rt
for f in le machine engine shanstr objectdat; do
  g++ -std=c++17 -O1 -w -Iport/src -c port/src/$f.cpp -o port/build/rt/$f.o &
done
ls port/gen/*.cpp | xargs -P 8 -I{} sh -c 'g++ -std=c++17 -O1 -w -Iport/src -c {} -o port/build/gen/$(basename {} .cpp).o'
wait
g++ -std=c++17 -O1 -w -Iport/src -c port/src/engine_tool.cpp -o port/build/rt/engine_tool.o
g++ port/build/rt/*.o port/build/gen/*.o -o port/build/engine_tool.exe
echo built port/build/engine_tool.exe
