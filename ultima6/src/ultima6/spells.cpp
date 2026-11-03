#include "ultima6/session.h"

#include "ultima6/describe.h"
#include "ultima6/stats_view.h"

#include <algorithm>
#include <cstdlib>

namespace Ultima6 {
namespace Ultima6 {

namespace {

// The path of one bolt of sub_37CBD: a straight line of pixels (16 to a square) from the middle of the start square to the middle of the goal square. Squares are
// collected in order until one lies beyond the reach (the distance table at ds:196C, at most `reach`) or blocks the bolt (flags2 bit 0x20 on the terrain or on any
// object there, unless it is next to the start and has bit 8). Returns whether the goal square was reached.
// APPROX(70): the step sizes and error terms of the original's Bresenham are not copied, and sub_18C09 gives only the top tile there.
bool boltPath(World &w, int z, int cx, int cy, int gx, int gy, int reach, std::vector<std::pair<int, int>> &squares) {
	static const uint8_t REACH[8][8] = {{0, 1, 2, 3, 4, 5, 6, 7}, {1, 1, 2, 3, 4, 5, 6, 7}, {2, 2, 2, 3, 4, 5, 6, 7}, {3, 3, 3, 4, 5, 6, 7, 7},
										{4, 4, 4, 5, 6, 7, 7, 8}, {5, 5, 5, 6, 7, 7, 8, 8}, {6, 6, 6, 7, 7, 8, 8, 8}, {7, 7, 7, 7, 8, 8, 8, 8}};
	int dxp = (gx - cx) * 16, dyp = (gy - cy) * 16;
	int major = std::max(std::abs(dxp), std::abs(dyp));
	int lastX = cx, lastY = cy;
	for (int step = 1; step <= major; ++step) {
		int sx = cx * 16 + 8 + (2 * dxp * step + (dxp < 0 ? -major : major)) / (2 * major);
		int sy = cy * 16 + 8 + (2 * dyp * step + (dyp < 0 ? -major : major)) / (2 * major);
		int qx = sx >> 4, qy = sy >> 4;
		if (qx == lastX && qy == lastY)
			continue;
		lastX = qx;
		lastY = qy;
		int adx = std::abs(qx - cx), ady = std::abs(qy - cy);
		if (adx > 7 || ady > 7 || REACH[adx][ady] > reach)
			return false;
		bool next = adx <= 1 && ady <= 1;
		auto stops = [&](int tile) {
			uint8_t f = w.flags.flags2[tile & (TileFlags::TILES - 1)];
			return (f & 0x20) && !(next && (f & 8));
		};
		std::vector<int> there;
		w.objectsAt(qx, qy, z, there);
		bool blocked = stops(w.map.tile(z, qx, qy));
		for (int i : there)
			blocked |= stops(w.baseTile.firstTile[w.objects[i].type] + w.objects[i].frame);
		if (blocked)
			return false;
		squares.push_back(std::make_pair(qx, qy));
	}
	return true;
}

} // namespace

// The spells that act on objects, on a square or on many people at once (cast_spell_effect, 25495..; its first-level jump table at 254BA sends each spell index to a
// group of handlers, only some of which have been read). Returns whether the spell was dealt with here; otherwise Magic::cast works it on the actor.
// APPROX(56): the effects follow the spell manual and the actors' status bits; the original's animations, projectile paths, messages and the many saving throws
// (sub_246AC, sub_24810, sub_24A61, sub_24BED, sub_24CF0, sub_24DCF, sub_24E78, sub_2502C, sub_25320) are not traced. Not done: Detect Magic/Trap, Help, Telekinesis, Trap,
// Untrap, Peer, Repel Undead, Animate, Conjure, Disable, Wind Change, Explosion, Insect Swarm, Pickpocket, Reveal, Seance, X-ray, Charm, Clone, Confuse,
// the winds, Hail Storm, Negate Magic, Replicate, Web, Chain Bolt, Enchant, Fear, Gate Travel, Wing Strike, Wizard Eye, Armageddon, Eclipse, Resurrect, Slime, Summon, Time Stop, Tremor
bool Game::spellSpecial(const Magic::SpellInfo &spell, int caster, int tx, int ty, int target) {
	ObjList &a = _world.actors;
	int index = spell.index();
	// the people near the target square (not the party), for the mass spells
	auto crowd = [&](std::vector<int> &out) {
		for (int actor = 0; actor < ObjList::ACTORS; ++actor) {
			int k = _world.actorObject(actor);
			if (k < 0 || _world.objects[k].deleted || (a.status[actor] & 0x80) || a.hp[actor] == 0 || _world.objects[k].z != z)
				continue;
			if (std::abs((int)_world.objects[k].x - tx) <= 5 && std::abs((int)_world.objects[k].y - ty) <= 5)
				out.push_back(actor);
		}
	};
	// sub_2227B on one actor: sleep, poison, paralysis and charm are taken off; returns whether there was anything to take off
	auto dispelActor = [&](int t) {
		bool any = false;
		int tk = _world.actorObject(t);
		if (a.status[t] & 0x04) {
			a.status[t] &= ~0x04;
			_world.objects[tk].type = a.baseObjFrame[t] & 0x3FF;
			_world.objects[tk].frame = (uint8_t)(a.baseObjFrame[t] >> 10);
			any = true;
		}
		if (a.status[t] & 0x08) {
			a.status[t] &= ~0x08;
			any = true;
		}
		if (a.status[t] & 0x02) {
			a.status[t] &= ~0x02;
			any = true;
		}
		if (_world.objects[tk].status & 0x04) {
			endCharm(t);
			any = true;
		}
		return any;
	};
	// sub_21D63 on one actor: returns false when the target resisted; a charmed target is set free
	auto charmActor = [&](int t, int c) {
		int tk = _world.actorObject(t);
		if (_combat.resists(t, c))
			return false;
		if (_world.objects[tk].status & 0x04) {
			endCharm(t);
			return true;
		}
		a.movementFlags[t] = (uint8_t)((a.movementFlags[t] & 0x9F) | (a.status[t] & 0x60));
		_world.objects[tk].status |= 0x04;
		a.status[t] = (uint8_t)((a.status[t] & 0x9F) | (a.status[c] & 0x60));
		a.worktype[t] = 8;
		message("\n" + _combat.nameOf(t) + " is charmed.\n");
		return true;
	};
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	int object = -1;
	for (size_t i = here.size(); i-- > 0;)
		if (_world.objects[here[i]].actor < 0 && !_world.objects[here[i]].invisible()) {
			object = here[i];
			break;
		}
	switch (index) {
	case 0: { // Create Food: 1-10 food (0x81) for the caster
		int n = 1 + _rng.next() % 10;
		_world.addToInventory(caster, 0x81, 0, n);
		message("Create " + std::to_string(n) + " food.\n");
		return true;
	}
	case 20: { // Sleep (sub_21B90): creatures with flag 1 say "No effect"; the target may resist ("Failed"); else it sleeps, without a word
		if (target < 0)
			return false;
		const CreatureStats *cs = Combat::stats(_world.objects[_world.actorObject(target)].type);
		if (cs && (cs->flags & 1)) {
			message("\nNo effect\n");
			return true;
		}
		if (_combat.resists(target, caster)) {
			message("\nFailed\n");
			return true;
		}
		a.status[target] |= 0x04;
		return true;
	}
	case 32: // Curse
		if (target < 0)
			return false;
		_world.objects[_world.actorObject(target)].status |= 0x40;
		return true;
	case 40: // Protection
		if (target < 0)
			return false;
		a.status[target] |= 0x01;
		return true;
	case 67: // Invisibility: not twice
		if (target < 0 || (_world.objects[_world.actorObject(target)].status & 0x02))
			return false;
		_world.objects[_world.actorObject(target)].status |= 0x02;
		return true;
	case 69: // Paralyze: "<name> is paralyzed."
		if (target < 0)
			return false;
		a.status[target] |= 0x02;
		message("\n" + _combat.nameOf(target) + " is paralyzed.\n");
		return true;
	case 101: { // Kill (sub_21E06): the dead and creatures with flag 2 are untouched ("No effect"); the target may resist; else it takes its whole hit points
		if (target < 0)
			return false;
		const CreatureStats *cs = Combat::stats(_world.objects[_world.actorObject(target)].type);
		if ((a.status[target] & 0x10) || (cs && (cs->flags & 2))) {
			message("\nNo effect\n");
			return true;
		}
		if (_combat.resists(target, caster)) {
			message("\nFailed\n");
			return true;
		}
		_combat.applyDamage(caster, target, a.hp[target]);
		return true;
	}
	case 37:   // Mass Awaken
	case 38:   // Mass Sleep
	case 54:   // Mass Dispel
	case 65:   // Explosion
	case 85:   // Mass Protect
	case 102:  // Mass Curse
	case 103:  // Mass Invisibility
	case 115:  // Mass Charm
	case 116: { // Mass Kill (sub_2362A): a bolt flies from the caster to the target square (else "Blocked"), sixteen more scatter from the target square; whoever stands on the squares they cross, and on the target square, is affected
		int ck = _world.actorObject(caster);
		int cx = _world.objects[ck].x, cy = _world.objects[ck].y;
		std::vector<std::pair<int, int>> path;
		if (!boltPath(_world, z, cx, cy, tx, ty, 8, path)) {
			message("\nBlocked\n");
			return true;
		}
		static const int8_t SX[16] = {2, 1, 0, 1, -1, -1, -2, -1, 0, -1, -2, -2, 2, 2, 1, 1}, SY[16] = {-1, 2, -2, -1, 2, -1, 0, 1, 2, -2, -1, 1, 1, 0, 1, -2};
		bool hit[ObjList::ACTORS] = {};
		auto mark = [&](int qx, int qy) {
			std::vector<int> there;
			_world.objectsAt(qx, qy, z, there);
			for (int i : there) {
				int t = _world.objects[i].actor;
				if (t >= 0 && t < ObjList::ACTORS && !(a.status[t] & 0x10))
					hit[t] = true;
			}
		};
		for (int b = 0; b < 16; ++b) {
			std::vector<std::pair<int, int>> squares;
			boltPath(_world, z, tx, ty, tx + SX[b], ty + SY[b], 3, squares);
			for (const std::pair<int, int> &q : squares)
				mark(q.first, q.second);
		}
		mark(tx, ty);
		for (int t = 0; t < ObjList::ACTORS; ++t) {
			if (!hit[t])
				continue;
			int tk = _world.actorObject(t);
			if (tk < 0)
				continue;
			World::Obj &o = _world.objects[tk];
			const CreatureStats *cs = Combat::stats(o.type);
			switch (index) {
			case 37:
				a.status[t] &= ~0x04;
				o.type = a.baseObjFrame[t] & 0x3FF;
				o.frame = (uint8_t)(a.baseObjFrame[t] >> 10);
				break;
			case 38: // sub_21B90
				if (!(cs && (cs->flags & 1)) && !_combat.resists(t, caster))
					a.status[t] |= 0x04;
				break;
			case 54: // sub_2227B
				dispelActor(t);
				break;
			case 65: // sub_222F2
				if (!(cs && (cs->flags & 8)))
					_combat.applyDamage(caster, t, cs && (cs->flags & 0x10) ? 2 * (1 + (int)(_rng.next() % 20)) : 1 + (int)(_rng.next() % 20));
				break;
			case 85:
				a.status[t] |= 0x01;
				break;
			case 102:
				o.status |= 0x40;
				break;
			case 103: // sub_21114: the vehicles and the big things do not turn invisible
				if (!(o.status & 0x02) && o.type < 0x1AA && o.type != 0x19B && o.type != 0x176 && o.type != 0x19D && o.type != 0x177)
					o.status |= 0x02;
				break;
			case 115:
				charmActor(t, caster);
				break;
			default: // sub_21E06
				if (!(a.status[t] & 0x10) && !(cs && (cs->flags & 2)) && !_combat.resists(t, caster))
					_combat.applyDamage(caster, t, a.hp[t]);
				break;
			}
		}
		if (index == 65 || index == 116) {
			// blow_up_door at the target square: doors are not destroyed yet
		}
		if (index != 65)
			message("\nSuccess\n");
		return true;
	}
	case 25: // Vanish (223BC case 25): only things whose tile has flag 8 of the third block go; the object is remembered for Reappear
		if (object < 0 || !(_world.flags.flags3[(_world.baseTile.firstTile[_world.objects[object].type] + _world.objects[object].frame) & (TileFlags::TILES - 1)] & 0x08))
			return false;
		_vanished = _world.objects[object];
		_haveVanished = true;
		_world.deleteObject(object);
		return true;
	case 19: // Reappear: the last thing that vanished comes back on the square
		if (!_haveVanished)
			return false;
		{
			World::Obj back = _vanished;
			back.x = (uint16_t)tx;
			back.y = (uint16_t)ty;
			back.z = (uint8_t)z;
			back.deleted = false;
			back.status = 0;
			_world.objects.push_back(back);
			_world.addToMap((int)_world.objects.size() - 1);
			_haveVanished = false;
		}
		return true;
	case 4: // Douse (sub_21E8F)
	case 8: { // Ignite: lights and the powder keg; a brazier past frame 1, a douse of an unlit light or an ignite of a lit one is "No effect"
		if (object < 0)
			return false;
		World::Obj &o = _world.objects[object];
		int type = o.type;
		bool lamp = type == 0x5A || type == 0xA4 || type == 0x7A || type == 0xFD || type == 0xCE || type == 0x91 || type == 0xDF;
		if (!lamp || (type == 0xCE && o.frame > 1) || (index == 4 && !(o.frame & 1)) || (index == 8 && (o.frame & 1))) {
			message("\nNo effect\n");
			return true;
		}
		if (type == 0xDF && o.frame == 0) {
			o.frame = 1; // the fuse
			_kegTimer = 3;
			_kegObject = object;
		} else if (o.frame & 1) {
			if (type == 0x5A)
				_world.deleteObject(object); // a torch is gone (its text is "burned out")
			else
				o.frame = (uint8_t)(o.frame & 2);
		} else {
			o.frame = (uint8_t)(o.frame | 1);
		}
		return true;
	}
	case 51:   // Fire Field
	case 55:   // Poison Field
	case 56:   // Sleep Field
	case 64: { // Energy Field (sub_2386F): a bolt flies to the square (else "Blocked"); the field object (0x13D fire, 0x13E poison, 0x140 sleep, 0x13F energy) is made there and kept only if it may stand there ("Success"), else "Not possible"
		int ck = _world.actorObject(caster);
		std::vector<std::pair<int, int>> path;
		if (!boltPath(_world, z, _world.objects[ck].x, _world.objects[ck].y, tx, ty, 8, path)) {
			message("\nBlocked\n");
			return true;
		}
		int type = index == 51 ? 0x13D : index == 55 ? 0x13E : index == 56 ? 0x140 : 0x13F;
		World::Obj f;
		f.status = 0;
		f.x = (uint16_t)tx;
		f.y = (uint16_t)ty;
		f.z = (uint8_t)z;
		f.type = (uint16_t)type;
		f.frame = 0;
		f.quantity = 1;
		f.quality = 0;
		f.actor = -1;
		f.parent = -1;
		_world.objects.push_back(f);
		int made = (int)_world.objects.size() - 1;
		MoveRules rules;
		rules.partyMayShuffle = false;
		if (!_movement.check(tx, ty, z, made, rules).passable) {
			_world.objects.pop_back();
			message("\nNot possible\n");
			return true;
		}
		_world.addToMap(made);
		message("\nSuccess\n");
		return true;
	}
	case 33: { // Dispel Field
		int radius = 0;
		bool any = false;
		for (int dx = -radius; dx <= radius; ++dx)
			for (int dy = -radius; dy <= radius; ++dy) {
				std::vector<int> at;
				_world.objectsAt(tx + dx, ty + dy, z, at);
				for (int i : at) {
					int type = _world.objects[i].type;
					if (type == 0x13D || type == 0x13E || type == 0x13F || type == 0x140 || type == 0xAF) {
						_world.deleteObject(i);
						any = true;
					}
				}
			}
		return any;
	}
	case 117: { // Resurrect: the dead body of an actor gets up again with full hit points and takes back what it carried
		if (object < 0 || _world.objects[object].type != 339 || _world.objects[object].quality == 0)
			return false;
		return _world.resurrect(_world.objects[object].quality);
	}
	case 83:   // Flame Wind
	case 87:   // Poison Wind
	case 98:   // Energy Wind
	case 113: { // Death Wind (sub_24E78, sub_38A7B, sub_37CBD): five bolts fly from the caster to the target square and its neighbours at right angles to the line
		int ck = _world.actorObject(caster);
		int cx = _world.objects[ck].x, cy = _world.objects[ck].y;
		auto random = [&](int lo, int hi) { return lo + (int)(_rng.next() % (hi - lo + 1)); };
		if (tx == cx && ty == cy) {
			message("\nNo effect\n");
			return true;
		}
		static const uint8_t DIRECTION[121] = {98, 99, 99, 99, 99, 100, 65, 66, 82, 82, 82, 98, 99, 99, 99, 100, 116, 65, 65, 66, 82, 82, 98, 99, 99, 100, 116, 116, 65, 65, 65, 66, 82,
											   98, 99, 100, 116, 116, 116, 65, 65, 65, 65, 66, 98, 100, 116, 116, 116, 116, 64, 64, 64, 64, 64, 0, 4, 4, 4, 4, 4, 48, 48, 48, 48, 32,
											   38, 6, 5, 5, 5, 5, 48, 48, 48, 32, 39, 38, 22, 6, 5, 5, 5, 48, 48, 32, 39, 39, 38, 22, 22, 6, 5, 5, 48, 32, 39, 39, 39, 38, 22, 22, 22,
											   6, 5, 32, 39, 39, 39, 39, 38, 22, 22, 22, 22, 6};
		static const int8_t DX[8] = {0, 1, 1, 1, 0, -1, -1, -1}, DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
		int ox = std::max(-5, std::min(5, tx - cx)), oy = std::max(-5, std::min(5, ty - cy));
		int code = DIRECTION[(oy + 5) * 11 + ox + 5];
		int hi = code >> 4, lo = code & 15;
		int px[5] = {tx, tx + DX[hi & 7], tx + DX[lo & 7], tx + 2 * DX[hi & 7], tx + 2 * DX[lo & 7]};
		int py[5] = {ty, ty + DY[hi & 7], ty + DY[lo & 7], ty + 2 * DY[hi & 7], ty + 2 * DY[lo & 7]};
		bool hit[ObjList::ACTORS] = {};
		for (int b = 0; b < 5; ++b) {
			std::vector<std::pair<int, int>> squares;
			boltPath(_world, z, cx, cy, px[b], py[b], 7, squares);
			for (const std::pair<int, int> &q : squares) {
				std::vector<int> there;
				_world.objectsAt(q.first, q.second, z, there);
				for (int i : there) {
					int t = _world.objects[i].actor;
					if (t >= 0 && t < ObjList::ACTORS && !(a.status[t] & 0x10))
						hit[t] = true;
				}
			}
		}
		for (int t = 0; t < ObjList::ACTORS; ++t) {
			if (!hit[t])
				continue;
			int tk = _world.actorObject(t);
			const CreatureStats *cs = tk >= 0 ? Combat::stats(_world.objects[tk].type) : nullptr;
			switch (index) {
			case 83: // sub_222F2
				if (!(cs && (cs->flags & 8)))
					_combat.applyDamage(caster, t, cs && (cs->flags & 0x10) ? random(1, 20) * 2 : random(1, 20));
				break;
			case 87: // sub_21BF2
				if (!(cs && (cs->flags & 4)) && !((_combat.adjustedStrength(t) / 2 + 30 - _combat.adjustedIntelligence(caster)) / 2 > random(1, 30)))
					a.status[t] |= 0x08;
				break;
			case 98:
				_combat.applyDamage(caster, t, random(1, 30));
				break;
			default: // sub_21E06: the living that are not flagged (2) and do not resist die
				if (!(a.status[t] & 0x10) && !(cs && (cs->flags & 2)) && !_combat.resists(t, caster))
					_combat.applyDamage(caster, t, a.hp[t]);
				break;
			}
		}
		return true;
	}
	case 84: { // Hail Storm (sub_24DCF, sub_38C40): 10-30 hailstones fall on and around the target square; each lands at (square * 16 + 1-30 + 1-30 - 23) pixels and takes one hit point from whoever stands on the square it lands on
		int stones = 10 + (int)(_rng.next() % 21);
		for (int n = 0; n < stones; ++n) {
			int px = tx * 16 + 1 + (int)(_rng.next() % 30) + 1 + (int)(_rng.next() % 30) - 23;
			int py = ty * 16 + 1 + (int)(_rng.next() % 30) + 1 + (int)(_rng.next() % 30) - 23;
			std::vector<int> there;
			_world.objectsAt(px >> 4, py >> 4, z, there);
			for (int i : there) {
				int t = _world.objects[i].actor;
				if (t >= 0 && t < ObjList::ACTORS && !(a.status[t] & 0x10))
					_combat.applyDamage(caster, t, 1);
			}
		}
		message("\nSuccess\n");
		return true;
	}
	case 3: // Dispel Magic (sub_2227B): "Failed" when there was nothing to take off
		if (target < 0)
			return false;
		if (!dispelActor(target))
			message("\nFailed\n");
		return true;
	case 50: // Disable (sub_21CB4 with 0x32): unless resisted the target is left with one hit point
		if (target < 0)
			return false;
		if (_combat.resists(target, caster)) {
			message("\nFailed\n");
			return true;
		}
		_combat.applyDamage(caster, target, a.hp[target] - 1 > 0 ? a.hp[target] - 1 : 0);
		return true;
	case 80: // Charm (sub_21D63): unless resisted the target takes the caster's side (the old side is kept in its movement flags), is charmed (object status 4) and fights (worktype 8)
		if (target < 0)
			return false;
		if (!charmActor(target, caster))
			message("\nFailed\n");
		return true;
	case 82: // Confuse (sub_23AD5 case 82)
	case 99: // Fear (sub_23A6A)
	case 41: // Repel Undead (sub_23AD5 case 41)
	case 71: { // Reveal (sub_23AD5 case 71): every square within seven of the party, the first object found there
		for (int ax = x - 7; ax <= x + 7; ++ax)
			for (int ay = y - 7; ay <= y + 7; ++ay) {
				std::vector<int> here2;
				_world.objectsAt(ax, ay, z, here2);
				if (here2.empty())
					continue;
				int k = here2.back();
				World::Obj &o = _world.objects[k];
				int t = o.actor;
				if (index == 71) {
					o.status &= ~0x02;
					if (t >= 0) {
						std::vector<int> inv;
						_world.inventory(t, inv, true);
						for (int i : inv)
							if (_world.objects[i].type == 0x102 && _world.objects[i].location() == ObjRecord::READIED)
								_world.objects[i].status = (uint8_t)((_world.objects[i].status & ~ObjRecord::LOCATION_MASK) | ObjRecord::IN_INVENTORY);
					}
					continue;
				}
				if (t < 0 || t >= 0x100)
					continue;
				const CreatureStats *cs = Combat::stats(o.type);
				if (index == 41) {
					if (cs && (cs->flags & 0x80) && !_combat.resists(t, caster))
						a.worktype[t] = 0x13;
				} else if (index == 99) {
					if ((a.status[t] & 0x60) != 0x40 && !_combat.resists(t, caster) && !(cs && (cs->flags & 2)))
						a.worktype[t] = 0x13;
				} else if ((a.status[t] & 0x60) == 0x20 && !_combat.resists(t, caster)) {
					a.worktype[t] = 8;
					a.status[t] &= 0x9F;
				}
			}
		message("\nSuccess\n");
		return true;
	}

	case 86: { // Negate Magic: the fields and the conditions of everybody near are gone
		std::vector<int> actors;
		tx = x;
		ty = y;
		crowd(actors);
		for (int t : actors) {
			a.status[t] &= ~0x07;
			_world.objects[_world.actorObject(t)].status &= ~(0x02 | 0x40);
		}
		return true;
	}
	case 70: { // Pickpocket: the target gives up the first thing it carries
		if (target < 0)
			return false;
		std::vector<int> inv;
		_world.inventory(target, inv, false);
		if (inv.empty()) {
			message("nothing!\n");
			return true;
		}
		World::Obj &it = _world.objects[inv[0]];
		it.status = (uint8_t)((it.status & ~ObjRecord::LOCATION_MASK) | ObjRecord::IN_INVENTORY);
		it.parent = caster;
		return true;
	}
	case 96: { // Chain Bolt (sub_220F8): the target takes 1-30 and the bolt jumps up to six times to the nearest other actor of the other side within eight squares, 1-30 each
		if (target < 0)
			return false;
		std::vector<bool> hit(ObjList::ACTORS, false);
		int current = target;
		_combat.applyDamage(caster, current, 1 + _rng.next() % 30);
		hit[current] = true;
		for (int jump = 0; jump < 6; ++jump) {
			int ck = _world.actorObject(current), best = -1, bestD = 1 << 20;
			for (int actor = 0; actor < ObjList::ACTORS; ++actor) {
				int k = _world.actorObject(actor);
				if (k < 0 || hit[actor] || _world.objects[k].deleted || a.hp[actor] == 0 || (a.status[actor] & 0x80) || _world.objects[k].z != z)
					continue;
				int d = std::max(std::abs((int)_world.objects[k].x - (int)_world.objects[ck].x), std::abs((int)_world.objects[k].y - (int)_world.objects[ck].y));
				if (d <= 8 && d < bestD) {
					bestD = d;
					best = actor;
				}
			}
			if (best < 0)
				break;
			current = best;
			hit[current] = true;
			_combat.applyDamage(caster, current, 1 + _rng.next() % 30);
		}
		return true;
	}
	case 112: { // Armageddon: everything not of the party on the level dies
		for (int actor = 0; actor < ObjList::ACTORS; ++actor) {
			int k = _world.actorObject(actor);
			if (k >= 0 && !_world.objects[k].deleted && a.hp[actor] && !(a.status[actor] & 0x80) && _world.objects[k].z == z)
				_combat.applyDamage(caster, actor, 255);
		}
		return true;
	}
	case 120: // Time Stop: ten minutes (byte_404BA = 0xA, 45501)
		_timeStop = 10;
		return true;
	case 100: // Gate Travel: "To phase " and the number of the moon gate (1-8)
		message("To phase ");
		_textActive = true;
		_textBuf.clear();
		_textMax = 1;
		_textPurpose = 6;
		return true;
	case 114: // Eclipse
		_world.actors.globals[0x12 + 15] = (uint8_t)(20 + _rng.next() % 20);
		return true;
	case 57: // Wind Change
		if (_world.actors.globals.size() > 9)
			_world.actors.globals[9] = (uint8_t)(_rng.next() % 8);
		return true;
	case 88: // Replicate: the object on the square is copied
		if (object < 0)
			return false;
		{
			World::Obj copy = _world.objects[object];
			_world.objects.push_back(copy);
			_world.addToMap((int)_world.objects.size() - 1);
		}
		return true;
	case 81: // Clone: a creature like the target stands beside it
		if (target < 0)
			return false;
		{
			int tk = _world.actorObject(target);
			int c = _creatures.spawn(_world.objects[tk].type, a.worktype[target], _world.objects[tk].x, _world.objects[tk].y, _world.objects[tk].z);
			if (c < 0)
				return false;
			a.status[c] = a.status[target] & ~0x80;
		}
		return true;
	case 49: // Conjure (spell_summon_daemon 23C?? with 0x31): a creature (by a roll of 0-15: 0x173, 0x156, 0x169, 0x158 or 0x166) for the party
	case 119: { // Summon: a daemon (0x16F) that may resist ("Hostile daemon summoned.", it then fights everybody), else "Success"
		int type = 0x16F;
		if (index == 49) {
			int r = _rng.next() % 16;
			type = r <= 3 ? 0x173 : r <= 6 ? 0x156 : r <= 9 ? 0x169 : r <= 12 ? 0x158 : 0x166;
		}
		int made = -1;
		for (int tries = 0; tries < 8 && made < 0; ++tries) {
			int sx = x + (int)(_rng.next() % 11) - 5, sy = y + (int)(_rng.next() % 11) - 5;
			MoveRules rules;
			rules.partyMayShuffle = false;
			if (!_movement.check(sx, sy, z, _leaderObject, rules).passable)
				continue;
			made = _creatures.spawn(type, 8, sx, sy, z);
		}
		if (made < 0)
			return true; // the creature is let go again
		bool hostile = false;
		if (index == 119) {
			const CreatureStats *cs = Combat::stats(type);
			int daemonInt = cs ? cs->intel : 10;
			hostile = daemonInt / 2 + 15 - _combat.adjustedIntelligence(caster) > 1 + (int)(_rng.next() % 30);
		}
		a.status[made] = (uint8_t)((a.status[made] & 0x9F) | (hostile ? 0x60 : (a.status[caster] & 0x60)));
		message(hostile ? "\nHostile daemon summoned.\n" : "\nSuccess\n");
		return true;
	}
	case 21: // Telekinesis: the object is used from where the caster stands
		if (object < 0)
			return false;
		_telekinesis = true;
		useObject(object);
		_telekinesis = false;
		return true;
	case 39: // Peer: the overhead map, as a gem shows it
	case 105: // Wizard Eye: (approximated by the same view)
		_gem = true;
		return true;
	case 121: { // Tremor (sub_24BED): in the 11 x 11 squares around the party every actor of the monsters' side (status bits 0x60 = 0x20) that does not fly (creature flag 0x100) takes
		// 1-30 points of damage; then "Success"
		for (int actor = 0; actor < ObjList::ACTORS; ++actor) {
			int k = _world.actorObject(actor);
			if (k < 0 || _world.objects[k].deleted || a.hp[actor] == 0 || (a.status[actor] & 0x60) != 0x20 || _world.objects[k].z != z)
				continue;
			const CreatureStats *cs = Combat::stats(_world.objects[k].type);
			if (cs && (cs->flags & 0x100))
				continue;
			if (std::abs((int)_world.objects[k].x - x) <= 5 && std::abs((int)_world.objects[k].y - y) <= 5)
				_combat.applyDamage(caster, actor, 1 + _rng.next() % 30);
		}
		message("\nSuccess\n");
		return true;
	}
	case 66: { // Insect Swarm (sub_2458F): eight swarms (0x157) of the caster's side, each put on the first of eight random squares within five of the party that it may enter (else it is let go), worktype 8
		for (int swarm = 0; swarm < 8; ++swarm)
			for (int tries = 0; tries < 8; ++tries) {
				int sx = x + (int)(_rng.next() % 11) - 5, sy = y + (int)(_rng.next() % 11) - 5;
				MoveRules rules;
				rules.partyMayShuffle = false;
				if (!_movement.check(sx, sy, z, _leaderObject, rules).passable)
					continue;
				int made = _creatures.spawn(0x157, 8, sx, sy, z);
				if (made >= 0)
					a.status[made] = (uint8_t)((a.status[made] & 0x9F) | (a.status[caster] & 0x60));
				break;
			}
		message("\nSuccess\n");
		return true;
	}
	case 53: { // Locate: where the party is, as the sextant says it
		int sx = z ? x * 4 : x, sy = z ? y * 4 : y;
		int lat = (sy - 360) >> 3, lon = (sx - 304) >> 3;
		message(std::to_string(std::abs(lat)) + "{" + (lat > 0 ? "S" : "N") + ", " + std::to_string(std::abs(lon)) + "{" + (lon > 0 ? "E\n" : "W\n"));
		return true;
	}
	case 23: // Unlock Magic
	case 36: // Lock
		if (object < 0)
			return false;
		{
			int type = _world.objects[object].type;
			if (type >= 0x129 && type <= 0x12C)
				useKeyOnDoor(object, false, index == 36, index == 23);
			else if (type == 0x62)
				useChest(object, false, index == 36, index == 23);
			else
				return false;
		}
		return true;
	default:
		return false;
	}
}

} // namespace Ultima6
} // namespace Ultima6
