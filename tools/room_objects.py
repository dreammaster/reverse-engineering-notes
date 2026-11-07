#!/usr/bin/env python3
"""Render a room as the game shows it (floor + walls + object sprites at their projected positions).
    python room_objects.py <level dir> <room> out.png
Objects are drawn in thing-list order with sprite frame 0; the true draw order and frame selection are not decoded."""
import glob, os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mjm, mpsections, roomview, objsprites, objplace, things, pcman


def draw_objects(canvas, secs, blocks, room, gx, gy):
    """Paint the objects of `room` into `canvas` (bytearray 320x200, colour 0 = transparent in sprites).
    -> [(thing id, block, (x, y, w, h) or None)]"""
    placed = []
    lists, thl = secs[2]['records'], secs[3]['records']
    rl = lists[room] if room < len(lists) and secs[2]['present'][room] else b''
    for k in range(len(rl) // 2):
        t = (rl[2 * k] << 8) | rl[2 * k + 1]
        r = thl[t] if t < len(thl) and secs[3]['present'][t] else b''
        if len(r) < 8 or r[3] & 0x80:      # flag 0x80 at +3: not in the room (inside a container / taken)
            continue
        b = struct.unpack('>H', r[6:8])[0]
        p = objsprites.parse_sprite_block(blocks[b][1]) if b < len(blocks) else None
        if not p:
            placed.append((t, b, None))
            continue
        hdr = struct.unpack('>16H', p[0])
        x, y = objplace.project(r[0], r[1], r[2], hdr, gy * 320 + gx)
        k = objsprites.initial_frame(p[2], len(p[1]))
        fw, fh, px = p[1][k]
        if p[2]:
            x, y = x + objsprites.frame_offsets(p[2])[k][0], y + objsprites.frame_offsets(p[2])[k][1]
        for yy in range(fh):
            for xx in range(fw):
                c = px[yy * fw + xx]
                if c and 0 <= y + yy < roomview.VIEW_H and 0 <= x + xx < 320:
                    canvas[(y + yy) * 320 + x + xx] = c
        placed.append((t, b, (x, y, fw, fh)))
    return placed


def door_sprites(mp1):
    """{block: (w, h, pixels with 0xFF = transparent)} for the door sprites (blocks 0..7, first frame)."""
    out = {}
    for i, (sz, blk) in enumerate(objsprites.blocks(mp1)[:8]):
        p = objsprites.parse_sprite_block(blk) if sz > 1 else None
        if p:
            fw, fh, px = p[1][0]
            out[i] = (fw, fh, bytes(c if c else 0xFF for c in px))
    return out


def render_room(d, room):
    mp1, mp2, mp3 = roomview.level_files(d)
    secs = mpsections.mp2_sections(mp2)
    w, h = roomview.room_size(secs, room)
    tiles, wtiles = roomview.load_tiles(mp3), roomview.load_wall_tiles(mp3)
    shell = bytearray(roomview.render_shell(tiles, wtiles, roomview.floor_maps(mp1)[room], roomview.wall_columns(mp1)[room], w, h, door_sprites(mp1)))
    gx, gy = roomview.floor_origin(w, h)
    placed = draw_objects(shell, secs, objsprites.blocks(mp1), room, gx, gy)
    return bytes(shell), placed, level_pal(mp1)


def level_pal(mp1):
    return [(((w >> 8) & 7) * 255 // 7, ((w >> 4) & 7) * 255 // 7, (w & 7) * 255 // 7) for w in roomview.level_palette(mp1)]


if __name__ == '__main__':
    from PIL import Image
    px, placed, pal = render_room(sys.argv[1], int(sys.argv[2]))
    im = Image.new("RGB", (320, roomview.VIEW_H), (0, 0, 0))
    for i, c in enumerate(px):
        if c != 0xFF:
            im.putpixel((i % 320, i // 320), pal[c])
    im.resize((640, 2 * roomview.VIEW_H), Image.NEAREST).save(sys.argv[3])
    print(len(placed), 'objects,', sum(1 for p in placed if p[2] is None), 'without a parsable sprite block')
