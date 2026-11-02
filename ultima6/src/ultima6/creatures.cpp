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
				spawn(o.type, o.quality, e.x, e.y, e.z);
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
			hatch(_eggs[n], hour);
		}
	}
}

// APPROX(20): not the original's exact behaviour, see docs/approximations.md
void Creatures::tick(int px, int py, int pz) {
	ObjList &a = _world.actors;
	// the party's companions fight back: each goes for the nearest hostile creature, and strikes it when next to it
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
	for (int actor = FIRST_TEMP_ACTOR; actor < ObjList::ACTORS; ++actor) {
		if (_world.actorFree(actor))
			continue;
		int k = _world.actorObject(actor);
		const World::Obj &o = _world.objects[k];
		if (a.hp[actor] == 0)
			continue;
		// the nearest party member
		int target = -1, best = 1 << 20;
		for (int i = 0; i < a.partySize; ++i) {
			int pk = _world.actorObject(a.partyRoster[i]);
			if (pk < 0 || _world.objects[pk].deleted || _world.objects[pk].z != o.z)
				continue;
			int d = std::max(std::abs((int)_world.objects[pk].x - (int)o.x), std::abs((int)_world.objects[pk].y - (int)o.y));
			if (d < best) {
				best = d;
				target = a.partyRoster[i];
			}
		}
		if (o.z != pz || std::abs((int)o.x - px) > DESPAWN_RANGE || std::abs((int)o.y - py) > DESPAWN_RANGE) {
			_world.deleteObject(k);
			a.hp[actor] = 0;
			continue;
		}
		const CreatureStats *s = Combat::stats(o.type);
		bool hostile = s && (s->align == CreatureStats::EVIL || s->align == CreatureStats::CHAOTIC) && a.worktype[actor] != 0x0C;
		if (hostile && target >= 0 && best <= HUNT_RANGE) {
			if (best <= 1) {
				_combat.attack(actor, target);
			} else {
				int tk = _world.actorObject(target);
				_actors.walkToward(actor, k, _world.objects[tk].x, _world.objects[tk].y, o.z);
			}
		} else if (_rng.next() % 3 == 0) {
			int d = _rng.next() % 8;
			int mask = o.z == 0 ? 0x3FF : 0xFF;
			int nx = (o.x + DX[d]) & mask, ny = (o.y + DY[d]) & mask;
			MoveRules rules;
			rules.partyMayShuffle = false;
			if (_movement.check(nx, ny, o.z, k, rules).passable) {
				_world.moveObject(k, nx, ny, o.z);
				_actors.face(actor, DY[d] < 0 ? 0 : DY[d] > 0 ? 2 : DX[d] > 0 ? 1 : 3);
			}
		}
	}
}

} // namespace Ultima6
} // namespace Ultima6
