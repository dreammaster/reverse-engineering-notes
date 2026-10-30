#!/bin/bash
# Builds the host (engine/host/host_test.exe) from xanth.asm / xanth.idb:
#   bash engine/host/build_host.sh <scratch dir>        (run from xanth/; refuses to run while ida.exe runs; work happens on a scratch copy of the IDB)
# 1. memory image + function table from the IDB   2. data symbol addresses of the functions to lift   3. lift 900+ functions (handlers + the game logic they call)
# 4. compile with the host.   The memory image is game data: it is written to game_out/image.bin (git-ignored), generated code goes to engine/host/gen (git-ignored).
set -e
SP="$1"; IDAT="/c/Program Files/IDA Pro 8.3/idat.exe"
if tasklist | grep -qi "ida.exe\|ida64.exe"; then echo "IDA is running - close it first"; exit 1; fi
cp xanth.idb "$SP/x2.idb"
cp reversing/scripts/dump_image.py reversing/scripts/dump_symbols.py "$SP/"
( cd "$SP" && "$IDAT" -A -Lim.log -S"dump_image.py image.bin funcs.json" x2.idb >/dev/null 2>&1 )
cp "$SP/image.bin" game_out/image.bin
( cd reversing/scripts
  python lift_handlers.py symbols "$SP/hsyms.json" --host
  ( cd "$SP" && "$IDAT" -A -Lsy.log -S"dump_symbols.py hsyms.json hsyms_out.json" x2.idb >/dev/null 2>&1 )
  python lift_handlers.py "$SP/hsyms_out.json" "$SP/funcs.json" --host )
cp engine/gen/cpu.h engine/host/gen/
cd engine/host
g++ -std=c++17 -O1 -static -w -DLIFT_STEP_LIMIT=3000000 -I gen -I . -I .. -o host_test.exe host.cpp host_impl.cpp host_machine.cpp host_test.cpp \
    gen/lifted_handlers.cpp gen/lifted_table.cpp gen/lifted_names.cpp gen/host_defaults.cpp ../legend_formats.cpp
echo built engine/host/host_test.exe
