"""
Names two more per-pixel helpers found via the DrawViewportSprite/
DrawPicture blit chain.

sub_2A217, called once from DrawRleMaskedShadedRun: stashes ax into
[bp-0x4C] then tail-jumps to the address held at [bp-0x2A] -- a
caller-configured function-pointer callback, invoked per pixel with
the current value stashed for it to read. Doesn't itself return (the
callback presumably returns to DrawRleMaskedShadedRun's own return
address). -> InvokePixelEffectCallback

sub_2A4B0, called from DrawPicture (not the sprite-blit chain):
splits al into a high nibble (hue group, 0-15) and low nibble
(shade), then searches a 16-entry stack table (at [bp-0x4A]) for an
entry whose high byte matches the hue group. No match (or table slot
0) leaves al unchanged. On a match: if the entry's low byte is 0x0F,
forces al=0xFF (a "treat this hue group as fully transparent"
marker); otherwise replaces just the hue-group nibble with the
entry's low byte, preserving the original shade nibble -- a
per-hue-group color remap/substitution, plausibly for status effects
(stun flash, poison tint, etc.) that recolor a sprite's whole hue band
without touching its shading. -> RemapOrMaskColorByHueTable

Run via:
    .\run_ida_script.ps1 name_pixel_hook_and_hue_remap.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x2A217: "InvokePixelEffectCallback",
    0x2A4B0: "RemapOrMaskColorByHueTable",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x2A217,
    "Stashes ax into [bp-0x4C] then tail-jumps to the caller-"
    "configured function pointer at [bp-0x2A] -- a per-pixel effect "
    "callback hook. Called from DrawRleMaskedShadedRun.",
    False,
)
ida_bytes.set_cmt(
    0x2A4B0,
    "Looks up al's high nibble (hue group) in a 16-entry stack table "
    "([bp-0x4A]); no match leaves al unchanged; a match with low "
    "byte 0x0F forces al=0xFF (hue-group-wide transparency); "
    "otherwise replaces just the hue-group nibble, keeping the shade "
    "nibble -- a per-hue-group color remap/mask, plausibly for "
    "status-effect tinting. Called from DrawPicture.",
    False,
)
