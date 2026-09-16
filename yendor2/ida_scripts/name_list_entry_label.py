"""
Names sub_24FFC: loads a record via sub_12554 (the EMS-paged 58-byte
record loader found much earlier this session), draws the icon at its
+8 field via DrawPicture, then builds a text label from two fields in
the SAME record-format buffer (word_2E546+0x13 and word_2E546+0x20,
joined by a fixed separator at 0x7960) using the StpCpy/StrCat/
TrimTrailingSpaces family, and displays it with writeString. This is
the same "icon + two-field label" pattern sub_14B24 (called by
DrawMessageBox, in the ShowPagedEntryScreen/ShowClueBook chain) uses,
just simpler/more direct -- confirms sub_12554's loaded-record layout
has an icon id at +8 and text fields at +0x13/+0x20. Not certain which
specific list (clue book vs. another catalog reusing the same paged-
record format) calls this one. -> DrawListEntryLabel

Run via:
    .\run_ida_script.ps1 name_list_entry_label.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x24FFC
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawListEntryLabel", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawListEntryLabel': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Loads a paged record (sub_12554), draws its icon (+8 field) via "
    "DrawPicture, and displays a label built from two text fields "
    "(+0x13, +0x20) joined by a fixed separator. Confirms sub_12554's "
    "record layout: icon id at +8, text fields at +0x13/+0x20. Same "
    "pattern as sub_14B24 (DrawMessageBox's helper) but simpler.",
    False,
)
