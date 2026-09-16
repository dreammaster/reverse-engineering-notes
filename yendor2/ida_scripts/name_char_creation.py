"""
Names two pieces of the character-creation wizard, found via
RunTitleScreen's 'I' handler:

- sub_1522E: calls three steps in sequence (sub_15E44, sub_15429,
  sub_1559A, each checking for ESC-cancel) then a finalize step
  (sub_15267). Matches the "CHARACTER CREATION"/"PICK A CLASS"/"MALE"/
  "FEMALE"/"PICK A PORTRAIT" string cluster found earlier this session
  right after g_pictureDir. Called from InitGame directly and from
  RunTitleScreen's 'I' key. -> RunCharacterCreation
- sub_15E44 (first step): allocates a fresh ~127KB buffer and clears two
  64000-byte (0x7D00 words) regions in it -- offscreen compositing
  buffers -- then draws g_pictureDir entry 6 (the male body silhouette)
  and entry 5 (sky/cloud background) into them via DrawPicture with
  _videoSegment redirected off-screen, not to the real video buffer.
  Building up a composite portrait image rather than drawing directly
  to screen. -> ComposeCharacterPortrait

Not yet named: sub_15429, sub_1559A, sub_15267 (the other two wizard
steps and the finalizer) -- not traced this round.

Run via:
    .\run_ida_script.ps1 name_char_creation.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x1522E: "RunCharacterCreation",
    0x15E44: "ComposeCharacterPortrait",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1522E,
    "Character creation wizard: three steps (sub_15E44/"
    "ComposeCharacterPortrait, sub_15429, sub_1559A, each ESC-"
    "cancelable) then a finalize step (sub_15267). Matches the "
    "CHARACTER CREATION/PICK A CLASS/MALE/FEMALE/PICK A PORTRAIT "
    "string cluster near g_pictureDir. Called from InitGame and from "
    "RunTitleScreen's 'I' key.",
    False,
)
