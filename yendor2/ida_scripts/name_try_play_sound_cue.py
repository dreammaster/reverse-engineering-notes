"""
Names another byte-for-byte-identical overlay-segment duplicate pair
(matching this session's established pattern: DrawShadowedText/Alt,
ConfirmContainerInteraction/ConfirmAlchemyInteraction,
PollForEscapeKeyOnly/Alt) -- a "drop this sound cue if the driver's
busy or the id is the no-op sentinel" gate.

Both: call WaitForSoundDriverIdle; if it had to wait (ZF set, driver
was busy), give up and return without playing anything. Otherwise
(driver was already free), check cx against the sentinel 0xFFFF; if
it matches (no sound requested), also return without playing.
Otherwise dispatch the sound via a still-unnamed per-segment helper
(sub_1616F / sub_11E39).

sub_16234 is called from sub_1559A (character creation step 3) among
others; sub_11EAE is called from sub_11A10 and sub_11E1C among
others -- each presumably the sound-triggering entry point for its
own overlay segment's UI code.

-> TryPlaySoundCue / TryPlaySoundCueAlt

Run via:
    .\run_ida_script.ps1 name_try_play_sound_cue.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x16234: "TryPlaySoundCue",
    0x11EAE: "TryPlaySoundCueAlt",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x16234,
    "Drops this sound cue (cx=sound/note id) if WaitForSoundDriverIdle "
    "had to wait (driver was busy) or cx==0xFFFF (no-op sentinel); "
    "otherwise dispatches via still-unnamed sub_1616F. Byte-for-byte "
    "identical to TryPlaySoundCueAlt (sub_11EAE) in a different "
    "overlay segment. Called from sub_1559A and others.",
    False,
)
ida_bytes.set_cmt(
    0x11EAE,
    "Byte-for-byte duplicate of TryPlaySoundCue (sub_16234); "
    "dispatches via still-unnamed sub_11E39. Called from sub_11A10, "
    "sub_11E1C, and others.",
    False,
)
