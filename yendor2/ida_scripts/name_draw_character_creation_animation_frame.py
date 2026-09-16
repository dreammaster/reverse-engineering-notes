"""
Names sub_152EF, called repeatedly from PlayCharacterCreationIntroAnimation
(its own opening animation, already named).

Restores the full 320x200 screen from the `fe` scratch buffer (undoing
whatever the previous frame drew), then walks a 7-entry cell table at
0x6D60 (the same address used elsewhere as the dungeon-viewport
scratch cell buffer, here repurposed for this animation) -- for each
entry with bit 0x8000 set ("active" for this frame), draws its
picture ([si+0x10], icon slot [si+6]) at a position offset by
word_2E402/word_2E406 and clipped against the screen edges (y=0 top,
y=0xC7 bottom), setting up word_32980/32982/32986/32988 (DrawPicture's
clip-rectangle globals) accordingly -- clearing the entry's active bit
entirely if it's now fully off-screen. Waits for at least one tick
(word_32956>=1) before returning. Reads as: draw one frame of the
character-creation intro's animated picture sequence (plausibly
falling/sliding sprites), with per-entry vertical clipping.
-> DrawCharacterCreationAnimationFrame

Run via:
    .\run_ida_script.ps1 name_draw_character_creation_animation_frame.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x152EF
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawCharacterCreationAnimationFrame", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawCharacterCreationAnimationFrame': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Restores the full screen from `fe`, then draws each active "
    "(bit 0x8000) entry of a 7-entry cell table at 0x6D60 -- picture "
    "+ position, clipped against the screen top/bottom, clearing the "
    "active bit once fully off-screen. Waits for a tick before "
    "returning. One animation frame of "
    "PlayCharacterCreationIntroAnimation.",
    False,
)
