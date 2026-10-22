"""
Headless driver: run a target script against one of the yendor1 .idb files
(sw.idb, prog0.idb, ...) via idat.exe, then export <name>.asm/<name>.idc and
save, all without opening the GUI. Derived from the yendor2/yendor3 driver of
the same name; the export paths come from the open database, so it is not tied
to any one executable.

Normally invoked through run_ida_script.ps1 / create_idb.ps1, which build the
idat.exe command line. The first idc.ARGV entry after the driver's own path is
the target script to exec; a second ARGV entry of "noexport" skips the asm/idc
export and the database save (read-only report scripts only -- idat.exe still
commits database writes on exit regardless, see run_ida_script.ps1).

Every step is logged to batch_run_and_export.log via plain Python file I/O
rather than print()/msg() -- console output from idat.exe in -A mode is not
reliably flushed before exit. The target script's own stdout is captured and
appended to the log too.

  - ida_loader.gen_file()'s fp argument needs a real SWIG FILE* --
    ida_diskio.fopenWT()/eclose(), not a plain Python open() handle.
"""

import contextlib
import io
import os
import sys
import traceback

import idc
import ida_auto
import ida_diskio
import ida_loader

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
LOG_PATH = os.path.join(SCRIPT_DIR, "batch_run_and_export.log")

# target scripts may `import vid_api` etc. from this directory
if SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SCRIPT_DIR)


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
