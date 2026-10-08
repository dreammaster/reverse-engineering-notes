#ifndef YENDOR23_GLOBALFLAGS_H
#define YENDOR23_GLOBALFLAGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * The quest/world-state flag bitfield (g_globalFlags, base 0x94D1),
 * accessed only through GetGlobalFlagBitAndWord/SetGlobalFlag/
 * ClearGlobalFlag/TestGlobalFlag (yendor2.asm:42355 on) rather than
 * direct bit-twiddling -- see file-formats.md's "Global quest/world-state
 * flags" section for the wider picture (per-record flag bank families,
 * ApplyItemEffectFlags, etc., not reimplemented here).
 *
 * The index scheme is 1-based and MSB-first, confirmed by tracing
 * GetGlobalFlagBitAndWord's exact division/shift sequence: index 1 is
 * bit 0x8000 of word 0, index 16 is bit 0x0001 of word 0 (not word 1 --
 * the division's zero-remainder case steps back one word), index 17 is
 * bit 0x8000 of word 1, and so on. monster.c's MonsterFieldFlagOnDeath/
 * FlagOnDeath2 (a signed index: positive sets, negative clears, zero is
 * a no-op) and the real death-flag table's indices (e.g. type id 15 ->
 * index 14 in Chapter 2) are exactly this index space.
 *
 * Where it is saved (found 2026-10-08): in the header record of CURGAME / SAVGAME, savegame.h's SaveHeaderGlobalFlags (+0xD2, SaveHeaderGlobalFlagsSize = 96 bytes: DS:0x94D1 in
 * Chapter 2 and 0xCFAF in Chapter 3 are both exactly +0xD2 from the block that holds the party position at +0x98), saveGlobalFlags(). This module still operates on a
 * caller-supplied buffer.
 */

/* buffer must be at least ((index + 15) / 16) * 2 bytes; false (no-op) if it's too small for size. */
bool globalFlagTest(const uint8_t *buffer, size_t size, unsigned index);
void globalFlagSet(uint8_t *buffer, size_t size, unsigned index);
void globalFlagClear(uint8_t *buffer, size_t size, unsigned index);

/*
 * GrantMonsterRewards'/ApplyItemEffectFlags' shared convention: a signed
 * flag index where positive sets, negative clears, and zero does nothing.
 * The magnitude is the 1-based index above.
 */
void globalFlagApplySigned(uint8_t *buffer, size_t size, int16_t signedIndex);

#endif
