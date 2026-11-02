#ifndef ULTIMA6_FILES_PORTRAIT_H
#define ULTIMA6_FILES_PORTRAIT_H

#include "files/data_file.h"

namespace Ultima6 {

/**
 * Conversation portraits: PORTRAIT.A (98 items), PORTRAIT.B (96) and PORTRAIT.Z (12 Avatar
 * heads), offset-table archives (see LibN) whose items are LZW streams (see lzw.h) of
 * 56 x 64 bitmaps, one byte per pixel.
 *
 * Which item an actor gets is read off GAME.EXE's sub_394A9:
 *   - actor 1 (the Avatar): PORTRAIT.Z item (portrait number chosen at creation) - the
 *     objlist stores it plus one at +0x1C72, 0 meaning no character
 *   - actors >= 224 by object type: 0x175 -> number 192, 0x17E -> 193, 0x16B -> 194
 *   - the others: number = actor (the item is number - 1, but actor 0 uses 0)
 * and a number below 98 is in PORTRAIT.A, otherwise item number - 98 of PORTRAIT.B.
 * Montage.SHP holds the same twelve Avatar heads (6 female, then 6 male).
 */
struct PortraitRef {
	char file; // 'A', 'B' or 'Z'
	int item;
};

struct Portrait {
	enum { WIDTH = 56, HEIGHT = 64, SIZE = WIDTH * HEIGHT, A_ITEMS = 98 };

	/** False if the actor has no portrait (the Avatar before a character exists). */
	static bool locate(int actor, int objectType, int avatarPortraitPlus1, PortraitRef &ref);
	/** Decodes an item of a portrait archive's bytes into 56 x 64 pixels. */
	static bool decode(const Bytes &archive, int item, Bytes &pixels);
};

} // namespace Ultima6

#endif
