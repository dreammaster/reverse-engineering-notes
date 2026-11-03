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
