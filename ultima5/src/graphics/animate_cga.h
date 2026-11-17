#ifndef _GRAPHICS_ANIMATE_CGA_H
#define _GRAPHICS_ANIMATE_CGA_H

#include "common/common.h"

// CGA (2 bits/pixel, 64 bytes/tile) counterparts of animate.h; see animate_cga.c.  `ts` is the decompressed TILES.4 data (0x8000 bytes).
void AnimateCga_Tileset(byte* ts);                                  // entry 60, carry clear: water, lava, flags, fireplaces
void AnimateCga_TimeTileset(byte* ts, int mode, byte hour, byte minute); // entry 6c: 0/1 ladder lines, 2 clock hands, 3/4 colour swaps
void AnimateCga_BuildMoongateTile(byte* ts, int visibleRows, byte floorType, byte* backup); // entry 60, carry set; backup: 0x40 bytes
void AnimateCga_RestoreMoongateTile(byte* ts, byte* backup);

// test access to the driver's pseudo-random seed (CGA.DRV:1408, starts at 0x7664)
void AnimateCga_SetSeed(u16 seed);
u16 AnimateCga_GetSeed(void);

#endif
