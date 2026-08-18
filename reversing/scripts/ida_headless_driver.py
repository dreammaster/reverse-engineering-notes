"""
IDAPython driver for HEADLESS batch runs -- invoked via idat.exe's -S switch,
never opened interactively. Do not run this from the IDA GUI's Alt-F7 dialog;
it expects idc.ARGV to be populated the way idat.exe's -S"script arg1 arg2"
syntax populates it, and it calls qexit() at the end, which will close an
interactive session too.

Invocation (see run_ida_headless.ps1 for the wrapper that builds this
command line and performs the pre-flight safety check):

    idat.exe -A -S"ida_headless_driver.py <target_script_abspath> <log_abspath>" <idb_path>

idc.ARGV layout when this script starts running:
    idc.ARGV[0] -- this script's own path (as IDA sees it)
    idc.ARGV[1] -- absolute path to the target script to exec
    idc.ARGV[2] -- absolute path to the log file to write

What this does, in order:
    1. Open the log file (plain text, direct file I/O -- see log() below).
       This is the actual source of truth for what happened. idat.exe -A's
       stdout is not reliably flushed or visible when run from a wrapper
       process, so nothing here relies on it.
    2. ida_auto.auto_wait() -- make sure the initial auto-analysis triggered
       by loading the database is fully settled before touching anything.
    3. exec() the target script, with sys.stdout redirected into a buffer
       that gets written into the SAME log file. This matters for target
       scripts that are still in DRY_RUN mode: their print() output is the
       only record of what a dry run would have done, and it would
       otherwise vanish into idat.exe's unreliable console.
    4. ida_auto.auto_wait() again -- the target script may have triggered
       more analysis work (e.g. applying new struct/type info can requeue
       affected functions); let that settle before exporting.
    5. Export .asm (OFILE_ASM) and .idc (OFILE_IDC) via ida_loader.gen_file().
       NOTE: gen_file()'s file-handle argument needs a real SWIG FILE*, not
       a plain Python open() handle -- obtained via ida_diskio.fopenWT() and
       closed via ida_diskio.eclose(). Passing a normal Python file object
       fails with "TypeError: argument 2 of type 'FILE *'".
    6. Save the database.
    7. qexit() with 0 on full success, 1 if the target script raised (the
       full traceback is in the log either way) or if the log the pre-flight
       PowerShell check didn't catch of a step failed unexpectedly.

If the target script raises, its exception is caught, logged in full
(traceback included), and the driver still proceeds to export + save --
whatever IDB-visible changes the target script made before raising already
happened in memory, and skipping the save/export would not undo them, only
throw away the export step for changes that DID apply. The log makes the
partial-failure state impossible to miss (a "TARGET SCRIPT RAISED" banner),
and the driver's own exit code is non-zero so the calling wrapper can tell
the run wasn't clean.
"""

import io
import os
import sys
import traceback
import contextlib
import datetime

import idc
import ida_auto
import ida_loader
import ida_diskio
import ida_ida
import ida_pro


# ---------------------------------------------------------------------------
# Logging: direct file I/O only. Every call opens, writes, flushes, fsyncs,
# and closes -- deliberately not keeping a handle open across the whole run
# and not batching writes, so a hard kill of idat.exe mid-run still leaves a
# log that's accurate up to the last completed step.
# ---------------------------------------------------------------------------

_LOG_PATH = None


def _init_log(path):
    global _LOG_PATH
    _LOG_PATH = path
    # Truncate/create fresh for this run.
    with open(_LOG_PATH, "w", encoding="utf-8") as f:
        f.write("")


def log(msg):
    line = "[{}] {}\n".format(
        datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), msg
    )
    with open(_LOG_PATH, "a", encoding="utf-8") as f:
        f.write(line)
        f.flush()
        os.fsync(f.fileno())


def log_block(header, text):
    """Log a multi-line block (e.g. a traceback or captured stdout) with a
    clear header/footer so it's unambiguous in the log file where it starts
    and ends, even if the block itself is empty."""
    log("----- {} (begin) -----".format(header))
    with open(_LOG_PATH, "a", encoding="utf-8") as f:
        if text:
            f.write(text)
            if not text.endswith("\n"):
                f.write("\n")
        f.flush()
        os.fsync(f.fileno())
    log("----- {} (end) -----".format(header))


