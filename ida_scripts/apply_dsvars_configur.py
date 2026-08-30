"""
IDA Pro script: name what little of CONFIGUR.EXE's data segment (`dseg`)
is identifiable.

CONFIGUR is a standalone Microsoft C program, not compiled BASIC, so it
has no `ds:<off>` module-scope variables in the LEGLIB sense. `_main` is
entirely stack-based -- it reads `DRCONFIG.DAT` into a local buffer,
examines it, prompts, and writes it back, touching **no** globals.

Everything in `dseg` is Microsoft C runtime library state (heap, errno,
argc/argv, the `_output`/printf formatter scratch, stdio buffers). This
names the handful of stock CRT globals whose role is clear, so the
listing reads like the MSC 5.x/6.0 runtime it is; the ~30-word
`_output` format-state block at 0x126EC..0x12710 is left as-is (they're
`_output`-local statics -- field width / precision / flags / counts).

Names by absolute EA (dseg is a normal C data segment).

    .\run_ida_script.ps1 -Idb configur -ScriptName apply_dsvars_configur.py
"""

import idc
import idautils
import ida_bytes

DRY_RUN = False

# (ea, name, comment)
VARS = [
    (0x1245B, "_errno",
     "C `errno` -- sub_10B06 (_maperror) xlat's the raw DOS error code "
     "through the 0x728 table and stores it here."),
    (0x12466, "_doserrno",
     "raw DOS error code (low byte) saved on entry to _maperror before "
     "translation."),
    (0x12463, "_osversion",
     "DOS version word (low byte = _osmajor). Set by __cinit; "
     "_maperror checks `>= 3` before using the extended errno mapping."),
    (0x12461, "_savedDS",
     "the DGROUP segment, stashed by `start` (`mov ss:[12461h], ds`) "
     "and reloaded into DS/ES by __setargv / __setenvp / __myalloc / "
     "__cinit."),
    (0x12494, "_STKHQQ",
     "Microsoft C stack-overflow limit -- __chkstk and _stackavail "
     "compare the prospective SP against it."),
    (0x12468, "_nfile",
     "size of the file-handle table -- _lseek / _write / _isatty "
     "bounds-check the fd against it."),

    (0x12B2B, "colorRegs",
     "static `union REGS` that setTextColor fills (AL / BL / BH bytes at "
     "12B2B / 12B2E / 12B2F) and passes twice to _int86(0x10) to set "
     "the text colour."),
]


def main():
    done = skip = 0
    for ea, name, cmt in VARS:
        cur = idc.get_name(ea)
        if DRY_RUN:
            print(f"  {ea:#x}: {cur!r} -> {name!r}")
            continue
        if cur == name:
            skip += 1
            continue
        ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, 2)
        ida_bytes.create_data(ea, ida_bytes.FF_WORD, 2, idc.BADADDR)
        if idc.set_name(ea, name, idc.SN_NOWARN | idc.SN_CHECK):
            idc.set_cmt(ea, cmt, 1)
            done += 1
        else:
            print(f"  [!] set_name failed at {ea:#x} -> {name}")
    print(f"\nnamed {done}, already-named {skip}" + ("  [DRY_RUN]" if DRY_RUN else ""))


main()
