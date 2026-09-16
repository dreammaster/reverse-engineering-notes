"""
Names 2 more resource-block-setup stubs (the LoadMasterPalette family):
configure a FileEntry struct for a single-record (size=1) read/write,
indexed by a specific global rather than a caller-supplied value.

sub_27DC6 (16 refs, called from RunShopScreen, LoadLockState, etc.):
table 0xCDE3, index word_32DCA. -> PrepareRecordAtIndexDCA

sub_27DE5 (30 refs, called from `start`, SaveCurrentGameToSlot,
HandleSearchCommand, etc.): table 0xCDDF, index word_32DC6.
-> PrepareRecordAtIndexDC6

Run via:
    .\run_ida_script.ps1 name_prepare_record_index_stubs.py
"""
import idc
import ida_name
import ida_bytes

for ea, name, table, idx in [
    (0x27DC6, "PrepareRecordAtIndexDCA", "0xCDE3", "word_32DCA"),
    (0x27DE5, "PrepareRecordAtIndexDC6", "0xCDDF", "word_32DC6"),
]:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(
        ea,
        f"Resource-block-setup stub (LoadMasterPalette family): "
        f"configures a FileEntry for a single-record read/write from "
        f"table {table}, indexed by {idx} rather than a caller-"
        f"supplied value.",
        False,
    )
