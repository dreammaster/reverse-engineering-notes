# The video/draw module (`.CC` member 8F99h)

`MM3.CC` member `8F99h` (15,570 bytes decoded; also holds the 256-colour master palette at `39Ch`, see `mm3-re.md` section 4) is
loaded at run time; its segment is `cs:word_24F63` in the resident code.  It starts with a table of 3-byte `jmp near` entries
(offsets `00h`..`30h`); the game calls them through tiny wrappers in `seg007` (`push cs:word_24F63; push <offset>; retf`).  The module
is loaded into the database as segment `vdrv` (`60000h`, `ida_scripts/load_driver.py`) and the wrappers are named
`vdrv_<offset>_<guess>`.

> The first BinDiff run named these wrappers `Music_*`, `Sound_*`, `Window_update`, ... because Xeen's sound-driver wrappers
> have the same shape.  They are video calls; the names have been replaced.

| Offset | Wrapper | Role (from the code of each entry) |
|---|---|---|
| `00h` | `vdrv_00_transition` | copies the screen to a buffer and installs a far-pointer callback (screen transitions) |
| `03h` | `vdrv_03_hideMouse` | hide the cursor |
| `06h` | `vdrv_06_closeWindows` | pop N windows of the 7-deep window stack, restoring the saved background |
| `09h` | -- | `retf` (unused) |
| `0Ch` | `vdrv_0C_showRaw` | load and show a 320x200 `.raw` image |
| `0Fh` | `vdrv_0F_fade` | palette fade in/out (`int 10h` function 1002h) |
| `12h` | -- | release screen buffers and close the cc file |
| `15h` | `vdrv_15_drawSprite` | draw frame N of a sprite resource at x, y (the line codec of `mm3-re.md` section 5) |
| `18h` | `vdrv_18_getMouse` | mouse position and buttons |
| `1Bh` | `vdrv_1B_animateCursor` | advance/draw the animated cursor |
| `1Eh` | `vdrv_1E_openWindow` | open a text window (rectangle, colours, text); window stack of 7 |
| `21h` | `vdrv_21_loadSprites` | load a sprite resource by name (returns a far pointer) |
| `24h` | `vdrv_24_freeSprites` | free it |
| `27h` | `vdrv_27_setCursor` | install the mouse cursor sprite (far pointer to a sprite set + frame), reset the mouse driver (`int 33h`) and set its range; the earlier name `showScreen` was wrong |
| `2Ah` | `vdrv_2A_starfield` | 74-particle starfield / comet transition effect driven by a far callback (intro, transitions) |
| `2Dh` | `vdrv_2D_printText` | print a string with the in-text control codes; **control code `05h` + four hex digits runs a draw list**, which is how the 3D view and the party HUD are drawn (full table and the list format in `view.md`) |
| `30h` | `vdrv_30_init` | module initialiser (called from `sub_24F1A` with the game's callback table) |

The sound/music code is a separate set of drivers (`ADLIB.DRV`, `ROLAND.DRV`, `BLASTER.DRV`, `COVOX.DRV`, `TANDY.DRV`, `IBM.DRV`, `DEMO.DRV`,
`TIMER.DRV`); `*.M` are the songs and `S1.S`-`S7.S` are **raw 8-bit unsigned PCM samples** (centre value 7Fh), not scene scripts as
`mm3-re.md` section 6 guessed.
