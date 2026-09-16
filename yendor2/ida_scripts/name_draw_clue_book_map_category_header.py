"""
Names sub_14122, called once from RunClueBookMapCategory (the clue
book's map-category screen handler).

Formats a label into buffer 0xAFA8 via sub_27B84(ax=0xAFA8, bx=4),
sets up a file-entry read (errorCode=9, record type 0x1A) and calls
FileEntry_Read + ErrorCheck to load a graphic, draws it via
DrawPicture at (0xA1,0) sized (0x90,0x73), then draws the formatted
label text at (0xAA,1), and finally DrawMouseCursor. Reads as the
header draw for a clue-book map category page: a category banner
picture plus its title text. -> DrawClueBookMapCategoryHeader

Run via:
    .\run_ida_script.ps1 name_draw_clue_book_map_category_header.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x14122
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawClueBookMapCategoryHeader", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawClueBookMapCategoryHeader': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Formats a label via sub_27B84 into buffer 0xAFA8, loads a "
    "graphic via FileEntry_Read (errorCode=9, record 0x1A) + "
    "ErrorCheck, draws it via DrawPicture at (0xA1,0) sized "
    "(0x90,0x73), draws the label text at (0xAA,1), then "
    "DrawMouseCursor. Called once from RunClueBookMapCategory.",
    False,
)
