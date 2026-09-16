"""
Names sub_2D498, called twice from the large unnamed combat dispatcher
sub_2C0FE -- resolves the last of this session's several deliberately-
unnamed "flagged but ambiguous" leads, now that all three functions it
calls (WaitForSoundDriverIdle, wait, TriggerSoundEvent) are named.

Takes a sound command in ax. Calls WaitForSoundDriverIdle:
- If the driver was already idle (no wait needed), the sound command
  is discarded unplayed -- just waits 6 ticks (a pacing delay) and
  returns.
- If the driver was busy and WaitForSoundDriverIdle had to wait for
  it, THEN triggers the sound command via TriggerSoundEvent once the
  driver is free.

This busy/idle handling is backwards from the more common "drop it if
busy" pattern seen in the sibling TryPlaySoundCue/Alt -- sound only
plays here when the driver *was* busy, not when it's already free.
Additionally, the ax==0 case contains a genuine anomaly IDA itself
flags (endp comment "sp-analysis failed"): the disassembly loops back
into its own `pop ax` instruction, which would read stale/garbage
stack data on a second iteration -- plausibly dead/unreachable code
(ax==0 never actually occurring as a sound command at this call site)
rather than a real second branch, but left exactly as observed rather
than reinterpreted. -> TriggerSoundEventAfterDriverWait

Run via:
    .\run_ida_script.ps1 name_trigger_sound_event_after_wait.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2D498
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TriggerSoundEventAfterDriverWait", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TriggerSoundEventAfterDriverWait': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "ax=sound command. If WaitForSoundDriverIdle didn't need to wait "
    "(already idle), discards ax and just waits 6 ticks -- no sound "
    "plays. If it DID wait (driver was busy), calls "
    "TriggerSoundEvent(ax) once free. Backwards from the sibling "
    "TryPlaySoundCue/Alt's 'drop if busy' pattern. The ax==0 path "
    "loops back into its own 'pop ax' (IDA flags sp-analysis failed "
    "here) -- plausibly unreachable dead code, left as observed. "
    "Called from sub_2C0FE.",
    False,
)
