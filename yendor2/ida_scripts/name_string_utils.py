"""
Names a small family of foundational C-runtime-equivalent string helpers,
found while tracing sub_14B24's string-building call sequence (originally
misread as box-drawing from its caller context alone -- it's actually
building a formatted label by copying/trimming/concatenating pieces).
Each read directly and is unambiguous:

- sub_28A5A: es:di=bx; scan for a null byte (max 255); ax = distance
  found - bx. Classic strlen. -> StrLen
- sub_23A64: si=ax (source), di=bx (dest); byte copy including the
  terminating null; returns bx = di (pointer to the copied terminator,
  i.e. the position ready for a further append -- stpcpy semantics, not
  plain strcpy's "return start"). -> StpCpy
- sub_1700E: di=bx (existing dest string); scans forward (max 1024) for
  its null terminator; then copies from si=ax onto the end including the
  new terminator; returns bx = pointer to the new terminator. strcat,
  with the same "return the end" convention as StpCpy. -> StrCat
- sub_16EDE: calls StrLen(bx) to find the end, then walks backward
  turning trailing 0x20 (space) bytes into 0x00 until a non-space is hit
  or the start (bx) is reached. -> TrimTrailingSpaces

Run via:
    .\run_ida_script.ps1 name_string_utils.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x28A5A: "StrLen",
    0x23A64: "StpCpy",
    0x1700E: "StrCat",
    0x16EDE: "TrimTrailingSpaces",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(0x28A5A, "strlen(bx): scans for a null byte (max 255 bytes), returns length in ax.", False)
ida_bytes.set_cmt(0x23A64, "stpcpy(dest=bx, src=ax): copies src including its null terminator into dest; returns bx = pointer to the copied terminator (ready for a further append).", False)
ida_bytes.set_cmt(0x1700E, "strcat(dest=bx, src=ax): finds dest's existing null terminator (scans up to 1024 bytes), then appends src including its terminator; returns bx = pointer to the new terminator.", False)
ida_bytes.set_cmt(0x16EDE, "rtrim(bx): finds the end via StrLen, then walks backward replacing trailing space (0x20) bytes with 0x00.", False)
