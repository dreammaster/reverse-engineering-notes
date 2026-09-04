# Screen layout — DOS windows & borders

The DOS build draws every screen region as a **framed window** (`WIZARDRY`
proc 19 opens one; `CONUNIT` `sub_159A` paints its border; proc 25 closes
it, restoring what it covered).  Windows form a stack — a screen composes
2–4 of them and their borders line up at the screen edges, which is the
"outer frame" you see in DOSBox.

The engine's port had been drawing bare text (and a few fake `+---+`
strings).  `engine/wiz/textscreen.h` now models the real thing:

* `openWindow(x, y, w, h, emphasis=false)` — push a frame, paint its
  border, blank the interior, make the interior the active window.
* `closeWindow()` — restore the covered cells, pop.
* `resetWindow()` — active window := interior of the innermost frame.

## Border glyphs

`CONUNIT sub_159A` writes literal glyph codes into the cell buffer; the
first ~24 glyphs of `200.CHARSET` are the line-art bank (confirmed by
rendering — `tools/monsters` style):

| code | glyph | | code | glyph |
|--:|---|--:|---|
| 1 | ╭ top-left | | 5 | │ right edge |
| 2 | ─ top edge | | 6 | ╰ bottom-left |
| 3 | ╮ top-right | | 7 | ─ bottom edge |
| 4 | │ left edge | | 8 | ╯ bottom-right |

`9..0x0E` are the **emphasis** style (double/heavy): `9` bottom, `0x0A`
top, `0x0B` left, `0x0C` right, `0x0D` = both left corners, `0x0E` = both
right corners.  Glyphs `0x0F` (│ centred), `0x10/0x12/0x13` (door frame
tee/edge), `0x11` (│││ triple), `0x16..0x17` (door hinge) feed the maze
wireframe — see `maze.md`.

## DOS window map

Every `WIZARDRY,19(x, y, w, h, prio, border, 0, 0)` call in the p-code
(`prio` = z-order & 0x3F; `border` 1 = framed, 0 = frameless fill).
`w`/`h` include the border; interior = `(x+1, y+1)` … `(x+w-2, y+h-2)`.

| screen | proc | x | y | w | h | note |
|---|---|--:|--:|--:|--:|---|
| **Castle hub / menus** | `GAMEUTIL 14/23`, `CASTLE 11`, `SHOPS 7`, `ROLLER 24`, `CASTASPE 6`≈ | 0 | 9 | 40 | 12 | the standard bottom menu box |
| party roster (top) | `CASTLE 10`, `ROLLER 23`, `GAMEUTIL 16/34/36` | 2 | 5/6 | 36 | 4/5 | small; sits above the menu |
| `CASTLE` full panel | `CASTLE 20` | 0 | 2 | 40 | 13 | party display + key prompt |
| **Tavern add/remove** | `GAMEUTIL 16` | 2 | 5 | 36 | 5 | |
| **Adventurer's Inn** | `UTILITIE 2` | 0 | 7 | 40 | 6 | + `(5,12,12,10)` room list |
| level-up sheet | `UTILITIE 17` | 4 | 12 | 32 | 8 | |
| **Temple** | `UTILITIE 32` | 8 | 4 | 24 | 10 | |
| **Boltac's** | `SHOPS 14` | 0 | 2 | 40 | 14 | inventory; + `(0,19,40,4)` prompt, `(0,10,40,4)` msg |
| **Roller** char sheet | `ROLLER 28` | 0 | 0 | 40 | 3 (title) · `(0,10,40,3)` · `(0,21,40,3)` · `(13,8,14,7)` picker | |
| roller point-alloc | `ROLLER 30` | 7/20 | 6 | 13 | 10 | two side-by-side |
| **Camp** menu | `CAMP 5` | 0 | 4 | 40 | 17 | + `(13,8,26,5)` sub-dialogs, `(0,21,40,4)` prompt |
| **Combat** monster area | `COMBAT 1` | 2 | 0 | 36 | 6 | frameless (`border`=0) |
| combat party status | `COMBAT 1` | 0 | 5 | 40 | 6 | |
| combat action menu | `CUTIL 13` | 9 | 13 | 22 | 10 | |
| **Rewards / chest** | `REWARDS 33` | 0 | 7 | 40 | 17 | + `(0,2,40,3)` banners, `(1,7,38,6)` |
| **Maze** menu bar | `RUNNER 72` | 0 | 0 | 40 | 3 | frameless |
| maze compass | `RUNNER 72` | 14 | 2 | 12 | 5 | frameless |
| maze party panel | `RUNNER 64` | 0 | 10 | 40 | 6 | |
| maze msg strip | `RUNNER 70/74` | 0/4 | 10 | 40/32 | 3 | |
| **KANJI input** | `KANJIREA 10` | 1 | 1 | 38 | 22 | the near-fullscreen frame |
| generic Y/N / prompt | `UTILITIE 31`, `ROLLER 15` | 0 | 4 | 40 | 3 | |
| generic dialog | `UTILITIE 25` | 8 | 8 | 24 | 15 | |

The maze **wireframe** window is the `WINDOW1` 36×20 cell grid; the engine
frames it as `(1, 2, 38, 22)` under the menu bar.  Its content is
line-art glyphs, not text — see `maze.md`.

## Status

* Primitive (`openWindow`/`closeWindow`/`frame`/`writeAt`, border glyphs)
  — **done**.
* **Combat** — 3 framed panels.
* **Maze** — menu bar + framed wireframe window (DOS 36×20 cell-glyph
  grid, geometry approximated — `maze.md`) + toggled party strip + msg
  strip.
* **Town** — framed party panel (building name on its border) + framed
  menu panel; the `+-----+` fakes are gone.
* **Camp** — one framed panel, "CAMP" on the border.
* **Roller** — Training Grounds menu + MAKECHAR scroll area framed.
* Still absolute-positioned (frame added around, content not yet inset):
  Boltac inventory, Inn room list, Temple, roller point-alloc, camp
  inspect/identify, cemetery, title.  Exact per-screen insets +
  embedded-title placement + window overlap/restore order need DOSBox
  refs.
