#!/bin/bash
# Produces game_out/insns_full.json: the decoded instructions of EVERY function of the retail program (game layer + engine/library half) plus the extra entry points
# IDA does not know (extra_entries.txt), for transpile_handlers.py --full. Works on a COPY of Shannara.idb (never touches the live one).
#   bash reversing/scripts/dump_full.sh        (run from the project root, c:/dev/legend/shannara; IDA must not be running)
set -e
IDAT="/c/Program Files/IDA Pro 8.3/idat.exe"
if tasklist | grep -qi "ida.exe\|ida64.exe"; then echo "IDA is running - close it first"; exit 1; fi
mkdir -p game_out
# every function of the function map (name + address), the game layer too
awk -F'\t' 'NR>1 { if (strtonum($1) < 0x9D000) print $2, $1 }' reversing/analysis/retail_function_map.tsv > game_out/all_names.txt   # code object only (IDA sometimes names data as functions)
cat reversing/scripts/extra_entries.txt >> game_out/all_names.txt
W=$(mktemp -d)
cp Shannara.idb "$W/work.idb"
cp game_out/all_names.txt "$W/names.txt"
cp reversing/scripts/dump_insns.py "$W/"
( cd "$W" && timeout 900 "$IDAT" -A -Lida.log -S"dump_insns.py names.txt out.json" work.idb > run.log 2>&1 ) || { echo "dump failed"; tail -20 "$W/ida.log"; exit 1; }
if grep -q Traceback "$W/ida.log"; then grep -A6 Traceback "$W/ida.log"; exit 1; fi
cp "$W/out.json" game_out/insns_full.json
rm -rf "$W"
python -c "import json; d=json.load(open('game_out/insns_full.json')); print(len(d), 'functions,', sum(len(v['insns']) for v in d.values()), 'instructions')"
