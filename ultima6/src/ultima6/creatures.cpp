#include "ultima6/creatures.h"

#include <algorithm>
#include <cstdlib>

namespace Ultima6 {
namespace Ultima6 {

static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

void Creatures::scan() {
	_eggs.clear();
	for (size_t i = 0; i < _world.objects.size(); ++i) {
		const World::Obj &o = _world.objects[i];
		if (o.type == EGG && o.onMap() && !o.deleted)
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
			if (o.type >= FIRST_CREATURE || o.type == 98 || o.quality != 0)
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
	rules.partyMayShuffle = false;
	if (!_movement.check(nx, ny, o.z, k, rules).passable)
		return false;
	_world.moveObject(k, nx, ny, o.z);
	_world.actors.movementFlags[actor] = (uint8_t)((_world.actors.movementFlags[actor] & ~7) | dir); // the way it faces
	_actors.face(actor, DY[dir] < 0 ? 0 : DY[dir] > 0 ? 2 : DX[dir] > 0 ? 1 : 3);
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
	rules.partyMayShuffle = false;
	bool open = _movement.check(nx, o.y, o.z, k, rules).passable || _movement.check(o.x, ny, o.z, k, rules).passable;
	if (!open || !_movement.check(nx, ny, o.z, k, rules).passable)
		return false;
	_world.moveObject(k, nx, ny, o.z);
	_actors.face(actor, DY[dir] < 0 ? 0 : DY[dir] > 0 ? 2 : DX[dir] > 0 ? 1 : 3);
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
	static const uint8_t DIST[8][8] = {{0, 1, 2, 3, 4, 5, 6, 7}, {1, 1, 2, 3, 4, 5, 6, 7}, {2, 2, 2, 3, 4, 5, 6, 7}, {3, 3, 3, 4, 5, 6, 7, 7},
									   {4, 4, 4, 5, 6, 7, 7, 8}, {5, 5, 5, 6, 7, 7, 8, 8}, {6, 6, 6, 7, 7, 8, 8, 8}, {7, 7, 7, 7, 8, 8, 8, 8}};
	ObjList &a = _world.actors;
	int k = _world.actorObject(actor);
	if (k < 0 || _world.objects[k].deleted || a.hp[actor] == 0) {
		a.moves[actor] = 0;
		return;
	}
	const World::Obj o = _world.objects[k];
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
	// wt_work (29134; worktypes 0x90, 0x94, 0x96, 0x97): one turn in eight a step, otherwise five moves passed; the step is along the axis the worker is further off its place on
	// (a signed random number, from sub_28F37, decides between the two ways) so that it hovers about its schedule place. APPROX(67): 0x96 and 0x97 near the player call
	// `wt_work_12` in the original, which is not traced
	if (wt == 0x90 || wt == 0x94 || wt == 0x96 || wt == 0x97) {
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
	int target = wt == 0x0C || wt == 0x8F ? -1 : pickTarget(actor, wt == 6);
	if (target >= 0) {
		const World::Obj &t = _world.objects[_world.actorObject(target)];
		int dx = std::abs((int)t.x - (int)o.x), dy = std::abs((int)t.y - (int)o.y);
		if (dx < 8 && dy < 8 && DIST[dx][dy] <= Combat::rangeOf(_combat.weaponType(actor))) {
			_combat.attack(actor, target);
			_world.spendMoves(actor, 10);
		} else if (wt != 3 || (a.status[actor] & 0x80)) {
			stepToward(actor, t.x, t.y);
		} else {
			_world.spendMoves(actor, 5);
		}
		return;
	}
	if (wt == 3) {
		_world.spendMoves(actor, 5);
		return;
	}
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

} // namespace Ultima6
} // namespace Ultima6
