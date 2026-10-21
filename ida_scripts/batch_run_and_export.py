"""
Headless driver: run a target script against whichever .idb was passed to
idat.exe, then export <same-stem>.asm/.idc and save, all without opening
the GUI. Export paths are derived from idc.get_idb_path().

Normally launched via run_ida_script.ps1. The first ARGV entry after the
driver is the target script; an optional second entry "noexport" skips the
asm/idc export and database save (for read-only discovery scripts).

Every step is logged to batch_run_and_export.log via plain file I/O, since
idat.exe -A console output is unreliable. Derived from the ultima project's
driver (IDA Pro 8.3, 16-bit idb).
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

            if do_export:
                log(fh, f"[*] exporting ASM to {asm_path}")
                fp = ida_diskio.fopenWT(asm_path)
                if fp is None:
                    raise RuntimeError(f"fopenWT failed for {asm_path}")
                try:
                    ok = ida_loader.gen_file(ida_loader.OFILE_ASM, fp, 0, idc.BADADDR, 0)
                finally:
                    ida_diskio.eclose(fp)
                log(fh, f"[*] gen_file(OFILE_ASM) returned {ok}")

                log(fh, f"[*] exporting IDC to {idc_path}")
                fp = ida_diskio.fopenWT(idc_path)
                if fp is None:
                    raise RuntimeError(f"fopenWT failed for {idc_path}")
                try:
                    ok = ida_loader.gen_file(ida_loader.OFILE_IDC, fp, 0, idc.BADADDR, 0)
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
