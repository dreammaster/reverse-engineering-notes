#!/bin/bash
# Builds port/build/full_tool.exe: the whole-program recompilation (game_out/gen_full, from transpile_handlers.py --full) on the host layer (DOS/DPMI/VESA/mouse emulation).
# Run from the repository's shannara/ directory after generating game_out/gen_full (see port/README.md).
set -e
GEN=${GEN:-game_out/gen_full}
mkdir -p port/build/genfull port/build/rtfull
for f in le machine crt host png; do
  g++ -std=c++17 -O1 -w -Iport/src -c port/src/$f.cpp -o port/build/rtfull/$f.o &
done
ls $GEN/*.cpp | xargs -P 8 -I{} sh -c 'g++ -std=c++17 -O1 -w -Iport/src -c {} -o port/build/genfull/$(basename {} .cpp).o'
wait
g++ -std=c++17 -O1 -w -Iport/src -c port/src/full_tool.cpp -o port/build/rtfull/full_tool.o
g++ -static-libstdc++ -static-libgcc $(ls port/build/rtfull/*.o | grep -v win_front.o) port/build/genfull/*.o -o port/build/full_tool.exe
g++ -std=c++17 -O1 -w -Iport/src -c port/src/win_front.cpp -o port/build/rtfull/win_front.o
g++ -static-libstdc++ -static-libgcc $(ls port/build/rtfull/*.o | grep -v full_tool.o) port/build/genfull/*.o -lgdi32 -luser32 -o port/build/shannara_win.exe
echo built port/build/full_tool.exe and shannara_win.exe
