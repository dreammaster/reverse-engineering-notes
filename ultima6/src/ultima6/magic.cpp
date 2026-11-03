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

const Magic::SpellInfo *Magic::byIndex(int index) {
	for (const SpellInfo &s : SPELLS)
		if (s.index() == index)
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

static const char *const WORDS[26] = {"An", "Bet", "Corp", "Des", "Ex", "Flam", "Grav", "Hur", "In", "Jux", "Kal", "Lor", "Mani",
									  "Nox", "Ort", "Por", "Quas", "Rel", "Sanct", "Tym", "Uus", "Vas", "Wis", "Xen", "Ylem", "Zu"}; // ds:0BF0 table

bool Magic::checkBook(int caster) {
	std::vector<int> inv;
	_world.inventory(caster, inv, false);
	for (int i : inv)
		if (_world.objects[i].type == SPELLBOOK && _world.objects[i].location() == ObjRecord::READIED)
			return true;
	_scroll.print("No spellbook is readied.\n");
	return false;
}

bool Magic::checkSpells(int caster) {
	std::vector<int> inv;
	_world.inventory(caster, inv, true);
	for (int i : inv)
		if (_world.objects[i].type == SPELL)
			return true;
	_scroll.print("No spells in the spellbook!\n");
	return false;
}

std::string Magic::announce(const SpellInfo &spell) const {
	std::string out = std::string(spell.name) + "\n\"";
	for (size_t i = 0; spell.invocation[i]; ++i) {
		int c = spell.invocation[i] - 'a';
		out += std::string(i ? " " : "") + (c >= 0 && c < 26 ? WORDS[c] : "?");
	}
	return out + "\"";
}

// the syllables of a spell as the staffs and Enchant print them: "Kal Wis Ylem" (sub_32BC7)
std::string Magic::words(const SpellInfo &spell) {
	std::string out;
	for (size_t i = 0; spell.invocation[i]; ++i) {
		int c = spell.invocation[i] - 'a';
		out += std::string(i ? " " : "") + (c >= 0 && c < 26 ? WORDS[c] : "?");
	}
	return out;
}

bool Magic::checkCost(int caster, const SpellInfo &spell) {
	if (_world.actors.level[caster] < spell.circle) {
		_scroll.print("\n\nYour level is not high enough.\n");
		return false;
	}
	if (_world.actors.unknown13f1[caster] < spell.circle) {
		_scroll.print("\n\nNot enough magic points.\n");
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
			_scroll.print("\n\nNo Reagents.\n");
			return false;
		}
	}
	return true;
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
	case 5: // Harm (sub_21CB4): resisted, else 1-10 points
		if (!haveTarget)
			break;
		if (_combat.resists(target, caster)) {
			_scroll.print("\nFailed\n");
			return true;
		}
		_combat.applyDamage(caster, target, random(1, 10));
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
	case 17: { // Magic Arrow (sub_21C45, sub_21338): a miss when half the target's dexterity + 30 less the caster's intelligence, halved, beats a roll of 1-30; else 1-10 points
		if (!haveTarget)
			break;
		int x = (_combat.adjustedDexterity(target) / 2 + 30 - _combat.adjustedIntelligence(caster)) / 2;
		if (x > random(1, 30)) {
			_scroll.print("\nFailed\n");
			return true;
		}
		_combat.applyDamage(caster, target, random(1, 10));
		return true;
	}
	case 18: { // Poison (sub_21BF2, sub_212F2): immune creatures (flag 4) say "No effect"; the target resists when half its strength + 30 less the caster's intelligence, halved, beats a roll of 1-30
		if (!haveTarget)
			break;
		const CreatureStats *s = Combat::stats(_world.objects[tk].type);
		if (s && (s->flags & 4)) {
			_scroll.print("\nNo effect\n");
			return true;
		}
		int x = (_combat.adjustedStrength(target) / 2 + 30 - _combat.adjustedIntelligence(caster)) / 2;
		if (x > random(1, 30)) {
			_scroll.print("\nFailed\n");
			return true;
		}
		a.status[target] |= 0x08;
		return true;
	}
	case 34: { // Fireball (sub_222F2): creatures with flag 8 are untouched ("No effect"), those with flag 0x10 take double of the roll 1-20
		if (!haveTarget)
			break;
		const CreatureStats *cs = Combat::stats(_world.objects[tk].type);
		if (cs && (cs->flags & 8))
			break;
		int dmg = random(1, 20);
		_combat.applyDamage(caster, target, cs && (cs->flags & 0x10) ? dmg * 2 : dmg);
		return true;
	}
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
