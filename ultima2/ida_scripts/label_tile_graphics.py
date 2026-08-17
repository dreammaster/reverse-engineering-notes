"""
IDA Pro script: resolve & label the 64 embedded overworld tile graphics.

Supersedes the file-offset-translation approach (ida_loader.get_fileregion_ea)
from the first draft of this script. Paul pointed out the IDB already has a
fully-resolved TILE_OFFSETS table (asm ~19157, xref'd from
draw_map_content and animate_water) with one word-sized pointer per
tile, straight to each tile's data -- much better ground truth than
guessing a fixed stride from the wiki's cited file offset 0x7C43.
get_fileregion_ea(0x7C43) landed 3 bytes into tile 0's *pixel* data (not
its start) and the guessed 64-byte stride drifted from the real one
tile-to-tile, which is why that version's visualization looked like
noise past tile 0.

Confirmed by hand from TILE_OFFSETS' targets in the .asm export
(counted byte_17A40's declared bytes, spot-checked headers on the
first, middle, and last entries):
  - Consecutive tile records are exactly 0x42 (66) bytes apart, for all
    64 tiles (64 * 66 = 0x1080 bytes total, contiguous from byte_17A40).
  - Every tile record starts with the same 2-byte header: 04h, 10h --
    read as (bytes-per-row=4, row-count=16), matching a 16x16px CGA
    Linear 2bpp tile (16px * 2 bits / 8 = 4 bytes/row * 16 rows = 64
    bytes) -- i.e. the original 64-bytes/tile *pixel* inference was
    right, it was just missing this 2-byte header before each tile.
  - So per record: byte 0 = 4 (row width in bytes), byte 1 = 0x10 (row
    count), bytes 2-65 = the 64 bytes of packed pixel data.

Second bug, found by actually running this script's first version:
`idautils.DataRefsFrom` only resolved tile 0's address, not tiles
1-63, even though the .asm text shows "offset labelname" for all 64
sub-entries of the `dw offset a,b,c,...` declaration. IDA apparently
only records a stored xref at the *first* sub-word of a multi-value
comma-separated data declaration, not each individual one, despite
rendering offset-name text for all of them. Fixed by reading the raw
word value and adding the segment base instead (same technique
resolve_command_jump_table.py used, for the same underlying reason) --
see get_tile_ea() below.

This script re-derives each tile's address from TILE_OFFSETS itself
(not the arithmetic above, so it stays correct if that assumption is
ever wrong for some tile), verifies the 66-byte-stride/[04h,10h]-header
pattern holds for *all* 64 (not just the ones spot-checked by hand),
re-visualizes a spread of tiles with the corrected header offset for a
final eyeball check, and -- non-dry-run -- comments each tile's header
byte and renames the still-auto-named ones (`byte_17A40`, `unk_18280`,
etc.) to `tile_NN`.
"""

import idc
import ida_bytes
import ida_segment
import idaapi

DRY_RUN = False

TABLE_NAME = "TILE_OFFSETS"
NUM_TILES = 64
HEADER_SIZE = 2
PIXEL_BYTES = 64
RECORD_SIZE = HEADER_SIZE + PIXEL_BYTES  # 66, expected uniform stride
EXPECTED_HEADER = (4, 0x10)

# Explicit picks rather than a computed spread: 0 (water) and 1 (swamp)
# already visually confirmed against the wiki's tile list; 6 (town) is
# next -- should show clearly building/wall-like structure if the
# header/stride decode is right, per
# https://moddingwiki.shikadi.net/wiki/Ultima_II_Map_Format's tile IDs.
PREVIEW_INDICES = [0, 1, 6, 20, 40, 63]

_SHADES = " .o#"  # 2-bit pixel value 0..3 -> light..dark, for eyeballing shapes


def unpack_row(byte_val):
    """4 pixels/byte, MSB-first, 2 bits/pixel -- standard CGA packing."""
    return "".join(_SHADES[(byte_val >> shift) & 0x3] for shift in (6, 4, 2, 0))


