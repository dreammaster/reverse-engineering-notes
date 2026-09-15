"""
Names sub_1D01E, found while tracing 0xAFA8 scratch-buffer usage near
the string-utility family named earlier this session (StrLen/StpCpy/
StrCat/TrimTrailingSpaces): es:di=bx (dest), cx=ah (count), al=fill
byte; `rep stosb` writes `count` copies of `al`, then writes a null
terminator right after; returns bx=di (pointer to the terminator --
same "return the end" convention as StpCpy/StrCat). Effectively
memset-and-null-terminate, used e.g. with al=' ' to blank out a text
buffer before rebuilding a label in it. -> StrFillN

Run via:
    .\run_ida_script.ps1 name_strfilln.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1D01E
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "StrFillN", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'StrFillN': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "StrFillN(dest=bx, count=ah, fill=al): writes `count` copies of "
    "`fill` into dest then a null terminator; returns bx = pointer to "
    "the terminator (same convention as StpCpy/StrCat). Used e.g. to "
    "blank a text buffer with spaces before rebuilding a label in it.",
    False,
)
