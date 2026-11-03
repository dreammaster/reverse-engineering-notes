#include "ultima6/actors.h"

#include "ultima6/session.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
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
		if (a.worktype[actor] == 0x91) {
			// the sleepers are about at the next schedule: sleep goes and the sprite comes back
			int ko = _world.actorObject(actor);
			if (ko >= 0 && _world.objects[ko].type == 0x92 && (a.baseObjFrame[actor] & 0x3FF)) {
				_world.objects[ko].type = a.baseObjFrame[actor] & 0x3FF;
				_world.objects[ko].frame = (uint8_t)(a.baseObjFrame[actor] >> 10);
			}
			a.status[actor] &= ~4;
		}
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
// ---- the path finder of walk_to_schedule_place (28AA7..28F2F), sub_2A0DA, sub_287E4, sub_28369 and wt_walk_path (295ED) ----------------------------------------------------------
// The 40 x 40 squares around the party (origin ((x - 16) & 0x3F8, (y - 16) & 0x3F8)) get a cost to enter (sub_2A0DA: 0xFF shut, else the terrain's movement number + 1, doors and furniture
// change it); a search spreads in two waves, from the actor (flag 0) and from the goal (flag 0x80), over a bucket queue indexed by the total cost (the newest square of a bucket comes out
// first) in the order west, east, north, south, until a square of one wave is reached by the other; then sub_28369 walks back both ways and stores the route as bytes
// (direction 0-3 for north, east, south, west, and 4 times the number of steps) in one of eight route tables. Actors on worktypes 0x83-0x85 follow that table (wt_walk_path).
// APPROX(1): only routes that lie inside the 40 x 40 squares are planned; the original also plans from outside (swapping the ends, leaving through an edge: word_4C421), and
// looks for the nearest object of a type (worktype 0x82, word_4C423); the search of those is not done. The order the objects that change the costs are met in (sub_19E9E) is the order
// of the object list here.
bool ActorManager::inWindow(int x, int y, int z, int px, int py, int pz, int &gx, int &gy) const {
	int mask = z == 0 ? 0x3FF : 0xFF;
	gx = (x - ((px - 16) & 0x3F8)) & mask;
	gy = (y - ((py - 16) & 0x3F8)) & mask;
	return z == pz && gx < 40 && gy < 40;
}

