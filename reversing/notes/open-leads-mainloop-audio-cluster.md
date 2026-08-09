# Open leads: mainloop's ambient-sound/plugin-hook callees

Chased `mainloop`'s remaining unmatched callees this round. Two resolved
(`get_hotspot_at`, `__GetLocationType`, `SpriteCache::operator[]` — logged
in `matches.json`), but a small cluster didn't resolve cleanly. Noting the
partial analysis rather than losing it.

## sub_4089CC — probably ambient-sound distance/volume, but architecture may differ from 2011

Computes `distx = playerchar->x - <stored X>`, `disty = playerchar->y -
<stored Y>`, squares and sums them, scales by a divisor read from a global
— structurally a dead ringer for the *logic* of
`get_volume_adjusted_for_distance(int volume, int sndX, int sndY, int
sndMaxDist)` (`Engine/AC.CPP:1502`), which does the same distance-based
volume falloff calc and also reads `playerchar->x/y` directly.

**But**: `get_volume_adjusted_for_distance` takes `sndX`/`sndY` as
*parameters* (stack args), while `sub_4089CC` reads them from fixed globals
(`dword_4EDA5C`, `dword_4EDA60`) with no visible stack-parameter access
pattern, and has no loop — whereas the 2011 caller,
`update_ambient_sound_vol()`, loops over `ambient[chan]` for
`chan < MAX_SOUND_CHANNELS`. Called from `PlayAmbientSound` and
`SetSoundVolume` (matches roles), but the calling-convention mismatch means
this might be an earlier (2002), simpler single-global-slot ambient sound
implementation that predates the `AmbientSound ambient[]` array + the
`update_ambient_sound_vol`/`get_volume_adjusted_for_distance` split — another
case like `sub_42B394`/`cc_run_code` and `do_conversation`/
`show_dialog_options` where the 2002 and 2011 code shapes diverge. Left
unmatched pending a closer read of the surrounding globals (are
`dword_4EDA5C` etc. actually a struct instance, not scalars?).

## sub_425720 — plugin/driver hook dispatcher, unidentified target

```
mov eax, dword_536F68        ; some global object pointer
cmp dword ptr [eax+74h], 0   ; vtable slot at offset 0x74 (index 29)
jz short loc_425737
mov ecx, dword_536F68
call dword ptr [ecx+74h]     ; call through vtable if non-null
loc_425737:
retn
```

No arguments passed. Called directly from `mainloop`. This shape (check a
vtable slot on a global driver-ish object, call it if populated) matches
the general pattern of `platform->RunPluginHooks(AGSE_xxx, ...)`-style
calls seen elsewhere in `AC.CPP`, but `RunPluginHooks` normally takes two
args (event code + data) and this call site passes none — so it's more
likely a different, no-arg hook on the `platform`-equivalent driver object
(`IAGSPlatformDriver`?), or `dword_536F68` isn't `platform` at all. Didn't
find a confident source match in the time available; worth checking IDA's
type info on `dword_536F68` (does it already have a struct/vtable type
assigned?) before guessing further from the `.asm` text alone.

## sub_4084E0 / sub_408392 / sub_425230 / sub_4083FC / sub_40A21C

A tightly-coupled cluster (calls between `sub_40A21C`↔`sub_4084E0`/
`sub_425230`, `sub_4083FC`↔`sub_408392`), also called from `FadeOut` (an
already-named function — possibly just Allegro's own `fade_out()`, not
AGS-specific). Bodies reference addresses in the `0x47Exxx` range
(`sub_47E990`, `sub_47E7A0`), which is inside the address range we've seen
belong to `Engine/libsrc/*` third-party audio codec libraries (apeg is
`0x46Exxx`, dumb is `0x47Cxxx`; `almp3.c`/`ALOGG.C` — the MP3/OGG plugin
code — would plausibly sit nearby). Likely audio-driver polling invoked
during a blocking palette fade, not core AGS game logic. Deprioritized per
the third-party-library guidance in `CLAUDE.md` — not pursued further this
round.
