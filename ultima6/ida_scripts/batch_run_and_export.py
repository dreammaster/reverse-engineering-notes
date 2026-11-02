"""
Headless driver: run a target script against whichever ultima6 .idb was passed
to idat.exe, then export <same-stem>.asm/.idc and save, all without opening the
GUI.

Derived from the ultima1 driver. Ultima 6 has several executables per game
(ultima6/u6, ultima6/md, ultima6/se), each with its own .idb, so the export
paths are derived from whatever IDB idat.exe actually opened.

USAGE (normally via run_ida_script.ps1; the IDA GUI must be closed first):

    idat.exe -A -S"batch_run_and_export.py <script.py> [noexport]" <db.idb>

The first ARGV entry after the driver's own path is the target script to exec.
It runs exactly as it would under Alt+F7 in the GUI; this wrapper only adds the
analysis-wait, export, save and exit around it. Pass "noexport" as the second
entry for read-only report scripts.

Every step is logged to batch_run_and_export.log (next to this file) via plain
file I/O, because idat.exe's console output in -A mode is not reliably flushed.
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


def export(fh, kind, path):
    log(fh, f"[*] exporting to {path}")
    fp = ida_diskio.fopenWT(path)
    if fp is None:
        raise RuntimeError(f"fopenWT failed for {path}")
    try:
        ok = ida_loader.gen_file(kind, fp, 0, idc.BADADDR, 0)
    finally:
        ida_diskio.eclose(fp)
    log(fh, f"[*] gen_file returned {ok}")


def main():
    with open(LOG_PATH, "a") as fh:
        log(fh, "=" * 60)
        log(fh, "[*] batch_run_and_export starting")
        try:
            log(fh, "[*] auto_wait (initial)")
            ida_auto.auto_wait()

            idb_path = idc.get_idb_path()
            stem = os.path.splitext(idb_path)[0]
            log(fh, f"[*] idb = {idb_path}")

            argv = idc.ARGV
            log(fh, f"[*] ARGV = {list(argv)}")
            if len(argv) < 2:
                log(fh, "[!] usage: -S\"batch_run_and_export.py <script_to_run.py> [noexport]\"")
                idc.qexit(1)
                return

            target_script = argv[1]
            do_export = not (len(argv) >= 3 and argv[2] == "noexport")

            log(fh, f"[*] executing {target_script}")
            with open(target_script, "r") as f:
                code = f.read()
            g = {"__name__": "__main__", "__file__": target_script}
            captured = io.StringIO()
            try:
                with contextlib.redirect_stdout(captured):
                    exec(compile(code, target_script, "exec"), g)
            finally:
                log(fh, "----- begin target script output -----")
                log(fh, captured.getvalue().rstrip("\n"))
                log(fh, "----- end target script output -----")

            log(fh, "[*] auto_wait (post-script)")
            ida_auto.auto_wait()

            if do_export:
                export(fh, ida_loader.OFILE_ASM, stem + ".asm")
                export(fh, ida_loader.OFILE_IDC, stem + ".idc")
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
