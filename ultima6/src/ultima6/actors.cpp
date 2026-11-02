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

// APPROX(6): not the original's exact behaviour, see docs/approximations.md
void ActorManager::face(int actor, int direction) {
	int k = _world.actorObject(actor);
	if (k < 0 || direction < 0 || direction > 3)
		return;
	static const int WALK[4] = {0, 1, 0, 2};
	_walkFrame[actor] = (_walkFrame[actor] + 1) % 4;
	World::Obj &o = _world.objects[k];
	if (o.frame < 16)
		o.frame = (uint8_t)(direction * 4 + WALK[_walkFrame[actor]]);
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
	face(actor, dx > 0 ? 1 : dx < 0 ? 3 : dy > 0 ? 2 : 0);
	return true;
}

// APPROX(2): not the original's exact behaviour, see docs/approximations.md
void ActorManager::tick(int px, int py, int pz) {
	const Schedule &sc = _world.schedule;
	ObjList &a = _world.actors;
	int walkers = 0;
	for (int actor = 2; actor < 0xE0; ++actor) {
		if (a.worktype[actor] != GO_TO_SCHEDULE)
			continue;
		int k = _world.actorObject(actor);
		if (k < 0)
			continue;
		Schedule::Entry e = sc.entry(sc.index[actor] + a.scheduleIndex[actor]);
		const World::Obj &o = _world.objects[k];
		if (o.x == e.x && o.y == e.y && o.z == e.z) {
			a.worktype[actor] = (uint8_t)e.worktype;
			continue;
		}
		bool near = o.z == pz && e.z == pz && std::abs(o.x - px) <= NEAR && std::abs(o.y - py) <= NEAR && std::abs(e.x - o.x) < 20 &&
					std::abs(e.y - o.y) < 20;
		if (near) {
			if (walkers >= 12)
				continue;
			++walkers;
			if (step(actor, k, e.x, e.y, e.z))
				continue;
		}
		// far from the party, or stuck: put the actor in its place (or next to it)
		MoveRules rules;
		rules.partyMayShuffle = false;
		bool placed = false;
		int mask = e.z == 0 ? 0x3FF : 0xFF;
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

} // namespace Ultima6
} // namespace Ultima6
