#include "ultima6/actors.h"

#include "ultima6/session.h"

#include <algorithm>
#include <cstdlib>
#include <queue>
#include <vector>

namespace Ultima6 {
namespace Ultima6 {

static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

void ActorManager::hourChanged(const GameClock &clock) {
	const Schedule &sc = _world.schedule;
	ObjList &a = _world.actors;
	int dow = clock.dayOfWeek();
	for (int actor = 2; actor < 0xE0; ++actor) {
		if (a.worktype[actor] == 0x9B)
			a.worktype[actor] = GO_TO_SCHEDULE;
		if (_world.actorObject(actor) < 0)
			continue;
		if (!(a.worktype[actor] >= 0x80 || a.worktype[actor] == 7))
			continue;
		if (a.status[actor] & 0x80)
			continue; // in the party
		int count = sc.entryCount(actor);
		int first = sc.index[actor];
		for (int i = first + count - 1; i >= first; --i) {
			Schedule::Entry e = sc.entry(i);
			if (e.hour != clock.hour)
				continue;
			if (e.dayOfWeek != dow && e.dayOfWeek != 0)
				continue;
			a.scheduleIndex[actor] = (uint8_t)(i - first);
			a.worktype[actor] = GO_TO_SCHEDULE;
			break;
		}
	}
}

int ActorManager::directionOf(int dx, int dy) {
	static const int TABLE[3][3] = {{7, 0, 1}, {6, -1, 2}, {5, 4, 3}};
	return TABLE[(dy > 0) - (dy < 0) + 1][(dx > 0) - (dx < 0) + 1];
}

// sub_26348: whether a square is shut to creatures (the terrain or an object that is not a party member stops them)
bool ActorManager::squareShut(int x, int y, int z) const {
	int mask = z == 0 ? 0x3FF : 0xFF;
	x &= mask;
	y &= mask;
	const TileFlags &tf = _world.flags;
	if (tf.flags1[_world.map.tile(z, x, y) & (TileFlags::TILES - 1)] & 2)
		return true;
	std::vector<int> there;
	_world.objectsAt(x, y, z, there);
	for (int i : there) {
		const World::Obj &o = _world.objects[i];
		if (o.actor >= 0 && o.actor < 0x100 && (_world.actors.status[o.actor] & 0x80))
			continue;
		if (tf.flags1[(_world.baseTile.firstTile[o.type] + o.frame) & (TileFlags::TILES - 1)] & 2)
			return true;
	}
	return false;
}

// sub_263D4: the frame an actor's sprite takes when it faces direction `dir` (0 north, 1 northeast, 2 east ... 7 northwest); its walking animation alternates by the flag 0x80 of the
// movement flags, and each kind of creature has its own frame layout (4 frames for each direction for people, 12 for type 0x16A, 3 for 0x16B, 2 for the animals, 1 for boats ...)
// APPROX(6): the parts of ships (0x19B, 0x19C) and of the big creatures (>= 0x1AA) that move together with a vehicle (word_3EFF3) are not turned here, and the creature 0x164 takes its
// frame from a counter instead of a random number
void ActorManager::face(int actor, int dir) {
	int k = _world.actorObject(actor);
	if (k < 0 || dir < 0 || dir > 7)
		return;
	World::Obj &o = _world.objects[k];
	uint8_t &flags = _world.actors.movementFlags[actor];
	int type = o.type, si = o.frame, di = 0;
	// the turn of a diagonal step: keep the way it faces when that is one of the two cardinal directions of the step, else the opposite of the other
	auto turn = [&](int cur) {
		if (!(dir & 1))
			return dir >> 1;
		return (((dir >> 1) - cur + 1) & 3) > 1 ? (cur + 2) & 3 : cur;
	};
	// the walking phase: 3 frames in a cycle 1, 2, 1, 0 ... given by the frame and the flag
	auto phase3 = [&](int ph) {
		if (ph == 0) {
			flags &= 0x7F;
			return 1;
		}
		if (ph == 1)
			return (flags & 0x80) ? 0 : 2;
		if (ph == 2)
			flags |= 0x80;
		return 1;
	};
	int px = 0, py = 0;
	bool shown = false; // sub_262DE: the actor is inside the 40 x 40 squares around the party (here: 20 each way)
	{
		int lead = _world.actorObject(_world.actors.partyRoster[0]);
		if (lead >= 0) {
			int mask = o.z == 0 ? 0x3FF : 0xFF;
			px = _world.objects[lead].x;
			py = _world.objects[lead].y;
			shown = _world.objects[lead].z == o.z && ((o.x - (px - 20)) & mask) <= 39 && ((o.y - (py - 20)) & mask) <= 39;
		}
	}
	if ((type >= 0x178 && type <= 0x183) || type == 0x199 || type == 0x19A) {
		bool seated = false;
		int seatFrame = 2;
		if (shown) {
			std::vector<int> there;
			_world.objectsAt(o.x, o.y, o.z, there);
			for (int i : there) {
				const World::Obj &s = _world.objects[i];
				if (s.type == 0xFC) {
					seated = true;
					seatFrame = s.frame;
					break;
				}
				if (s.type == 0x147 && s.frame == 2) {
					seated = true;
					break;
				}
			}
		}
		if (seated) {
			o.frame = (uint8_t)(seatFrame * 4 + 3);
		} else {
			di = turn(si >> 2);
			o.frame = (uint8_t)(di * 4 + phase3(si & 3));
		}
	} else if (type == 0x16A) {
		di = turn(si / 12);
		int ph = si % 12, s;
		if (ph == 3) {
			flags &= 0x7F;
			s = 7;
		} else if (ph == 7) {
			s = (flags & 0x80) ? 3 : 0xB;
		} else {
			if (ph == 0xB)
				flags |= 0x80;
			s = 7;
		}
		o.frame = (uint8_t)(di * 12 + s);
	} else if (type == 0x164) {
		o.frame = (uint8_t)(_walkFrame[actor]++ % 3);
	} else if (type == 0x16B) {
		di = turn(si / 3);
		o.frame = (uint8_t)(di * 3 + phase3(si % 3));
	} else if (type == 0x15C || type == 0x15D || type == 0x15E || type == 0x156 || type == 0x166 || type == 0x169 || type >= 0x1AA || type == 0x15F || type == 0x15A ||
			   (type >= 0x16F && type <= 0x174) || type == 0x188) {
		di = si >> 1;
		if (!(type >= 0x1AA)) {
			int d = dir;
			if (d & 1) {
				static const int DXT[8] = {0, 1, 1, 1, 0, -1, -1, -1}, DYT[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
				if (squareShut(o.x, o.y - DYT[d], o.z))
					d = (d & 4) + 2;
				else if (squareShut(o.x - DXT[d], o.y, o.z))
					d = (d + 1) & 4;
			}
			if (!(d & 1))
				di = d >> 1;
			else
				di = (((d >> 1) - di + 1) & 3) > 1 ? (di + 2) & 3 : di;
		}
		o.frame = (uint8_t)(di * 2 + ((si & 1) ? 0 : 1));
	} else if (type == 0x162 || type == 0x167 || type == 0x19E || type == 0x184) {
		o.frame = (uint8_t)turn(si);
	} else if (type == 0x1A8) {
		di = turn(si >> 3);
		o.frame = (uint8_t)(di * 8 + ((si & 4) ? 3 : 7));
	}
}

// APPROX(1): not the original's exact behaviour, see docs/approximations.md
// Breadth-first search for the first step towards (tx, ty).
bool ActorManager::step(int actor, int objIndex, int tx, int ty, int z) {
	const World::Obj o = _world.objects[objIndex];
	int mask = z == 0 ? 0x3FF : 0xFF;
	const int R = 20, W = 2 * R + 1;
	int cx = o.x + (tx - o.x) / 2, cy = o.y + (ty - o.y) / 2;
	std::vector<int> from(W * W, -2);
	std::queue<int> q;
	auto cell = [&](int x, int y) { return (y - (cy - R)) * W + (x - (cx - R)); };
	auto inside = [&](int x, int y) { return x >= cx - R && x <= cx + R && y >= cy - R && y <= cy + R; };
	MoveRules rules;
	rules.partyMayShuffle = false;
	int sx = o.x, sy = o.y;
	if (!inside(sx, sy))
		return false;
	from[cell(sx, sy)] = -1;
	q.push(cell(sx, sy));
	int found = -1, guard = 0;
	while (!q.empty() && found < 0 && ++guard < W * W) {
		int c = q.front();
		q.pop();
		int x = cx - R + c % W, y = cy - R + c / W;
		for (int d = 0; d < 8; ++d) {
			int nx = x + DX[d], ny = y + DY[d];
			if (!inside(nx, ny) || from[cell(nx, ny)] != -2)
				continue;
			if (!_movement.check(nx & mask, ny & mask, z, objIndex, rules).passable)
				continue;
			if (DX[d] && DY[d] && !_movement.check((x + DX[d]) & mask, y & mask, z, objIndex, rules).passable &&
				!_movement.check(x & mask, (y + DY[d]) & mask, z, objIndex, rules).passable)
				continue;
			from[cell(nx, ny)] = c;
			if (nx == tx && ny == ty) {
				found = cell(nx, ny);
				break;
			}
			q.push(cell(nx, ny));
		}
	}
	if (found < 0)
		return false;
	int c = found;
	while (from[c] >= 0 && from[from[c]] >= 0)
		c = from[c];
	int nx = cx - R + c % W, ny = cy - R + c / W;
	int dx = nx - o.x, dy = ny - o.y;
	_world.moveObject(objIndex, nx & mask, ny & mask, z);
	face(actor, directionOf(dx, dy));
	return true;
}

// APPROX(2): not the original's exact behaviour, see docs/approximations.md
void ActorManager::scheduleUpdate(int px, int py, int pz) {
	const Schedule &sc = _world.schedule;
	ObjList &a = _world.actors;
	for (int actor = 2; actor < 0xE0; ++actor) {
		if (a.worktype[actor] != GO_TO_SCHEDULE)
			continue;
		int k = _world.actorObject(actor);
		if (k < 0 || _world.objects[k].deleted)
			continue;
		Schedule::Entry e = sc.entry(sc.index[actor] + a.scheduleIndex[actor]);
		const World::Obj &o = _world.objects[k];
		if (o.x == e.x && o.y == e.y && o.z == e.z) {
			a.worktype[actor] = (uint8_t)e.worktype;
			continue;
		}
		bool near = o.z == pz && e.z == pz && std::abs(o.x - px) <= NEAR && std::abs(o.y - py) <= NEAR && std::abs(e.x - o.x) < 20 &&
					std::abs(e.y - o.y) < 20;
		if (near)
			continue; // it walks, a step whenever the scheduler gives it a turn
		// far from the party: put the actor in its place (or next to it)
		MoveRules rules;
		rules.partyMayShuffle = false;
		int mask = e.z == 0 ? 0x3FF : 0xFF;
		bool placed = false;
		for (int r = 0; r <= 2 && !placed; ++r)
			for (int dy = -r; dy <= r && !placed; ++dy)
				for (int dx = -r; dx <= r && !placed; ++dx) {
					if (std::max(std::abs(dx), std::abs(dy)) != r)
						continue;
					int nx = (e.x + dx) & mask, ny = (e.y + dy) & mask;
					if (_movement.check(nx, ny, e.z, k, rules).passable) {
						_world.moveObject(k, nx, ny, e.z);
						placed = true;
					}
				}
		a.worktype[actor] = (uint8_t)e.worktype;
	}
}

void ActorManager::act(int actor, int px, int py, int pz) {
	(void)px;
	(void)py;
	ObjList &a = _world.actors;
	int k = _world.actorObject(actor);
	if (k < 0 || a.worktype[actor] != GO_TO_SCHEDULE) {
		a.moves[actor] = 0; // nothing to do this minute
		return;
	}
	const Schedule &sc = _world.schedule;
	Schedule::Entry e = sc.entry(sc.index[actor] + a.scheduleIndex[actor]);
	const World::Obj o = _world.objects[k];
	if (o.x == e.x && o.y == e.y && o.z == e.z) {
		a.worktype[actor] = (uint8_t)e.worktype;
		a.moves[actor] = 0;
		return;
	}
	int cost = _world.moveCost(o.x, o.y, o.z);
	if (o.z == pz && step(actor, k, e.x, e.y, e.z))
		_world.spendMoves(actor, cost);
	else
		a.moves[actor] = 0; // no way: wait
}

} // namespace Ultima6
} // namespace Ultima6
