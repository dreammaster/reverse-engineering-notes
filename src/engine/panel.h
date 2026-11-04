/*
 * panel.h - the frame around the room view: ANDATA.DAT block 1 (docs/formats.md 6.3, tools/panel.py).
 * Verified: a composite of floor + walls + objects + this frame equals 98.2% of the original's 320x200 screen (level 1 room 0;
 * the rest is the player, one animated object and the status text).
 */
#ifndef CADAVER_PANEL_H
#define CADAVER_PANEL_H

#include <stddef.h>
#include <stdint.h>

/* ANDATA.DAT (unpacked): u16be count, count x u16be sizes, blocks back to back. Returns 0 on success. */
int andata_block(const uint8_t *data, size_t len, int index, const uint8_t **block, size_t *blen);

/* Draw the frame wings over a 320x200 canvas of colour indices (TILE_TRANSPARENT = 0xFF counts as 0). `b1` = block 1.
 * Returns 0 on success, -1 if the block is too short. */
int panel_apply_wings(const uint8_t *b1, size_t len, uint8_t *canvas);

#endif
