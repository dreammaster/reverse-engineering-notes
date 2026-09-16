"""
Names two small, self-contained helpers found this round:

sub_2D3FE (0x2D3FE, called twice from the still-unnamed combat
dispatcher sub_2C0FE): draws a picture (ax=id, bx=x), guarding with
word_328C6 bit 0 around the DrawPicture call, then returns the next
frame index -- incrementing ax and wrapping back to a base
(word_332EC) once it reaches base+count (word_332EC+word_332EE). A
generic "draw this animation frame, return the next (wrapping) frame
index" cycler. -> DrawAnimationFrameAndAdvance

sub_2776F (0x2776F, called 3 times from sub_2772C with a count in ax
and a data-block address in bx): stores the count in word_36863 (the
same global FindItemInsideContainer's family uses for a found-item
id -- reused here for an unrelated count value), then writes the block
via the same sub_27E3A/FileEntry_Write(errorCode=0xB) pattern used
elsewhere for CURGAME writes. sub_2772C's own structure (3 near-
identical count-then-write pairs at party-record offsets +0x17E/+0x180,
+0x1A4/+0x1A6, +0x1CA/+0x1CC, only when non-empty) isn't itself named
-- the identity of those 3 sub-blocks isn't confirmed, so only naming
the clearer, reusable low-level write helper. -> WriteContainerSubBlock

Run via:
    .\run_ida_script.ps1 name_anim_frame_and_write_block.py
"""
import idc
import ida_name
import ida_bytes

renames = [
    (0x2D3FE, "DrawAnimationFrameAndAdvance"),
    (0x2776F, "WriteContainerSubBlock"),
]
for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x2D3FE,
    "Draws a picture (ax=id, bx=x) guarded by word_328C6 bit 0, then "
    "returns the next frame index (ax+1, wrapping to word_332EC once "
    "it reaches word_332EC+word_332EE). Called from the still-unnamed "
    "combat dispatcher sub_2C0FE.",
    False,
)
ida_bytes.set_cmt(
    0x2776F,
    "Writes a data block (bx=address, ax=count, stored via word_36863) "
    "using the sub_27E3A/FileEntry_Write(errorCode=0xB) pattern. Called "
    "3 times from sub_2772C for 3 party-record sub-blocks whose "
    "identity isn't confirmed.",
    False,
)
