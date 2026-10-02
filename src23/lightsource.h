#ifndef YENDOR23_LIGHTSOURCE_H
#define YENDOR23_LIGHTSOURCE_H

#include <stdbool.h>
#include <stdint.h>

#include "party.h"

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
 * also exists in the real catalog; that transition belongs to a
 * genuinely separate mechanic, `TickAilmentDuration`, that ticks a real
 * item slot rather than a standalone counter -- see
 * `lightSourceTickItemSlot` below. lightSourceApply/Tick only manage the
 * 3 standalone duration counters and their own "currently lit" flag
 * bits -- the exact scope of the original two functions, confirmed by
 * direct read.)
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

enum { LightSourceCount = 3, LightSpellTimerCount = 6 };

/*
 * Index into LightSourceState's own arrays -- not the real item ids, just
 * a dense slot index matching the original's own field order
 * (word_36C85/36C89/36C8B).
 */
typedef enum { LightSourceCandle = 0, LightSourceTorch = 1, LightSourceGeneric = 2 } LightSourceKind;

typedef struct {
    /*
     * word_36C85/36C89/36C8B (== DS:0x9425/0x9429/0x942B, the same
     * physical words under two IDA names -- the segment-offset aliasing
     * this project keeps hitting): a *count of lit light sources* per
     * kind, not a duration. lightSourceApply increments it,
     * lightSourceTick/lightSourceTickItemSlot decrement it, and the
     * matching litFlags bit clears when it reaches 0.
     */
    uint16_t litCount[LightSourceCount];
    uint16_t litFlags; /* word_36C79 (DS:0x9419): 0x2000/0x800/0x400 (candle/torch/generic), 0x8000 "timers pending", 0x100..0x8 spell timers */
    /* word_36C93..36C9D == DS:0x9433.. (the same array TickWorldAilmentTimers walks): 6 light-spell duration timers. */
    uint16_t timers[LightSpellTimerCount];
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

/*
 * TickAilmentDuration (yendor2.asm:27869, instruction-identical in
 * Chapter 3) -- the item-slot-level half of the light-source system,
 * genuinely separate from lightSourceApply/Tick's own standalone
 * duration counters above (both happen to clear the same `litFlags`
 * bits when their own tracking reaches 0, but the exact relationship
 * between "the currently lit item instance(s) in inventory" this
 * function tracks and the single active duration lightSourceTick
 * manages isn't resolved -- reimplemented faithfully as two
 * independent mechanisms, since that's what the disassembly does, not
 * forced into one model). Shares `src23/party.h`'s generic 4-byte item
 * slot shape (`itemSlotId`/`itemSlotExtra`) -- `TickWorldAilments`
 * calls the original both over a fixed 6-entry global table and over
 * every party member's own 8 main inventory slots, so `slot` can be
 * either.
 *
 * A no-op (false) unless the slot's own id is one of the 3 "lit"
 * light-source item ids (9 candle, 0xF torch, 0xC generic -- the exact
 * same ids lightSourceTick's own actionId dispatches on). On a match:
 * decrements the slot's own itemSlotExtra by elapsedMinutes (a real
 * elapsed-time parameter here, unlike lightSourceTick's fixed
 * decrement-by-1). If that would reach 0 or below: the slot's own id
 * is incremented (9->10, 0xC->0xD, 0xF->0x10 -- confirmed against real
 * `WORLD.DAT` data: USED CANDLE/LIGHT/TORCH), `litCount` for that
 * light source is decremented, and only once *that* count itself
 * reaches 0 is the matching `litFlags` bit cleared -- so several lit
 * instances of the same light-source type can coexist, and the UI
 * "currently lit" flag only clears once the last one burns out.
 */
bool lightSourceTickItemSlot(LightSourceState *state, uint8_t *slot, uint16_t elapsedMinutes);

/*
 * ApplyEncodedItemEffect's word_33302 bit 0x80 branch (yendor2.asm:51308,
 * "MINER'S LIGHT I"/"MINER'S LIGHT II"/"INFINITE ILLUMINATION" -- the only
 * records in either game that set it, confirmed against real WORLD.DAT):
 * the spell record's own offset 0x2A word (1-6, `SpellFieldTimerSlot`
 * in spellrecord.h) selects one of 6 timers and offset 0x2C
 * (`SpellFieldTimerDuration`) is its duration. Sets timers[slot-1] and
 * its litFlags bits (0x8000 "timers pending" | 0x100 >> (slot-1)).
 * Returns false for a slot outside 1-6 (the original loops forever there;
 * not reproduced).
 */
bool lightSourceArmSpellTimer(LightSourceState *state, unsigned slot, uint16_t duration);

/*
 * TickWorldAilmentTimers (yendor2.asm:27925, instruction-identical in
 * Chapter 3). A no-op unless litFlags bit 0x8000 is set; otherwise clears
 * it and counts every running timer down by elapsedMinutes -- a timer that
 * reaches 0 or below is zeroed and its own bit (0x100>>i) cleared, one
 * still running re-arms 0x8000 so the next tick runs again. A zero timer
 * just gets its bit cleared.
 */
void lightSourceTickTimers(LightSourceState *state, uint16_t elapsedMinutes);

#endif
