#!/usr/bin/env python3
"""
Floor tiles and floor maps (LEVn.MP3 / LEVn.MP1).  See docs/formats.md sections 6.5 / 6.6.

LEVn.MP3:  u32be tile_bytes, then tile_bytes/288 tiles.  Tile = 32x18 pixels, 18 rows of 16 bytes,
           each row = two 16-pixel groups of four big-endian plane words (Atari-ST interleaved,
           plane 0 = colour bit 0).  The game derives the mask as the OR of the four plane words.
           A second block follows: u32be length (40960) and 80 tiles of 32x32 (512 bytes each, same layout) =
           the isometric wall blocks (load_wall_tiles).  Tile 0 is empty.  (CAD.EXE 1000:B7B5..B87x reads 288 bytes per tile via mjm_read and
           builds the 360-byte mask+4-plane RAM tile at 36EF:000A + 360*id; used by 1000:D1FA.)
LEVn.MP1:  72-byte preface, then three sections (each starts on an even offset; palette follows, see level_palette). A section {u32be data_len, u32be alloc_len, u16be count,
           count x (u16be size, u16be offset), data}; record i = floor map of room i: w*h tile ids
           (row major, 0 = no tile), with w, h = bytes 4 and 5 of MP2 section-0 record i.
Drawing (1000:D1FA): tile (col,row) at screen (x0 + 16*(col-row), y0 + 8*(col+row)), rows top to bottom.

    python roomview.py <level dir> <room> out.png      e.g. cadaver1/DATA/1 0 room0.png
"""
import glob, os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mjm, mpsections, pcman


def load_tiles(mp3):
    n = struct.unpack('>I', mp3[:4])[0] // 288
    tiles = []
    for k in range(n):
        t = mp3[4 + 288 * k:4 + 288 * (k + 1)]
        px = bytearray(32 * 18)
        for y in range(18):
            for g in range(2):
                ws = struct.unpack('>4H', t[y * 16 + g * 8:y * 16 + g * 8 + 8])
                for b in range(16):
                    bits = [(ws[p] >> (15 - b)) & 1 for p in range(4)]
                    px[y * 32 + g * 16 + b] = sum(bits[p] << p for p in range(4)) if any(bits) else 0xFF
        tiles.append(bytes(px))
    return tiles


def load_wall_tiles(mp3):
    """Second MP3 block: u32be length (40960 = 80 tiles), then tiles of 32x32 pixels, 512 bytes each
    (32 rows of 16 bytes, same ST-interleaved layout as the floor tiles).  Returns 32*32 byte pixel arrays."""
    n1 = struct.unpack('>I', mp3[:4])[0]
    n2 = struct.unpack('>I', mp3[4 + n1:8 + n1])[0]
    blk = mp3[8 + n1:8 + n1 + n2]
    tiles = []
    for k in range(n2 // 512):
        t = blk[k * 512:(k + 1) * 512]
        px = bytearray(32 * 32)
        for y in range(32):
            for g in range(2):
                ws = struct.unpack('>4H', t[y * 16 + g * 8:y * 16 + g * 8 + 8])
                for b in range(16):
                    bits = [(ws[p] >> (15 - b)) & 1 for p in range(4)]
                    px[y * 32 + g * 16 + b] = sum(bits[p] << p for p in range(4)) if any(bits) else 0xFF
        tiles.append(bytes(px))
    return tiles


def floor_maps(mp1):
    p = 0x48
    dl, al, cnt = struct.unpack('>IIH', mp1[p:p + 10])
    dire = p + 10
    data = dire + 4 * cnt
    ents = struct.unpack('>%dH' % (2 * cnt), mp1[dire:data])
    return [mp1[data + ents[2 * i + 1]:data + ents[2 * i + 1] + ents[2 * i]] for i in range(cnt)]


def level_palette(mp1):
    """The level palette: 16 big-endian ST words right after MP1's three sections
    (each section starts on an even offset), followed by a fixed 1790-byte trailer."""
    p = 0x48
    for _ in range(3):
        dl, al, cnt = struct.unpack('>IIH', mp1[p:p + 10])
        p = (p + 10 + 4 * cnt + dl + 1) & ~1
    return list(struct.unpack('>16H', mp1[p:p + 32]))


# CAD.EXE ss:4D30 / GAME.EXE (same bytes): video offset (stride 40 bytes, x*8 pixels) of tile (0,0),
# indexed [(height-3)*8 + (width-3)]; used by 1000:D2C5.  A 10x10 room starts at pixel (144, 29).
FLOOR_ORIGIN = [3218,2898,2896,2576,2574,2254,1932,1930,2898,2898,2576,2576,2254,2254,1932,1932,2900,2578,2578,2256,2256,1934,1934,1932,2900,2900,2578,2258,2256,1936,1934,1614,2582,2260,2260,1938,1938,1616,1616,1614,2264,1942,1942,1940,1940,1618,1618,1296,1944,1944,1942,1622,1620,1620,1618,1178,1946,1944,1944,1622,1622,1300,1180,1178]


def floor_origin(w, h):
    """Screen pixel (x, y) of the top-left of tile (0,0) as the game places a w x h room (3..10)."""
    v = FLOOR_ORIGIN[(h - 3) * 8 + (w - 3)]
    return (v % 40) * 8, v // 40


def room_size(mp2_secs, room):
    r = mp2_secs[0]['records'][room]
    return (r[4], r[5]) if len(r) >= 6 else (0, 0)


def render_floor(tiles, fmap, w, h):
    """-> (width, height, pixel bytes with 0xFF = empty)"""
    W, H = 16 * (w + h) + 16, 8 * (w + h) + 18
    canvas = bytearray([0xFF]) * (W * H)
    x0 = 16 * h
    for row in range(h):
        for col in range(w):
            tid = fmap[row * w + col]
            if tid == 0 or tid >= len(tiles):
                continue
            ox, oy = x0 + 16 * (col - row), 8 * (col + row)
            t = tiles[tid]
            for y in range(18):
                for x in range(32):
                    c = t[y * 32 + x]
                    if c != 0xFF:
                        canvas[(oy + y) * W + ox + x] = c
    return W, H, bytes(canvas)


def level_files(d):
    g = lambda pat: glob.glob(os.path.join(d, pat))[0]
    return [mjm.mjm_unpack(open(g(p), 'rb').read()) for p in ('LEV*.MP1', 'LEV*.MP2', 'LEV*.MP3')]


if __name__ == '__main__':
    from PIL import Image
    mp1, mp2, mp3 = level_files(sys.argv[1])
    room = int(sys.argv[2])
    w, h = room_size(mpsections.mp2_sections(mp2), room)
    fm = floor_maps(mp1)[room]
    W, H, px = render_floor(load_tiles(mp3), fm, w, h)
    pal = pcman.palette_rgb()
    im = Image.new('RGB', (W, H), (40, 40, 60))
    for i, c in enumerate(px):
        if c != 0xFF:
            im.putpixel((i % W, i // W), pal[c])
    im.resize((W * 3, H * 3), Image.NEAREST).save(sys.argv[3])
    print('room %d: %dx%d cells -> %s' % (room, w, h, sys.argv[3]))
