"""
Names sub_17A21, called once from RunShopScreen in the branch taken
when the shop's hit-test table (0x5AC0) returns index 1 for a click
at the current cursor position -- plausibly a specific UI element
like a door/exit icon (exact element not independently confirmed).

Gated on word_328C6 bits 0x80/0x20 (shop-mode flags): if not skipped,
and word_32DCE bit 0x2 is clear, triggers a sound (ax=6) and
decrements a counter at [word_32DC4+2]. Always finishes by writing
back a WORLD.DAT-backed record (FileEntry type 0xA, via sub_27DC6)
through FileEntry_Write + ErrorCheck. Reads as a shop-related action
that plays a sound, decrements some per-slot counter, and persists
the change -- exact semantic of the counter (stock count vs. a UI
state counter) not confirmed. -> TriggerShopExitSoundAndPersist

Run via:
    .\run_ida_script.ps1 name_shop_exit_sound_and_persist.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x17A21
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TriggerShopExitSoundAndPersist", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TriggerShopExitSoundAndPersist': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Gated on word_328C6 bits 0x80/0x20; if not skipped and "
    "word_32DCE bit 0x2 is clear, plays a sound (ax=6) and "
    "decrements [word_32DC4+2]. Always writes back a WORLD.DAT "
    "record via sub_27DC6 + FileEntry_Write. Called once from "
    "RunShopScreen, reached via its hit-test table index 1.",
    False,
)