bool ActorManager::plan(int actor, int tx, int ty, int px, int py, int pz) {
	int k = _world.actorObject(actor);
	const World::Obj &o = _world.objects[k];
	int ax, ay, gx, gy;
	if (!inWindow(o.x, o.y, o.z, px, py, pz, ax, ay) || !inWindow(tx, ty, o.z, px, py, pz, gx, gy))
		return false;
	int left = (px - 16) & 0x3F8, top = (py - 16) & 0x3F8;
	int mask = o.z == 0 ? 0x3FF : 0xFF;
	const TileFlags &tf = _world.flags;
	const int T = TileFlags::TILES - 1;
	// the costs (sub_2A0DA)
	uint8_t cost[40][40];
	int terrain[40][40];
	for (int y = 0; y < 40; ++y)
		for (int x = 0; x < 40; ++x) {
			int tile = _world.map.tile(o.z, (left + x) & mask, (top + y) & mask) & T;
			terrain[y][x] = tile;
			uint8_t f = tf.flags1[tile];
			cost[y][x] = (f & 2) ? 0xFF : (uint8_t)((f >> 4) + 1);
		}
	auto mark = [&](int tile, int x, int y) { // sub_29FD5
		tile &= T;
		uint8_t f = tf.flags1[tile];
		if (cost[y][x] == 0xFF) {
			if ((tf.flags1[terrain[y][x]] & 1) && !(f & 2))
				cost[y][x] = (uint8_t)((f >> 4) + 1);
		} else if (f & 2) {
			cost[y][x] = 0xFF;
		} else if (!(f & 4)) {
			cost[y][x] = (uint8_t)std::min(0xFF, cost[y][x] + (f >> 4));
		}
	};
	for (size_t i = 0; i < _world.objects.size(); ++i) {
		const World::Obj &ob = _world.objects[i];
		if (ob.deleted || !ob.onMap() || ob.z != o.z)
			continue;
		int x = (ob.x - left) & mask, y = (ob.y - top) & mask;
		if (x >= 40 || y >= 40)
			continue;
		int tile = _world.baseTile.firstTile[ob.type] + ob.frame;
		uint8_t f2 = tf.flags2[tile & T];
		if (ob.type >= 0x129 && ob.type <= 0x12C) {
			if (ob.frame < 8 && cost[y][x] < 0xFF)
				++cost[y][x];
			else
				cost[y][x] = 0xFF;
			if (f2 & 0x40) {
				if (y)
					cost[y - 1][x] = 0xFF;
			} else if (x) {
				cost[y][x - 1] = 0xFF;
			}
		} else if (ob.type == 0x116 || ob.type == 0x118) {
			if (cost[y][x] < 0xFF)
				++cost[y][x];
		} else if (ob.actor < 0) {
			mark(tile, x, y);
			if (f2 & 0x80) {
				if (x)
					mark(tile - 1, x - 1, y);
				if ((f2 & 0x40) && y)
					mark(tile - 2, x, y - 1);
				if ((f2 & 0x40) && x && y)
					mark(tile - 3, x - 1, y - 1);
			} else if ((f2 & 0x40) && y) {
				mark(tile - 1, x, y - 1);
			}
		}
	}
	// the search (sub_287E4)
	uint8_t visit[40][40];
	std::memset(visit, 0xFF, sizeof visit);
	int head[256], next[256], nodeX[256], nodeY[256];
	for (int i = 0; i < 256; ++i) {
		head[i] = -1;
		next[i] = i + 1;
	}
	next[255] = -1;
	int freeHead = 0, maxBucket = 0, cur = 0;
	bool found = false;
	int slot = -1;
	for (int s = 0; s < 8; ++s) { // sub_29B97: the first table that is free (or whose walker has stopped walking)
		if (_owner[s] && !(_world.actors.worktype[_owner[s]] >= 0x83 && _world.actors.worktype[_owner[s]] <= 0x86))
			_owner[s] = 0;
		if (!_owner[s]) {
			slot = s;
			break;
		}
	}
	if (slot < 0)
		return false;
	static const int DX4[4] = {0, 1, 0, -1}, DY4[4] = {-1, 0, 1, 0};
	auto trace = [&](int x, int y) { // sub_28369
		uint8_t *p = _path[slot];
		int len = 0, ox = x, oy = y;
		p[0] = 0;
		int dirLast = 0;
		while (visit[y][x] != 0) {
			int best = 0xFF, bd = 0;
			for (int s = 0; s < 4; ++s) {
				int nx = x + DX4[s], ny = y + DY4[s];
				if (nx < 0 || nx > 39 || ny < 0 || ny > 39)
					continue;
				int v = visit[ny][nx];
				if (v < best || (v == best && dirLast == s)) {
					best = v;
					bd = s;
				}
			}
			x += DX4[bd];
			y += DY4[bd];
			dirLast = bd;
			if (len && (p[0] & 3) == (bd ^ 2)) {
				p[0] += 4;
			} else {
				if (len < 31)
					++len;
				for (int i = len; i > 0; --i)
					p[i] = p[i - 1];
				p[0] = (uint8_t)((bd ^ 2) + 4);
			}
		}
		x = ox;
		y = oy;
		visit[y][x] = 0xFF;
		dirLast = 0;
		while (visit[y][x] > 0x80) {
			int best = 0xFF, bd = 0;
			for (int s = 0; s < 4; ++s) {
				int nx = x + DX4[s], ny = y + DY4[s];
				if (nx < 0 || nx > 39 || ny < 0 || ny > 39 || visit[ny][nx] < 0x80)
					continue;
				int v = visit[ny][nx];
				if (v < best || (v == best && dirLast == s)) {
					best = v;
					bd = s;
				}
			}
			x += DX4[bd];
			y += DY4[bd];
			dirLast = bd;
			if (len >= 31)
				break;
			if (len && (p[len - 1] & 3) == bd)
				p[len - 1] += 4;
			else
				p[len++] = (uint8_t)(bd + 4);
		}
		p[len] = 0;
	};
	auto insert = [&](int c, int x, int y, int flag) {
		if (found || c > 0x7F || x < 0 || x > 39 || y < 0 || y > 39)
			return;
		int v = visit[y][x];
		if (v != 0xFF && (v & 0x80) != flag) {
			found = true;
			trace(x, y);
			return;
		}
		if ((visit[y][x] & 0x7F) <= c)
			return;
		int n = freeHead;
		if (n < 0)
			return;
		if (c > maxBucket)
			maxBucket = c;
		freeHead = next[n];
		next[n] = head[c];
		nodeX[n] = x;
		nodeY[n] = y;
		head[c] = n;
		visit[y][x] = (uint8_t)(c | flag);
	};
	insert(0, ax, ay, 0);
	insert(0, gx, gy, 0x80);
	while (head[cur] >= 0 && !found) {
		int n = head[cur];
		head[cur] = next[n];
		int x = nodeX[n], y = nodeY[n];
		next[n] = freeHead;
		freeHead = n;
		int flag = visit[y][x] & 0x80, d = visit[y][x] & 0x7F;
		auto c = [&](int cx, int cy) { return cx < 0 || cx > 39 || cy < 0 || cy > 39 ? 0xFF : (int)cost[cy][cx]; };
		insert(d + c(x - 1, y), x - 1, y, flag);
		insert(d + c(x + 1, y), x + 1, y, flag);
		insert(d + c(x, y - 1), x, y - 1, flag);
		insert(d + c(x, y + 1), x, y + 1, flag);
		if (head[cur] < 0)
			while (cur < maxBucket && head[cur] < 0)
				++cur;
	}
	ObjList &a = _world.actors;
	if (!found) {
		if (a.worktype[actor] == 0x81)
			a.worktype[actor] = 0x86;
		return true;
	}
	a.worktype[actor] = 0x83;
	a.unknown18f1[actor] = (uint8_t)slot;
	_owner[slot] = actor;
	_index[slot] = 0;
	_rep[slot] = 0;
	return true;
}

