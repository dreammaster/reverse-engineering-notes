# Retail `Shannara.idb` (SHANNARA.LE) — relationship to the demo, and the game layer

Headless pass (2026-10-12) over copies of `Shannara.idb`, `Shannara Demo.idb`
and `..\xanth\Xanth.idb` via `idat.exe`. Nothing was run against a live
database (IDA was not open); results were applied to the real IDBs afterwards
with backups in `reversing/backups/`.

## 1. What `Shannara.idb` is

| | Demo (`DEMO.LE`) | Retail (`SHANNARA.LE`) |
|---|---|---|
| Original path | `D:\sources\legend\shandemo\DEMO.LE` | `D:\sources\legend\shannara\SHANNARA.LE` |
| Entry | `0x15E638` | `0x7BC20` |
| Code segment | `0x157000–0x180000` | `0x10000–0x9D000` |
| Data segment | `0x187000–0x1AFA50` | `0xA0000–0xEE1B0` |
| Functions | 1,165 (1,140 named) | 2,286 (879 named, 1,407 `sub_`) |
| Build string | "Shannara CD-ROM Demo" | `1.0.0  11-13-1995` |

**Retail is the full game**, same engine, same toolchain (Watcom + DOS/4GW LE),
but a **different build**: shared functions are frequently a different size
(e.g. `load_pic` 609 vs 388 bytes, `load_palette` 384 vs 190, `init_audio`
1517 vs 1343) and retail uses Watcom register-convention symbol spellings
(`main_`, `printf_`, `memset_` …). So the demo is *not* a byte-identical subset
and addresses/sizes do not transplant directly. Names do transplant — and for
the shared engine, most already had (658 same-named functions, 522 of them the
same size).

### Layout of the retail code segment

```
0x10000 – 0x75000   game layer: ~1,100 UNNAMED functions (≈400 KB) — object model,
                    rooms/NPC logic, verb handlers, message text. <-- the real work
0x75000 – 0x9D000   engine/library: pic/font/region/list/audio/AIL/CRT — nearly all named
```

Only 10 additional demo→retail name transfers were found (bigram-Jaccard of
mnemonic sequences ≥ 0.86, mutual-best, unique): `set_music_volume`,
`reset_mouse_limits`, `_XMI_read_log?`, `load_font_palette?`, `reset_bevel_style?`,
`q_kill?`, `AILXMIDI_start_?`, `_XMI_serve?`, `make_voice?` (+ `unload_pics`, which
could not be renamed because the name is already taken elsewhere in the IDB).
Conclusion: **further demo-side naming work has little left to give retail;**
effort should go into the retail game layer directly.

### Xanth is not a useful comparison here

`Xanth.idb` is `xanth_decoded.exe` — a **16-bit real-mode** RTLink-flattened EXE
(same toolchain family as Gateway), 2,710 functions. A mnemonic-sequence match
against either 32-bit Shannara IDB returns **0** matches. A name transplant would
need a semantic (string/call-graph) matcher. The previously noted
`xanth_vs_Shannara Demo.BinDiff` is **no longer in `..\xanth\`** (only the two
`.BinExport`s, the IDBs and two zips remain), so the BinDiff step on the roadmap
is dropped unless that file turns up.

## 2. Game-layer findings (retail)

Working names use the project's trailing-`?` convention where not fully
verified. Applied via `reversing/scripts/apply_retail_findings.py`.

### 2.1 Object table and attribute bits (high confidence on structure)

- `word_217A0` = number of objects (ids are 1-based; every accessor rejects
  `id <= 0 || id > word_217A0`).
- `byte_20918` = **5-byte records `{u8 type; u32 dataPtr}`** indexed `(id-1)*5`
  (`off_20919` is the pointer half). Types seen: 1–5.
- `sub_1A200` → `object_get_location?`: returns the word at `+0Ah` of the data
  record for types 2 and 5, `+0Bh` for type 3, else 0 (likely the
  parent/container id).
- Attribute flags live in per-object bitmaps. A second **5-byte descriptor table**
  (`byte_208A0` / `dword_208A1`, indexed `(attr-1)*5`) says, per attribute,
  `{u8 bitOffset; u32 typeMask}`; an attribute applies to an object only if
  `(typeMask >> (type-1)) & 1`. The helper family:

| Address | Name | Behaviour |
|---|---|---|
| `0x1A104` | `object_attr_locate?` | `(obj, attr, &bit)` → byte* holding the flag, or 0 |
| `0x1A184` | `object_has_attr` | test the bit (≈299 callers) |
| `0x1A1B0` | `object_set_attr` | set the bit |
| `0x1A1D8` | `object_clear_attr` | clear the bit |

### 2.2 Generic bit arrays (high confidence)

`0x21F30 bitset_test` (≈566 callers), `0x21F80 bitset_set`, `0x21FC4
bitset_clear`: 1-based bit index, LSB-first inside each byte
(`n-1 → byte (n-1)>>3, bit (n-1)&7`, `0` is a no-op/false). These are the
game-flag primitives.

### 2.3 `format_noun_phrase?` — `0x1C0E8` (≈2,495 callers)

The single most-called routine in the binary. A **grammar helper** that turns
(object id, form selector) into a word for `%s` inserts in message text
("You already have%s.", "%s glad that this dark, foreboding figure…"). The
selector (low 3 bits of the first argument; object id in the upper 16)
picks article/pronoun/possessive: `the / some / a / an`, `he / she / it /
they`, `your / their …`, using `object_has_attr` bits (including bit `0x16` =
female) and comparing against the two player-character ids
(`word_ADEB0` / `word_ADEB2`) to substitute "your"/"you". This is why the game
text is full of bare `%s` — the engine assembles sentences from object
attributes, which a reimplementation must reproduce exactly.

### 2.4 `run_command?` — `0x1B7B0`

Verb–object dispatch entry: `(verb=eax, dobj=dx, iobj=bx)`. Maintains a
re-entrancy counter (`dword_ADE20`) and a repeat guard (`dword_CD518`; after 4
identical consecutive commands prints *"Perhaps now is not the best time to
try…"*), stores the current command in `dword_CD918` (verb) /
`dword_CD924+2` (dobj) / `dword_CD92C+2` (iobj), then walks per-object
"handler present" checks (`sub_21AF4(obj, 1..6)` for the player, dobj and
iobj) before falling back to a default response (`sub_1E23C`). Strings such as
*"No input verb"*, *"Take which Object?"*, *"Give orders to…"* confirm this is
a verb + noun interface, consistent with the Later-engine classification.

### 2.5 `show_dialog?` — `0x26414`

A 6 KB UI routine (≈588 callers): hide mouse, `set_font(10)`, `set_font_color(0xFF)`,
build a text window from the 6-byte entries at `word_BBA0E`, run it with the
mouse pushed/popped. Working name only; not traced.

## 3. Next steps (retail)

1. Name the callees of `run_command?` (`sub_21AF4`, `sub_1E110`, `sub_1E23C`) and
   the object-list helpers (`sub_1A0C4`, `sub_1A254/2A0/2EC/348/400/49C/4E8`,
   `sub_10F70`) — `sub_1B470` looks like an object *move/reparent* using a
   child/sibling list.
2. Find where per-object handlers are registered/dispatched (the
   handler-existence flags consulted by `run_command?`); that is the retail
   analogue of Gateway's `proc_table`/`Logic_call` and decides whether the
   room/NPC logic is compiled code (as in Gateway) or data.
3. Extract the message strings with their referencing function to map NPC/room
   logic regions (e.g. Allanon lines cluster together).
4. Retail `Shannara.asm` is exported locally (15 MB, git-ignored) for grepping.
