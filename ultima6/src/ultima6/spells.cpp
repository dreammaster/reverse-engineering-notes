#include "ultima6/session.h"

#include "ultima6/describe.h"
#include "ultima6/stats_view.h"

#include <algorithm>
#include <cstdlib>

namespace Ultima6 {
namespace Ultima6 {

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
	case 37: // Mass Awaken
	case 38: // Mass Sleep
	case 102: // Mass Curse
	case 116: { // Mass Kill
		std::vector<int> actors;
		crowd(actors);
		for (int t : actors) {
			if (index == 37)
				a.status[t] &= ~0x04;
			else if (index == 38)
				a.status[t] |= 0x04;
			else if (index == 102)
				_world.objects[_world.actorObject(t)].status |= 0x40;
			else
				_combat.applyDamage(caster, t, 255);
		}
		return true;
	}
	case 85: // Mass Protect: the party
	case 103: // Mass Invis
		for (int m = 0; m < a.partySize; ++m) {
			int t = a.partyRoster[m];
			if (index == 85)
				a.status[t] |= 0x01;
			else
				_world.objects[_world.actorObject(t)].status |= 0x02;
		}
		return true;
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
	case 51: // Fire Field
	case 55: // Poison Field
	case 56: // Sleep Field
	case 64: { // Energy Field: a field object (0x13D fire, 0x13E poison, 0x13F sleep, 0xAF energy) on the square if nothing blocks it
		int type = index == 51 ? 0x13D : index == 55 ? 0x13E : index == 56 ? 0x13F : 0xAF;
		bool blocked = _world.flags.flags1[_world.map.tile(z, tx, ty) & (TileFlags::TILES - 1)] & 0x02;
		for (int i : here)
			if (_world.objects[i].actor >= 0 || (_world.flags.flags1[(_world.baseTile.firstTile[_world.objects[i].type] + _world.objects[i].frame) & (TileFlags::TILES - 1)] & 0x02))
				blocked = true;
		if (blocked)
			return false;
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
		_world.addToMap((int)_world.objects.size() - 1);
		return true;
	}
	case 33: // Dispel Field
	case 54: { // Mass Dispel: every field within three squares
		int radius = index == 54 ? 3 : 0;
		bool any = false;
		for (int dx = -radius; dx <= radius; ++dx)
			for (int dy = -radius; dy <= radius; ++dy) {
				std::vector<int> at;
				_world.objectsAt(tx + dx, ty + dy, z, at);
				for (int i : at) {
					int type = _world.objects[i].type;
					if (type == 0x13D || type == 0x13E || type == 0x13F || type == 0xAF) {
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
	case 65: // Explosion
	case 83: // Flame Wind
	case 84: // Hail Storm
	case 87: // Poison Wind
	case 98: // Energy Wind
	case 113: { // Death Wind: everybody (not the party) within the reach of the spell is hurt, poisoned or killed
		int radius = index == 65 ? 1 : 3;
		for (int actor = 0; actor < ObjList::ACTORS; ++actor) {
			int k = _world.actorObject(actor);
			if (k < 0 || _world.objects[k].deleted || (a.status[actor] & 0x80) || a.hp[actor] == 0 || _world.objects[k].z != z)
				continue;
			if (std::abs((int)_world.objects[k].x - tx) > radius || std::abs((int)_world.objects[k].y - ty) > radius)
				continue;
			if (index == 87)
				a.status[actor] |= 0x08;
			else if (index == 113)
				_combat.applyDamage(caster, actor, 255);
			else
				_combat.applyDamage(caster, actor, 1 + _rng.next() % (index == 65 ? 30 : 20));
		}
		return true;
	}
	case 3: { // Dispel Magic (sub_2227B): sleep, poison, paralysis and charm are taken off; "Failed" when there was nothing to take off
		if (target < 0)
			return false;
		bool any = false;
		int tk = _world.actorObject(target);
		if (a.status[target] & 0x04) {
			a.status[target] &= ~0x04;
			_world.objects[tk].type = a.baseObjFrame[target] & 0x3FF;
			_world.objects[tk].frame = (uint8_t)(a.baseObjFrame[target] >> 10);
			any = true;
		}
		if (a.status[target] & 0x08) {
			a.status[target] &= ~0x08;
			any = true;
		}
		if (a.status[target] & 0x02) {
			a.status[target] &= ~0x02;
			any = true;
		}
		if (_world.objects[tk].status & 0x04) {
			endCharm(target);
			any = true;
		}
		if (!any)
			message("\nFailed\n");
		return true;
	}
	case 50: // Disable (sub_21CB4 with 0x32): unless resisted the target is left with one hit point
		if (target < 0)
			return false;
		if (_combat.resists(target, caster)) {
			message("\nFailed\n");
			return true;
		}
		_combat.applyDamage(caster, target, a.hp[target] - 1 > 0 ? a.hp[target] - 1 : 0);
		return true;
	case 80: { // Charm (sub_21D63): unless resisted the target takes the caster's side (the old side is kept in its movement flags), is charmed (object status 4) and fights (worktype 8)
		if (target < 0)
			return false;
		int tk = _world.actorObject(target);
		if (_combat.resists(target, caster)) {
			message("\nFailed\n");
			return true;
		}
		if (_world.objects[tk].status & 0x04) {
			endCharm(target);
			return true;
		}
		a.movementFlags[target] = (uint8_t)((a.movementFlags[target] & 0x9F) | (a.status[target] & 0x60));
		_world.objects[tk].status |= 0x04;
		a.status[target] = (uint8_t)((a.status[target] & 0x9F) | (a.status[caster] & 0x60));
		a.worktype[target] = 8;
		message("\n" + _combat.nameOf(target) + " is charmed.\n");
		return true;
	}
	case 115: { // Mass Charm
		std::vector<int> actors;
		crowd(actors);
		for (int t : actors)
			a.status[t] = (uint8_t)((a.status[t] & 0x9F) | 0x40);
		return true;
	}
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
	case 66: { // Insect Swarm: so are those near the party, less
		int radius = 3;
		for (int actor = 0; actor < ObjList::ACTORS; ++actor) {
			int k = _world.actorObject(actor);
			if (k < 0 || _world.objects[k].deleted || a.hp[actor] == 0 || (a.status[actor] & 0x80) || _world.objects[k].z != z)
				continue;
			if (std::abs((int)_world.objects[k].x - x) <= radius && std::abs((int)_world.objects[k].y - y) <= radius)
				_combat.applyDamage(caster, actor, 1 + _rng.next() % 10);
		}
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
