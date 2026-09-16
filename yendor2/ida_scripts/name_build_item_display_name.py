"""
Names sub_14B24, called from BuildClueEntryText and
ShowClueBookItemDetail (the F8 clue book's item-detail cluster, sibling
to the already-named BuildMonsterDisplayName). Calls
LoadItemCatalogRecord(ax) to load an item's 58-byte catalog record into
the scratch buffer word_2E546 points at, then builds a single
space-joined display string from 3 of its text fields (+0x13, +0x20,
+0x2D), trimming each with TrimTrailingSpaces before joining via
StpCpy/StrCat and the same single-space separator string (0x7960) used
by BuildMonsterDisplayName. Exact per-field semantics (name/material/
type?) not independently confirmed. -> BuildItemDisplayName

Run via:
    .\run_ida_script.ps1 name_build_item_display_name.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x14B24
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "BuildItemDisplayName", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'BuildItemDisplayName': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Builds '<+0x13> <+0x20> <+0x2D>' (space-separated, trimmed) from "
    "an item catalog record loaded via LoadItemCatalogRecord -- the "
    "item-detail sibling of BuildMonsterDisplayName (same separator "
    "string at 0x7960). Called from BuildClueEntryText and "
    "ShowClueBookItemDetail. Per-field semantics not confirmed.",
    False,
)
