// CGA tile animation: a port of the routines of CGA.DRV that work on the 2 bits/pixel tileset (TILES.4).
//
// The driver functions are the ones the EGA port in animate.c (written from the Tandy driver) reproduces for 4 bits/pixel tiles:
//   entry 60, carry clear  (CGA.DRV +144c)  AnimateTileset        water, lava, flags, fireplaces ...
//   entry 60, carry set    (CGA.DRV +189a)  moongate tile         builds the scratch tile that the caller then draws
//   entry 6c               (CGA.DRV +1d71)  time/status tileset   ladder lines, clock hands, colour swaps
// A CGA tile is 64 bytes (16 rows of 4 bytes, 4 pixels per byte), so every offset below is the byte offset into TILES.4 data
// (tile number * 0x40); the same tile has twice that offset in the EGA tileset.  tools/verify_cga_anim.py runs the original
// driver code in an emulator on random tilesets and compares the result byte for byte with this port.
//
// The tiles from 0x7a00 on (tile numbers 0x1e8-0x1eb, unused by the maps) are scratch space the driver fills with noise masks.

#include "common/common.h"
#include "common/memory.h"

#include "animate_cga.h"

#include <string.h>

#define TILE 0x40

// CGA.DRV:1408
static u16 s_seed = 0x7664;

// CGA.DRV:1d69 / 1d6d (the saved ladder lines)
static byte s_lineBackup1[4];
static byte s_lineBackup2[4];

void AnimateCga_SetSeed(u16 seed)
{
    s_seed = seed;
}

u16 AnimateCga_GetSeed(void)
{
    return s_seed;
}

// +144c: the noise masks: four tiles at 7a00: the seed AND aaaa, the seed AND 5555, and a copy of each
static void GenerateMasks(byte* ts)
{
    int off = 0x7a00;

    for (int i = 0; i < 0x20; i++)
    {
        u16 ax = (u16)(s_seed + 0x9248);
        ax = MEM_Ror16(ax, 3);
        ax = (u16)((ax ^ 0x9248) + 0x11);
        s_seed = ax;

        MEM_WriteU16LE(&ts[off], (u16)(ax & 0xaaaa));
        MEM_WriteU16LE(&ts[off + 0x80], (u16)(ax & 0xaaaa));
        MEM_WriteU16LE(&ts[off + 0x40], (u16)(ax & 0x5555));
        MEM_WriteU16LE(&ts[off + 0xc0], (u16)(ax & 0x5555));
        off += 2;
    }
}

// move the 16 rows of a tile down by one row; the last row becomes the first
static void ShiftDown(byte* ts, int tile)
{
    byte last[4];

    memcpy(last, &ts[tile + 0x3c], 4);
    memmove(&ts[tile + 4], &ts[tile], 0x3c);
    memcpy(&ts[tile], last, 4);
}

// dst = (dst AND NOT mask) OR (src AND mask) for one tile
static void MergeTile(byte* ts, int dst, int mask, int src)
{
    for (int i = 0; i < TILE; i++)
    {
        ts[dst + i] = (byte)((ts[dst + i] & (byte)~ts[mask + i]) | (ts[src + i] & ts[mask + i]));
    }
}

// dst = dst XOR (noise AND mask) for one tile
static void XorTile(byte* ts, int dst, int mask, int noise)
{
    for (int i = 0; i < TILE; i++)
    {
        ts[dst + i] ^= ts[noise + i] & ts[mask + i];
    }
}

static void InvertTiles(byte* ts, int off, int tiles)
{
    for (int i = 0; i < tiles * TILE; i++)
    {
        ts[off + i] = (byte)~ts[off + i];
    }
}

// swap the bytes at off + 1 and off + 9 (a column of the flag/fire pictures moves one row)
static void SwapBytes(byte* ts, int off)
{
    byte t = ts[off + 1];
    ts[off + 1] = ts[off + 9];
    ts[off + 9] = t;
}

// +1884 (one pair), +1891 (the pair one byte further) and +1894 (both)
static void SwapOne(byte* ts, int off)
{
    SwapBytes(ts, off);
}

