#include "ultima6/combat.h"

#include "files/converse.h"
#include "ultima6/describe.h"

#include <algorithm>

namespace Ultima6 {
namespace Ultima6 {

#include "ultima6/combat_tables.inc"

enum { OBJ_BLOOD = 338, OBJ_DEAD_BODY = 339, OBJ_LORD_BRITISH = 409 };

const CreatureStats *Combat::stats(int type) {
	for (const CreatureStats &c : CREATURES)
		if (c.type == type)
			return &c;
	return nullptr;
}

int Combat::weaponDamageOf(int type) {
	for (const auto &w : WEAPON_DAMAGE)
		if (w.type == type)
			return w.damage;
	return -1;
}

int Combat::armourOf(int type) {
	for (const auto &a : ARMOUR)
		if (a.type == type)
			return a.armour;
	return 0;
}

int Combat::weaponType(int actor) const {
	std::vector<int> inv;
	_world.inventory(actor, inv, false);
	int best = -1, bestDamage = -1;
	for (int i : inv) {
		const World::Obj &o = _world.objects[i];
		if (o.location() != ObjRecord::READIED)
			continue;
		int d = weaponDamageOf(o.type);
		if (d > bestDamage) {
			bestDamage = d;
			best = o.type;
		}
	}
	if (best >= 0)
		return best;
	int k = _world.actorObject(actor);
	return k >= 0 ? _world.objects[k].type : 0;
}

int Combat::armourClass(int actor) const {
	std::vector<int> inv;
	_world.inventory(actor, inv, false);
	int ac = 0;
	for (int i : inv)
		if (_world.objects[i].location() == ObjRecord::READIED)
			ac += armourOf(_world.objects[i].type);
	return ac;
}

std::string Combat::nameOf(int actor) const {
	const ObjList &a = _world.actors;
	for (int i = 0; i < a.partySize; ++i)
		if (a.partyRoster[i] == actor && i < ObjList::PARTY_NAMES)
			return (const char *)a.partyNames[i];
	int k = _world.actorObject(actor);
	int type = k >= 0 ? _world.objects[k].type : 0;
	ConverseRef ref;
	ConverseScript s;
	if (ConverseScript::locate(actor, type, ref) && s.load(_world.converse[ref.second ? 1 : 0], ref.item)) {
		std::string n = s.name();
		std::replace(n.begin(), n.end(), '_', '.');
		return n;
	}
	return Describe::tileName(_world, _world.baseTile.firstTile[type & (BaseTile::TYPES - 1)], false);
}

// sub_2C8BF (hit check), sub_2B3D7 / sub_2B401 (adjusted strength and dexterity)
int Combat::adjustedStrength(int actor) const {
	if (actor < 0 || actor >= ObjList::ACTORS)
		return 1;
	int k = _world.actorObject(actor);
	int v = _world.actors.strength[actor];
	if (k >= 0 && (_world.objects[k].status & 0x40)) // cursed
		v = v - 3 < 1 ? 1 : v - 3;
	return v;
}

int Combat::adjustedDexterity(int actor) const {
	if (actor < 0 || actor >= ObjList::ACTORS)
		return 1;
	int k = _world.actorObject(actor);
	int v = _world.actors.dexterity[actor];
	if (k >= 0 && (_world.objects[k].status & 0x40))
		v = v - 3 < 1 ? 1 : v - 3;
	if (_world.actors.status[actor] & 0x04) // asleep
		v = 1;
	return v;
}

bool Combat::hitCheck(int attacker, int defender, int weapon) {
	int ka = _world.actorObject(attacker), kd = _world.actorObject(defender);
	if (ka < 0 || kd < 0)
		return false;
	if (weapon == 48) // the glass sword never misses
		return true;
	const CreatureStats *own = stats(_world.objects[ka].type);
	bool useStrength = weapon == 4 || weapon == 34 || weapon == 39 || weapon == 44; // spiked helm, club, mace, two-handed hammer
	if (own && weapon == _world.objects[ka].type && (own->flags & CreatureStats::STRENGTH_BASED))
		useStrength = true;
	int atk = useStrength ? adjustedStrength(attacker) : adjustedDexterity(attacker);
	int foeDex = adjustedDexterity(defender);
	return random(1, 30) >= (foeDex + 30 - atk) / 2; // a roll below half the difference misses
}

void Combat::die(int actor) {
	int k = _world.actorObject(actor);
	if (k < 0)
		return;
	World::Obj body = _world.objects[k];
	std::vector<int> inv;
	_world.inventory(actor, inv, false);
	_world.actors.hp[actor] = 0;
	_world.actors.worktype[actor] = 0;
	_world.actors.status[actor] |= 0x10; // dead
	if (actor == 1)
		return; // the Avatar's death is handled by the game
	// a dead body takes what the actor carried
	World::Obj corpse;
	corpse.status = 0;
	corpse.x = body.x;
	corpse.y = body.y;
	corpse.z = body.z;
	corpse.type = OBJ_DEAD_BODY;
	corpse.frame = 0;
	corpse.quantity = 1;
	corpse.quality = 0;
	corpse.actor = -1;
	corpse.parent = -1;
	_world.objects.push_back(corpse);
	int c = (int)_world.objects.size() - 1;
	_world.addToMap(c);
	for (int i : inv) {
		World::Obj &o = _world.objects[i];
		o.status = (uint8_t)((o.status & ~ObjRecord::LOCATION_MASK) | ObjRecord::IN_CONTAINER);
		o.parent = c;
	}
	// leave the party
	ObjList &a = _world.actors;
	for (int i = 0; i < a.partySize; ++i) {
		if (a.partyRoster[i] == actor) {
			for (int j = i; j + 1 < a.partySize; ++j) {
				a.partyRoster[j] = a.partyRoster[j + 1];
				std::copy(a.partyNames[j + 1], a.partyNames[j + 1] + ObjList::NAME_SIZE, a.partyNames[j]);
			}
			--a.partySize;
			break;
		}
	}
	_world.deleteObject(k);
}

bool Combat::attack(int attacker, int defender) {
	int ka = _world.actorObject(attacker), kd = _world.actorObject(defender);
	if (ka < 0 || kd < 0)
		return false;
	int weapon = weaponType(attacker);
	std::string name = nameOf(defender);

	if (!hitCheck(attacker, defender, weapon)) {
		return false;
	}
	int maxDamage = weaponDamageOf(weapon);
	if (maxDamage < 0) {
		const CreatureStats *own = stats(weapon);
		maxDamage = own ? own->dmg : 1;
	}
	int damage = maxDamage;
	if (maxDamage > 1 && maxDamage != 255)
		damage = random(1, maxDamage);
	int ac = armourClass(defender);
	if (damage != 255 && ac > 0)
		damage -= random(1, ac);
	if (damage <= 0) {
		_scroll.print(name + " grazed.\n");
		return false;
	}
	return applyDamage(attacker, defender, damage);
}

// APPROX(16): not the original's exact behaviour, see docs/approximations.md
bool Combat::applyDamage(int attacker, int defender, int damage) {
	int kd = _world.actorObject(defender);
	if (kd < 0)
		return false;
	ObjList &a = _world.actors;
	std::string name = nameOf(defender);
	if (_world.objects[kd].type == OBJ_LORD_BRITISH) {
		a.hp[defender] = 255; // he cannot be hurt
		return false;
	}
	if (damage >= a.hp[defender]) {
		const CreatureStats *foe = stats(_world.objects[kd].type);
		if (foe && (foe->align == CreatureStats::EVIL || foe->align == CreatureStats::CHAOTIC)) {
			int gain = (foe->str + foe->dex + foe->intel + foe->hp + foe->dmg + foe->exp) / 4;
			a.exp[attacker] = (uint16_t)(a.exp[attacker] + gain);
		}
		die(defender);
		_scroll.print(name + " killed!\n");
		return true;
	}
	a.hp[defender] = (uint8_t)(a.hp[defender] - damage);
	// how badly hurt: the quarters of the maximum hit points
	int most = std::max(1, std::min(255, a.level[defender] * 30));
	int quarter = (a.hp[defender] * 4) / most;
	if (quarter < 4) {
		std::string s = name + " "; // sub_2C97F: "`%s " then "critical!" or "heavily|lightly|barely wounded."
		if (quarter == 0)
			s += "critical!\n";
		else
			s += std::string(quarter == 1 ? "heavily" : quarter == 2 ? "lightly" : "barely") + " wounded.\n";
		_scroll.print(s);
	}
	return false;
}

} // namespace Ultima6
} // namespace Ultima6
