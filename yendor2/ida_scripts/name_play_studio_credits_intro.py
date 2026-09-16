"""
Names sub_11A10 and its two effect helpers sub_11D66/sub_11DE2,
called once from `start` (not InitGame -- distinct from the earlier
PlayTitleScreenSequence) as an elaborate multi-scene animated credits
sequence: plays music track 0x14, and interleaves several
DrawPicture calls (picture ids 7/8/9/0xA/0xB/0x21/0x64) with multiple
DrawShadowedTextAlt credit-text panels (each its own wait/fade
timing), sound cues (TriggerSoundEvent, TryPlaySoundCueAlt,
PlaySoundSequenceGH), and the two effect helpers below, before
loading a new master palette (record 4) and finishing with 3
sub_2589A calls (likely a wipe/transition primitive) and a final
optional wait-for-driver gate. Reads as the game's studio/publisher
credits intro cinematic. -> PlayStudioCreditsIntro

sub_11D66 (called once, between two credit-text panels): loops 0x41
times, each iteration drawing two overlapping pictures (background +
a bgTransparent-4 overlay) at a shrinking y position while a
manually-incremented reveal-height counter (word_32988) grows by 3
-- a progressive reveal/wipe effect under a credits line, gated by a
busy-wait on word_32956 reaching 3 each iteration (an external
tick-driven timer). -> PlayCreditsWipeAnimation

sub_11DE2 (called once, later in the sequence, right after a credits
text panel): plays a 14-frame picture sequence (ids 0x22-0x2F),
2 ticks per frame via `wait` -- a short animated flourish (flame/
sparkle/spinner style) under a credits line. -> PlayCreditsFrameAnimation

Run via:
    .\run_ida_script.ps1 name_play_studio_credits_intro.py
"""
import idc
import ida_name
import ida_bytes

for ea, name, desc in [
    (0x11A10, "PlayStudioCreditsIntro",
     "The game's studio/publisher credits intro cinematic: music, "
     "several picture reveals, multiple DrawShadowedTextAlt credit "
     "panels, sound cues, and 2 effect helpers "
     "(PlayCreditsWipeAnimation, PlayCreditsFrameAnimation), before "
     "loading a new master palette and finishing with a 3-part wipe/"
     "transition. Called once from `start`."),
    (0x11D66, "PlayCreditsWipeAnimation",
     "Progressive reveal/wipe effect: loops 0x41 times drawing two "
     "overlapping pictures at a shrinking y while a reveal-height "
     "counter grows by 3, gated by a tick-driven busy-wait each "
     "iteration. Called once from PlayStudioCreditsIntro."),
    (0x11DE2, "PlayCreditsFrameAnimation",
     "Plays a 14-frame picture sequence (ids 0x22-0x2F), 2 ticks per "
     "frame -- a short animated flourish. Called once from "
     "PlayStudioCreditsIntro."),
]:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(ea, desc, False)
