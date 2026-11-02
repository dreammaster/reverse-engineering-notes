#include "ultima6/magic.h"

#include <algorithm>

namespace Ultima6 {
namespace Ultima6 {

#include "ultima6/spell_table.inc"

static const int REAGENT_TYPE[8] = {69, 70, 65, 66, 71, 67, 68, 72};
static const char *const REAGENT_NAME[8] = {"mandrake root", "nightshade", "black pearl", "blood moss", "spider silk", "garlic", "ginseng", "sulfurous ash"};

const Magic::SpellInfo *Magic::find(int circle, const std::string &invocation) {
	for (const SpellInfo &s : SPELLS)
		if (s.circle == circle && invocation == s.invocation)
			return &s;
	return nullptr;
}

bool Magic::bookHas(int caster, int spellIndex) {
	std::vector<int> inv;
	_world.inventory(caster, inv, true);
	bool any = false;
	for (int i : inv) {
		const World::Obj &o = _world.objects[i];
		if (o.type == SPELL && (o.quality == ALL_SPELLS || o.quality == spellIndex))
			any = true;
	}
	return any;
}

bool Magic::canCast(int caster, const SpellInfo &spell) {
	std::vector<int> inv;
	_world.inventory(caster, inv, false);
	bool book = false;
	for (int i : inv)
		book |= _world.objects[i].type == SPELLBOOK && _world.objects[i].location() == ObjRecord::READIED;
	if (!book) {
		_scroll.print("\nNo spellbook is readied.\n");
		return false;
	}
	if (!bookHas(caster, spell.index())) {
		_scroll.print("\nThat spell is not in thy spellbook!\n");
		return false;
	}
	if (_world.actors.level[caster] < std::min(8, spell.circle)) {
		_scroll.print("\nYour level is not high enough.\n");
		return false;
	}
	if (_world.actors.unknown13f1[caster] < spell.circle) {
		_scroll.print("\nNot enough magic points.\n");
		return false;
	}
	std::vector<int> all;
	_world.inventory(caster, all, true);
	for (int b = 0; b < 8; ++b) {
		if (!(spell.reagents & (1 << b)))
			continue;
		bool has = false;
		for (int i : all)
			has |= _world.objects[i].type == REAGENT_TYPE[b];
		if (!has) {
			_scroll.print(std::string("\nNo ") + REAGENT_NAME[b] + ".\n");
			return false;
		}
	}
	return true;
}

void Magic::pay(int caster, const SpellInfo &spell) {
	_world.actors.unknown13f1[caster] = (uint8_t)(_world.actors.unknown13f1[caster] - spell.circle);
	for (int b = 0; b < 8; ++b)
		if (spell.reagents & (1 << b))
			_world.removeFromInventory(caster, REAGENT_TYPE[b], 0, 1);
}

// APPROX(24): not the original's exact behaviour, see docs/approximations.md
bool Magic::cast(int caster, const SpellInfo &spell, int target) {
	ObjList &a = _world.actors;
	int tk = target >= 0 ? _world.actorObject(target) : -1;
	bool haveTarget = tk >= 0 && !_world.objects[tk].deleted;
	std::string name = std::string(spell.name);
	int index = spell.index();
	std::vector<uint8_t> &g = a.globals;
	switch (index) {
	case 5: // Harm
		if (!haveTarget)
			break;
		_combat.applyDamage(caster, target, 5);
		return true;
	case 6: // Heal
		if (!haveTarget || a.hp[target] == 0)
			break;
		a.hp[target] = (uint8_t)std::min<int>(std::max(1, std::min(255, a.level[target] * 30)), a.hp[target] + random(1, 30));
		return true;
	case 52: // Great Heal
		if (!haveTarget || a.hp[target] == 0)
			break;
		a.hp[target] = (uint8_t)std::max(1, std::min(255, a.level[target] * 30));
		return true;
	case 9: // Light
		g[0x12 + TIMER_LIGHT] = 100;
		return true;
	case 35: // Great Light
		g[0x12 + TIMER_LIGHT] = 255;
		return true;
	case 16: // Infravision
		g[0x12 + TIMER_INFRAVISION] = 20;
		return true;
	case 17: { // Magic Arrow
		if (!haveTarget)
			break;
		int dex = a.dexterity[target], intel = a.intelligence[caster];
		if (((dex / 2.0f) + 30 - intel) / 2 <= random(1, 30)) {
			_combat.applyDamage(caster, target, random(1, 10));
			return true;
		}
		_scroll.print("\nFailed\n");
		return true;
	}
	case 18: { // Poison
		if (!haveTarget)
			break;
		const CreatureStats *s = Combat::stats(_world.objects[tk].type);
		if (!s || (s->flags & (1 << 12))) {
			_scroll.print("\nNo effect\n");
			return true;
		}
		if (((a.strength[target] / 2 + 30 - a.intelligence[caster]) / 2) <= random(1, 30)) {
			a.status[target] |= 0x08;
			_world.objects[tk].status |= 0x08;
			_scroll.print("\n" + _combat.nameOf(target) + " poisoned!\n");
			return true;
		}
		_scroll.print("\nFailed\n");
		return true;
	}
	case 34: // Fireball
		if (!haveTarget)
			break;
		_combat.applyDamage(caster, target, random(1, 30));
		return true;
	case 68: // Lightning
		if (!haveTarget)
			break;
		_combat.applyDamage(caster, target, random(1, 30));
		return true;
	default:
		break;
	}
	_scroll.print("\nNo effect\n");
	return false;
}

} // namespace Ultima6
} // namespace Ultima6
