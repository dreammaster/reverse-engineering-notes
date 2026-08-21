"""
Headless driver: run a target script against whichever gate*.idb (this
project's two databases -- gate.idb, gatemain.idb) was passed to idat.exe,
then export <same-stem>.asm/.idc and save, all without opening the GUI.

Ported verbatim from the sibling ultima1 project's ida_scripts driver
(itself confirmed working 2026-08-18/19 against a 16-bit MS-DOS idb) --
this file derives the export paths from whatever IDB idat.exe actually
opened (idc.get_idb_path()), so it works unchanged for either of
Gateway's two databases without hardcoding a filename.

USAGE (from a shell, IDA GUI must be closed first -- the .idb gets locked):

    "C:\\Program Files\\IDA Pro 8.3\\idat.exe" -A ^
        -S"C:\\dev\\legend\\gateway\\ida_scripts\\batch_run_and_export.py C:\\dev\\legend\\gateway\\ida_scripts\\some_script.py" ^
        "C:\\dev\\legend\\gateway\\gatemain.idb"

The first idc.ARGV entry after the driver's own path is the target script
to exec (e.g. an apply_*.py or a one-off fix/discovery script). That
script's own main()/module-level code runs exactly as it would under
Alt+F7 -- this wrapper just adds the analysis-wait, export, save, and exit
around it.

If the target script itself doesn't want an export (e.g. a pure read-only
discovery/report script), pass a second ARGV entry of "noexport" to skip
the asm/idc export and database save steps:

    -S"batch_run_and_export.py identify.py noexport"

Every step is logged to batch_run_and_export.log via plain Python file
I/O rather than print()/msg() -- console output from idat.exe in -A mode
is not reliable, so the log file is the source of truth for what happened
on any given run, including exceptions. The target script's own stdout is
captured and appended to the log too.

Two non-obvious things this needed (carried over from ultima1/ultima2):
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
