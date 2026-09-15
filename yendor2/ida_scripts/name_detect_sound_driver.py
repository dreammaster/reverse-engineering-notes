"""
Names sub_284CB, called from sub_283EA (not traced): detects an
installed sound/music driver by scanning DOS interrupt vectors
(0x80-0xBE) for one matching a known 5-byte signature (at 0xCD6), or
falling back to "not found" (setting g_driverStateFlags bit 0x40) if
every vector in range is an unused IRET stub or nothing matches.

On a match: records the found vector number, sets up a call through
it (byte_28616/28617), computes a buffer size from a table
(0xCECB, _val44 entries, scaled from bytes to paragraphs), allocates
it (allocMem) into word_3195E, and sets g_driverStateFlags bits 0/1
(driver detected/active), clearing the "not found" bit.

-> DetectSoundDriver

Run via:
    .\run_ida_script.ps1 name_detect_sound_driver.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x284CB
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DetectSoundDriver", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DetectSoundDriver': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Scans DOS interrupt vectors (0x80-0xBE) for an installed sound/"
    "music driver's 5-byte signature; on a match, allocates its "
    "buffer and sets g_driverStateFlags bits 0/1 (detected/active), "
    "else sets bit 0x40 (not found). Called from sub_283EA.",
    False,
)
