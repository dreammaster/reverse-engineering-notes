#!/usr/bin/env python3
"""
Object sprites in LEVn.MP1 section 2.  See docs/formats.md section 6.5.

Section 2 = u32be data_len, u32be alloc_len (255000), u16be count (255), count x (u16be size, u16be offset), data.
Record i is stored in floor(size/2) bytes (the declared `size` is the size in RAM; sum of floor(size/2) == data_len); records follow each other
(size 0/1 entries are empty).  A *sprite block* is

    32-byte header (not yet decoded)
    frames, back to back:  u16be w (16-pixel groups, 1..4), u16be h (rows),  h*w*8 bytes of
                           Atari-ST interleaved pixels (per row: for each 16-pixel group four big-endian plane words,
                           plane 0 = colour bit 0); colour 0 = transparent (the game derives the mask as the OR of the planes)

Most blocks parse this way and consume their stored size exactly (96 of 124 in level 1); the others
(headers/tables, e.g. the 29 424-byte block 8) are not sprites.   python objsprites.py <MP1> out.png
"""
import os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mjm, roomview


def blocks(mp1):
    p = 0x48
    for _ in range(2):
        dl, al, cnt = struct.unpack('>IIH', mp1[p:p + 10])
        p = (p + 10 + 4 * cnt + dl + 1) & ~1
    dl, al, cnt = struct.unpack('>IIH', mp1[p:p + 10])
    dire = p + 10
    data = dire + 4 * cnt
    ents = struct.unpack('>%dH' % (2 * cnt), mp1[dire:data])
    out, pos = [], 0
    for i in range(cnt):
        sz = ents[2 * i]
        stored = sz // 2
        out.append((sz, mp1[data + pos:data + pos + stored]))
        pos += stored
    return out


def _frames(blk, p):
    """Frames back to back from offset p to the end of the block, or None."""
    frames = []
    while p + 4 <= len(blk):
        w, h = struct.unpack('>HH', blk[p:p + 4])
        if not (1 <= w <= 8 and 1 <= h <= 200) or p + 4 + h * w * 8 > len(blk):
            break
        data = blk[p + 4:p + 4 + h * w * 8]
        px = bytearray(w * 16 * h)
        for y in range(h):
            for g in range(w):
                ws = struct.unpack('>4H', data[y * w * 8 + g * 8:y * w * 8 + g * 8 + 8])
                for b in range(16):
                    px[y * w * 16 + g * 16 + b] = sum(((ws[pl] >> (15 - b)) & 1) << pl for pl in range(4))
        frames.append((w * 16, h, bytes(px)))
        p += 4 + h * w * 8
    return frames if frames and p == len(blk) else None


def parse_sprite_block(blk):
    """-> (header32, [(w_px, h, pixels)], extra) or None.  pixels: w_px*h colour indices, 0 = transparent.
    Plain blocks: header, frames.  Animated blocks: header, then `extra` = [u8 nframes, u8 0, u16 script_bytes, u16 0, u16 h0,
    script (script_bytes bytes), u16 table_bytes = 8*nframes, table (8-byte entries)], then the nframes frames
    (docs/formats.md 6.12).  `extra` is empty for plain blocks."""
    if len(blk) < 36:
        return None
    nf = blk[32]
    if 1 <= nf <= 32 and blk[33] == 0 and len(blk) >= 42:
        sl = struct.unpack('>H', blk[34:36])[0]
        p = 40 + sl
        if p + 2 <= len(blk) and struct.unpack('>H', blk[p:p + 2])[0] == 8 * nf:
            fr = _frames(blk, p + 8 * nf)
            if not (fr and len(fr) == nf):
                fr = _frames(blk[:-4], p + 8 * nf)           # creature sheets carry 4 extra bytes after the frames
            if fr and len(fr) == nf:
                return blk[:32], fr, blk[32:p + 8 * nf]
    fr = _frames(blk, 32)
    return (blk[:32], fr, b'') if fr else None


def frame_offsets(extra):
    """Per-frame (dx, dy) offsets (signed pixels, relative to the projected sprite position) of an animated block."""
    if not extra:
        return None
    nf, sl = extra[0], struct.unpack('>H', extra[2:4])[0]
    return [struct.unpack('>2h', extra[8 + sl + 8 * k + 2:8 + sl + 8 * k + 6]) for k in range(nf)]


def script(extra):
    """The animation script words (frame numbers; 0xFC00-0xFDFF look like commands, 0xFD00 ends the loop)."""
    if not extra:
        return []
    sl = struct.unpack('>H', extra[2:4])[0]
    return list(struct.unpack('>%dH' % (sl // 2), extra[8:8 + sl]))


def initial_frame(extra, nframes):
    for w in script(extra):
        if w < nframes:
            return w
    return 0


if __name__ == '__main__':
    from PIL import Image
    mp1 = mjm.mjm_unpack(open(sys.argv[1], 'rb').read())
    pal = [(((w >> 8) & 7) * 255 // 7, ((w >> 4) & 7) * 255 // 7, (w & 7) * 255 // 7) for w in roomview.level_palette(mp1)]
    sprites = []
    for i, (sz, blk) in enumerate(blocks(mp1)):
        r = parse_sprite_block(blk)
        if r:
            sprites.append((i, r[1]))
    cell_w, cell_h, per_row = 40, 50, 20
    nframes = sum(len(f) for _, f in sprites)
    sheet = Image.new('RGB', (per_row * cell_w, ((nframes + per_row - 1) // per_row) * cell_h), (60, 60, 90))
    n = 0
    for i, frames in sprites:
        for (w, h, px) in frames:
            ox, oy = (n % per_row) * cell_w + 2, (n // per_row) * cell_h + 2
            for y in range(min(h, cell_h - 2)):
                for x in range(min(w, cell_w - 2)):
                    c = px[y * w + x]
                    if c:
                        sheet.putpixel((ox + x, oy + y), pal[c])
            n += 1
    sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST).save(sys.argv[2])
    print('%d sprite blocks, %d frames -> %s' % (len(sprites), nframes, sys.argv[2]))
