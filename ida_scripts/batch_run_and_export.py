r"""
Headless driver: run a target script against whichever .idb was passed to
idat.exe, then export <same-stem>.asm/.idc and save, all without opening the GUI.

Shared by every game in this repo (mm2/, ...).  The export paths are derived from
the IDB idat.exe actually opened (idc.get_idb_path()).  Normally driven through
run_ida_script.ps1 (same folder); to call it directly:

    "C:\Program Files\IDA Pro 8.3\idat.exe" -A ^
        -S"C:\dev\might_and_magic\ida_scripts\batch_run_and_export.py C:\path\to\script.py" ^
        "C:\dev\might_and_magic\mm2\mm2.idb"

If the target script itself doesn't want an export (e.g. a pure read-only
discovery/report script), pass a second ARGV entry of "noexport" to skip
the asm/idc export and database save steps:

    -S"batch_run_and_export.py identify.py noexport"

Every step is logged to batch_run_and_export.log via plain Python file
I/O rather than print()/msg() -- console output from idat.exe in -A mode
is not reliable, so the log file is the source of truth for what happened
on any given run, including exceptions. The target script's own stdout is
captured and appended to the log too.

Pattern carried over from the ultima2 project's ida_scripts driver
(confirmed working there 2026-08-18, IDA Pro 8.3, idat.exe, 16-bit idb).
Two non-obvious things it needed:
  - ida_loader.gen_file()'s fp argument needs a real SWIG FILE* --
    ida_diskio.fopenWT()/eclose(), not a plain Python open() handle
    (that raises "TypeError: argument 2 of type 'FILE *'").
  - idat.exe's stdout/msg() output is not reliably flushed/visible
    before qexit(); don't depend on console output for diagnosis.
"""

import contextlib
import io
import os
import traceback

import idc
import ida_auto
import ida_diskio
import ida_loader

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
LOG_PATH = os.path.join(SCRIPT_DIR, "batch_run_and_export.log")


def log(fh, msg):
    fh.write(msg + "\n")
    fh.flush()


def main():
    with open(LOG_PATH, "a") as fh:
        log(fh, "=" * 60)
        log(fh, "[*] batch_run_and_export starting")
        try:
            log(fh, "[*] auto_wait (initial)")
            ida_auto.auto_wait()

            idb_path = idc.get_idb_path()
            stem = os.path.splitext(idb_path)[0]
            asm_path = stem + ".asm"
            idc_path = stem + ".idc"
            log(fh, f"[*] idb = {idb_path}")

            argv = idc.ARGV
            log(fh, f"[*] ARGV = {list(argv)}")
            if len(argv) < 2:
                log(fh, "[!] usage: -S\"batch_run_and_export.py <script_to_run.py> [noexport]\"")
                idc.qexit(1)
                return

            target_script = argv[1]
            do_export = not (len(argv) >= 3 and argv[2] == "noexport")

            log(fh, f"[*] reading {target_script}")
            with open(target_script, "r") as f:
                code = f.read()

            log(fh, f"[*] executing {target_script}")
            g = {"__name__": "__main__", "__file__": target_script}
            captured = io.StringIO()
            with contextlib.redirect_stdout(captured):
                exec(compile(code, target_script, "exec"), g)
            log(fh, "[*] captured stdout from target script:")
            log(fh, "----- begin target script output -----")
            log(fh, captured.getvalue().rstrip("\n"))
            log(fh, "----- end target script output -----")
            log(fh, f"[*] finished executing {target_script}")

            log(fh, "[*] auto_wait (post-script)")
            ida_auto.auto_wait()

            # A target script may restrict the export to one address range (used for the
            # MM2 overlay databases, which embed the whole resident image but should only
            # export their own overlay code):  EXPORT_RANGE = (start_ea, end_ea)
            ea1, ea2 = g.get("EXPORT_RANGE", (0, idc.BADADDR))

            if do_export:
                log(fh, f"[*] exporting ASM to {asm_path}")
                fp = ida_diskio.fopenWT(asm_path)
                if fp is None:
                    raise RuntimeError(f"fopenWT failed for {asm_path}")
                try:
                    ok = ida_loader.gen_file(ida_loader.OFILE_ASM, fp, ea1, ea2, 0)
                finally:
                    ida_diskio.eclose(fp)
                log(fh, f"[*] gen_file(OFILE_ASM) returned {ok}")

                log(fh, f"[*] exporting IDC to {idc_path}")
                fp = ida_diskio.fopenWT(idc_path)
                if fp is None:
                    raise RuntimeError(f"fopenWT failed for {idc_path}")
                try:
                    ok = ida_loader.gen_file(ida_loader.OFILE_IDC, fp, ea1, ea2, 0)
                finally:
                    ida_diskio.eclose(fp)
                log(fh, f"[*] gen_file(OFILE_IDC) returned {ok}")

                log(fh, f"[*] saving database to {idb_path}")
                ida_loader.save_database(idb_path, 0)
            else:
                log(fh, "[*] noexport requested -- skipping asm/idc export and save")

            log(fh, "[*] done, exiting 0")
            idc.qexit(0)

        except Exception:
            log(fh, "[!] EXCEPTION:")
            log(fh, traceback.format_exc())
            idc.qexit(1)


main()
