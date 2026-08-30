"""
IDA Pro script: renames for configur.idb (CONFIGUR.EXE) -- the
**floppy-drive / disk-layout configuration utility**.

CONFIGUR.EXE is the odd one out: a small standalone **Microsoft C**
program (no LEGLIB, no int-3Fh thunks, IDA's C loader + FLIRT already
recovered the whole MSC C runtime). The only application code is `_main`
plus six tiny BIOS screen/keyboard helpers.

What it does: opens `DRCONFIG.DAT`, shows the game's current disk
configuration ("configured for a High Density floppy or Hard Disk" /
"four 360K 5.25\" floppy disks using drive %c:" / "two 720K 3.5\" ..."),
asks "Would you like to change this configuration (Y/N)?" and "... to
utilize two drives to reduce disk swaps (Y/N)?", prompts "Please enter
the letter of the drive to use for Disk 1 (A-Z)?", and writes the new
`DRCONFIG.DAT` back ("Legacy of the Ancients Configuration has been
updated."). It does **not** touch graphics / sound hardware.

Not part of the compiled-BASIC pipeline -- run this directly:
  .\run_ida_script.ps1 -Idb configur -ScriptName apply_renames_configur.py
"""

import idc
import idautils

DRY_RUN = False

# (ea, new_name, note)
RENAMES = [
    (0x10500, "setTextColor",
     'set the text attribute for subsequent printf output (int 10h '
     'AH=10h) -- arg!=0 selects the highlight colour (6/7), else normal '
     '(0x20).'),
    (0x1053C, "readKeyUpper",
     'wait for a keypress (_bios_keybrd(0)) and return it folded to '
     'upper case.'),
    (0x1056D, "getVideoPage",
     'int 10h AH=0Fh -- cache the active display page (BH) in cs:byte_1056C.'),
    (0x1057B, "clearScreen",
     'blank the whole 80x25 screen (int 10h AH=06h scroll-up, full '
     'window, attribute 7).'),
    (0x10594, "gotoXY",
     'position the cursor (int 10h AH=02h) at (col=arg0, row=arg1) on '
     'the cached page.'),
    (0x105AC, "clearRegion",
     'blank a rectangular region (int 10h AH=06h) -- args col1,row1,'
     'col2,row2.'),
]

MAIN_CMT = (
    "CONFIGUR.EXE main: read/validate DRCONFIG.DAT, print the current "
    "disk configuration, and (on Y) prompt for the new drive letter(s) "
    "and rewrite the file. Standalone MSC -- not a LEGLIB client."
)


def main():
    done = skip = 0
    for ea, name, note in RENAMES:
        cur = idc.get_name(ea)
        if DRY_RUN:
            print(f"  {ea:#x}: {cur!r} -> {name!r}")
            continue
        if cur != name and not idc.set_name(ea, name, idc.SN_NOWARN):
            print(f"  [!] set_name failed at {ea:#x} -> {name}")
            continue
        idc.set_func_cmt(ea, note, 1)
        done += 1 if cur != name else 0
        skip += 1 if cur == name else 0

    if not DRY_RUN:
        for ea in (0x10010,):
            if idc.get_func_attr(ea, idc.FUNCATTR_START) == ea:
                idc.set_func_cmt(ea, MAIN_CMT, 1)

    total = sum(1 for _ in idautils.Functions())
    app = sum(1 for f in idautils.Functions()
              if not (idc.get_func_flags(f) & idc.FUNC_LIB)
              and not idc.get_func_name(f).startswith(("start",)))
    print(f"\napplied {done}, already-named {skip}" + ("  [DRY_RUN]" if DRY_RUN else ""))
    print(f"functions: {total} total, ~{app} application")


main()
