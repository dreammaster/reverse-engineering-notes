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
		int dead = _world.objects[object].quality;
		if (!(a.status[dead] & 0x10))
			return false;
		a.status[dead] &= ~0x10;
		a.hp[dead] = (uint8_t)ActorStats::maxHealth(_world, dead);
		if (!(a.status[dead] & 0x80))
			a.worktype[dead] = 0x81;
		for (size_t i = 0; i < _world.objects.size(); ++i)
			if (!_world.objects[i].deleted && _world.objects[i].location() == ObjRecord::IN_CONTAINER && _world.objects[i].parent == object) {
				_world.objects[i].status = (uint8_t)((_world.objects[i].status & ~ObjRecord::LOCATION_MASK) | ObjRecord::IN_INVENTORY);
				_world.objects[i].parent = dead;
			}
		_world.deleteObject(object);
		return true;
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