def get_tile_ea(table_ea, index, seg_start):
    """Read the raw word and add the segment base, rather than relying on
    a stored xref -- for a `dw offset a,b,c,...` declaration IDA only
    records a dref at the array's first sub-word (confirmed: index 0
    resolved via DataRefsFrom, indices 1-63 didn't, even though the .asm
    text shows "offset labelname" for all of them). Same technique
    resolve_command_jump_table.py used for the same reason."""
    entry_ea = table_ea + index * 2
    word_val = ida_bytes.get_word(entry_ea)
    return seg_start + word_val


def main():
    table_ea = idc.get_name_ea_simple(TABLE_NAME)
    if table_ea == idaapi.BADADDR:
        print(f"[!] {TABLE_NAME} not found by name -- has it been "
              f"renamed since this script was written?")
        return

    seg = ida_segment.getseg(table_ea)
    if seg is None:
        print(f"[!] no segment found containing {table_ea:X}")
        return

    tiles = [(i, get_tile_ea(table_ea, i, seg.start_ea)) for i in range(NUM_TILES)]

    print(f"{TABLE_NAME} @ {table_ea:X}, resolved {len(tiles)}/{NUM_TILES} "
          f"tile addresses\n")

    # Verify uniform stride + header across the *whole* table, not just
    # the hand-checked spot samples from the docstring.
    problems = []
    for (i, ea), (_, ea2) in zip(tiles, tiles[1:]):
        if ea2 - ea != RECORD_SIZE:
            problems.append(f"tile {i} @ {ea:X}: stride to next tile is "
                             f"0x{ea2 - ea:X}, expected 0x{RECORD_SIZE:X}")
    for i, ea in tiles:
        header = (ida_bytes.get_byte(ea), ida_bytes.get_byte(ea + 1))
        if header != EXPECTED_HEADER:
            problems.append(f"tile {i} @ {ea:X}: header is "
                             f"{header}, expected {EXPECTED_HEADER}")
    if problems:
        print(f"[!] {len(problems)} tile(s) don't match the expected "
              f"pattern -- inspect before applying:")
        for p in problems:
            print(f"    {p}")
        print()
    else:
        print(f"all {len(tiles)} tiles match: 0x{RECORD_SIZE:X}-byte "
              f"stride, {EXPECTED_HEADER} header\n")

    by_index = dict(tiles)
    preview_indices = [i for i in PREVIEW_INDICES if i < len(tiles)]

    print(f"visualization of tiles {preview_indices} (pixel bytes only, "
          f"2-byte header stripped):\n")
    for i in preview_indices:
        ea = by_index[i]
        pixel_ea = ea + HEADER_SIZE
        print(f"  -- tile {i:2} @ {ea:X} (pixels @ {pixel_ea:X}) --")
        for row in range(PIXEL_BYTES // 4):
            row_ea = pixel_ea + row * 4
            row_bytes = [ida_bytes.get_byte(row_ea + b) for b in range(4)]
            hex_str = " ".join(f"{b:02X}" for b in row_bytes)
            art_str = "".join(unpack_row(b) for b in row_bytes)
            print(f"    {hex_str}   {art_str}")
        print()

    if DRY_RUN:
        print("[dry] nothing changed. If the visualization above looks "
              "like real tile shapes now (and no [!] problems were "
              "printed above), set DRY_RUN = False to add header "
              "comments and rename tile_00..tile_63.")
        return

    for i, ea in tiles:
        new_name = f"tile_{i:02}"
        if idc.get_name(ea) != new_name:
            idc.set_name(ea, new_name, idc.SN_NOWARN)
        idc.set_cmt(ea, f"overworld tile {i}: byte0=row width in bytes, "
                         f"byte1=row count, then {PIXEL_BYTES} bytes CGA "
                         f"Linear 2bpp pixel data (16x16px)", 0)

    print(f"\nDone. Renamed {len(tiles)} tiles to tile_00..tile_"
          f"{len(tiles) - 1:02}. Re-export the .asm and update "
          f"docs/file-formats.md with the confirmed layout.")


if __name__ == "__main__":
    main()
