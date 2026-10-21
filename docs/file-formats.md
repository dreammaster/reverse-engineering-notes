# Fountain of Dreams — data file formats

Everything below was derived from `KEH.EXE`/`FOD.EXE` and **verified by decoding
the real files** with the prototype readers in `tools/` (`fodlib.py`,
`rendermap.py` → renders a map to PNG using the TILES file; the output matches a
real FoD town map). All integers are little-endian.

## Compression: LZSS

`u32 outputSize`, then groups of 8 items prefixed by a flag byte (bit0 first):
bit=1 → literal byte; bit=0 → 2 bytes `lo, hi`: offset = `lo | (hi & 0xF0) << 4`
into a 4096-byte ring buffer, length = `(hi & 0x0F) + 3`. Ring is pre-filled with
`0x20`, write pointer starts at `0xFEE`. (`KEH lzssDecode` = `tools/fodlib.py:lzss`.)

## Archive files `KMAP KSCR KANI FMAP FSCR FANI DMAP DSCR DANI`

Start with a table of `u16 offset, u16 size` (28 entries for MAP/SCR, 53 for ANI).
`offset` is in **16-byte paragraphs** from the start of the file; `size` is in
bytes. Chunk index = map number - 1 (maps are numbered 1-28). The table at KEH dseg+125h, indexed by
map number, gives the file set: `0,2,2,2,2,3,3,3,3,3,3,3,2,3,2,3,3,3,1,1,1,1,1,1,1,1,1,1,2`
- 1 = `K*` files, 2 = `F*`, 3 = `D*` (verified: FMAP holds chunks 0-3, 11, 13, 27; DMAP 4-10, 12, 14-16;
KMAP 17-26).

* `*MAP` chunk: raw map (not compressed) — see below.
* `*SCR` chunk: LZSS-compressed script bank — see below.
* `*ANI` chunk: animation scripts for the map's tiles (used by `animationTick`).
  Format not decoded yet.

### Map chunk

```
+000 u16 width            +002 u16 height
+004 u16 flags            (bit15: reload pristine when re-entered)
+006 u8  minutes per step +007 u8 random encounter %
+008 u8  random-group templates   +009 u8 monster count
+00A u8  ?                +00B u8 group count
+00D u8  string index for "walked off the map"
+01E u16 stored-member block offset (0 = none)  (members stored with `retrieveStoreMember`)
+404 u16 shops offset    +406 u16 monsters offset  +408 u16 groups offset(*)
+40A u16 groups offset   +40C u16 ground items offset
+40E u16 ground item count  +410 u16 count (limit)  +412 u16 capacity (256)
+414 cells[width*height], 8 bytes each, row-major
then: shops, monsters[count] (0x68 bytes), groups[count] (0x66), ground items[256] (6 bytes)
```
(*) +408 equals the groups offset when there are no extra records; verified:
`monsters offset + monsterCount*0x68 == groups offset`, `groups offset +
groupCount*0x66 == ground items offset`, `ground items offset + 256*6 == chunk
size`.

Cell: `u16 tile` (low 11 bits tile number, high 5 bits = overlay id), `u8 flags`
(bit0 passable, bit6 cursor, bit7 item/seen), `u8`, `u16`, `u16 script index`
(0FFFFh = none). Record layouts for monsters, groups, ground items, shops:
`docs/keh-engine.md`.

### Script bank (`*SCR`, after LZSS)

`u16 offsets[1500]` (byte offsets from the start of the decoded bank; unused
entries point to an empty block), followed by the script blocks. A map cell's
script index selects `offsets[index]`.

Block: `u16 eventMask`, `u16 stringAreaOffset` (relative to the block), then
records until the string area; strings are NUL-terminated and referenced by
index. Record = `u8 head` (low nibble = event class, high nibble = parameter
signature), `u8 opcode`, parameters:

| sig | params |
|---|---|
| 0 | none | 1 | B | 2 | BB | 3 | W | 4 | BBB | 5 | B W | 6 | BBBB | 7 | BBW | 8 | WW | 9 | BBBW | 10 | BBBWW |

(B = byte, W = word; the opcode's byte length equals the table in
`docs/keh-engine.md`.) Verified on real data, e.g. block `10 1F 00 10 17 00 …` =
`playSound 0; printString 0`, `43 22 00 09 00` = skill check (attribute 0,
difficulty 9), `53 14 04 20 00` = IF (cond 4, else-jump 0x20).

## Other files

| File | Format |
|---|---|
| `TILES` | LZSS → 128000 bytes = 1000 tiles × 128 (16×16, 4bpp, 8 bytes/row, high nibble = left pixel) |
| `FONT` | `u16 count` (209) then count × 32 bytes (8×8, 4bpp) |
| `BORDERS` | 6000 bytes: 5000 = 40×25 border cell map (4 variants × 1000 at +0/+1000/+2000/+3000/+4000) + 1000 bytes of menu strings/positions (read by FOD at offset 5000) |
| `GLOBALS` | header 0x2A bytes: `u8 scaleA(1) u8 scaleB(2) u16 rangeBands(10) u16 (1) u16 (10) "feet"`; then items × 0x18: `char name[20]`, `u8 flags` (bit0 weapon, bit1 armor, bits2-3 ingest effect, 1Ch = unique), `u8 weaponDef`, `u8`, `u8 uniqueBit`. 118 items. |
| `WEAPONS` | `u16 count (24)`, count × 0x3C weapon defs, then fire-mode records (0x12 each) referenced by `modesOfs` |
| `ARCHTYPE` | `u16 count, u16 ?, u16 professionsOfs, u16 enemiesOfs, …` professions × 0x80 then enemy defs × 0x40 |
| `HDSPCT` | `u16 attrCount(7) u16 activeCount(16) u16 passiveCount(16)` then 22-byte names, skill thresholds at +13Ah, status table at +3ECh |
| `PACKETS` | 24 × `u16 count`, 24 × `u16 firstEntry`, then 6-byte entries `{item u8, pct u8, min u16, max u16}` |
| `SERVICES` | services/shops records (name + stat modifiers), ~32-byte entries |
| `DISK1..4` | `Disk1` save/state struct, 0xEC0 = 3776 bytes (`docs/fod-menu.md`, `docs/keh-engine.md`) |
| `?PICT` | picture sets (`DPICT KPICT EPICT FPICT TPICT`): first dword = size, LZSS, then picture records |
| `HDSPCT`, `GANI`, … | see above / not yet decoded |

Palette (VGA, 6-bit): `0,0,0 / 0,0,27h / 3,25h,3 / 0,1Bh,1Bh / 20h,0,0 / 1Bh,0,1Bh /
20h,1Bh,5 / 26h,26h,26h / 10h,10h,10h / 0,0,38h / 0,3Fh,0 / 0,3Fh,3Fh / 3Ah,20h,10h /
3Fh,0,3Fh / 3Fh,3Fh,0 / 3Fh,3Fh,3Fh` (FOD `MASTER_PALETTE`). CGA/Tandy/EGA modes
map the same 4-bit indices.
