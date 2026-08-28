# Low-level "graphics pen" primitives (below the `gx*` blitter layer)

Resolves several of the 22 unnamed `sub_XXXXXX` functions flagged as
follow-ups in `engine_overview.md`. These sit **below** the `gx*` virtual-buffer
blitter API documented previously: where `gx*` moves whole rectangular
buffers around, this layer is a classic single-pixel/line "graphics pen" API
in the tradition of Turbo C / MS C `graph.h` (`_setcolor`, `_setpixel`,
`_lineto`, `_setwritemode`) — which fits, since the MS C graphics library
(`_GrInit`, `videoconfig`, etc.) is already confirmed statically linked in.
All addresses are in `Shannara Demo.asm`.

## State globals (already named in the binary)

- `_gr_color` (word) — current pen/fill color.
- `_gr_op` (word) — current raster-op / write-mode selector, 0-4.
- `_cpx`, `_cpy` (dword, fixed-point — only the high word, `>>16`, is used
  as the integer coordinate) — current pen position, i.e. classic
  MoveTo/LineTo cursor state.
- `_clipx1`, `_clipx2`, `_clipy1`, `_clipy2` — active clip rectangle,
  checked by every primitive below before touching video memory.

## Resolved functions

| Address | Suggested name | Behavior |
|---|---|---|
| `sub_167CE8` | `set_gr_op(int op)` | `_gr_op = ax; return 0`. Sibling setter to the already-named `set_gr_color`. |
| `sub_167CF4` | `get_gr_color()` | `return _gr_color`. Called from `gxVirtualVirtual` to save/restore the pen color around a blit. |
| `sub_167D08` | `get_pixel(x, y)` | Clip-tests `(x,y)` against `_clip*`; if outside, returns 0. Otherwise `gxVideoAddr(x,y)` then reads one byte. |
| `sub_167D54` | `put_pixel(x, y, color=bl)` | Clip-tests, then dispatches on `_gr_op` (5-way jump table, only 2 cases implemented): op 0 = direct byte write (`*addr = color`); op 3 = XOR (`*addr ^= color`); **ops 1/2/4 intentionally do nothing to video memory** but still call `gxDirtyDisplay(x,y)` — i.e. those raster-op values are defined/reserved in the enum but have no visible effect when plotting a single pixel (possibly meaningful only for the line/fill primitives below). Always ends by marking `(x,y)` dirty via `gxDirtyDisplay`, confirming `gx*`'s dirty-rect tracking is driven from this primitive layer. |
| `sub_167334` | `draw_line(x1, y1, x2, y2)` (working name) | The real line-draw primitive: clips the endpoints against `_clip*`, resolves the start address via `gxVideoAddr`, then walks pixels. Only distinguishes `_gr_op == 0` (direct write, `_gr_color`) vs. `!= 0` (XOR) — simpler than `put_pixel`'s full 5-way dispatch. Handles the vertical/horizontal-run case explicitly (`cmp ax,bx` / `cmp dx,cx` special-cased for axis-aligned lines, `+0x280` stride step per pixel matching the 640-wide SVGA mode picked by `graphics_mode(5)`); the general diagonal case (past `loc_167424`, not fully traced this pass) uses two more jump tables (`jpt_1674AD`, `jpt_16768D`) that look like the same "op 0 / op 3(XOR) / else no-op" pattern per-axis. 768 lines total — the diagonal/Bresenham branch is the bulk of it and is a good next target if pixel-exact line rendering matters for reimplementation. |
| `sub_1672D0` | `line_to(x, y)` (working name) | Thin wrapper: takes a target point in `(eax,edx)`, calls `draw_line(_clipx1>>16, _cpx>>16, x, y)`-ish (from `_cpx`/current pen pos to the target — exact arg order not 100% pinned), then updates `_cpx`/`_cpy` to the new point. Classic `LineTo`-style cursor-relative draw. Called from `_draw_bevel` (button/panel 3D border rendering) and from `draw_rect_`. |
| `sub_15E930` | *(stub, working name `dib_cache_probe`?)* | `init_dib_file` (DIB/bitmap file loader, called from `load_dib`) calls this twice with a filename pointer in `eax`, but the function body is dead code: `xor eax,eax; mov eax,eax; ret` — **always returns 0, parameter completely unused**. Either a compiled-out cache-lookup hook or a check that's unconditionally false in this build. Worth checking whether the full game's binary has real logic at the equivalent location — if this is a shared-engine hook, the demo may simply have it stubbed out while the full game doesn't (or it's dead in both). |

## Not yet resolved (same numeric neighborhood, lower priority)

`sub_167D08`'s and `sub_167D54`'s callers beyond the ones noted, and the
Bresenham/diagonal branch of `sub_167334`, weren't traced line-by-line this
pass — flagged for a follow-up if pixel-perfect rendering parity matters.

## Classification of the remaining "unnamed" functions

Cross-referencing each `sub_XXXXXX`'s caller/DATA-xref shows most of the
remaining unnamed functions are **not** unexplored game logic — they're
internals of the already-identified third-party libraries, just not worth
individually naming:

| Address | Caller/xref | Library |
|---|---|---|
| `sub_1652E0` (2511 lines) | `q_expand` | Q-codec block decompressor — already flagged in `engine_overview.md` as deep codec internals, deliberately not fully traced. |
| `sub_1652A5` | `AILDEBUG_start` (DATA xref) | Miles/AIL internal |
| `sub_174500` | `AILXMIDI_start_` (DATA xref) | Miles/AIL XMIDI internal |
| `sub_170A10` | `_AIL_sample_user_data` | Miles/AIL internal |
| `sub_17ABC6`, `sub_17AB54`, `sub_17ABFA` | `_DoINTR`, `__int386x` | Watcom CRT DOS-extender interrupt-call internals |
| `sub_17DBD6`, `sub_17DBDC` | `__set_errno`, `__set_doserrno` | Watcom CRT errno internals |
| `sub_17A84B` | `__MkTmpFile_` | Watcom CRT internal |
| `sub_17DCC0` (275 lines) | `read_pt_len` | Watcom CRT-area (0x17D region, alongside the MS graphics library) — not yet characterized, likely CRT/graphics-library internal given its neighborhood, but not confirmed. |
| `sub_17DC6C` | data-table entry in `dseg02` | Not yet characterized. |
| `nullsub_3`, `nullsub_5` | — | Empty stubs. |

Net effect: of the 22 originally-unnamed functions, **7 are now resolved as
genuine engine/game code** (the drawing-primitives table above) and **13 are
confirmed third-party-library internals** (table above, low priority to
chase further), leaving `sub_1652E0` (codec, known-deep) and `sub_17DCC0` /
`sub_17DC6C` (uncharacterized) as the only real remaining unknowns.
