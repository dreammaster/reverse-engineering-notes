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

/*
 * Learning a spell from a spell scroll (InteractWithContainer, yendor2.asm:
 * 53457, the "study it" branch, with MarkIneligiblePartyMembers :53582 deciding
 * who may): the member must NOT already know it (their PartyFieldFlagBankCA
 * ability bank, indexed by the 1-based spell id), their secondary-class status
 * bits (PartyStatusSecondaryClassMask, 0x3F) must share a bit with the spell's
 * SpellFieldClassEligibility (masked to 6 bits), and their level must be at
 * least SpellFieldRequiredLevel (signed). Members who fail are greyed out in
 * the original's picker; picking one anyway just warns. spellLearn then sets
 * the bank bit -- the same bank partyKnownAbilityIds lists.
 *
 * (The "cast it from the scroll" branch is ApplyEncodedItemEffect with its
 * "already resolved" flag set -- see combat.h -- gated by the same
 * context rules as spellCanCast but without the MP/ore costs.)
 */
bool spellCanLearn(const uint8_t *spellRecord, unsigned spellId, const uint8_t *partyRecord);
void spellLearn(uint8_t *partyRecord, unsigned spellId);

/* The in-combat / out-of-combat gate alone (spellCanCast without the costs): what a scroll checks before casting. */
bool spellUsableInContext(const uint8_t *spellRecord, bool inCombat);

#endif