// wt_walk_path: one step along the actor's table; when the table is used up the actor is at its place (or plans again), and a blocked step is tried again twice (0x83 -> 0x84 -> 0x85) before it gives up (0x86)
void ActorManager::follow(int actor, int destX, int destY, int px, int py, int pz) {
	ObjList &a = _world.actors;
	int slot = a.unknown18f1[actor] & 7;
	int entry = _path[slot][_index[slot] & 31];
	int k = _world.actorObject(actor);
	const World::Obj o = _world.objects[k];
	if (entry == 0) {
		_owner[slot] = 0;
		if (std::max(std::abs(o.x - destX), std::abs(o.y - destY)) < 2) {
			arrive(actor, px, py, pz);
		} else {
			a.worktype[actor] = GO_TO_SCHEDULE;
		}
		_world.spendMoves(actor, 5);
		return;
	}
	int dir = (entry & 3) * 2;
	_world.spendMoves(actor, _world.moveCost(o.x, o.y, o.z));
	int mask = o.z == 0 ? 0x3FF : 0xFF;
	int nx = (o.x + DX[dir]) & mask, ny = (o.y + DY[dir]) & mask;
	MoveRules rules;
	rules.partyMayShuffle = false;
	if (_movement.check(nx, ny, o.z, k, rules).passable) {
		_world.moveObject(k, nx, ny, o.z);
		a.movementFlags[actor] = (uint8_t)((a.movementFlags[actor] & ~7) | dir);
		face(actor, dir);
		if ((entry >> 2) <= ++_rep[slot]) {
			++_index[slot];
			_rep[slot] = 0;
		}
		a.worktype[actor] = 0x83;
		return;
	}
	if (a.worktype[actor] == 0x83) {
		a.worktype[actor] = 0x84;
		a.moves[actor] = 0;
	} else if (a.worktype[actor] == 0x84) {
		a.worktype[actor] = 0x85;
		a.moves[actor] = 0;
	} else if (a.worktype[actor] == 0x85) {
		a.worktype[actor] = 0x86;
		a.moves[actor] = 0;
		_owner[slot] = 0;
		if (o.x + DX[dir] == destX && o.y + DY[dir] == destY) {
			arrive(actor, px, py, pz);
		}
	}
}

