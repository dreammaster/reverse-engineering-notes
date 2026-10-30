#!/bin/bash
# Regenerates every table that embeds IDB names (run from xanth/ after apply_xanth_manual.py --export xanth.asm):
#   bash reversing/scripts/refresh_tables.sh <scratch dir>
# Uses a scratch copy of xanth.idb for the idat dumps (never touches the original), refuses to run while ida.exe runs.
set -e
SP="$1"; IDAT="/c/Program Files/IDA Pro 8.3/idat.exe"
if tasklist | grep -qi "ida.exe\|ida64.exe"; then echo "IDA is running - close it first"; exit 1; fi
cp xanth.idb "$SP/x2.idb"
cp reversing/scripts/dump_save_regions.py reversing/scripts/dump_conversations.py "$SP/"
( cd "$SP" && "$IDAT" -A -Lsr.log -S"dump_save_regions.py sr.json" x2.idb >/dev/null 2>&1; "$IDAT" -A -Lconv.log -S"dump_conversations.py conv.json" x2.idb >/dev/null 2>&1 )
cd reversing/scripts
python sr_to_tsv.py "$SP/sr.json"
python xanth_conversations.py "$SP/conv.json" ../../game/XANTHSTR.DAT ../../game_out
python extract_xanth_digest.py ../../xanth.asm ../analysis/xanth_handler_digest.txt
python xanth_flags.py
