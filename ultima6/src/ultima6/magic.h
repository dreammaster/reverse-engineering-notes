#ifndef ULTIMA6_ULTIMA6_MAGIC_H
#define ULTIMA6_ULTIMA6_MAGIC_H

#include "engine/random.h"
#include "ultima6/combat.h"
#include "ultima6/text_window.h"
#include "ultima6/world.h"

#include <string>

namespace Ultima6 {
namespace Ultima6 {

/**
 * Casting. A spell is named by its circle (1-8) and the first letters of its syllables ("im" is Heal, "vz" Mass Sleep). The caster must
 * have a spellbook readied that holds the spell (spell objects, type 58, whose quality is (circle - 1) * 16 + number - 1;
 * quality 255 stands for every spell), a level of at least the circle, as many magic points as the circle, and one of each reagent
 * the spell needs. The points and reagents are used up. Not all spells have their effect here yet: the others say "No effect".
 */
class Magic {
public:
	enum Select { SEL_NONE, SEL_LOCATION, SEL_LOCATION_PROJ, SEL_ACTOR, SEL_ACTOR_PROJ, SEL_OBJ, SEL_OBJ_PROJ, SEL_ACTOR_OR_OBJ, SEL_ACTOR_OR_OBJ_PROJ };
	enum { SPELLBOOK = 57, SPELL = 58, ALL_SPELLS = 255 };
	enum Timer { TIMER_LIGHT = 0, TIMER_INFRAVISION = 1 };

	struct SpellInfo {
		const char *name;
		const char *invocation;
		int reagents;
		int circle, number;
		Select select;
		int index() const {
			return (circle - 1) * 16 + (number - 1);
		}
	};

	Magic(World &world, Combat &combat, BorlandRandom &rng, TextWindow &scroll) : _world(world), _combat(combat), _rng(rng), _scroll(scroll) {
	}

	static const SpellInfo *find(int circle, const std::string &invocation);

	/** check_spellbook (GAME.EXE 25D7F-ish): a readied spellbook (type 57); prints "No spellbook is readied." if there is none. */
	bool checkBook(int caster);
	/** cmd_cast: "No spells in the spellbook!" when the books hold nothing. */
	bool checkSpells(int caster);
	/** The words of power of a spell as cmd_cast prints them: the name, then the quoted syllables (An, Bet, Corp ... Zu for the letters A-Z). */
	std::string announce(const SpellInfo &spell) const;
	/** The checks cmd_cast makes after printing the spell, in its order: level ("Your level is not high enough."), magic points, reagents ("No Reagents."). */
	bool checkCost(int caster, const SpellInfo &spell);
	/** Checks the caster can cast; prints the reason and returns false if not. Nothing is used up. */
	bool canCast(int caster, const SpellInfo &spell);
	/** Uses the magic points and reagents. */
	void pay(int caster, const SpellInfo &spell);
	/** Works the spell on an actor (target < 0: no actor) or, for those that need none, on the caster. Returns whether it did anything. */
	bool cast(int caster, const SpellInfo &spell, int target);

private:
	bool bookHas(int caster, int spellIndex);
	int random(int lo, int hi) {
		return lo + _rng.next() % (hi - lo + 1);
	}

	World &_world;
	Combat &_combat;
	BorlandRandom &_rng;
	TextWindow &_scroll;
};

} // namespace Ultima6
} // namespace Ultima6

#endif
