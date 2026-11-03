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
	case 20: // Sleep
		if (target < 0)
			return false;
		a.status[target] |= 0x04;
		return true;
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
	case 67: // Invisibility
		if (target < 0)
			return false;
		_world.objects[_world.actorObject(target)].status |= 0x02;
		return true;
	case 69: // Paralyze
		if (target < 0)
			return false;
		a.status[target] |= 0x02;
		return true;
	case 101: // Kill
		if (target < 0)
			return false;
		_combat.applyDamage(caster, target, 255);
		return true;
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
	case 25: // Vanish: an object becomes invisible
		if (object < 0)
			return false;
		_world.objects[object].status |= 0x02;
		return true;
	case 19: // Reappear: what is invisible on the square can be seen again
		for (int i : here)
			_world.objects[i].status &= ~0x02;
		return true;
	case 4: // Douse
	case 8: // Ignite
		if (object < 0 || !(_world.objects[object].type == 0x7A || _world.objects[object].type == 0x91 || _world.objects[object].type == 0xA4 ||
							_world.objects[object].type == 0xCE || _world.objects[object].type == 0xFD || _world.objects[object].type == 0x5A))
			return false;
		if (index == 4)
			_world.objects[object].frame = (uint8_t)(_world.objects[object].frame & 2);
		else
			_world.objects[object].frame = (uint8_t)(_world.objects[object].frame | 1);
		return true;
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
	case 3: // Dispel Magic: protection, invisibility and curses are taken off the target
		if (target < 0)
			return false;
		a.status[target] &= ~0x01;
		_world.objects[_world.actorObject(target)].status &= ~(0x02 | 0x40);
		return true;
	case 50: // Disable
		if (target < 0)
			return false;
		a.status[target] |= 0x02;
		return true;
	case 80: // Charm: the target takes the party's side
		if (target < 0)
			return false;
		a.status[target] = (uint8_t)((a.status[target] & 0x9F) | 0x40);
		return true;
	case 115: { // Mass Charm
		std::vector<int> actors;
		crowd(actors);
		for (int t : actors)
			a.status[t] = (uint8_t)((a.status[t] & 0x9F) | 0x40);
		return true;
	}
	case 82: { // Confuse: everybody near turns on everybody
		std::vector<int> actors;
		crowd(actors);
		for (int t : actors)
			a.status[t] = (uint8_t)(a.status[t] | 0x60);
		return true;
	}
	case 99: // Fear: the creatures near flee (worktype 7)
	case 41: { // Repel Undead: so do the undead
		std::vector<int> actors;
		tx = x;
		ty = y;
		crowd(actors);
		for (int t : actors) {
			const CreatureStats *cs = Combat::stats(_world.objects[_world.actorObject(t)].type);
			if (index == 99 || (cs && (cs->flags & CreatureStats::UNDEAD)))
				a.worktype[t] = 7;
		}
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
	case 71: { // Reveal: whatever is invisible near the party shows itself
		for (size_t i = 0; i < _world.objects.size(); ++i) {
			World::Obj &o = _world.objects[i];
			if (!o.deleted && o.onMap() && o.z == z && std::abs((int)o.x - x) <= 10 && std::abs((int)o.y - y) <= 10)
				o.status &= ~0x02;
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
	case 96: { // Chain Bolt: the target and its neighbours are struck
		if (target < 0)
			return false;
		int tk = _world.actorObject(target);
		for (int actor = 0; actor < ObjList::ACTORS; ++actor) {
			int k = _world.actorObject(actor);
			if (k < 0 || _world.objects[k].deleted || a.hp[actor] == 0 || (a.status[actor] & 0x80) || _world.objects[k].z != z)
				continue;
			if (std::abs((int)_world.objects[k].x - (int)_world.objects[tk].x) <= 2 && std::abs((int)_world.objects[k].y - (int)_world.objects[tk].y) <= 2)
				_combat.applyDamage(caster, actor, 1 + _rng.next() % 30);
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
	case 120: // Time Stop
		_world.actors.globals[0x12 + 14] = (uint8_t)(10 + _rng.next() % 10);
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
	case 49: // Conjure: a creature of a random kind
	case 119: { // Summon
		static const int KINDS[] = {0x16B, 0x16C, 0x17C, 0x182, 0x188, 0x1A8};
		int c = _creatures.spawn(KINDS[_rng.next() % 6], 0x08, x + 1, y, z);
		if (c < 0)
			return false;
		a.status[c] = 0x40; // the caster's side
		return true;
	}
	case 21: // Telekinesis: the object is used from where the caster stands
		if (object < 0)
			return false;
		_telekinesis = true;
		useObject(object);
		_telekinesis = false;
		return true;
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
