# MM2.EXE layout — why IDA warned about "trailing information"

**Short answer:** yes, something more was needed.  `MM2.EXE` is an MS-C program linked with
**Phoenix Plink86** (the string `Copyright (C) 1984, 1985, 1986 by Phoenix Software Associates Ltd.`
is in the stub).  IDA's MZ loader only maps what the MZ header describes; the game is really made of

1. the **resident image** (the range the MZ header describes),
2. the **DGROUP** (initialised data + BSS) — this is the "trailing data" — and
3. **14 code overlays**, the `*.OVL` files next to the EXE.

Loading only `MM2.EXE` therefore gives a disassembly with empty data and no overlay code.
`tools/plink_info.py` dumps all of this from the real files; `tools/mm2_layout.py` is the parser
(also used by the IDA scripts).

## MZ header vs. file

| | |
|---|---|
| File size | 77,824 (0x13000) |
| Header | 0x800 bytes, 500 relocations |
| Image (per header) | ends at 0x8610 → **0x7E10 bytes** are what IDA maps (base 1000h, `10000h–17E10h`) |
| Trailing data | 43,504 (0xA9F0) bytes at 0x8610 — not covered by the header |
| Entry | `177D:0488` (the Plink86 stub), then `jmp far` to the C startup at `cs:0036` |
| Initial SS:SP | `1822:0800` (rel. paragraphs) |

Everything below uses paragraph numbers relative to the load base; IDA's MZ loader relocates by
+0x1000 paragraphs, so rel `0x0D85` = IDA selector `0x1D85`.

## Resident image (IDA `10000h–17E10h`)

| IDA range | Contents |
|---|---|
| `10000–16AD0` | MS-C code (`_TEXT`), C runtime incl. `__FF_MSGBANNER`, startup |
| `16AD0–16BF0` | small data block |
| `16BF0–177D0` | **Plink86 segment table** (16-byte rows, `plink_segtable`), the overlay file-name strings, then the **219 overlay thunks** (from `16D8A`) |
| `177D0–17D30` | Plink86 runtime (stub code; `start` at `177D:0488`, its own code segment) |
| `17D30–17E10` | Plink86 runtime data (file handle, current file name buffer, …) |

There is a single 64K code segment, `cs = load base`.  The overlay windows sit *inside it*, straight
after the resident image, so overlay code and resident code use plain near calls/jumps.

## Overlays

Two shared windows in the code segment (offsets in `cs`):

| Window | cs offset | Size | Overlays that load there |
|---|---|---|---|
| `OVL_A` | `7E10–C130` | 17,184 | `1MENU2`, `2COMBAT`, `2PLAY` |
| `OVL_B` | `C130–D850` | 5,920 | `1MENU1`, `1RETINN`, `2MISC`, `2MISC2`, `2CAST1`, `2CAST2`, `2CMDS`, `2CAVES`, `2BRAIN`, `2SMITH`, `2TEMPLE` |

`.OVL` files are raw code: the file body is exactly the window contents (plus a relocation
table in front for `1MENU1.OVL` only — 2 entries, both `mov dx, seg cs`).  Only one overlay per
window can be resident, which is why they overlap.

### Segment table (`16BF0`, 16-byte rows)

The loader (`plink_read_segment`, `17981`) indexes it 1-based; row *k* uses these words, and
also reads the *next* row's first words:

| Off | Meaning |
|---|---|
| +6 | flags: `8000` currently loaded, `4000` preload at startup; `FFFF` ends the table |
| +8 | number of relocation entries (4 bytes each, table padded to whole paragraphs, stored at the record's file position) |
| +A / +C | load window, rel. paragraphs `[start, end)` |
| +E | offset (within the table's segment) of the NUL-terminated file name |
| +10h (dword) | paragraph offset of the record in its file (= next row's +0) |
| +14h | paragraphs to read from the file (= next row's +4) |

| # | File | Window | Body bytes |
|--:|---|---|--:|
| 1 | `1MENU2.OVL` | A | 7,280 |
| 2 | `2COMBAT.OVL` | A | 15,952 |
| 3 | `2PLAY.OVL` | A | 17,184 |
| 4 | `1MENU1.OVL` | B | 3,488 (+16 reloc) |
| 5 | `1RETINN.OVL` | B | 2,784 |
| 6 | `2MISC.OVL` | B | 3,808 |
| 7 | `2MISC2.OVL` | B | 3,968 |
| 8 | `2CAST1.OVL` | B | 4,448 |
| 9 | `2CAST2.OVL` | B | 4,688 |
| 10 | `2CMDS.OVL` | B | 4,432 |
| 11 | `2CAVES.OVL` | B | 5,488 |
| 12 | `2BRAIN.OVL` | B | 5,104 |
| 13 | `2SMITH.OVL` | B | 5,920 |
| 14 | `2TEMPLE.OVL` | B | 3,824 |
| 15 | `MM2.EXE` (**DGROUP**) | rel `0D85–18A2` | 43,472 read, 45,520 mapped |

Each `.OVL` file size equals its computed size exactly (plus the 16-byte reloc paragraph for
`1MENU1`), which is how the mapping above was verified.

### Thunks (`cs:6D8A–77CE`, 219 × 12 bytes)

```
9A <off seg>   call far plink_thunk_entry   ; loads the overlay if not resident
dw  idx        8000h | row number; 8000h = resident target (nothing to load)
EA <off seg>   jmp far real_target
```

*All* calls into an overlay go through a thunk (68 of them target overlays), and — a Plink86
quirk — overlay code also calls **resident** functions through thunks (151 thunks with index
`8000h`), so `call thk_res_XXXX` inside an overlay is a call to resident code.  IDA names them
`thk_<overlay or res>_<target offset>`.

## DGROUP

* Loaded by the stub from `MM2.EXE` itself: 2 relocation entries at file `0x8620`, body at file
  `0x8630`, size `0xA9D0`, at rel `0D85:0` (IDA `1D850–28220`).
* The C startup (`cs:0040…`) sets `SS = DS = DGROUP`, `SP = 0xA9CE` (with 2 KB more after it for the
  stack: IDA `28220–28A20`) and zeroes `[546Ch, A9D0)`, so the **initialised data is `0000–546C`**
  (21,612 bytes) and the rest is BSS.
* The two relocations are at DGROUP offsets `478A` and `481E` (far pointers to DGROUP itself).
* Starts with `F9 C3` (`stc; retn`), then `Version 1.01`, town names, class/race/skill tables, …

## How the databases model this

IDA can't hold overlapping memory, so:

* **`mm2.idb`** — resident image, thunk table, Plink86 runtime, **DGROUP loaded with relocs**,
  empty `OVL_A`/`OVL_B` windows, `ds` defaulting to DGROUP for the game code.
* **`ovl/<NAME>.idb`** (14) — a *copy of mm2.idb* with that overlay loaded at its true address in its
  window, so overlay → resident references (thunks, data) resolve.  Its `.asm`/`.idc` export only
  the overlay's range; the resident part is not repeated.

Rebuilding and syncing: [overview.md](overview.md).

## Not (yet) resolved

* Which game states trigger which overlay load — the resident code paths that call each thunk
  still need naming.
* `MM2.BAK` in the GOG folder is byte-identical to `MM2.EXE`; unrelated to the layout.
