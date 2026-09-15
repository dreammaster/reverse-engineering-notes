"""
Names sub_25608, called from sub_1E64A and sub_209D2 (both not
traced, presumably movement/screen-update paths): computes a coarse
map-region index from the party's position (word_36CF7/36CF9, divided
into 0x28x0x18 zones), and if it differs from the last known region
(word_2E4A8, cached), reads a WORLD.DAT record for the new region
(sub_2801A + FileEntry_Read) and plays its associated music track
(word_38808) via the already-named PlayMusicTrack.

The ambient-music region trigger: background music changes as the
party crosses between coarse map zones.

-> UpdateAmbientMusicForRegion

Run via:
    .\run_ida_script.ps1 name_update_ambient_music.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x25608
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "UpdateAmbientMusicForRegion", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'UpdateAmbientMusicForRegion': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Computes a coarse map-region index from the party's position; if "
    "it changed since last checked (word_2E4A8), reads the new "
    "region's WORLD.DAT record and plays its music track "
    "(PlayMusicTrack) -- the ambient-music region trigger. Called "
    "from sub_1E64A and sub_209D2.",
    False,
)
