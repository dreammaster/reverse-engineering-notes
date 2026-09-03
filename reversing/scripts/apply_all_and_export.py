"""
IDAPython driver script -- run via IDA's batch mode (idat.exe -A -S) with the
Rob Blanc 1 IDB open. Chains the individual apply_*.py scripts and then
re-exports rob_blanc_1.asm/.idc from the freshly-updated database, so a full
"apply everything and snapshot" pass can happen in one command instead of
several manual Alt-F7 invocations plus manual File > Produce file exports.

Usage (from a shell, NOT from inside this repo's own Python venv -- this
must run under IDA's own bundled Python via idat.exe; run from the repo
root C:\dev\ags so the relative script/idb paths below resolve):

    idat.exe -A -S"reversing/scripts/apply_all_and_export.py" ^
        -L"reversing/scripts/logs/apply_all.log" rob_blanc_1.idb

Steps performed, in order:
  1. apply_matches.py's main()   -- renames + comments from matches.json
  2. apply_structs.py's main()   -- struct/type declarations from SAFE_DECLS
  3. Explicit database save (idat also auto-flushes on exit, but this makes
     the intent explicit and gives an early failure signal if something is
     wrong with the output path).
  4. Re-export rob_blanc_1.asm (OFILE_ASM, with GENFLG_ASMTYPE so struct-typed
     field accesses stay symbolic, matching how the existing .asm was
     produced) and rob_blanc_1.idc (OFILE_IDC) into the repo root.

Safe to re-run: both apply_*.py scripts are themselves idempotent, and
gen_file() always overwrites its target path wholesale.
"""
import os
import sys

try:
    import idc
    import ida_loader
    import ida_auto
    IN_IDA = True
except ImportError:
    IN_IDA = False

SCRIPT_DIR = r"C:\dev\ags\reversing\scripts"
REPO_ROOT = r"C:\dev\ags"


def main():
    if not IN_IDA:
        print("This script must be run inside IDA (idc not importable).")
        return

    if SCRIPT_DIR not in sys.path:
        sys.path.insert(0, SCRIPT_DIR)

    print("=== Waiting for auto-analysis to finish ===")
    ida_auto.auto_wait()

    print("=== Step 1/4: apply_matches.py ===")
    import apply_matches
    apply_matches.main()

    print("=== Step 2/4: apply_structs.py ===")
    import apply_structs
    apply_structs.main()

    print("=== Step 3/4: saving database ===")
    ida_auto.auto_wait()
    idc.save_database(idc.get_idb_path(), 0)
    print("Database saved: " + idc.get_idb_path())

    print("=== Step 4/4: re-exporting .asm/.idc ===")
    min_ea = idc.get_inf_attr(idc.INF_MIN_EA)
    max_ea = idc.get_inf_attr(idc.INF_MAX_EA)

    asm_path = os.path.join(REPO_ROOT, "rob_blanc_1.asm")
    n = idc.gen_file(idc.OFILE_ASM, asm_path, min_ea, max_ea, idc.GENFLG_ASMTYPE)
    print("ASM export -> %s : %d lines (or -1 on error)" % (asm_path, n))

    idc_path = os.path.join(REPO_ROOT, "rob_blanc_1.idc")
    n2 = idc.gen_file(idc.OFILE_IDC, idc_path, min_ea, max_ea, 0)
    print("IDC export -> %s : %d lines (or -1 on error)" % (idc_path, n2))

    print("=== Done ===")
    idc.qexit(0)


if __name__ == "__main__":
    main()
