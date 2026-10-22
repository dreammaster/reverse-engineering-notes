# Wasteland 1 (WL.EXE) — data formats and engine notes

Sources: `wasteland1/Wasteland.idb` (WL.EXE unpacked, 706 functions, hand-written 8086 assembly with register calling
conventions, one 52 KB code segment `sg01a2` at 0x10000), the Wasteland Suite sources in `wasteland1/docs/wasteland_suite`
(format reference), and the DOS data files (`wasteland1/game`, from the GOG `rom/data`). C++ readers are in
`engine/src/wasteland1` (library `wl`, tool `wlrun`).

## Files

| File | Content |
|---|---|
| `GAME1` / `GAME2` | disk 1 / 2: 20 + 22 map blocks, one savegame block, 3 / 1 shop item lists. Blocks start with `msq0`/`msq1`. |
| `ALLHTDS1/2` | tilesets (4 + 5): Huffman compressed 16x16 4bpp tiles, 8-byte header `u32 size, "msq", disk`. |
| `ALLPICS1/2` | 33 / 33 portrait animations: base frame (96 px wide) + animation block, both Huffman compressed. |
| `COLORF.FNT`, `CURS`, `IC0_9.WLF`, `MASKS.WLF`, `TITLE.PIC`, `END.CPA`, `WLA.BIN`, `TRANSTBL`, `INFO` | font, cursors, icons, sprite masks, title picture, ending animation, executable tables, misc. |

## Codecs

* **Bit reader**: MSB first for Huffman data; the string coder reads 5-bit codes LSB first.
* **Huffman tree** (serialised in front of the data): bit 0 = inner node (left subtree, one separator bit, right subtree),
  bit 1 = leaf followed by 8 payload bits. The data follows immediately; the next MSQ header begins at the next byte.
* **Rotating XOR** (map/savegame blocks): the first two bytes are `e1,e2`; key = `e1 ^ e2`; each byte is `b ^ key`,
  `key = (key + 0x1F) & 0xFF`; checksum `sum(-b)` compared with `e1 | e2<<8`.
* **Vertical XOR** (pictures, tiles): after Huffman decoding every row is XORed with the row above (width/2 bytes per row).
* **MSQ blocks in GAME1/2**: scan for `msq` + disk digit; the obfuscated part covers the bytes up to `stringsOffset`
  (word at `size*size*3/2` of the plain data), the remainder (strings, Huffman tile map) is stored plain.

## Map block (after de-obfuscation)

```
size*size/2   action classes (nibbles, high nibble = even x)
size*size     action ids
0x2C          central directory: stringsOffset, monsterNamesOffset, monsterDataOffset, 16 action-class table offsets
              (classes 1..15), nibble6Offset (128 u16 special action ids), npcOffset, 0
1             map size
10            info: unknown0, unknown1, encounterFrequency, tileset, lastMonster, maxEncounters, backgroundTile,
              timeFactor(u16), unknown9
37            battle string ids
...           action tables (offset table per class, then action bytes), NPC records (0x100 each), monster names (NUL
              terminated) and monster data (8 bytes: xp u16, skill, randomDamage, groupSize<<4|ac, fixedDmg<<4|weapon, type, picture),
              strings (60 byte char table + offset table + 5-bit LSB-first codes, 4 strings per group)
tile map      u32 size, u32 unknown, Huffman tree + size*size tile ids (stored plain after the obfuscated part)
```

Tile ids >= 10 index the map's tileset (`tileset - firstTilesetOfDisk`), 16x16 4bpp. The colours are the 16 EGA colours.

## Savegame block (4614 bytes)

0x800 bytes = `Saved`: 4 parties x 14 (`members[8]`, x, y, map, prevX, prevY, prevMap), 64 unknown, viewport x/y (party x = vx+9,
y = vy+4), 3 unknown, current member count, current party, current map, total members, total groups, ..., minute, hour, serial,
then 7 characters of 0x100 bytes (name[14], ST, IQ, LK, SP, AG, DX, CH, money[3], sex, nationality, AC, maxCon, con, weapon,
skillPoints, experience[3], level, armor, lastCon, afflictions, npc flag, refusal flags, join string, willingness, rank[25], ...,
skills at 0x80 (30 pairs id/level), items at 0xBD (30 pairs id/quantity)).

## WL.EXE code map (work in progress)

* 0x10000–0x11000 low level: EGA planar blitters, title picture, mouse cursor, `setVideoMode`.
* 0x11000–0x11C00 `start`, file access (`openFile`, `mainFileRead`, ...), `allPicsRead`, Huffman tree builder/decoder (0x11B83, 0x11C28).
* 0x11C00–0x16000 game logic (party handling, combat round at 0x12D84, action interpreter, ...).
* 0x161C0–0x1C000 title/roster/character screens, text output, input, mouse, timers.

## Static recompilation of WL.EXE

The whole game logic is hand written 8086 assembly with register arguments, so instead of re-deriving every routine by
hand the engine translates it mechanically and verifies the result against the oracle:

* `engine/src/wasteland1/tools/wlimage.cpp` unpacks WL.EXE (EXEPACK) and overlays WLA.BIN at 1000:0 (the program does the same when it
  starts). `-DWL_EXE=<path to WL.EXE>` makes CMake do this; alternatively `ida_scripts/export_image.py` exports the image.
* `engine/src/wasteland1/tools/recomp/recomp.py <image> entries.txt out.cpp [--interp ...] [--check]` decodes the 8086 instructions
  from every function start in `entries.txt` (817 IDA functions plus indirect targets such as 9E24/9EAE) and emits one C++
  function `f_XXXX(Machine&, Regs&)` per entry: registers are variables, flags are booleans, `call` pushes the return
  address on the emulated stack, local jumps are `goto`. Indirect calls/jumps go through the generated `dispatch` switch;
  an indirect target that is not a function start falls back to the 8086 interpreter (`x86::Cpu`) and is reported.
  The routines that rewrite their own code (06C9, 4330, 4388, 43A8, 43D0) are never translated; they always run on the
  interpreter, which calls back into the generated functions for everything else.
* `engine/src/wasteland1/recomp_rt.cpp` is the machine around it: DOS file calls (`open/read/write/create/seek` on the game
  directory, case insensitive), BIOS keyboard and video calls, the EGA ports, the PIT counter used for speed measurement,
  and the timer interrupt (delivered at backward branches, either every N branches for deterministic tests or at 18.2 Hz
  of wall clock time for play).
* `wlrc <image> <gamedir> key/keys/wait/shot/save/diff ...` runs the recompiled game headless; `wlplay <image|WL.EXE>
  <gamedir>` is the Win32 front-end (3x window, BIOS key codes).
* Verification: `wlrctest` runs every function on both the interpreter and the generated code from the same saved states
  (random register inputs) and compares registers, flags, 1 MB of memory and the EGA planes: about 690 of 817 functions
  per state compare equal; the rest wait for keys, loop forever on random input, or call through null function pointers
  (artefacts of the random inputs, explained one by one). `src/wasteland1/tools/recomp/difftest.py` plays a key script on both and
  compares screenshots. The title screen and the map are pixel identical; only timer driven sprite animation differs.
* `--check` wraps every function with a stack balance check (`stackCheck`), which found the one emitter bug that matters
  here: a function whose entry is not its lowest instruction must start with `goto entry`.

Known gaps: PC speaker sound (ports 42h/43h/61h are ignored), mouse (int 33h reports no driver), absolute disk calls
(int 25h/26h fail, the game then uses its file based path).
