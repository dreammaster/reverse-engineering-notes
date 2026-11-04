#!/usr/bin/env python3
"""
Placement of room objects ("things") on screen, as CAD.EXE object_project (1000:5A61) does it.  docs/formats.md 6.12.

A thing record (LEVn.MP2 section 3, compact layout) gives x1 (+0), y1 (+1), z0 (+2) and the sprite block index (+6);
the block's 32-byte header (as big-endian words h[0..15]) gives the box size xs=h[8], ys=h[9], zs=h[10]:
the object occupies x0..x1, y0..y1 (x0 = x1-xs+1, y0 = y1-ys+1) and z0..z0+zs-1.  Positions are in 1/8 cell units.

Screen position of the sprite's top-left pixel (verified exactly against the running game, 17 of 17 sprites of level 1 room 0):
    P = cell_px(x1>>3, y1>>3) + SUB[(y1&7)*8 + (x1&7)] - 320*N - 16*(h[2]-1) + (2*(8-(xs&7)) if xs&7 else 0)
    N = z0 - h[1] + xs + ys + zs                        # rows
    x = P % 320, y = P // 320
where cell_px(cx, cy) = origin + 2576*cx + 2544*cy (8 rows and +-16 pixels per cell; pixels counted along the 320-pixel scanline; origin from roomview.floor_origin).
"""
import os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# CAD.EXE ds:F8CE (file offset 125678), as pixels (offset*8 + shift): position of sub-cell (x&7, y&7) -> index y*8+x
SUB = [0, 322, 644, 966, 1288, 1610, 1932, 2254, 318, 640, 962, 1284, 1606, 1928, 2250, 2892, 636, 958, 1280, 1602, 1924, 2246, 2888, 3210,
       954, 1276, 1598, 1920, 2242, 2564, 3526, 3528, 1272, 1594, 1916, 2238, 2560, 3202, 3844, 3846, 1590, 1912, 2234, 2556, 3198, 3520,
       4162, 4164, 1908, 2230, 2872, 3194, 3516, 3838, 4480, 4482, 2226, 2868, 3190, 3512, 3834, 4156, 4478, 4800]


def project(x1, y1, z0, hdr, origin_px):
    """-> (screen x, screen y) of the sprite's top-left pixel.  hdr: the 16 big-endian words of the block header;
    origin_px = pixel index (y*320+x) of tile (0,0)'s top-left (roomview.floor_origin)."""
    xs, ys, zs = hdr[8], hdr[9], hdr[10]
    n = z0 - hdr[1] + xs + ys + zs
    p = origin_px + 2576 * (x1 >> 3) + 2544 * (y1 >> 3) + SUB[(y1 & 7) * 8 + (x1 & 7)]
    p -= 320 * n + 16 * (hdr[2] - 1)
    if xs & 7:
        p += 2 * (8 - (xs & 7))
    return p % 320, p // 320


def box(x1, y1, z0, hdr):
    return x1 - hdr[8] + 1, y1 - hdr[9] + 1, z0 + hdr[10] - 1
