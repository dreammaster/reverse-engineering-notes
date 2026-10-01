#ifndef YENDOR23_LIGHTSOURCE_H
#define YENDOR23_LIGHTSOURCE_H

#include <stdbool.h>
#include <stdint.h>

/*
 * The party's light-source burn-down mechanic. `ApplyStatusEffect`
 * (yendor2.asm:29214, a `HandleGameCommand` top-level handler -- lighting
 * a candle/torch/light-source) and `TickStatusEffects` (yendor2.asm:29153,
 * called once per 5-minute world tick from `TickWorldAilments`) manage 3
 * independent light sources. Confirmed against real `WORLD.DAT` item data
 * in both games (the item ids these two functions dispatch on are exactly
 * an unlit/lit pair for 3 real items):
 *
 *   CANDLE (item 8)       -> LIT CANDLE (item 9)
 *   LIGHT SOURCE (item 0xB) -> LIT LIGHT (item 0xC)
 *   TORCH (item 0xE)      -> LIT TORCH (item 0xF)
 *
 * (A third item id per family -- 10/0xD/0x10, USED CANDLE/LIGHT/TORCH --
 * also exists in the real catalog, but neither of these two functions
 * writes it; that transition belongs to a separate, not-yet-reimplemented
 * mechanic that ticks the item slot itself, see file-formats.md's "world
 * ailments" section. lightSourceApply/Tick only manage the 3 standalone
 * duration counters and their own "currently lit" flag bits -- the exact
 * scope of the original two functions, confirmed by direct read.)
 *
 * Lighting one (`g_currentActionId` == 8/0xE/0xB in the original) arms
 * its own duration counter and sets its own "lit" flag bit.
 * `TickStatusEffects` (`g_currentActionId` == 9/0xF/0xC) decrements that
 * same counter by the elapsed time; at or below 0, it clears the flag bit
 * and zeroes the counter (the light has gone out). Both functions also
 * increment/decrement an icon-bar occupancy count
 * (`ResolveIconBarBaseAddress`) and play a sound -- pure UI/audio side
 * effects, not modeled here.
 */

enum { LightSourceCount = 3 };

/*
 * Index into LightSourceState's own arrays -- not the real item ids, just
 * a dense slot index matching the original's own field order
 * (word_36C85/36C89/36C8B).
 */
typedef enum { LightSourceCandle = 0, LightSourceTorch = 1, LightSourceGeneric = 2 } LightSourceKind;

typedef struct {
    uint16_t duration[LightSourceCount]; /* word_36C85/36C89/36C8B, in that order (candle/torch/generic) */
    uint16_t litFlags;                   /* word_36C79 bits 0x2000/0x800/0x400 (candle/torch/generic) */
} LightSourceState;

/*
 * ApplyStatusEffect's own real item-id dispatch. actionId must be the
 * "light it" id (8 candle, 0xE torch, 0xB generic) or this is a no-op
 * (matching the original's own `else` branch, which only defensively
 * zeroes 3 unrelated globals -- see file-formats.md) and returns false.
 * On a match: sets the matching litFlags bit and increments the matching
 * duration counter (the original increments rather than sets outright,
 * so lighting an already-lit source extends it rather than resetting it
 * -- reproduced exactly).
 */
bool lightSourceApply(LightSourceState *state, unsigned actionId);

/*
 * TickStatusEffects' own real item-id dispatch. actionId must be the
 * "already lit" id (9 candle, 0xF torch, 0xC generic) or this is a no-op
 * and returns false. Decrements the matching duration counter by exactly
 * 1 -- the original takes no elapsed-time parameter at all, just `sub
 * ..., 1` per call, so duration is counted in "calls" (5-minute ticks,
 * matching TickWorldAilments' own call cadence) rather than raw minutes.
 * If the decrement would reach 0 or below, clamps it to 0 and clears the
 * matching litFlags bit (the light burns out).
 */
bool lightSourceTick(LightSourceState *state, unsigned actionId);

#endif