static void SwapSecond(byte* ts, int off)
{
    SwapBytes(ts, off + 1);
}

static void SwapBoth(byte* ts, int off)
{
    SwapBytes(ts, off);
    SwapBytes(ts, off + 1);
}

// entry 60, carry clear
void AnimateCga_Tileset(byte* ts)
{
    GenerateMasks(ts);

    // water 1, 2, 3 and lava
    ShiftDown(ts, 0x0040);
    ShiftDown(ts, 0x0080);
    ShiftDown(ts, 0x00c0);
    ShiftDown(ts, 0x23c0);

    // 1557: 16 tiles at 1800 take the water (tile at 00c0) where the mask tiles at 1c00 are set
    for (int i = 0; i < 0x10; i++)
    {
        MergeTile(ts, 0x1800 + i * TILE, 0x1c00 + i * TILE, 0x00c0);
    }

    // 15c7
    MergeTile(ts, 0x4200, 0x4000, 0x7ac0);
    // 1625
    MergeTile(ts, 0x6d00, 0x4000, 0x7a40);

    // 1683: water behind bridges and the like
    for (int i = 0; i < 4; i++)
    {
        MergeTile(ts, 0x0d00 + i * TILE, 0x3400 + i * TILE, 0x00c0);
    }

    // 16f1: the masks at 3400 are inverted for the next merge and restored afterwards
    InvertTiles(ts, 0x3400, 4);
    for (int i = 0; i < 4; i++)
    {
        MergeTile(ts, 0x3900 + i * TILE, 0x3400 + i * TILE, 0x00c0);
    }
    InvertTiles(ts, 0x3400, 4);

    // 178b: sparkle on water tiles (noise masks from the generator above)
    for (int i = 0; i < 4; i++)
    {
        XorTile(ts, 0x2c00 + i * TILE, 0x3000 + i * TILE, 0x7a80);
    }
    // 17b8
    for (int i = 0; i < 4; i++)
    {
        XorTile(ts, 0x2f00 + i * TILE, 0x3300 + i * TILE, 0x7a80);
    }
    // 17e5
    XorTile(ts, 0x3780, 0x3080, 0x7ac0);

    // 180a: flags, fireplaces ... one pair of bytes per bit of the seed
    if (s_seed & 0x01) SwapBoth(ts, 0x0480);
    if (s_seed & 0x02) SwapOne(ts, 0x0508);
    if (s_seed & 0x04) SwapBoth(ts, 0x0540);
    if (s_seed & 0x08) SwapBoth(ts, 0x0f80);
    if (s_seed & 0x10) SwapSecond(ts, 0x4844);
    if (s_seed & 0x20) SwapOne(ts, 0x48c4);
    if (s_seed & 0x40) SwapSecond(ts, 0x4b44);
    if (s_seed & 0x01) SwapOne(ts, 0x4bc4);
}

// entry 60, carry set: the scratch tile 0116 (4580) becomes the floor with the top rows of the moongate at its bottom
void AnimateCga_BuildMoongateTile(byte* ts, int visibleRows, byte floorType, byte* backup)
{
    int floorOff = floorType == 0xff ? 0x1100 : 0x0140; // red brick floor / grass

    memcpy(backup, &ts[0x4580], TILE);
    memcpy(&ts[0x4580], &ts[floorOff], TILE);

    if (visibleRows != 0)
    {
        int bytes = visibleRows * 4;
        memcpy(&ts[0x4580 + TILE - bytes], &ts[0x3700], bytes); // 3700: the moongate tile
    }
}

void AnimateCga_RestoreMoongateTile(byte* ts, byte* backup)
{
    memcpy(&ts[0x4580], backup, TILE);
}

// ---------------------------------------------------------------------------------------------------------------------------
// entry 6c

// swap the two bits of every pixel (colours 1 and 2) of `tiles` tiles
static void SwapPixelBits(byte* ts, int off, int tiles)
{
    for (int i = 0; i < tiles * TILE; i += 2)
    {
        u16 v = MEM_ReadU16LE(&ts[off + i]);
        MEM_WriteU16LE(&ts[off + i], (u16)(((v & 0x5555) << 1) | ((v & 0xaaaa) >> 1)));
    }
}

