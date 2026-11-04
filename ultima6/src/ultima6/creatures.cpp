#include "ultima6/creatures.h"
#include "ultima6/game_tables.h"

#include <algorithm>
#include <cstdlib>

namespace Ultima6 {
namespace Ultima6 {

static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
// the distance table at ds:196C: how far a missile or blow must reach to cover (dx, dy)
static const uint8_t DIST[8][8] = {{0, 1, 2, 3, 4, 5, 6, 7}, {1, 1, 2, 3, 4, 5, 6, 7}, {2, 2, 2, 3, 4, 5, 6, 7}, {3, 3, 3, 4, 5, 6, 7, 7},
								   {4, 4, 4, 5, 6, 7, 7, 8}, {5, 5, 5, 6, 7, 7, 8, 8}, {6, 6, 6, 7, 7, 8, 8, 8}, {7, 7, 7, 7, 8, 8, 8, 8}};

// Arithmetic as the 16-bit code does it: every product and sum is a word
static int w16(int v) {
	return (int16_t)v;
}

static int posX(const World &w, int actor) {
	int k = w.actorObject(actor);
	return k >= 0 ? w.objects[k].x : 0;
}

static int posY(const World &w, int actor) {
	int k = w.actorObject(actor);
	return k >= 0 ? w.objects[k].y : 0;
}

void Creatures::scan() {
	_eggs.clear();
	for (size_t i = 0; i < _world.objects.size(); ++i) {
		const World::Obj &o = _world.objects[i];
		if (o.type == activeTables().eggType && o.onMap() && !o.deleted)
			_eggs.push_back((int)i);
	}
	_active.assign(_eggs.size(), false);
	_scanned = true;
}

int Creatures::count() const {
	int n = 0;
	for (int a = FIRST_TEMP_ACTOR; a < ObjList::ACTORS; ++a)
		n += !_world.actorFree(a);
	return n;
}

int Creatures::spawn(int type, int worktype, int x, int y, int z) {
	int actor = -1;
	for (int a = FIRST_TEMP_ACTOR; a < ObjList::ACTORS; ++a)
		if (_world.actorFree(a)) {
			actor = a;
			break;
		}
	if (actor < 0)
		return -1;
	// the square, or the nearest free one within a few squares
	MoveRules rules;
	rules.partyMayShuffle = false;
	int k = _world.addActor(actor, type, x, y, z);
	int mask = z == 0 ? 0x3FF : 0xFF;
	bool placed = false;
	for (int r = 0; r <= 3 && !placed; ++r)
		for (int dy = -r; dy <= r && !placed; ++dy)
			for (int dx = -r; dx <= r && !placed; ++dx) {
				if (std::max(std::abs(dx), std::abs(dy)) != r)
					continue;
				int nx = (x + dx) & mask, ny = (y + dy) & mask;
				if (r == 0 || _movement.check(nx, ny, z, k, rules).passable) {
					if (r != 0)
						_world.moveObject(k, nx, ny, z);
					placed = true;
				}
			}
	ObjList &a = _world.actors;
	const CreatureStats *s = Combat::stats(type);
	a.strength[actor] = (uint8_t)(s ? s->str : 10);
	a.dexterity[actor] = (uint8_t)(s ? s->dex : 10);
	a.intelligence[actor] = (uint8_t)(s ? s->intel : 10);
	a.hp[actor] = (uint8_t)(s ? s->hp : 30);
	a.level[actor] = (uint8_t)((a.hp[actor] + 29) / 30);
	a.exp[actor] = 100;
	a.worktype[actor] = (uint8_t)worktype;
	a.moves[actor] = a.dexterity[actor];
	a.status[actor] = (s && (s->align == CreatureStats::EVIL || s->align == CreatureStats::CHAOTIC)) ? 0x20 : 0; // attacks evil... i.e. hostile to the party
	a.talkFlags[actor] = 0;
	return actor;
}

// APPROX(19): not the original's exact behaviour, see docs/approximations.md
void Creatures::hatch(int egg, int hour) {
	const World::Obj &e = _world.objects[egg];
	int quality = e.quality;
	bool daytime = hour >= 6 && hour < 19;
	bool ok = quality < 10 || quality >= 30 || (quality < 20 && daytime) || (quality >= 20 && quality < 30 && !daytime);
	if (!ok)
		return;
	if (e.status & 0x40) // an egg that has been used up (sub_366B0 36D3C)
		return;
	int chance = (_rng.next() % 100) + 1;
	if (!(e.quantity == 100 || chance <= e.quantity))
		return;
	for (size_t i = 0; i < _world.objects.size(); ++i) {
		const World::Obj &o = _world.objects[i];
		if (o.deleted || o.location() != ObjRecord::IN_CONTAINER || o.parent != egg)
			continue;
		int qty = o.quantity ? o.quantity : 1;
		if (o.type == 413) // silver serpents hatch once per egg
			qty = 1;
		else if (e.quantity != 100)
			qty = (_rng.next() % qty) + 1;
		for (int n = 0; n < qty; ++n) {
			if (activeTables().u6Creatures ? (o.type >= FIRST_CREATURE || o.type == 98 || o.quality != 0) : Combat::stats(o.type) != nullptr)
{
				int actor = spawn(o.type, o.quality, e.x, e.y, e.z);
				// the egg's quality names the side of what hatches: quality % 10 - 1 is shifted into the status bits 0x60 (sub_366B0 36793-36941)
				if (actor >= 0 && quality % 10 != 0)
					_world.actors.status[actor] = (uint8_t)((_world.actors.status[actor] & 0x9F) | (((quality % 10) - 1) << 5));
			}
			else {
				// a plain object (reagents and the like grow where the egg is)
				World::Obj t;
				t.status = 0x01;
				t.x = e.x;
				t.y = e.y;
				t.z = e.z;
				t.type = o.type;
				t.frame = o.frame;
				t.quantity = 1;
				t.quality = 0;
				t.actor = -1;
				t.parent = -1;
				_world.objects.push_back(t);
				_world.addToMap((int)_world.objects.size() - 1);
			}
		}
	}
}

void Creatures::checkEggs(int px, int py, int pz, int hour) {
	if (!_scanned)
		scan();
	for (size_t n = 0; n < _eggs.size(); ++n) {
		const World::Obj &e = _world.objects[_eggs[n]];
		int dx = std::abs((int)e.x - px), dy = std::abs((int)e.y - py);
		if (_active[n] && (e.z != pz || dx >= SPAWN_RANGE || dy >= SPAWN_RANGE))
			_active[n] = false;
		if (dx < SPAWN_RANGE && dy < SPAWN_RANGE && e.z == pz && !_active[n]) {
			_active[n] = true;
			if (dx > 8 || dy > 8) // sub_36D4E: an egg within eight squares in both directions is left alone (creatures do not appear next to the party)
				hatch(_eggs[n], hour);
		}
	}
}

// APPROX(20): not the original's exact behaviour, see docs/approximations.md
// The party's companions fight back: each goes for the nearest hostile creature within 5 squares, and strikes it when next to it.
void Creatures::partyFights() {
	ObjList &a = _world.actors;
	for (int i = 1; i < a.partySize; ++i) {
		int member = a.partyRoster[i];
		int mk = _world.actorObject(member);
		if (mk < 0 || _world.objects[mk].deleted || a.hp[member] == 0)
			continue;
		const World::Obj &m = _world.objects[mk];
		int foe = -1, best = 1 << 20;
		for (int actor = FIRST_TEMP_ACTOR; actor < ObjList::ACTORS; ++actor) {
			int k = _world.actorObject(actor);
			if (k < 0 || _world.objects[k].deleted || a.hp[actor] == 0 || _world.objects[k].z != m.z)
				continue;
			const CreatureStats *s = Combat::stats(_world.objects[k].type);
			if (!s || !(s->align == CreatureStats::EVIL || s->align == CreatureStats::CHAOTIC))
				continue;
			int d = std::max(std::abs((int)_world.objects[k].x - (int)m.x), std::abs((int)_world.objects[k].y - (int)m.y));
			if (d < best) {
				best = d;
				foe = actor;
			}
		}
		if (foe < 0 || best > 5)
			continue;
		if (best <= 1) {
			_combat.attack(member, foe);
		} else {
			int fk = _world.actorObject(foe);
			_actors.walkToward(member, mk, _world.objects[fk].x, _world.objects[fk].y, m.z);
		}
	}
}

// Temporary actors that are far from the party disappear.
void Creatures::despawn(int px, int py, int pz) {
	ObjList &a = _world.actors;
	for (int actor = FIRST_TEMP_ACTOR; actor < ObjList::ACTORS; ++actor) {
		if (_world.actorFree(actor))
			continue;
		int k = _world.actorObject(actor);
		const World::Obj &o = _world.objects[k];
		if (o.z != pz || std::abs((int)o.x - px) > DESPAWN_RANGE || std::abs((int)o.y - py) > DESPAWN_RANGE) {
			_world.deleteObject(k);
			a.hp[actor] = 0;
		}
	}
}

// sub_29046: one step of an actor in a direction (0 north, clockwise, two for a quarter turn). The cost of leaving the square is paid whether the step works or not.
bool Creatures::stepDir(int actor, int dir) {
	int k = _world.actorObject(actor);
	const World::Obj o = _world.objects[k];
	dir &= 7;
	_world.spendMoves(actor, _world.moveCost(o.x, o.y, o.z));
	int mask = o.z == 0 ? 0x3FF : 0xFF;
	int nx = (o.x + DX[dir]) & mask, ny = (o.y + DY[dir]) & mask;
	MoveRules rules;
	rules.partyMayShuffle = _shuffle;
	if (!_movement.check(nx, ny, o.z, k, rules).passable)
		return false;
	_world.moveObject(k, nx, ny, o.z);
	_world.actors.movementFlags[actor] = (uint8_t)((_world.actors.movementFlags[actor] & ~7) | dir); // the way it faces
	_actors.face(actor, dir);
	return true;
}

// sub_291E0: a diagonal step between a vertical and a horizontal direction; one of the two squares it cuts across must be open as well as the square it ends on.
bool Creatures::stepDiag(int actor, int vertical, int horizontal) {
	int k = _world.actorObject(actor);
	const World::Obj o = _world.objects[k];
	if (o.type == 0x19D)
		return false;
	int dir = vertical == 0 && horizontal == 6 ? 7 : (vertical + horizontal) >> 1;
	_world.spendMoves(actor, _world.moveCost(o.x, o.y, o.z));
	int mask = o.z == 0 ? 0x3FF : 0xFF;
	int nx = (o.x + DX[dir]) & mask, ny = (o.y + DY[dir]) & mask;
	MoveRules rules;
	rules.partyMayShuffle = _shuffle;
	bool open = _movement.check(nx, o.y, o.z, k, rules).passable || _movement.check(o.x, ny, o.z, k, rules).passable;
	if (!open || !_movement.check(nx, ny, o.z, k, rules).passable)
		return false;
	_world.moveObject(k, nx, ny, o.z);
	_actors.face(actor, dir);
	return true;
}

// sub_29317: a step towards a square. The side to step to along each axis comes from the signs of the differences (a random one when it is zero); whether to try the
// vertical or the horizontal step first is chosen at random in proportion to the distances (near the target, by which is longer); then the straight steps, the
// diagonal one and finally, now and then, a step the other way are tried until one works. Every try costs the moves of leaving the square.
bool Creatures::stepToward(int actor, int tx, int ty) {
	int k = _world.actorObject(actor);
	const World::Obj o = _world.objects[k];
	int dx = tx - o.x, dy = ty - o.y;
	if ((dx == 0 && dy == 0) || _world.actors.worktype[actor] == 0x0E) {
		_world.spendMoves(actor, 5);
		return false;
	}
	int horizontal = dx == 0 ? (_rng.next() % 2 ? 2 : 6) : (dx < 0 ? 6 : 2);
	int vertical = dy == 0 ? (_rng.next() % 2 ? 0 : 4) : (dy < 0 ? 0 : 4);
	int adx = std::abs(dx), ady = std::abs(dy);
	bool verticalFirst;
	if (adx < 4 && ady < 4)
		verticalFirst = adx < ady ? true : (adx == ady ? (_rng.next() % 2) != 0 : false);
	else
		verticalFirst = (1 + (int)(_rng.next() % (adx + ady))) > adx;
	bool moved;
	if (verticalFirst) {
		moved = stepDir(actor, vertical) || stepDiag(actor, vertical, horizontal) || stepDir(actor, horizontal);
		if (!moved && _rng.next() % 2 == 0)
			moved = stepDir(actor, horizontal ^ 4);
	} else {
		moved = stepDir(actor, horizontal) || stepDiag(actor, vertical, horizontal) || stepDir(actor, vertical);
		if (!moved && _rng.next() % 2 == 0)
			moved = stepDir(actor, vertical ^ 4);
	}
	if (!moved)
		_world.spendMoves(actor, _world.moveCost(o.x, o.y, o.z));
	return moved;
}

// wt_0x99 (2DD5E): the nearest actor of the other side within 8 squares (squared distance, ties by chance), or -1. The sides are the status bits 0x60: 0x40 the party's
// kind and 0x20 the monsters'; an actor with neither only fights the party when its worktype is 0x9B; retreating worktypes (7, 9, 0x13, 0x99, 0x9A) take anyone.
// With `strongest` (wt_combat_berserk, 2EC2A) the choice among the actors within eight squares is the one whose strength + dexterity + intelligence is the greatest, and an actor
// without a side fights anybody.
int Creatures::pickTarget(int actor, bool strongest) {
	ObjList &a = _world.actors;
	int k = _world.actorObject(actor);
	const World::Obj me = _world.objects[k];
	int side = a.status[actor] & 0x60, wt = a.worktype[actor];
	int best = -1, bestDist = 0x7FFF;
	for (int t = 0; t < ObjList::ACTORS; ++t) {
		int tk = _world.actorObject(t);
		if (t == actor || tk < 0 || _world.objects[tk].deleted || (a.status[t] & 0x10) || _world.objects[tk].z != me.z)
			continue;
		int ts = a.status[t] & 0x60;
		bool take = wt == 9 || wt == 0x9A || wt == 0x13 || wt == 7 || wt == 0x99;
		if (!take) {
			if (side == 0)
				take = wt == 0x9B && ts == 0x40;
			else if (side == 0x60 && ts == 0x60)
				take = false;
			else if (!ts)
				take = false;
			else if (side == 0x40 && !(ts & 0x20))
				take = false;
			else if (side == 0x20 && !(ts & 0x40))
				take = false;
			else
				take = true;
		}
		if (!take)
			continue;
		const World::Obj &o = _world.objects[tk];
		int ddx = std::abs((int)o.x - (int)me.x), ddy = std::abs((int)o.y - (int)me.y);
		if (std::max(ddx, ddy) > 8)
			continue;
		int d = ddx * ddx + ddy * ddy;
		if (strongest) {
			int sum = a.strength[t] + a.dexterity[t] + a.intelligence[t];
			if (sum > bestDist || best < 0) {
				bestDist = sum;
				best = t;
			}
			continue;
		}
		if (d < bestDist || (d == bestDist && _rng.next() % 2)) {
			bestDist = d;
			best = t;
		}
	}
	return best;
}

// One action of a creature the turn scheduler has picked (actor_worktype_act, 29BDA): wt_immobile (0xE), wt_wander (0xC, 0x8F) and wt_combat_assault (the rest here, 2DF2E):
// a target of the other side is fought when it is in reach (the distance table at ds:196C against the weapon's range, 10 moves for a blow) and approached otherwise;
// with no target the creature wanders, or (worktype 3) passes. APPROX(57): the line-of-fire test (sub_2BB2F) and the side step it causes are left out.
void Creatures::act(int actor, int px, int py, int pz) {
	(void)pz;
	ObjList &a = _world.actors;
	int k = _world.actorObject(actor);
	if (k < 0 || _world.objects[k].deleted || a.hp[actor] == 0) {
		a.moves[actor] = 0;
		return;
	}
	// actor_worktype_act (29BDA) at its end: an action that used up no moves costs 10
	_shuffle = false;
	int before = (int8_t)a.moves[actor];
	turn(actor, px, py);
	if ((int8_t)a.moves[actor] == before)
		_world.spendMoves(actor, 10);
}

void Creatures::turn(int actor, int px, int py) {
	ObjList &a = _world.actors;
	const World::Obj o = _world.objects[_world.actorObject(actor)];
	int wt = a.worktype[actor];
	// the guards (0xF, 0x10, 0x8B-0x8E, actor_worktype_act 29ED9): half the turns are passed; otherwise a step in the direction the guard faces (at its post a guard of 0x8B-0x8E
	// goes the way its worktype says: north, east, south, west) and, if that is blocked, the opposite way
	if (wt == 0x0F || wt == 0x10 || (wt >= 0x8B && wt <= 0x8E)) {
		if (_rng.next() % 2 == 0) {
			_world.spendMoves(actor, 5);
			return;
		}
		int dir = a.movementFlags[actor] & 7;
		if (wt >= 0x8B) {
			const Schedule &sc = _world.schedule;
			Schedule::Entry e = sc.entry(sc.index[actor] + a.scheduleIndex[actor]);
			if (o.x == e.x && o.y == e.y)
				dir = (wt - 0x8B) * 2;
		}
		if (!stepDir(actor, dir))
			stepDir(actor, dir ^ 4);
		return;
	}
	// wt_work_12 (29740, worktype 0x12): the member of the party nearest to the person is walked up to; once next to one the person goes back to its schedule (sub_298D0: the arrest
	// it would otherwise make depends on byte_48B40, which nothing sets)
	if (wt == 0x12) {
		if (workApproach(actor) >= 0)
			a.worktype[actor] = 0x81;
		return;
	}
	// the hourly bell (0x98) sounds when the clock has struck; nothing else is done
	if (wt == 0x98) {
		_world.spendMoves(actor, 5);
		return;
	}
	// worktypes 0x96 and 0x97 with the player within 4 squares: the person walks up to the nearest member (wt_work_12) and, next to one, speaks to the party (sub_29B78 / sub_1EBEC) and goes back to work (0x90)
	if ((wt == 0x96 || wt == 0x97) && std::abs((int)o.x - px) < 5 && std::abs((int)o.y - py) < 5) {
		if (workApproach(actor) >= 0) {
			a.worktype[actor] = 0x90;
			_talkRequest = actor;
		}
		return;
	}
	if (wt == 0x90 || wt == 0x94 || wt == 0x96 || wt == 0x97) {
		work(actor);
		return;
	}
	// worktype 0x99: three turns in four the person works; otherwise it goes for the nearest foe within five squares of its place, waking one next to it (sub_2C20F) and going back to its place
	if (wt == 0x99) {
		const Schedule &sc = _world.schedule;
		Schedule::Entry e = sc.entry(sc.index[actor] + a.scheduleIndex[actor]);
		if (_rng.next() % 4 != 0) {
			work(actor);
			return;
		}
		int foe = pickTarget(actor);
		if (foe < 0) {
			work(actor);
			return;
		}
		const World::Obj &t = _world.objects[_world.actorObject(foe)];
		if (std::max(std::abs((int)t.x - e.x), std::abs((int)t.y - e.y)) >= 5) {
			work(actor);
			return;
		}
		if (std::max(std::abs((int)t.x - (int)o.x), std::abs((int)t.y - (int)o.y)) < 2) {
			wake(foe);
			stepToward(actor, e.x, e.y);
		} else {
			stepToward(actor, t.x, t.y);
		}
		return;
	}
	// wt_tangle (2F4E6, 0xD): a vine strikes a target within one square one time in two (10 moves); APPROX(67): what it does further away is not traced
	if (wt == 0x0D) {
		int foe = pickTarget(actor);
		if (foe >= 0) {
			const World::Obj &t = _world.objects[_world.actorObject(foe)];
			if (std::abs((int)t.x - (int)o.x) < 2 && std::abs((int)t.y - (int)o.y) < 2 && _rng.next() % 2) {
				_combat.attack(actor, foe);
				_world.spendMoves(actor, 10);
				return;
			}
		}
		_world.spendMoves(actor, 5);
		return;
	}
	if (wt == 0x0E) { // wt_immobile: five moves passed (its blows at enemies in reach are not done)
		_world.spendMoves(actor, 5);
		return;
	}
	// wt_combat_retreat (2EEC2, worktypes 7, 9, 0x13, 0x9A): too far from the leader (more than 7) it stops; otherwise it keeps away from the nearest other actor (the point
	// opposite to it) when that one is within 8 squares (worktype 7) or 3 (the others), fighting back when cornered (except 9 and 0x9A), or wanders; a retreating (7) creature
	// recovers a hit point one turn in four and returns to its combat worktype once it has a quarter of its hit points
	if (wt == 7 || wt == 9 || wt == 0x13 || wt == 0x9A) {
		if (std::max(std::abs((int)o.x - px), std::abs((int)o.y - py)) > 7) {
			a.worktype[actor] = 0;
			return;
		}
		int foe = pickTarget(actor);
		if (foe < 0) {
			if (_rng.next() % 8 == 0)
				stepDir(actor, (_rng.next() % 4) * 2);
			else
				_world.spendMoves(actor, 5);
		} else {
			const World::Obj &t = _world.objects[_world.actorObject(foe)];
			int dx = (int)t.x - (int)o.x, dy = (int)t.y - (int)o.y, range = wt == 7 ? 8 : 3;
			if (std::abs(dx) < range && std::abs(dy) < range) {
				if (!stepToward(actor, o.x - dx, o.y - dy) && wt != 9 && wt != 0x9A && std::max(std::abs(dx), std::abs(dy)) <= 1) {
					_combat.attack(actor, foe);
					_world.spendMoves(actor, 10);
				}
			} else if ((a.status[actor] & 0x60) == 0x40 && wt != 9 && wt != 0x9A) {
				stepToward(actor, px, py);
			} else {
				_world.spendMoves(actor, 5);
			}
		}
		if (wt == 7 && _rng.next() % 4 == 0) {
			int most = std::max(1, std::min(255, a.level[actor] * 30));
			if (most > a.hp[actor])
				++a.hp[actor];
			if (a.hp[actor] * 4 / most > 0)
				a.worktype[actor] = a.unknown12f1[actor] ? a.unknown12f1[actor] : 8;
		}
		return;
	}
	// wt_combat_like (2F06F, 0xA) and wt_combat_unfriendly (2F1A7, 0xB): with a party member less than three squares away in both directions the actor comes towards the party
	// (like) or, half the time, fights (unfriendly); otherwise half of the turns are a walk towards a spot ten squares off along a random axis and half five passed moves
	if (wt == 0x0A || wt == 0x0B) {
		bool near = false;
		for (int m = 0; m < a.partySize; ++m) {
			int mk = _world.actorObject(a.partyRoster[m]);
			if (mk >= 0 && std::abs((int)_world.objects[mk].x - (int)o.x) < 3 && std::abs((int)_world.objects[mk].y - (int)o.y) < 3)
				near = true;
		}
		if (near && _rng.next() % 2 == 0) {
			if (wt == 0x0A) {
				stepToward(actor, px, py);
				return;
			}
			wt = 8; // unfriendly: wt_combat_assault
		} else {
			if (_rng.next() % 2) {
				int tx = o.x, ty = o.y;
				int off = _rng.next() % 2 ? 10 : -10;
				if (_rng.next() % 2)
					tx += off;
				else
					ty += off;
				stepToward(actor, tx, ty);
			} else {
				_world.spendMoves(actor, 5);
			}
			return;
		}
	}
	if (wt == 3)
		combatFront(actor, px, py);
	else if (wt == 4)
		combatRear(actor, px, py);
	else if (wt == 5)
		combatFlank(actor, px, py);
	else
		assault(actor, px, py);
}

// wt_combat_assault (2DF2E): see Creatures::act; the worktypes that are not handled elsewhere end here
void Creatures::assault(int actor, int px, int py) {
	ObjList &a = _world.actors;
	const World::Obj o = _world.objects[_world.actorObject(actor)];
	int wt = a.worktype[actor];
	int target = wt == 0x0C || wt == 0x8F ? -1 : pickTarget(actor, wt == 6);
	if (target >= 0) {
		const World::Obj &t = _world.objects[_world.actorObject(target)];
		int dx = std::abs((int)t.x - (int)o.x), dy = std::abs((int)t.y - (int)o.y);
		int range = Combat::rangeOf(_combat.weaponType(actor)), gx = t.x, gy = t.y;
		if (dx < 8 && dy < 8 && DIST[dx][dy] <= range) {
			if (lineOfFire(actor, t.x, t.y, range)) {
				_combat.attack(actor, target);
				_world.spendMoves(actor, 10);
				return;
			}
			// no clear shot: a quarter turn about the target, the way chosen by chance
			if (_rng.next() % 2) {
				gx = o.x - ((int)t.y - o.y);
				gy = o.y + ((int)t.x - o.x);
			} else {
				gx = ((int)t.y - o.y) + o.x;
				gy = o.y - ((int)t.x - o.x);
			}
		}
		if (wt != 3 || (dx < 8 && dy < 8 && DIST[dx][dy] <= range))
			stepToward(actor, gx, gy);
		else
			_world.spendMoves(actor, 5);
		return;
	}
	if (wt == 3) {
		_world.spendMoves(actor, 5);
		return;
	}
	wander(actor, px, py);
}

// wt_work (29134): one turn in eight a step, otherwise five moves passed; the step is along the axis the worker is further off its place on (a signed random number,
// from sub_28F37, decides between the two ways) so that it hovers about its schedule place
void Creatures::work(int actor) {
	ObjList &a = _world.actors;
	const World::Obj o = _world.objects[_world.actorObject(actor)];
	if (_rng.next() % 8 != 0) {
		_world.spendMoves(actor, 5);
		return;
	}
	const Schedule &sc = _world.schedule;
	Schedule::Entry e = sc.entry(sc.index[actor] + a.scheduleIndex[actor]);
	auto laplace = [&]() {
		int n = 0;
		while (_rng.next() % 2)
			++n;
		return _rng.next() % 2 ? -n : n;
	};
	int dx = (int)o.x - e.x, dy = (int)o.y - e.y, dir;
	if (std::abs(dy) - std::abs(dx) >= laplace())
		dir = laplace() > dy ? 4 : 0;
	else
		dir = laplace() > dx ? 2 : 6;
	stepDir(actor, dir);
}

// wt_work_12 (29740): the party member nearest the person (a squared distance under 50, ties by chance) that it can see is walked up to; its number once the person is next to it, else -1
int Creatures::workApproach(int actor) {
	ObjList &a = _world.actors;
	int cx = posX(_world, actor), cy = posY(_world, actor), best = 50, found = -1;
	for (int m = 0; m < a.partySize; ++m) {
		int member = a.partyRoster[m], k = _world.actorObject(member);
		if (k < 0 || _world.objects[k].deleted || (a.status[member] & 0x10) || !validTarget(actor, member))
			continue;
		int dx = _world.objects[k].x - cx, dy = _world.objects[k].y - cy, d = w16(w16(dx * dx) + w16(dy * dy));
		if (d < best || (d == best && _rng.next() % 2)) {
			best = d;
			found = member;
		}
	}
	if (found < 0) {
		stepToward(actor, _f.partyX, _f.partyY);
		return -1;
	}
	stepToward(actor, posX(_world, found), posY(_world, found));
	if (std::abs(posX(_world, found) - posX(_world, actor)) > 1 || std::abs(posY(_world, found) - posY(_world, actor)) > 1)
		return -1;
	return found;
}

// sub_2C20F: a sleeper wakes (its own sprite comes back); the one it is said to be about to be revealed to is noticed (movement flag 8) from the second time on
void Creatures::wake(int actor) {
	ObjList &a = _world.actors;
	int k = _world.actorObject(actor);
	if (k < 0)
		return;
	if (_revealed && actor < 0x100)
		a.movementFlags[actor] |= 8;
	if (a.status[actor] & 0x04) {
		a.status[actor] &= ~0x04;
		_world.objects[k].type = a.baseObjFrame[actor] & 0x3FF;
		_world.objects[k].frame = (uint8_t)(a.baseObjFrame[actor] >> 10);
	}
	_revealed = true;
}

// sub_2C2E5 2C576-2C632 with sub_2C14F: a slime (movement class 0x30) that is hurt and lives divides: on one of up to eight random squares next to it (each try picks a square
// within one of it, its own square included) that it may enter, a copy of it (all its numbers, hit points as they are now) appears: "Slime divides!"
void Creatures::divide(int actor) {
	ObjList &a = _world.actors;
	int k = _world.actorObject(actor);
	if (k < 0 || actor >= 0x100 || MovementClasses::classOf(_world.objects[k].type) != 0x30)
		return;
	const World::Obj o = _world.objects[k];
	MoveRules rules;
	rules.partyMayShuffle = false;
	int mask = o.z == 0 ? 0x3FF : 0xFF;
	for (int tries = 0; tries < 8; ++tries) {
		int nx = (o.x + (int)(_rng.next() % 3) - 1) & mask, ny = (o.y + (int)(_rng.next() % 3) - 1) & mask;
		if (!_movement.check(nx, ny, o.z, k, rules).passable)
			continue;
		int n = spawn(o.type, a.worktype[actor], nx, ny, o.z);
		if (n < 0)
			return;
		int kn = _world.actorObject(n);
		_world.objects[kn].status = _world.objects[k].status;
		if (o.type != 0x177)
			_world.objects[kn].frame = _world.objects[k].frame;
		_world.objects[kn].quantity = _world.objects[k].quantity;
		_world.objects[kn].quality = _world.objects[k].quality;
		a.status[n] = a.status[actor];
		a.strength[n] = a.strength[actor];
		a.dexterity[n] = a.dexterity[actor];
		a.intelligence[n] = a.intelligence[actor];
		a.level[n] = a.level[actor];
		a.exp[n] = a.exp[actor];
		a.hp[n] = a.hp[actor];
		a.worktype[n] = a.worktype[actor];
		a.unknown12f1[n] = a.unknown12f1[actor];
		a.unknown13f1[n] = a.unknown13f1[actor];
		a.scheduleIndex[n] = a.scheduleIndex[actor];
		a.unknown18f1[n] = a.unknown18f1[actor];
		a.movementFlags[n] = a.movementFlags[actor];
		a.moves[n] = 0;
		_scroll.print("Slime divides!\n");
		return;
	}
}

// wt_wander (2954B)
void Creatures::wander(int actor, int px, int py) {
	ObjList &a = _world.actors;
	const World::Obj o = _world.objects[_world.actorObject(actor)];
	// wt_wander (2954B): a party member that is not on the leader's square walks towards the leader one turn in four
	if ((a.status[actor] & 0x80) && !(o.x == px && o.y == py) && _rng.next() % 4 == 0) {
		stepToward(actor, px, py);
		return;
	}
	// ... otherwise one turn in eight a step in a random one of the four directions, five moves passed else
	if (_rng.next() % 8 == 0)
		stepDir(actor, (_rng.next() % 4) * 2);
	else
		_world.spendMoves(actor, 5);
}

// sub_19FFD: the first living actor (from 1) standing on a square of the party's level, or -1
int Creatures::actorAt(int x, int y) const {
	for (int i = 1; i < ObjList::ACTORS; ++i) {
		int k = _world.actorObject(i);
		if (k < 0 || _world.objects[k].deleted || (_world.actors.status[i] & 0x10))
			continue;
		const World::Obj &o = _world.objects[k];
		if (o.x == x && o.y == y && o.z == _fz)
			return i;
	}
	return -1;
}

// sub_2B827: whether an actor can be a target of another: not invisible (but party members see each other), not dead, not the hidden kind (type 0x165, tile flags3 bit 0x10)
// and on the same level
bool Creatures::validTarget(int actor, int target) const {
	const ObjList &a = _world.actors;
	int ka = _world.actorObject(actor), kt = _world.actorObject(target);
	if (ka < 0 || kt < 0)
		return false;
	const World::Obj &t = _world.objects[kt];
	bool ok = !(t.status & 2);
	if ((a.status[actor] & 0x80) && !ok && target < 0x100 && (a.status[target] & 0x80))
		ok = true;
	if (target < 0x100 && (a.movementFlags[target] & 0x10))
		ok = false;
	if (t.type == 0x165 && t.frame == 0)
		ok = false;
	if (_world.flags.flags3[(_world.baseTile.firstTile[t.type] + t.frame) & (TileFlags::TILES - 1)] & 0x10)
		ok = false;
	if (_world.objects[ka].z != t.z)
		ok = false;
	return ok;
}

// sub_2BB2F: whether a missile flies from the actor to a square. APPROX(57): sub_37CBD flies it with its own Bresenham and may change the square it hits; here it is the
// pixel line of World::boltPath
bool Creatures::lineOfFire(int actor, int tx, int ty, int range) const {
	const World::Obj &o = _world.objects[_world.actorObject(actor)];
	std::vector<std::pair<int, int>> path;
	if (_world.boltPath(o.z, o.x, o.y, tx, ty, range, path))
		return true;
	// a target standing on a square that blocks is still struck from the square before it
	int lx = path.empty() ? o.x : path.back().first, ly = path.empty() ? o.y : path.back().second;
	return std::max(std::abs(lx - tx), std::abs(ly - ty)) <= 1;
}

// sub_2A4B6, once at the start of every round of turns: the party's centre (leaving out the members on worktypes 5 and 6) and its enemies' (the other side's actors within 24
// squares, those retreating (7) only within 5), the actor of each side that lies furthest ahead and furthest to either side (only the actors in the front, worktypes 2 and 3,
// count), and the first such actor of the other side.
void Creatures::formation(int px, int py, int pz) {
	ObjList &a = _world.actors;
	_fz = pz;
	_winX = (px - 16) & 0x3F8;
	_winY = (py - 16) & 0x3F8;
	auto X = [&](int i) { return posX(_world, i); };
	auto Y = [&](int i) { return posY(_world, i); };
	auto alive = [&](int i) {
		int k = _world.actorObject(i);
		return k >= 0 && !_world.objects[k].deleted;
	};
	int n = 0;
	long sumX = 0, sumY = 0;
	for (int m = 0; m < a.partySize; ++m) {
		int actor = a.partyRoster[m];
		if (!alive(actor) || a.worktype[actor] == 5 || a.worktype[actor] == 6)
			continue;
		++n;
		sumX += X(actor);
		sumY += Y(actor);
	}
	if (n) {
		_f.partyX = (int)((sumX + (a.partySize >> 1)) / n);
		_f.partyY = (int)((sumY + (a.partySize >> 1)) / n);
	} else {
		_f.partyX = px;
		_f.partyY = py;
	}
	n = 0;
	sumX = sumY = 0;
	for (int i = 1; i < ObjList::ACTORS; ++i) {
		if (!alive(i) || (a.status[i] & 0x10) || !(a.status[i] & 0x20))
			continue;
		int x = X(i), y = Y(i);
		if (a.worktype[i] == 7 && (std::abs(x - px) > 5 || std::abs(y - py) > 5))
			continue;
		if (std::abs(x - px) >= 24 || std::abs(y - py) >= 24)
			continue;
		++n;
		sumX += x;
		sumY += y;
	}
	_f.enemies = n;
	int facing = a.movementFlags[a.partyRoster[0]] & 7;
	if (n) {
		_f.enemyX = (int)((sumX + (n >> 1)) / n);
		_f.enemyY = (int)((sumY + (n >> 1)) / n);
		int vx = _f.enemyX - _f.partyX, vy = _f.enemyY - _f.partyY;
		int front = -0x8000, right = 0, left = 0, enemyRight = 0, enemyLeft = 0;
		_f.enemyRight = _f.enemyLeft = _f.lead = -1;
		for (int i = 0; i < ObjList::ACTORS; ++i) {
			if (!alive(i) || (a.status[i] & 0x10) || (a.worktype[i] != 3 && a.worktype[i] != 2))
				continue;
			int dx = X(i) - _f.partyX, dy = Y(i) - _f.partyY;
			int ahead = w16(w16(dx * vx) + w16(dy * vy)), across = w16(w16(dx * vy) - w16(dy * vx));
			if (a.status[i] & 0x80) {
				if (ahead > front) {
					front = ahead;
					_f.partyFront = i;
				}
				if (across > right) {
					right = across;
					_f.partyRight = i;
				}
				if (across < left) {
					left = across;
					_f.partyLeft = i;
				}
			} else {
				if (_f.lead < 0)
					_f.lead = i;
				if (across > enemyRight) {
					enemyRight = across;
					_f.enemyRight = i;
				}
				if (across < enemyLeft) {
					enemyLeft = across;
					_f.enemyLeft = i;
				}
			}
		}
	} else {
		_f.enemyX = px + DX[facing];
		_f.enemyY = py + DY[facing];
	}
	if (_f.enemyX == _f.partyX && _f.enemyY == _f.partyY) {
		_f.enemyX += DX[facing];
		_f.enemyY += DY[facing];
	}
}

// sub_2E0FE: the actor goes to its place in the line. The place is on the line across the way from the party's centre (from the leader, for a party member) to the enemies'
// (to the enemy that leads, for an enemy), as far along it as the leader of that line stands, and to the side the actor is already on; a place taken by someone else is
// shifted along the line, to whichever side lies nearer. Returns false when the actor has acted, true when it has not (it passed, or is in its place or the place lies
// outside the 40 x 40 squares), and then the caller goes on to fight.
bool Creatures::formationWalk(int actor, int px, int py) {
	ObjList &a = _world.actors;
	bool party = (a.status[actor] & 0x80) != 0;
	if (!(_f.enemies && _f.lead != 0) && !party) {
		_world.spendMoves(actor, 5);
		return true;
	}
	if (!party && _f.lead < 0) { // the original reads past the table here
		_world.spendMoves(actor, 5);
		return true;
	}
	auto X = [&](int i) { return posX(_world, i); };
	auto Y = [&](int i) { return posY(_world, i); };
	int cx = X(actor), cy = Y(actor);
	int ref, p0x, p0y, vx, vy;
	if (party) {
		ref = a.partyRoster[0];
		p0x = X(ref);
		p0y = Y(ref);
		vx = w16(_f.enemyX - p0x);
		vy = w16(_f.enemyY - p0y);
	} else {
		ref = _f.lead;
		p0x = _f.partyX;
		p0y = _f.partyY;
		vx = w16(X(ref) - p0x);
		vy = w16(Y(ref) - p0y);
	}
	int cross = w16(w16((cx - p0x) * vy) - w16((cy - p0y) * vx));
	bool onLeft = cross > 0;
	if (cross == 0)
		onLeft = _rng.next() % 2 != 0;
	int stepX = vy > 0 ? 1 : -1, stepY = vx > 0 ? -1 : 1;
	if (!onLeft) {
		stepX = -stepX;
		stepY = -stepY;
	}
	int along = w16(w16((X(ref) - p0x) * vx) + w16((Y(ref) - p0y) * vy));
	if (a.partyRoster[0] == ref)
		along = w16(along + std::abs(vx) + std::abs(vy));
	int len2 = w16(w16(vx * vx) + w16(vy * vy));
	if (len2 == 0)
		len2 = 1;
	int tx = w16(w16(along * vx) / len2) + p0x, ty = w16(w16(along * vy) / len2) + p0y;
	_shuffle = false;
	for (int guard = 0; guard < 400; ++guard) {
		if (tx < _winX || _winX + 0x27 < tx || ty < _winY || _winY + 0x27 < ty || (cx == tx && cy == ty)) {
			_shuffle = true;
			return true;
		}
		if (actorAt(tx, ty) < 0)
			break;
		int ax = w16(w16((tx + stepX - p0x) * vx) + w16((ty - p0y) * vy));
		int ay = w16(w16((tx - p0x) * vx) + w16((ty + stepY - p0y) * vy));
		if (std::abs(w16(ax - along)) < std::abs(w16(ay - along)))
			tx += stepX;
		else
			ty += stepY;
	}
	_shuffle = true;
	int saved = (int8_t)a.moves[actor];
	if (!stepToward(actor, tx, ty)) {
		a.moves[actor] = (uint8_t)saved;
		assault(actor, px, py);
		return false;
	}
	if (party && !(X(actor) == tx && Y(actor) == ty)) {
		if (_f.enemies == 0)
			a.moves[actor] = (uint8_t)saved;
		if (!stepToward(actor, tx, ty))
			assault(actor, px, py);
	}
	return false;
}

// wt_combat_front (2E454): the actor takes its place in the line; once there an enemy 8 or more squares from the party waits, the enemy that leads now and then (one turn in
// eight) goes for the party's centre, and everyone else fights as wt_combat_assault does
void Creatures::combatFront(int actor, int px, int py) {
	ObjList &a = _world.actors;
	if (!formationWalk(actor, px, py))
		return;
	if (!(a.status[actor] & 0x80)) {
		const World::Obj &o = _world.objects[_world.actorObject(actor)];
		if (std::max(std::abs((int)o.x - _f.partyX), std::abs((int)o.y - _f.partyY)) >= 8) {
			_world.spendMoves(actor, 5);
			return;
		}
		if (actor == _f.lead && _rng.next() % 8 == 0) {
			stepToward(actor, _f.partyX, _f.partyY);
			return;
		}
	}
	assault(actor, px, py);
}

// wt_combat_rear (2E4B9): the actor keeps behind the front line of its side (the front-line actors, worktype 3, of the same side): behind the foremost of them it moves
// along the line to the side the actor is not yet past the flank actors on; otherwise it steps back; a party member more than 3 squares from the player comes to the player.
// When it cannot move it fights.
void Creatures::combatRear(int actor, int px, int py) {
	ObjList &a = _world.actors;
	bool party = (a.status[actor] & 0x80) != 0;
	auto X = [&](int i) { return posX(_world, i); };
	auto Y = [&](int i) { return posY(_world, i); };
	int ref1, ref2, bx, by, vx, vy;
	if (party) {
		if (_f.enemies == 0) {
			stepToward(actor, px, py);
			return;
		}
		ref1 = _f.partyRight;
		ref2 = _f.partyLeft;
		bx = _f.partyX;
		by = _f.partyY;
		vx = _f.enemyX - _f.partyX;
		vy = _f.enemyY - _f.partyY;
	} else {
		if (_f.enemies == 0) {
			_world.spendMoves(actor, 5);
			return;
		}
		ref1 = _f.enemyLeft < 0 ? actor : _f.enemyLeft;
		ref2 = _f.enemyRight < 0 ? actor : _f.enemyRight;
		bx = _f.enemyX;
		by = _f.enemyY;
		vx = _f.partyX - _f.enemyX;
		vy = _f.partyY - _f.enemyY;
	}
	int foremost = 0x7FFF, side = a.status[actor] & 0x60;
	for (int i = 0; i < ObjList::ACTORS; ++i) {
		int k = _world.actorObject(i);
		if (k < 0 || _world.objects[k].deleted || (a.status[i] & 0x10) || a.worktype[i] != 3 || (a.status[i] & 0x60) != side)
			continue;
		int d = w16(w16((X(i) - bx) * vx) + w16((Y(i) - by) * vy));
		if (d < foremost)
			foremost = d;
	}
	int ax = X(actor), ay = Y(actor);
	int mine = w16(w16((ax - bx) * vx) + w16((ay - by) * vy));
	int saved = (int8_t)a.moves[actor];
	bool failed;
	if (party && std::max(std::abs(ax - px), std::abs(ay - py)) > 3) {
		failed = !stepToward(actor, px, py);
	} else if (mine >= foremost) {
		failed = !stepToward(actor, ax - vx, ay - vy);
	} else {
		int across = w16(w16((ax - bx) * vy) - w16((ay - by) * vx));
		int across1 = w16(w16((X(ref1) - bx) * vy) - w16((Y(ref1) - by) * vx));
		int across2 = w16(w16((X(ref2) - bx) * vy) - w16((Y(ref2) - by) * vx));
		if (across1 < across)
			failed = !stepToward(actor, ax - vy, ay + vx);
		else if (across2 > across)
			failed = !stepToward(actor, ax + vy, ay - vx);
		else
			failed = true;
	}
	if (failed) {
		a.moves[actor] = (uint8_t)saved;
		assault(actor, px, py);
	}
}

// wt_combat_flank (2E7E2): with enemies about (and, for an enemy, the player within 7 squares) the actor picks the foe that lies furthest to the side it is on of the line
// between the two sides (a foe must be one it can see, of the other side and near: within 7 of the player or within 5 of the actor; the retreating kinds only within 5 of the
// player), shoots it when it is in reach and the line of fire is clear, turns a quarter turn about it when it is not, and otherwise comes round at the side. Without
// enemies a party member only comes to within 2 squares of the player.
void Creatures::combatFlank(int actor, int px, int py) {
	ObjList &a = _world.actors;
	bool party = (a.status[actor] & 0x80) != 0;
	auto X = [&](int i) { return posX(_world, i); };
	auto Y = [&](int i) { return posY(_world, i); };
	int ax = X(actor), ay = Y(actor);
	if (_f.enemies == 0 || (!party && std::max(std::abs(ax - px), std::abs(ay - py)) > 7)) {
		int saved = (int8_t)a.moves[actor];
		if (party && std::max(std::abs(ax - px), std::abs(ay - py)) > 2)
			stepToward(actor, px, py);
		a.moves[actor] = (uint8_t)saved;
		_world.spendMoves(actor, 5);
		return;
	}
	int mySide = a.status[actor] & 0x60;
	int vx = _f.enemyX - _f.partyX, vy = _f.enemyY - _f.partyY;
	int cross = w16(w16((ax - _f.partyX) * vy) - w16((ay - _f.partyY) * vx));
	bool onLeft = cross > 0;
	if (cross == 0)
		onLeft = _rng.next() % 2 != 0;
	int sideX, sideY;
	if (onLeft) {
		sideX = vy;
		sideY = -vx;
	} else {
		sideX = -vy;
		sideY = vx;
		cross = -cross;
	}
	int best = -0x8000, foe = -1;
	for (int i = 0; i < ObjList::ACTORS; ++i) {
		int k = _world.actorObject(i);
		if (k < 0 || _world.objects[k].deleted || (a.status[i] & 0x10) || (a.status[i] & 0x60) == mySide || !validTarget(actor, i))
			continue;
		if (mySide == 0x40 && !(a.status[i] & 0x20))
			continue;
		if (mySide == 0x20 && !(a.status[i] & 0x40))
			continue;
		int tx = X(i), ty = Y(i), wt = a.worktype[i];
		if ((wt == 7 || wt == 9 || wt == 0x9A) && (tx < px - 5 || tx > px + 5 || ty < py - 5 || ty > py + 5))
			continue;
		if (!(std::abs(tx - px) <= 7 && std::abs(ty - py) <= 7) && (std::abs(tx - ax) > 5 || std::abs(ty - ay) > 5))
			continue;
		int c = w16(w16((tx - _f.partyX) * vy) - w16((ty - _f.partyY) * vx));
		if (!onLeft)
			c = -c;
		if (c > best) {
			best = c;
			foe = i;
		}
	}
	if (foe < 0) {
		wander(actor, px, py);
		return;
	}
	int tx = X(foe), ty = Y(foe);
	int weapon = _combat.weaponType(actor), range = Combat::rangeOf(weapon);
	int adx = std::abs(tx - ax), ady = std::abs(ty - ay);
	if (adx < 8 && ady < 8 && DIST[adx][ady] <= range) {
		if (lineOfFire(actor, tx, ty, range)) {
			_combat.attack(actor, foe);
			_world.spendMoves(actor, 10);
			return;
		}
		// no clear shot: a quarter turn about the foe, the way chosen by chance
		int gx, gy;
		if (_rng.next() % 2) {
			gx = ax - (ty - ay);
			gy = ay + (tx - ax);
		} else {
			gx = (ty - ay) + ax;
			gy = ay - (tx - ax);
		}
		tx = gx;
		ty = gy;
	} else {
		tx += sideX > 0 ? 1 : (sideX < 0 ? -1 : 0);
		ty += sideY > 0 ? 1 : (sideY < 0 ? -1 : 0);
	}
	int saved = (int8_t)a.moves[actor];
	if (!stepToward(actor, tx, ty)) {
		a.moves[actor] = (uint8_t)saved;
		assault(actor, px, py);
	}
}

} // namespace Ultima6
} // namespace Ultima6
