"""
Names sub_28412 -- the long-open "sound-driver dispatch" lead first
flagged early this session, now resolvable thanks to naming
PlayPcSpeakerBeep (was sub_16DEA) this round.

Its own pre-existing comment already explained the shape: a sound
event dispatcher taking a command in ax. If the Sound Blaster driver
isn't active (g_driverStateFlags bit 3 clear), only ax==3 does
anything -- it calls PlayPcSpeakerBeep, a PC-speaker fallback beep;
every other command is silently ignored. If the driver *is* active,
it reads driver data via FileEntry_Read (FileEntry bx=0x9043),
ErrorChecks it, then forwards to the driver's own routine via
g_soundDriverFarPtr with bx=6 and es:di pointing past a small header
-- likely "load+play a sound effect" in the driver's own protocol,
though command 6's exact meaning per that external protocol still
isn't confirmed (that's the loaded driver's own dispatch, not
resolvable from this binary alone). -> TriggerSoundEvent

Also corrects PlayPcSpeakerBeep's comment: it's reached via ax==3 in
the driver-inactive fallback path, not "command 6" as speculated when
it was first named -- command 6 is the *active*-driver forwarding
value, a different thing entirely.

Run via:
    .\run_ida_script.ps1 name_trigger_sound_event.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x28412
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TriggerSoundEvent", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TriggerSoundEvent': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Sound event dispatcher, command in ax. If the driver isn't "
    "active (g_driverStateFlags bit 3 clear), only ax==3 does "
    "anything (PlayPcSpeakerBeep fallback); else no-op. If active, "
    "reads driver data (FileEntry bx=0x9043) then forwards to "
    "g_soundDriverFarPtr(bx=6) -- likely 'load+play a sound effect' "
    "in the driver's own protocol. Called from start.",
    False,
)

# Correct the earlier PlayPcSpeakerBeep comment: it's reached via ax==3
# (driver-inactive fallback), not "command 6" (the active-driver value).
ida_bytes.set_cmt(
    0x16DEA,
    "Classic PC speaker beep: programs PIT channel 2 (0x34DE/0x708 "
    "divisor) via ports 0x43/0x42, gates it to the speaker via port "
    "0x61 bits 0-1, waits 4 ticks, then disables it. Called from "
    "TriggerSoundEvent's driver-inactive fallback path (ax==3).",
    False,
)
