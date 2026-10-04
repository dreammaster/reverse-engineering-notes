#ifndef YENDOR23_CLUEBOOK_H
#define YENDOR23_CLUEBOOK_H

#include <stdint.h>

#include "game.h"
#include "viewrender.h"

/*
 * The on-line clue book's navigation bar: DrawClueBookNavBar (yendor2.asm:7728, yendor3.asm:15479). `flags` is g_clueBookNavFlags:
 * bit 0x40 shows the hint "d LIST" at (11, 185), bit 0x20 the hint "MAP c" at (272, 185) (font 0, colour 0xF, transparent), and exactly
 * one of 0x8000, 0x4000, 0x2000, 0x1000, 0x800, 0x400, 0x200 marks the current category tab. The seven tabs are 16 x 16 icons
 * (category 8) at x = 62 + 30k, y = 180; a tab's picture is its base id, +1 when it is the selected one. Base ids: Chapter 2
 * 0x20, 0x145, 0x147, 0x153, 0x149, 0x14B, 0x14D; Chapter 3 0x1E, 0x20, 0x22, 0x24, 0x26, 0x28, 0x2A.
 */
enum { ClueTabCount = 7, ClueNavHintList = 0x40, ClueNavHintMap = 0x20, ClueTabFirstBit = 0x8000 };

void clueNavBarDraw(const ViewRenderer *r, uint16_t flags);

/* The picture id of tab `tab` (0-6) for the flags. */
unsigned clueTabPicture(GameKind game, unsigned tab, uint16_t flags);

#endif
