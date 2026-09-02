"""
Headless driver: run a target script against whichever *.idb idat.exe opened,
then export <stem>.asm / <stem>.idc and save -- no GUI.

    "C:\\Program Files\\IDA Pro 8.3\\idat.exe" -A ^
        -S"ida_scripts\\batch_run_and_export.py ida_scripts\\some_script.py [noexport]" ^
        system_interp.idb

Pattern carried over verbatim from the ultima1 project (confirmed working
2026-08, IDA 8.3, idat.exe, 16-bit idb). Non-obvious bits:
  - ida_loader.gen_file() needs a real SWIG FILE* from ida_diskio.fopenWT(),
    not a Python handle.
  - idat.exe stdout/msg() is not reliably flushed before qexit(); the log
    file is the source of truth.
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
            ida_auto.auto_wait()

            idb_path = idc.get_idb_path()
            stem = os.path.splitext(idb_path)[0]
            asm_path = stem + ".asm"
            idc_path = stem + ".idc"
            log(fh, f"[*] idb = {idb_path}")

            argv = idc.ARGV
            log(fh, f"[*] ARGV = {list(argv)}")
            if len(argv) < 2:
                log(fh, '[!] usage: -S"batch_run_and_export.py <script.py> [noexport]"')
                idc.qexit(1)
                return

            target_script = argv[1]
            do_export = not (len(argv) >= 3 and argv[2] == "noexport")

            with open(target_script, "r") as f:
                code = f.read()

            log(fh, f"[*] executing {target_script}")
            g = {"__name__": "__main__", "__file__": target_script}
            captured = io.StringIO()
            with contextlib.redirect_stdout(captured):
                exec(compile(code, target_script, "exec"), g)
            log(fh, "----- begin target script output -----")
            log(fh, captured.getvalue().rstrip("\n"))
            log(fh, "----- end target script output -----")

            ida_auto.auto_wait()

            if do_export:
                for kind, path in ((ida_loader.OFILE_ASM, asm_path),
                                   (ida_loader.OFILE_IDC, idc_path)):
                    fp = ida_diskio.fopenWT(path)
                    if fp is None:
                        raise RuntimeError(f"fopenWT failed for {path}")
                    try:
                        ok = ida_loader.gen_file(kind, fp, 0, idc.BADADDR, 0)
                    finally:
                        ida_diskio.eclose(fp)
                    log(fh, f"[*] gen_file -> {path}: {ok}")
                ida_loader.save_database(idb_path, 0)
                log(fh, "[*] saved database")
            else:
                log(fh, "[*] noexport -- skipped export/save")

            log(fh, "[*] done, exiting 0")
            idc.qexit(0)

        except Exception:
            log(fh, "[!] EXCEPTION:")
            log(fh, traceback.format_exc())
            idc.qexit(1)


main()
