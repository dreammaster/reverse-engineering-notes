"""
Names sub_2A653, called 9 times from DrawPicture and sibling
picture-drawing code -- this resolves the "how does the per-cell
color/remap parameter actually work" open question from
file-formats.md's tile-table section.

DrawPicture copies global word_32926 into its own stack local
[bp+var_21] at entry (confirmed: `mov ax, word_32926; mov [bp+var_21],
al`), and sub_2A653, sharing that frame, reads it as its shade-shift
delta:

- If the delta is 0, or the input color (al) is >= 0xD0 (a reserved
  high palette range, e.g. UI colors), returns al unchanged.
- Otherwise treats al as a color within a 16-entry "hue block"
  (typical VGA RPG palette layout: 16 hues x 16 shades) -- computes
  the block's floor (al & 0xF0) and ceiling (al | 0x0F), adds the
  delta to al, and clamps the result to stay within that same block
  (never crossing into a different hue) rather than wrapping.

In short: a signed shade-shift within one palette hue-block, clamped
at the block's own boundaries -- exactly the kind of primitive a
distance/light-based dungeon-corridor dimming effect would use.
Resolves (without fully tracing the two tile-lookup tables
themselves) that the "picture-id-like values" feeding word_32926 are
shade deltas, not remap/color-table indices as such.
-> ShiftPaletteShadeClamped

Run via:
    .\run_ida_script.ps1 name_shift_palette_shade.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2A653
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShiftPaletteShadeClamped", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShiftPaletteShadeClamped': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Shifts color al by the shared [bp+var_21] delta (DrawPicture's "
    "copy of word_32926) within its 16-entry palette hue-block "
    "(floor al&0xF0, ceiling al|0x0F), clamped at the block edges; "
    "no-op if delta==0 or al>=0xD0. A distance/light dimming shade "
    "primitive. Called 9x from DrawPicture and sibling picture-draw "
    "code.",
    False,
)