# ---------------------------------------------------------------------------
# Export helpers
# ---------------------------------------------------------------------------


def export_file(ofile_type, out_path, flags, label):
    log("Exporting {} -> {}".format(label, out_path))
    fp = ida_diskio.fopenWT(out_path)
    if not fp:
        raise IOError("ida_diskio.fopenWT() failed to open {} for writing".format(out_path))
    try:
        ida_loader.gen_file(
            ofile_type,
            fp,
            ida_ida.inf_get_min_ea(),
            ida_ida.inf_get_max_ea(),
            flags,
        )
    finally:
        ida_diskio.eclose(fp)
    size = os.path.getsize(out_path) if os.path.exists(out_path) else -1
    log("Wrote {} ({} bytes)".format(out_path, size))


def export_asm(out_path):
    export_file(ida_loader.OFILE_ASM, out_path, ida_loader.GENFLG_ASMTYPE, ".asm")


def export_idc(out_path):
    export_file(ida_loader.OFILE_IDC, out_path, 0, ".idc")


# ---------------------------------------------------------------------------
# Target script execution
# ---------------------------------------------------------------------------


def run_target_script(target_path):
    log("Reading target script: {}".format(target_path))
    with open(target_path, "r", encoding="utf-8") as f:
        source = f.read()

    target_globals = {
        "__name__": "__main__",
        "__file__": target_path,
    }

    captured = io.StringIO()
    log("Executing target script with stdout captured...")
    raised = None
    try:
        with contextlib.redirect_stdout(captured):
            exec(compile(source, target_path, "exec"), target_globals)
    except SystemExit as e:
        # A target script calling sys.exit()/exit() is not a failure by
        # itself -- record the code and move on.
        log("Target script called sys.exit({})".format(e.code))
    except BaseException:
        raised = traceback.format_exc()
    finally:
        log_block("TARGET SCRIPT STDOUT", captured.getvalue())

    if raised is not None:
        log("TARGET SCRIPT RAISED -- see traceback below. Continuing to "
            "export/save so whatever DID apply before the exception is not "
            "lost, but this run's exit code will be non-zero.")
        log_block("TARGET SCRIPT TRACEBACK", raised)
        return False

    log("Target script completed without raising.")
    return True


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------


def main():
    argv = idc.ARGV
    if len(argv) < 3:
        # Can't log this to the file yet since we don't have the path --
        # this is the one failure mode that can only go to stderr. The
        # PowerShell wrapper always supplies both args, so reaching this
        # means the driver was invoked incorrectly (e.g. run by hand).
        sys.stderr.write(
            "ida_headless_driver.py: expected idc.ARGV = "
            "[driver, target_script_path, log_path], got: {}\n".format(list(argv))
        )
        ida_pro.qexit(2)
        return

    target_script = argv[1]
    log_path = argv[2]

    _init_log(log_path)
    log("=== ida_headless_driver.py starting ===")
    log("idc.ARGV = {}".format(list(argv)))
    log("Target script: {}".format(target_script))
    log("IDB path (idc.get_idb_path()): {}".format(idc.get_idb_path()))
    log("Input file (idc.get_input_file_path()): {}".format(idc.get_input_file_path()))

    exit_code = 0

    try:
        if not os.path.isfile(target_script):
            raise IOError("Target script does not exist: {}".format(target_script))

        log("Waiting for initial auto-analysis to settle...")
        ida_auto.auto_wait()
        log("Initial auto-analysis settled.")

        ok = run_target_script(target_script)
        if not ok:
            exit_code = 1

        log("Waiting for auto-analysis triggered by the target script to settle...")
        ida_auto.auto_wait()
        log("Post-script auto-analysis settled.")

        idb_path = idc.get_idb_path()
        base, _ = os.path.splitext(idb_path)
        asm_path = base + ".asm"
        idc_path = base + ".idc"

        export_asm(asm_path)
        export_idc(idc_path)

        log("Saving database...")
        ida_loader.save_database(idb_path, 0)
        log("Database saved to {}".format(idb_path))

    except BaseException:
        log("UNHANDLED EXCEPTION IN DRIVER ITSELF (not the target script) --"
            " see traceback below.")
        log_block("DRIVER TRACEBACK", traceback.format_exc())
        exit_code = 1

    log("=== ida_headless_driver.py finished, exit_code={} ===".format(exit_code))
    ida_pro.qexit(exit_code)


main()
