"""
Names sub_16DEA, called once from the long-open sound-driver dispatch
sub_28412 -- a classic PC speaker beep routine.

Programs the 8253/8254 PIT channel 2 with a divisor (0x708) against
base ax=0x34DE (computing a tone frequency), writes it via port 0x43/
0x42 (channel 2 mode/data), then sets PPI port 0x61 bits 0-1 to gate
the timer output to the speaker and enable it, waits 4 ticks (via the
already-named `wait`), then clears those bits to silence it. The
textbook PC-speaker "beep for N ticks" primitive.

This resolves part of the sub_28412 open lead flagged much earlier
this session ("sound-driver dispatch, command 6's meaning still
unconfirmed") -- sub_16DEA is very likely what one of its dispatch
branches (plausibly command 6, a PC-speaker fallback beep when no
Sound Blaster is configured) actually does, though sub_28412's own
dispatch structure itself isn't retraced this round.
-> PlayPcSpeakerBeep

Run via:
    .\run_ida_script.ps1 name_play_pc_speaker_beep.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x16DEA
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "PlayPcSpeakerBeep", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'PlayPcSpeakerBeep': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Classic PC speaker beep: programs PIT channel 2 (0x34DE/0x708 "
    "divisor) via ports 0x43/0x42, gates it to the speaker via port "
    "0x61 bits 0-1, waits 4 ticks, then disables it. Called from the "
    "sound-driver dispatch sub_28412 -- plausibly its PC-speaker "
    "fallback path.",
    False,
)