// the hand of the clock: tile 3e80 is the clock (two frames of 40 bytes), the hand sets pixels at these positions
static const u16 s_clockRowOffsets[12] = {0x08, 0x08, 0x08, 0x0c, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0c, 0x08, 0x08};
static const u16 s_clockColumnOffsets[12] = {0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01};
static const byte s_clockHandMasks[12] = {0xc0, 0x30, 0x30, 0x30, 0x30, 0x30, 0xc0, 0x03, 0x03, 0x03, 0x03, 0x03};

// +1df2: the background of one clock frame
static void ClearClockHandBackground(byte* ts, int si)
{
    ts[si + 1] = (byte)((ts[si + 1] & 0xfc) | 0x02);
    ts[si + 2] = 0xa0;
    ts[si + 5] = (byte)((ts[si + 5] & 0xfc) | 0x02);
    ts[si + 6] = (byte)((ts[si + 6] & 0xcf) | 0x20);
    ts[si + 9] = (byte)((ts[si + 9] & 0xfc) | 0x02);
    ts[si + 10] = 0xa0;
}

// +1e7e
static void DrawClockHand(byte* ts, int index)
{
    if (index >= 12)
        return; // the driver would read past its tables here (hour >= 24 or minute >= 60); cannot happen in the game

    int si = 0x3e80 + s_clockRowOffsets[index] + s_clockColumnOffsets[index];

    ts[si] |= s_clockHandMasks[index];
    ts[si + TILE] |= s_clockHandMasks[index];
}

// the tiles whose colours 1 and 2 are swapped by mode 4 (the ending), +1f22
static const u16 s_swapList4[] = {0x1100, 0x1700, 0x1740, 0x2400, 0x2480, 0x2500, 0x2580, 0x26c0, 0x2ac0, 0x2b00, 0x2bc0,
                                  0x2c00, 0x2c40, 0x2fc0, 0x3700, 0x4200, 0x4380, 0x4680, 0x4e00, 0x4e40, 0x4e80, 0x4ec0};

// mode: ax; hour: bl; minute: bh
void AnimateCga_TimeTileset(byte* ts, int mode, byte hour, byte minute)
{
    if (mode == 0)
    {
        // the ladder tiles show a line of the top row at row 14 while the party is in a town (backup, then copy row 0 over row 14)
        memcpy(s_lineBackup1, &ts[0x3238], 4);
        memcpy(&ts[0x3238], &ts[0x3200], 4);
        memcpy(s_lineBackup2, &ts[0x45f8], 4);
        memcpy(&ts[0x45f8], &ts[0x45c0], 4);
    }
    else if (mode == 1)
    {
        memcpy(&ts[0x3238], s_lineBackup1, 4);
        memcpy(&ts[0x45f8], s_lineBackup2, 4);
    }
    else if (mode == 2)
    {
        ClearClockHandBackground(ts, 0x3e88);
        ClearClockHandBackground(ts, 0x3ec8);

        if (hour != 0 && hour >= 12)
            hour -= 12;
        DrawClockHand(ts, hour);

        // the minute hand: minute / 5, by repeated subtraction as the driver does
        {
            byte m = minute;
            int step = 0;
            while (m >= 5)
            {
                step++;
                m -= 5;
            }
            DrawClockHand(ts, step);
        }
    }
    else if (mode == 4)
    {
        for (int i = 0; i < (int)(sizeof(s_swapList4) / sizeof(s_swapList4[0])); i++)
        {
            SwapPixelBits(ts, s_swapList4[i], 1);
        }
    }
    else
    {
        // modes 3 and above 4 (+1ea2)
        SwapPixelBits(ts, 0x0140, 1);
        SwapPixelBits(ts, 0x0780, 1);
        SwapPixelBits(ts, 0x07c0, 1);
        SwapPixelBits(ts, 0x1300, 1);
        SwapPixelBits(ts, 0x3280, 1);
        SwapPixelBits(ts, 0x0800, 7);
        SwapPixelBits(ts, 0x0c00, 8);
        SwapPixelBits(ts, 0x1800, 0x10);
    }
}
