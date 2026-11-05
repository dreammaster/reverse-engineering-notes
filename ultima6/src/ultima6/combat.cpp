#include "ultima6/combat.h"

#include "files/converse.h"
#include "ultima6/describe.h"
#include "ultima6/game_tables.h"
#include "ultima6/movement.h"

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

// sub_26D19: an actor that is held (movement flag 0x10, dragged under) gets one chance in its turn: a party member cries "ARGH!"; a roll of 1 to 30 below its (adjusted) strength and it is
// "regurgitated!" and free (and goes on with its turn), else the hold does 1 to 15 damage (from the Avatar) and the turn and the moves are lost.
bool Combat::dragStruggle(int actor) {
	ObjList &a = _world.actors;
	if (actor >= ObjList::ACTORS || !(a.movementFlags[actor] & 0x10))
		return false;
	if (a.worktype[actor] == 2)
		_scroll.print("\n" + nameOf(actor) + ":\n>ARGH!\n");
	if (random(1, 30) < adjustedStrength(actor)) {
		if (onSfx)
			onSfx(0xA, 0);
		_scroll.print(nameOf(actor) + " regurgitated!\n");
		a.movementFlags[actor] &= ~0x10;
		return false;
	}
	applyDamage(1, actor, random(1, 15));
	a.moves[actor] = 0;
	return true;
}

int Combat::distance(int dx, int dy) {
	static const uint8_t DIST[8][8] = {{0, 1, 2, 3, 4, 5, 6, 7}, {1, 1, 2, 3, 4, 5, 6, 7}, {2, 2, 2, 3, 4, 5, 6, 7}, {3, 3, 3, 4, 5, 6, 7, 7},
									   {4, 4, 4, 5, 6, 7, 7, 8}, {5, 5, 5, 6, 7, 7, 8, 8}, {6, 6, 6, 7, 7, 8, 8, 8}, {7, 7, 7, 7, 8, 8, 8, 8}};
	return dx < 8 && dy < 8 ? DIST[dx][dy] : 8;
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
	int k = _world.actorObject(actor);
	if (k >= 0 && (_world.objects[k].status & 0x40)) // cursed (sub_2B9AF)
		ac -= 3;
	if (actor >= 0 && actor < ObjList::ACTORS && (_world.actors.status[actor] & 0x01)) // protected
		ac += 3;
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
	return attackWith(attacker, defender, weaponType(attacker));
}

// perform_attack 2D2F8-2D376: a weapon is a missile when the target is not next to the attacker, or when it is one of the throwing and shooting weapons (the morning star 0x28 and the
// halberd 0x2F reach two squares but are not missiles)
bool Combat::isMissile(int attacker, int defender, int weapon) const {
	int ka = _world.actorObject(attacker), kd = _world.actorObject(defender);
	if (ka < 0 || kd < 0 || weapon == 0x28 || weapon == 0x2F)
		return false;
	static const int MISSILES[] = {0x21, 0x25, 0x29, 0x2A, 0x31, 0x32, 0x53, 0x5B, 0x4F, 0x50};
	bool missile = std::abs((int)_world.objects[ka].x - (int)_world.objects[kd].x) > 1 || std::abs((int)_world.objects[ka].y - (int)_world.objects[kd].y) > 1;
	for (int m : MISSILES)
		missile |= weapon == m;
	return missile;
}

// sub_2BA3E: who stops an actor from shooting. An actor going about its own affairs (worktype 2 to 15) that remembers who last struck it (objlist +0x18F1) and has that one next to it,
// alive, awake, free to move and not stopped by time (during Time Stop only if of the party) cannot fire a missile: "<name> interferes!". -1 if nobody does.
int Combat::interferer(int attacker, bool timeStopped) const {
	const ObjList &a = _world.actors;
	if (attacker >= 0x100 || a.worktype[attacker] <= 1 || a.worktype[attacker] >= 0x10)
		return -1;
	int opp = a.unknown18f1[attacker];
	if (opp == attacker)
		return -1;
	int ko = _world.actorObject(opp), ka = _world.actorObject(attacker);
	if (ko < 0 || ka < 0 || _world.objects[ko].deleted)
		return -1;
	if ((a.status[opp] & 0x16) || (a.movementFlags[opp] & 0x10))
		return -1;
	if (std::abs((int)_world.objects[ko].x - (int)_world.objects[ka].x) >= 2 || std::abs((int)_world.objects[ko].y - (int)_world.objects[ka].y) >= 2)
		return -1;
	if (timeStopped && !(a.status[opp] & 0x80))
		return -1;
	return opp;
}

