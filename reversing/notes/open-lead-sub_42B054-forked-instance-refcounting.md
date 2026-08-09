# RESOLVED: sub_42B054 = ccFreeInstance

**Update:** resolved during dedicated `ccInstance` struct-recovery work
(see `reversing/notes/struct-layout-drift.md`). Reading `ccCreateInstanceEx`
(already matched) directly showed it storing the source `ccScript*` at
instance offset `+0x9A4` and incrementing that script's own `+0x1C4C` field.
That is exactly `[Block+0x9A4]` / `[nested+0x1C4C]` from the description
below, in reverse. `Common/CSRUN.CPP:1042`'s `ccFreeInstance` matches
`sub_42B054` line for line:
```c
if (cinst->instanceof != NULL) {
  cinst->instanceof->instances--;
  if (cinst->instanceof->instances == 0) {
    simp.remove_range((char *)&cinst->globaldata[0], cinst->globaldatasize);
    simp.remove_range((char *)&cinst->code[0], cinst->codesize * sizeof(long));
  }
}
```
The 2011/2002 "logic difference" hypothesized below never existed --
`post_script_cleanup`'s simplified `if (forked) ccFreeInstance(...)` in the
*caller* just doesn't show what `ccFreeInstance` itself still does
internally. `sub_42A969` (the shared helper called twice) is very likely
`simp.remove_range`. Logged in `matches.json` with full evidence.

Original write-up kept below for the record.

---

# Open lead: sub_42B054 (fork-instance cleanup, refcounted in 2002)

Called from `post_script_cleanup` (`+66`, very early) and
`EndSkippingUntilCharStops` (per an xref that's likely actually a following
unnamed function sharing no clear boundary — not necessarily
`EndSkippingUntilCharStops` itself, since that function is only ~15 lines
and this xref offset is implausibly large for it; needs checking directly
in IDA rather than trusting the flattened `.asm` text for that one).

Takes one pointer param (labeled `Block` by IDA, generic naming, not
necessarily `BITMAP*`). Its body:
- reads a nested pointer at `[Block+0x9A4]`
- if non-null, decrements a counter at `[nested+0x1C4C]`
- only when that counter hits zero, calls `sub_42A969` twice (a shared
  helper, args pulled from `[Block+4]`/`[Block+8]` and
  `[Block+0xC]`/`[Block+0x10]<<2`) against a shared context object
  (`unk_534930`)

This is a genuine **reference-counting** pattern on the nested object. The
2011 source's equivalent cleanup in `post_script_cleanup`
(`Engine/AC.CPP:3098-3099`) is just:
```c
if (scripts[num_scripts-1].forked)
  ccFreeInstance(scripts[num_scripts-1].inst);
```
— an unconditional free gated by a boolean flag, no counter anywhere.
`ccFreeInstance` itself (`Common/CSRUN.CPP:1042`) isn't matched yet either,
so it's possible `sub_42B054` doesn't correspond to `ccFreeInstance` at all,
but to something upstream/different that existed only in 2002 (e.g. a
shared-global-data refcounting scheme for forked script instances that got
simplified away by 2011, if forking stopped sharing state the same way).

Not matched — this looks like a real logic difference, not just a
renamed/relocated function. Worth revisiting once `ccFreeInstance` and the
`ExecutingScript`/`ccInstance` field layout at the relevant offsets are
better understood (ties into the `ccInstance` struct-drift note in
`reversing/notes/struct-layout-drift.md` — the offsets here, e.g. 0x9A4,
are suspiciously close to `ccInstance`'s known 2002 malloc size of 0x9A8).
