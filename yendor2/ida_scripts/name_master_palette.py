"""
Names sub_27CB0 -- one of the ~27-function "resource block setup" stub
cluster at 0x27B42-0x2801A that was deliberately left unnamed
(document_resource_stubs.py) since none of them could be told apart
from static analysis alone. This one, however, is now confirmed: it's
called from ShowIntroPicture right before a loop that builds the fade
source palette buffer, configuring a FileEntry read of exactly 0x300
(768) bytes from WORLD.DAT at offset 0x8270A -- verified by reading
those exact bytes directly from game/WORLD.DAT: a well-formed 256-entry
VGA DAC palette (6-bit 0-63 per channel), confirmed visually by
re-rendering all 10 PICTURES.VGA catalog entries with it in true color
(extract_pic.py --palette) -- the GameDialog panel's SAVE/LOAD/etc.
labels and the wolf/character images all came out looking correct.

-> LoadMasterPalette

Run via:
    .\run_ida_script.ps1 name_master_palette.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x27CB0
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "LoadMasterPalette", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'LoadMasterPalette': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Configures a FileEntry read of the game's master 256-color VGA "
    "palette from WORLD.DAT (offset 0x8270A, 768 bytes = 256 RGB "
    "triples, 6-bit DAC values 0-63 -- confirmed by reading it directly "
    "and re-rendering PICTURES.VGA's catalog in true color). Called from "
    "ShowIntroPicture. One of the resource-block-setup stub family "
    "(document_resource_stubs.py) -- the only one confirmed so far.",
    False,
)
