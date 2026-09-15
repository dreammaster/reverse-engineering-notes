"""
Names sub_267F0 and sub_2681B, both called twice from
DrawPartyMemberPortrait: small overlay icon drawers at fixed position
offsets, each drawing the "+1" (highlighted) variant of an icon from
word_2E546's [+8] field.

sub_267F0 (-> DrawPortraitOverlayIconA): position offset (0x28, 0x40).
sub_2681B (-> DrawPortraitOverlayIconB): position offset (0x26, 0x24).

Exact narrative (what these two overlay icons represent) not
confirmed -- named by their confirmed mechanism and call site.

Run via:
    .\run_ida_script.ps1 name_portrait_overlay_icons.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x267F0: "DrawPortraitOverlayIconA",
    0x2681B: "DrawPortraitOverlayIconB",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x267F0,
    "Draws the '+1' (highlighted) variant of word_2E546's [+8] icon at "
    "a fixed offset (0x28,0x40) from the portrait position. Called "
    "from DrawPartyMemberPortrait. Exact narrative not confirmed.",
    False,
)
ida_bytes.set_cmt(
    0x2681B,
    "Sibling of DrawPortraitOverlayIconA, at offset (0x26,0x24). "
    "Called from DrawPartyMemberPortrait.",
    False,
)