bool Combat::attackWith(int attacker, int defender, int weapon, bool commanded) {
	int ka = _world.actorObject(attacker), kd = _world.actorObject(defender);
	if (ka < 0 || kd < 0)
		return false;
	if (onSwing && !isMissile(attacker, defender, weapon))
		onSwing(defender);
	if (!hitCheck(attacker, defender, weapon))
		return false;
	int maxDamage = weaponDamageOf(weapon);
	if (maxDamage < 0) {
		const CreatureStats *own = stats(weapon);
		maxDamage = own ? own->dmg : 1;
	}
	// perform_attack 2D6EC-2D75D: what is hit by a kind of creature with movement flag 0x80 takes half (rounded up) from every weapon but the glass sword, the triple crossbow and the
	// magic bow; the 0x5B does no harm at all; a glass sword that hits shatters
	bool killed = false;
	if (defender != attacker) {
		int cls = MovementClasses::classOf(_world.objects[kd].type);
		if (cls >= 0 && (MovementClasses::flags(cls) & 0x80) && weapon != 0x30 && weapon != 0x32 && weapon != 0x36)
			maxDamage = (maxDamage + 1) >> 1;
		if (weapon != 0x5B)
			killed = strike(attacker, defender, maxDamage, commanded);
		if (weapon == 0x30) {
			_scroll.print("Thy sword hath shattered!\n");
			std::vector<int> inv;
			_world.inventory(attacker, inv, false);
			for (int i : inv)
				if (_world.objects[i].type == 0x30 && _world.objects[i].location() == ObjRecord::READIED) {
					_world.deleteObject(i);
					break;
				}
		}
	}
	return killed;
}

// sub_2B9AF the armour class (what is worn, less 3 for a cursed wearer and plus 3 for a protected one) is armourClass; sub_2CD10 is this: the damage rolled from 1 to the most the blow can do
// (a blow of 255 or 1 is as it is), less a number from 1 to the armour class; nothing left and the target wakes up and is "grazed". Else a target that is busy with its own affairs (worktype
// 2 to 15) next to its attacker remembers who it was (objlist +0x18F1), the damage is dealt (applyDamage), and `commanded` (a command of the player, the target the one he chose)
// makes a person who is on no side react (sub_2CC0E): yelling for help, fighting back (worktype 0x9B) or taking a side. The kind of creature that struck adds its own effects:
// movement class 8 drags the victim under (movement flag 0x10), class 0 (the acid slug) dissolves a piece of armour, class 0x17 robs one of its food, and the poisonous (flag 0x40) poison it one
// time in four ("poisoned!").
bool Combat::strike(int attacker, int target, int damage, bool commanded) {
	ObjList &a = _world.actors;
	int kt = _world.actorObject(target), ka = _world.actorObject(attacker);
	if (kt < 0 || ka < 0)
		return false;
	std::string name = nameOf(target);
	if (_world.objects[ka].type == 0x165 && _world.objects[ka].frame == 0)
		_world.objects[ka].frame = 1;
	if (damage == -1)
		damage = 1;
	else if (damage > 1 && damage < 255)
		damage = random(1, damage);
	int ac = target < 0x100 ? armourClass(target) : 0;
	if (ac > 0 && damage < 255)
		damage -= random(1, ac);
	bool killed = false;
	if (damage <= 0) {
		if (a.status[target] & 0x04) { // sub_2C20F: a sleeper wakes
			if (onSfx)
				onSfx(4, 0);
			a.status[target] &= ~0x04;
			_world.objects[kt].type = a.baseObjFrame[target] & 0x3FF;
			_world.objects[kt].frame = (uint8_t)(a.baseObjFrame[target] >> 10);
		}
		_scroll.print(name + " grazed.\n");
	} else {
		if (target < 0x100 && a.worktype[target] > 1 && a.worktype[target] < 0x10 && std::abs((int)_world.objects[kt].x - (int)_world.objects[ka].x) < 2 &&
			std::abs((int)_world.objects[kt].y - (int)_world.objects[ka].y) < 2)
			a.unknown18f1[target] = (uint8_t)attacker;
		killed = applyDamage(attacker, target, damage);
	}
	if (target >= 0x100)
		return killed;
	// sub_2CC0E: a person on no side, not of the party, answers a blow of the one who is steering
	if (commanded && !(a.status[target] & 0xE0)) {
		int cls = MovementClasses::classOf(_world.objects[kt].type);
		if (a.worktype[attacker] == 2 && cls >= 0 && (MovementClasses::flags(cls) & 0x4000) && damage > 0) {
			_scroll.print(name + " yells for help!\n");
			alertTownspeople();
		}
		if (a.worktype[target] >= 0x80) {
			if (a.worktype[attacker] == 2) {
				if (_world.objects[kt].type == 0x188) {
					_world.objects[kt].type = a.baseObjFrame[target] & 0x3FF;
					_world.objects[kt].frame = (uint8_t)(a.baseObjFrame[target] >> 10);
				}
				a.worktype[target] = 0x9B;
			}
		} else {
			a.status[target] = (uint8_t)((a.status[target] & 0x9F) | ((a.status[attacker] & 0x60) == 0x40 ? 0x60 : 0x40));
			if (target >= 0xE0)
				a.worktype[target] = 8;
		}
	}
	if (killed || a.hp[target] == 0)
		return killed;
	// the effects of the kind of creature that struck
	int cls = MovementClasses::classOf(_world.objects[ka].type);
	auto pick = [&](int mode) { // sub_2CA89: one of the pieces of armour (mode 0) or foods (mode 1) the target carries, by chance
		static const int ARMOUR_PIECES[] = {2, 3, 4, 5, 7, 0xA, 0xB, 0xC, 0xD, 0xF, 0x13, 0x14, 0x15, 0x16, 0x23, 0x26, 0};
		static const int FOODS[] = {0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x87, 0xD1, 0xD2, 0xD3, 0};
		const int *list = mode ? FOODS : ARMOUR_PIECES;
		std::vector<int> inv;
		_world.inventory(target, inv, false);
		int found = -1;
		for (int i : inv) {
			bool match = false;
			for (int n = 0; list[n]; ++n)
				match |= _world.objects[i].type == list[n];
			if (match && (found < 0 || random(0, 1) != 0))
				found = i;
		}
		return found;
	};
	if (cls == 8) {
		if (onSfx)
			onSfx(9, 0);
		_scroll.print(name + " dragged under!\n");
		a.movementFlags[target] |= 0x10;
	}
	if (cls == 0) {
		int item = pick(0);
		if (item >= 0) {
			if (onSfx)
				onSfx(0xB, 0);
			_scroll.print("A slug dissolves " + name + "'s " + Describe::tileName(_world, _world.baseTile.firstTile[_world.objects[item].type], false) + "!\n");
			_world.deleteObject(item);
		}
	}
	if (cls == 0x17) {
		int item = pick(1);
		if (item >= 0) {
			if (onSfx)
				onSfx(0xC, 0);
			_scroll.print(name + " has been robbed!\n");
			_world.deleteObject(item);
		}
	}
	if (cls >= 0 && (MovementClasses::flags(cls) & 0x40) && random(0, 3) == 0) {
		_scroll.print(name + " poisoned!\n");
		a.status[target] |= 0x08;
	}
	return killed;
}

