#ifndef YENDOR23_SPELLCAST_H
#define YENDOR23_SPELLCAST_H

#include <stdbool.h>
#include <stdint.h>

#include "savegame.h"
#include "spellrecord.h"

/*
 * Casting a spell from the alchemy/spell screen (RunAlchemyScreen,
 * yendor2.asm:24615; CheckSpellCastability :25259 and DeductAlchemySpellCosts
 * :25654, instruction-identical in Chapter 3). The screen's own list building
 * is party.h's partyKnownAbilityIds; this is the per-spell "can I cast it
 * now" test and the payment.
 *
 * spellCanCast mirrors CheckSpellCastability:
 *  - context: while in combat (g_uiScratchFlags4 bit 0x1000), a spell with
 *    SpellFlagsANotInCombat (FlagsA 0x400 -- the exploration spells:
 *    projectiles, light, jump, rest, unlock, mark) is refused; outside combat,
 *    a spell with FlagsB 0x2000 or 0x1000 (the two attack-the-engaged-monsters
 *    branches) is refused. Real data bears this out: every projectile and
 *    utility spell carries 0x400, every combat attack carries 0x2000/0x1000.
 *  - NUORE (SpellFieldNuoreCost) and MAGIC ORE (SpellFieldOreCost) must each
 *    be available (counter >= cost; a zero cost is not checked), and the
 *    caster's current MP must be at least SpellFieldMpCost (a signed
 *    comparison, as in the original).
 * The original's screen also skips the check entirely for an incapacitated
 * caster (the list is built but nothing is marked castable); callers do that.
 *
 * spellDeductCosts mirrors DeductAlchemySpellCosts: MP is subtracted from the
 * caster WITHOUT clamping (the castability check guarantees it's enough), and
 * the two ore counters through the clamped BCD subtraction.
 */
bool spellCanCast(const uint8_t *spellRecord, const uint8_t *casterRecord, SaveGame *save, bool inCombat);

void spellDeductCosts(const uint8_t *spellRecord, uint8_t *casterRecord, SaveGame *save);

#endif
