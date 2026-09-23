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
 * NOT confirmed: how many total flags exist, or whether g_globalFlags is
 * itself part of a CURGAME section (0x94D1 doesn't match any of
 * savegame.h's known section offsets) or purely in-memory/session-only --
 * left as an open question. This module operates on a caller-supplied
 * byte buffer rather than assuming a fixed size, so it stays correct
 * regardless of how that's eventually resolved.
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