// sub_2CAFE: karma falls by 5; every actor of the party's level that is on no side, not of the party, not dead, asleep or held: a guard (0x17E) within 32 squares comes to arrest (worktype
// 0x12); the others within 6 squares fight (0x9B) if they were at a pastime (worktype above 0x86; at a walk 0x80-0x86 they go on) or run away (0x13) if they were not on a schedule
void Combat::alertTownspeople() {
	ObjList &a = _world.actors;
	if (a.globals.size() > 8)
		a.globals[8] = (uint8_t)(a.globals[8] > 5 ? a.globals[8] - 5 : 0);
	int lk = _world.actorObject(a.partyRoster[0]);
	if (lk < 0)
		return;
	int px = _world.objects[lk].x, py = _world.objects[lk].y, pz = _world.objects[lk].z;
	for (int i = 0; i < 0x100; ++i) {
		int k = _world.actorObject(i);
		if (k < 0 || _world.objects[k].deleted || _world.objects[k].z != pz || (a.status[i] & 0x10) || (a.status[i] & 0x80) || (a.status[i] & 0x60))
			continue;
		int dist = std::max(std::abs((int)_world.objects[k].x - px), std::abs((int)_world.objects[k].y - py));
		if (_world.objects[k].type == 0x17E) {
			if (dist < 0x20)
				a.worktype[i] = 0x12;
			continue;
		}
		if (a.worktype[i] == 0x0E || a.worktype[i] == 0 || (a.status[i] & 0x16) || (a.movementFlags[i] & 0x10) || dist >= 6)
			continue;
		if (a.worktype[i] >= 0x80) {
			if (a.worktype[i] <= 0x86)
				continue;
			if (_world.objects[k].type == 0x188) {
				_world.objects[k].type = a.baseObjFrame[i] & 0x3FF;
				_world.objects[k].frame = (uint8_t)(a.baseObjFrame[i] >> 10);
			}
			a.worktype[i] = 0x9B;
		} else {
			a.worktype[i] = 0x13;
		}
	}
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