// sub_27FE6 (27FE6-28361): an actor that has come to its schedule place takes up the entry's worktype and its pose: asleep in a bed (0x91: the sleeping figure 0x92 on a bed 0xA3 whose frame is
// 0 or 6), seated (0x92, 0x95: the chair or bed under it decides the frame, see face), standing and facing a direction (0x87-0x8E); an actor that has not reached its place goes on
// (worktype 0x81). Waking up (any other worktype) takes the sleep away and the sprite back.
// 0x93 (a chair beside a table 0x77, sub_27E95), 0x95 (the figure 0x188 at a chair) and 0x98 (the bell) follow the original; 0x93 is a chair that has a table beside it in the four directions.
void ActorManager::arrive(int actor, int px, int py, int pz) {
	ObjList &a = _world.actors;
	int k = _world.actorObject(actor);
	if (k < 0)
		return;
	const Schedule &sc = _world.schedule;
	Schedule::Entry e = sc.entry(sc.index[actor] + a.scheduleIndex[actor]);
	World::Obj &o = _world.objects[k];
	bool at = o.x == e.x && o.y == e.y && o.z == e.z;
	int wt = e.worktype;
	a.worktype[actor] = (uint8_t)wt;
	int gx, gy;
	bool shown = inWindow(o.x, o.y, o.z, px, py, pz, gx, gy);
	if (wt == 0x91) {
		at = true;
		a.status[actor] |= 4;
		if (!shown) {
			a.worktype[actor] = 0x80;
		} else {
			std::vector<int> there;
			_world.objectsAt(o.x, o.y, o.z, there);
			for (int i : there) {
				const World::Obj &bed = _world.objects[i];
				if (bed.type == 0xA3 && (bed.frame == 0 || bed.frame == 6)) {
					o.type = 0x92;
					o.frame = bed.frame ? 1 : 0;
					break;
				}
			}
		}
	} else {
		a.status[actor] &= ~4;
		if (o.type == 0x92 && (a.baseObjFrame[actor] & 0x3FF)) {
			o.type = a.baseObjFrame[actor] & 0x3FF;
			o.frame = (uint8_t)(a.baseObjFrame[actor] >> 10);
		}
		// sub_27EF4: the thing of the kind the pose needs on the actor's own square (mode 0 a chair: 0xFC, or 0x147 of frame 2; mode 2 a chair with a table 0x77 beside it, mode 3 the bell 0x1A3)
		auto seatAt = [&](int mode) {
			static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1}, DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
			std::vector<int> there;
			_world.objectsAt(o.x, o.y, o.z, there);
			for (int i : there) {
				const World::Obj &s = _world.objects[i];
				if (s.actor >= 0)
					continue;
				if (mode == 0 && (s.type == 0xFC || (s.type == 0x147 && s.frame == 2)))
					return i;
				if (mode == 3 && s.type == 0x1A3)
					return i;
				if (mode == 2 && s.type == 0xFC) {
					int mask = o.z == 0 ? 0x3FF : 0xFF;
					for (int d = 0; d < 8; d += 2) {
						std::vector<int> near;
						_world.objectsAt((o.x + DX[d]) & mask, (o.y + DY[d]) & mask, o.z, near);
						for (int n : near)
							if (_world.objects[n].type == 0x77)
								return i;
					}
				}
			}
			return -1;
		};
		if (wt == 0x92 || wt == 0x95) {
			at = true;
			if (!shown) {
				a.worktype[actor] = 0x80;
			} else {
				int seat = seatAt(0);
				if (seat >= 0 && wt == 0x95) { // the chair is pulled in and the figure sits at it (type 0x188, frame 4)
					_world.objects[seat].frame = 2;
					face(actor, 4);
					o.type = 0x188;
					o.frame = 4;
				} else if (seat >= 0) {
					face(actor, 0);
				}
			}
		} else if (wt == 0x93) { // at a table: the chair is turned to the table (its frame is the way to it) and the person faces it
			at = true;
			if (!shown) {
				a.worktype[actor] = 0x80;
			} else {
				static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1}, DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
				int chair = seatAt(2);
				if (chair < 0) {
					face(actor, 4);
				} else {
					int mask = o.z == 0 ? 0x3FF : 0xFF, dir = 0;
					for (; dir < 8; dir += 2) {
						std::vector<int> near;
						_world.objectsAt((o.x + DX[dir]) & mask, (o.y + DY[dir]) & mask, o.z, near);
						bool table = false;
						for (int n : near)
							table |= _world.objects[n].type == 0x77;
						if (table)
							break;
					}
					_world.objects[chair].frame = (uint8_t)(((dir + 1) >> 1) & 3);
					face(actor, dir);
				}
			}
		} else if (wt == 0x98) { // the bell: only the person standing at it
			at = true;
			if (!shown)
				a.worktype[actor] = 0x80;
		} else if (wt >= 0x87 && wt <= 0x8E) {
			int dir = ((wt + 0x79) & 3) * 2;
			a.movementFlags[actor] = (uint8_t)((a.movementFlags[actor] & ~7) | dir);
			face(actor, dir);
		}
	}
	if (!at) {
		a.worktype[actor] = GO_TO_SCHEDULE;
		if (o.type == 0x188)
			o.type = 0x182;
	}
}

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
			arrive(actor, px, py, pz);
			continue;
		}
		int g1, g2;
		bool near = inWindow(o.x, o.y, o.z, px, py, pz, g1, g2) && inWindow(e.x, e.y, e.z, px, py, pz, g1, g2);
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
		arrive(actor, px, py, pz);
	}
}

void ActorManager::act(int actor, int px, int py, int pz) {
	ObjList &a = _world.actors;
	int k = _world.actorObject(actor);
	int wt = a.worktype[actor];
	if (k < 0 || !(wt == GO_TO_SCHEDULE || (wt >= 0x83 && wt <= 0x85))) {
		a.moves[actor] = 0; // nothing to do this minute
		return;
	}
	const Schedule &sc = _world.schedule;
	Schedule::Entry e = sc.entry(sc.index[actor] + a.scheduleIndex[actor]);
	const World::Obj o = _world.objects[k];
	if (wt != GO_TO_SCHEDULE) {
		follow(actor, e.x, e.y, px, py, pz);
		return;
	}
	if (o.x == e.x && o.y == e.y && o.z == e.z) {
		arrive(actor, px, py, pz);
		a.moves[actor] = 0;
		return;
	}
	if (!plan(actor, e.x, e.y, px, py, pz))
		a.moves[actor] = 0; // out of the window or all tables in use: wait
}

} // namespace Ultima6
} // namespace Ultima6
