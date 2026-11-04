#include "ultima6/combat.h"

#include "files/converse.h"
#include "ultima6/describe.h"
#include "ultima6/game_tables.h"

#include <algorithm>

namespace Ultima6 {
namespace Ultima6 {

#include "ultima6/combat_tables.inc"

enum { OBJ_BLOOD = 338, OBJ_DEAD_BODY = 339, OBJ_LORD_BRITISH = 409 };

const CreatureStats *u6CreatureTable(int &count) {
	count = (int)(sizeof(CREATURES) / sizeof(CREATURES[0]));
	return CREATURES;
}

const CreatureStats *Combat::stats(int type) {
	const GameTables &t = activeTables();
	for (int i = 0; i < t.creatureCount; ++i)
		if (t.creatures[i].type == type)
			return &t.creatures[i];
	return nullptr;
}

void u6WeaponTables(GameTables &t) {
	for (const auto &w : WEAPON_DAMAGE)
		t.damage.push_back(TypeValue{w.type, w.damage});
	for (const auto &r : WEAPON_RANGE)
		t.range.push_back(TypeValue{r.type, r.range});
	for (const auto &a : ARMOUR)
		t.armour.push_back(TypeValue{a.type, a.armour});
}

int Combat::weaponDamageOf(int type) {
	for (const TypeValue &w : activeTables().damage)
		if (w.type == type)
			return w.value;
	return -1;
}

int Combat::rangeOf(int type) {
	for (const TypeValue &r : activeTables().range)
		if (r.type == type)
			return r.value;
	return 1;
}

int Combat::armourOf(int type) {
	for (const TypeValue &a : activeTables().armour)
		if (a.type == type)
			return a.value;
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
	ConverseScript s;
	if (_world.script(actor, s)) {
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

int Combat::adjustedIntelligence(int actor) const {
	if (actor < 0 || actor >= ObjList::ACTORS)
		return 1;
	int k = _world.actorObject(actor);
	int v = _world.actors.intelligence[actor];
	if (k >= 0 && (_world.objects[k].status & 0x40))
		v = v - 3 < 1 ? 1 : v - 3;
	return v;
}

bool Combat::resists(int target, int caster) {
	int x = adjustedIntelligence(target) / 2 + 15 - adjustedIntelligence(caster);
	return x > random(1, 30);
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
	// what the dead become (sub_2B5A4): people turn into a body (type 0x153, its frame by race, table ds:1A29); creatures flagged 0x800 or 0x1000 vanish; the rest stay as they are
	// APPROX(16): the original changes the actor's own object (its carried things stay with the actor); here a body object stands in its place and takes the things (or they are laid on the square for those that vanish)
	static const uint16_t BODY[][2] = {{0x16F, 0x153}, {0x16B, 0x153}, {0x173, 0x153}, {0x172, 0x553}, {0x1A8, 0xD54}, {0x17C, 0x953}, {0x17B, 0x953}, {0x183, 0xD53}, {0x17A, 0xD53},
									   {0x17E, 0x1153}, {0x180, 0x1553}, {0x179, 0x1553}, {0x178, 0x1953}, {0x19A, 0x1D53}, {0x182, 0x1D53}, {0x188, 0x1D53}, {0x17D, 0x2153},
									   {0x181, 0x2153}, {0x187, 0x2153}, {0x17F, 0x2153}, {0x170, 0x2553}, {0x16A, 0xD55}};
	const CreatureStats *own = stats(body.type);
	int cflags = own ? own->flags : 0;
	int word = -1;
	for (const auto &row : BODY)
		if (row[0] == body.type)
			word = row[1];
	World::Obj corpse;
	corpse.status = 0;
	corpse.x = body.x;
	corpse.y = body.y;
	corpse.z = body.z;
	corpse.actor = -1;
	corpse.parent = -1;
	corpse.quantity = 1;
	corpse.quality = (uint8_t)actor; // whose body it is (APPROX(56): the original's link is not traced)
	bool leaveBody = true;
	if (word >= 0 && !(cflags & 0x1000)) {
		corpse.type = (uint16_t)(word & 0x3FF);
		corpse.frame = (uint8_t)(word >> 10);
	} else if (cflags & (0x800 | 0x1000)) {
		leaveBody = false;
	} else {
		corpse.type = body.type;
		corpse.frame = body.frame;
	}
	if (cflags & 0x2000) {
		World::Obj blood = corpse;
		blood.type = 0x152;
		blood.frame = (uint8_t)(_rng.next() % 3);
		blood.quality = 0;
		blood.status = 0x20;
		_world.objects.push_back(blood);
		_world.addToMap((int)_world.objects.size() - 1);
	}
	int c = -1;
	if (leaveBody) {
		_world.objects.push_back(corpse);
		c = (int)_world.objects.size() - 1;
		_world.addToMap(c);
	}
	for (int i : inv) {
		World::Obj &o = _world.objects[i];
		if (c >= 0) {
			o.status = (uint8_t)((o.status & ~ObjRecord::LOCATION_MASK) | ObjRecord::IN_CONTAINER);
			o.parent = c;
		} else {
			o.status = (uint8_t)(o.status & ~ObjRecord::LOCATION_MASK);
			o.x = body.x;
			o.y = body.y;
			o.z = body.z;
			o.parent = -1;
			_world.addToMap(i);
		}
	}
	// leave the party
	ObjList &a = _world.actors;
	for (int i = 1; i < a.partySize; ++i) { // the leader stays in the roster (sub_2C2E5 2C4AC)
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

// APPROX(16): not the original's exact behaviour, see docs/approximations.md (sub_2C2E5: ships and doors take no damage)
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
	int type = _world.objects[kd].type;
	if (type == 0x1A7 || damage < 0)
		return false; // a balloon takes nothing
	if ((type == 0x19E || type == 0x19F) && a.partySize > 0) {
		// a boat that is hit passes the blow on to a random member of the party (sub_2C2E5 2C350)
		int member = a.partyRoster[a.partySize > 1 ? random(0, a.partySize - 1) : 0];
		if (member != defender)
			return applyDamage(attacker, member, damage);
	}
	if (damage >= a.hp[defender]) {
		const CreatureStats *foe = stats(type);
		// sub_2C2E5 2C3FC-2C490: killing somebody who is on no side costs 5 karma; killing one of the evil side (status 0x20) earns a quarter of the sum of the creature's numbers
		if (!(a.status[defender] & 0x60) && a.globals.size() > 8)
			a.globals[8] = (uint8_t)(a.globals[8] > 5 ? a.globals[8] - 5 : 0);
		if ((a.status[defender] & 0x20) && foe) {
			int gain = (foe->str + foe->dex + foe->intel + foe->hp + foe->dmg + foe->exp) / 4;
			a.exp[attacker] = (uint16_t)(a.exp[attacker] + gain);
		}
		die(defender);
		_scroll.print(name + " killed!\n");
		return true;
	}
	a.hp[defender] = (uint8_t)(a.hp[defender] - damage);
	if (a.worktype[defender] == 9)
		a.worktype[defender] = 8;
	if (onHurt)
		onHurt(defender);
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
