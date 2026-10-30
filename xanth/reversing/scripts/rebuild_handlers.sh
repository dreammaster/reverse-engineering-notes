#!/bin/bash
# Rebuilds engine/gen (run from xanth/reversing/scripts after xanth.asm was re-exported):
#   bash rebuild_handlers.sh <scratch dir>        (scratch dir is used for a copy of xanth.idb; refuses to run while ida.exe runs)
# 1. digest -> 2. every handler translated from the digest -> 3. symbol dump + every handler lifted from the assembly (test build in engine/gen_all)
# 4. both versions run under mock hosts and compared -> handlers whose traces differ go to engine/gen/handlers_mismatch.txt
# 5. regenerate: digest translation only for the agreeing handlers, instruction-level lift for the rest.
set -e
SP="$1"; IDAT="/c/Program Files/IDA Pro 8.3/idat.exe"
if tasklist | grep -qi "ida.exe\|ida64.exe"; then echo "IDA is running - close it first"; exit 1; fi
python extract_xanth_digest.py ../../xanth.asm ../analysis/xanth_handler_digest.txt
rm -f ../../engine/gen/handlers_mismatch.txt
python gen_handlers.py
python lift_handlers.py symbols "$SP/asyms.json" --all
cp ../../xanth.idb "$SP/x2.idb"; cp dump_symbols.py "$SP/"
( cd "$SP" && "$IDAT" -A -Lsy.log -S"dump_symbols.py asyms.json asyms_out.json" x2.idb >/dev/null 2>&1 )
python lift_handlers.py "$SP/asyms_out.json" --all
cp ../../engine/gen/cpu.h ../../engine/gen_all/cpu.h
python make_trace_test.py
python gen_handlers.py
python lift_handlers.py "$SP/asyms_out.json"
python lift_handlers.py "$SP/asyms_out.json" --all
cp ../../engine/gen/cpu.h ../../engine/gen_all/cpu.h
python make_trace_test.py
