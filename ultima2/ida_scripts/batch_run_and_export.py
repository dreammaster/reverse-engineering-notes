"""
Headless driver: run one of the apply_*.py scripts against ultima2.idb via
idat.exe, then export ultima2.asm/.idc and save, all without opening the GUI.

USAGE (from a shell, IDA GUI must be closed first -- the .idb gets locked):

    "C:\\Program Files\\IDA Pro 8.3\\idat.exe" -A ^
        -S"C:\\dev\\ultima2\\ida_scripts\\batch_run_and_export.py C:\\dev\\ultima2\\ida_scripts\\apply_structs.py" ^
        "C:\\dev\\ultima2\\ultima2.idb"

The first idc.ARGV entry after the script path is the target script to exec
(e.g. apply_renames.py, apply_structs.py, or a one-off fix script that
already has DRY_RUN = False). That script's own main()/module-level code
runs exactly as it would under Alt+F7 -- this wrapper just adds the
analysis-wait, export, save, and exit around it.

Every step is logged to batch_run_and_export.log via plain Python file
I/O rather than print()/msg() -- console output from idat.exe in -A mode
was unreliable during development, so the log file is the source of
truth for what happened on any given run, including exceptions.

Confirmed working 2026-08-18 (IDA Pro 8.3, idat.exe, 16-bit ultima2.idb)
against apply_structs.py. Two things this needed that a naive port of
the Alt+F7 workflow wouldn't guess:
  - ida_loader.gen_file()'s fp argument needs a real SWIG FILE* --
    ida_diskio.fopenWT()/eclose(), not a plain Python open() handle
    (that raises "TypeError: argument 2 of type 'FILE *'").
  - idat.exe's stdout/msg() output is not reliably flushed/visible
    before qexit(); don't depend on console output for diagnosis. This
    also means the TARGET script's own print() calls (e.g. a dry-run
    script's [dry]/[!] lines) need to be captured explicitly -- this
    driver redirects sys.stdout during the exec() and writes whatever
    it captured into the log too, so DRY_RUN=True scripts are just as
    inspectable headlessly as DRY_RUN=False ones.
"""

import contextlib
import io
import traceback

import idc
import ida_auto
import ida_diskio
import ida_loader

ASM_PATH = r"C:\dev\ultima2\ultima2.asm"
IDC_PATH = r"C:\dev\ultima2\ultima2.idc"
LOG_PATH = r"C:\dev\ultima2\ida_scripts\batch_run_and_export.log"


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

            argv = idc.ARGV
            log(fh, f"[*] ARGV = {list(argv)}")
            if len(argv) < 2:
                log(fh, "[!] usage: -S\"batch_run_and_export.py <script_to_run.py>\"")
                idc.qexit(1)
                return

            target_script = argv[1]
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

            log(fh, f"[*] exporting ASM to {ASM_PATH}")
            fp = ida_diskio.fopenWT(ASM_PATH)
            if fp is None:
                raise RuntimeError(f"fopenWT failed for {ASM_PATH}")
            try:
                ok = ida_loader.gen_file(ida_loader.OFILE_ASM, fp, 0, idc.BADADDR, 0)
            finally:
                ida_diskio.eclose(fp)
            log(fh, f"[*] gen_file(OFILE_ASM) returned {ok}")

            log(fh, f"[*] exporting IDC to {IDC_PATH}")
            fp = ida_diskio.fopenWT(IDC_PATH)
            if fp is None:
                raise RuntimeError(f"fopenWT failed for {IDC_PATH}")
            try:
                ok = ida_loader.gen_file(ida_loader.OFILE_IDC, fp, 0, idc.BADADDR, 0)
            finally:
                ida_diskio.eclose(fp)
            log(fh, f"[*] gen_file(OFILE_IDC) returned {ok}")

            idb_path = idc.get_idb_path()
            log(fh, f"[*] saving database to {idb_path}")
            ida_loader.save_database(idb_path, 0)

            log(fh, "[*] done, exiting 0")
            idc.qexit(0)

        except Exception:
            log(fh, "[!] EXCEPTION:")
            log(fh, traceback.format_exc())
            idc.qexit(1)


main()
