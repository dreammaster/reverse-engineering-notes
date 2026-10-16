#!/bin/bash
# Safe headless workflow: apply an IDAPython script to a COPY of an IDB, check it,
# back the original up and install the result.
#   reversing/scripts/apply_to_idb.sh Shannara.idb reversing/scripts/apply_retail_objects.py [--export Shannara.asm]
# Run from the project root (c:/dev/legend/shannara). Refuses to run while ida.exe is running.
set -e
IDB="$1"; SCRIPT="$2"; shift 2
EXPORT=""; [ "$1" = "--export" ] && EXPORT="$2"
IDAT="/c/Program Files/IDA Pro 8.3/idat.exe"
if tasklist | grep -qi "ida.exe\|ida64.exe"; then echo "IDA is running - close it first"; exit 1; fi
WORK="$(mktemp -d)"; ROOT="$(pwd)"
cp "$IDB" "$WORK/work.idb"
mkdir -p "$WORK/reversing" && cp -r reversing/scripts reversing/analysis "$WORK/reversing/"
( cd "$WORK" && timeout 280 "$IDAT" -A -L"ida.log" -S"reversing/scripts/$(basename "$SCRIPT")" work.idb > run.log 2>&1 ) || { echo "script run failed/timed out"; grep -A6 Traceback "$WORK/ida.log" || tail -20 "$WORK/ida.log"; exit 1; }
if grep -q Traceback "$WORK/ida.log"; then echo "script raised an exception:"; grep -A6 Traceback "$WORK/ida.log"; exit 1; fi
grep -E "renamed|skipped|WARN|Traceback|Error" "$WORK/run.log" | head -20 || true
STAMP=$(date +%Y%m%d_%H%M%S)
cp "$IDB" "reversing/backups/$(basename "$IDB" .idb).$STAMP.bak"
cp "$WORK/work.idb" "$IDB"
echo "installed (backup reversing/backups/$(basename "$IDB" .idb).$STAMP.bak)"
if [ -n "$EXPORT" ]; then
  cp "$WORK/work.idb" "$WORK/exp.idb"
  ( cd "$WORK" && timeout 280 "$IDAT" -A -S"reversing/scripts/unfold_functions.py" exp.idb >/dev/null 2>&1; timeout 280 "$IDAT" -A -S"reversing/scripts/export_asm.py out.asm" exp.idb >/dev/null 2>&1 )
  cp "$WORK/out.asm" "$EXPORT"; echo "exported $EXPORT"
fi
rm -rf "$WORK"
