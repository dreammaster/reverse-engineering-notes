"""
Names sub_11E1C, called once from sub_11A10 -- plays two sound events
in sequence, each preceded by a driver-idle wait.

Calls TryPlaySoundCueAlt(cx=0xFFFF) -- always a no-op for the actual
cue (0xFFFF is that function's own "no sound" sentinel), but its
leading WaitForSoundDriverIdle call still runs as a side effect, so
this is really just "wait for the driver to go idle." Then triggers
sound event 0x47 ('G') via TriggerSoundEvent, waits again the same
way, then triggers sound event 0x48 ('H'). A two-part sound cue,
plausibly a short jingle; the specific event ids' meaning isn't
confirmed. -> PlaySoundSequenceGH

Run via:
    .\run_ida_script.ps1 name_play_sound_sequence_gh.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x11E1C
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "PlaySoundSequenceGH", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'PlaySoundSequenceGH': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Waits for driver idle (via TryPlaySoundCueAlt(cx=0xFFFF), always "
    "a no-op cue but still runs its idle-wait), triggers sound event "
    "0x47, waits again, triggers event 0x48. A two-part sound cue. "
    "Called from sub_11A10.",
    False,
)
