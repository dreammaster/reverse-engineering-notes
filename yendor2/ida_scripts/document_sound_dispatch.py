"""
Documents (comment only -- confidence isn't high enough yet to rename)
sub_28412, traced this session as part of following up on
ShutdownAudioDrivers/g_soundDriverFarPtr: when g_driverStateFlags bit3
(driver active) is set, reads data via FileEntry_Read using a FileEntry
at a fixed low-memory address (bx=0x9043 -- the same FileEntry instance
the 0x27CFE-family "resource block setup" stub cluster configures),
checks it with ErrorCheck, then hands the loaded driver segment
(word_3292E) + a fixed offset (es:0x14 -- likely past a small header) to
the sound driver via g_soundDriverFarPtr with command number 6. Probably
"load and play a sound effect" given the shape, but command "6"'s exact
meaning isn't confirmed (the driver's own command numbering is opaque
without its source/docs), so not renaming from a guess. sub_2849C
(command 7, tests g_driverStateFlags bit1) and sub_284CB (saves/replaces
an interrupt vector, cx=0x3F suggests IRQ-related setup) look related
and are worth tracing together in a future pass.

Run via:
    .\run_ida_script.ps1 document_sound_dispatch.py
"""
import ida_bytes

ida_bytes.set_cmt(
    0x28412,
    "Sound driver dispatch, called with a command in AX. If the driver "
    "isn't active (g_driverStateFlags bit3 clear), only handles AX==3 "
    "(via sub_16DEA) and otherwise no-ops. When active: reads data via "
    "FileEntry_Read using the FileEntry at bx=0x9043 (same fixed "
    "instance the 0x27CFE-family resource stubs configure), ErrorChecks "
    "it, then calls g_soundDriverFarPtr with bx=6 and es:di pointing "
    "past a small header (es:0x14) in the loaded driver segment "
    "(word_3292E). Likely 'load+play a sound effect', but command 6's "
    "exact meaning per the driver's own protocol isn't confirmed -- see "
    "ida_scripts/document_sound_dispatch.py.",
    False,
)
print("done")
