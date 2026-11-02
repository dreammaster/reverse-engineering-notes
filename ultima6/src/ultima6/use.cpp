#include "ultima6/session.h"

#include "ultima6/describe.h"
#include "ultima6/stats_view.h"

#include <algorithm>
#include <cstdlib>

namespace Ultima6 {
namespace Ultima6 {

// The 85 object types cmd_use accepts (the table at cs:6467 and ds:1C30; sub_2F86E tests a type against it). Handlers not written yet are marked below.
static const uint16_t USE_TYPES[85] = {
	0x3E, 0x3F, 0x40, 0x49, 0x4D, 0x4E, 0x57, 0x5A, 0x5D, 0x5F, 0x60, 0x61, 0x62, 0x67, 0x68, 0x73, 0x74, 0x75, 0x7A, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x87, 0x91,
	0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0xA4, 0xA9, 0xAE, 0xB4, 0xB5, 0xB6, 0xB8, 0xBA, 0xC0, 0xCE, 0xD1, 0xD2, 0xD4, 0xDD, 0xDF, 0xE9, 0xEA, 0xEC, 0xF2, 0xF3, 0xF4,
	0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFD, 0x108, 0x109, 0x10C, 0x10E, 0x113, 0x116, 0x118, 0x120, 0x128, 0x129, 0x12A, 0x12B, 0x12C, 0x131, 0x139, 0x14E, 0x19C, 0x19E,
	0x19F, 0x1A3, 0x1A4, 0x1A7, 0x1AC, 0x1AE, 0x1AF};

static bool usable(int type) {
	for (uint16_t t : USE_TYPES)
		if (t == type)
			return true;
	return false;
}

static bool isFood(int type) {
	switch (type) {
	case 0x5F: case 0x60: case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x87: case 0xB4: case 0xB8: case 0xD1: case 0xD2: case 0x109:
		return true;
	default:
		return false;
	}
}

static bool isSearchable(int type) {
	switch (type) {
	case 0x73: case 0x74: case 0x75: case 0xBA: case 0xC0: case 0x10E: case 0x19C: case 0x19E: case 0x19F: case 0x1A7:
		return true;
	default:
		return false;
	}
}

static bool isLight(int type) {
	return type == 0x7A || type == 0x91 || type == 0xA4 || type == 0xCE || type == 0xFD;
}

int Game::actingActor() const {
	return _world.actors.partyRoster[_member];
}

bool Game::carriedObject(const World::Obj &o) const {
	return o.location() == ObjRecord::IN_INVENTORY || o.location() == ObjRecord::READIED;
}

// One of a stack is used up (a byte quantity above 1 counts down), anything else is deleted (sub_1AC65).
void Game::consumeOne(int idx) {
	World::Obj &o = _world.objects[idx];
	if (_world.isStackable(o.type) && o.quantity > 1)
		--o.quantity;
	else
		_world.deleteObject(idx);
}

// cmd_use (35809) after the target is chosen: what the object is called, then the handler of its type. The caller prints the next prompt.
void Game::useObject(int idx) {
	World::Obj &o = _world.objects[idx];
	bool carried = carriedObject(o);
	message(o.type == 0x1AF ? std::string("horse") : Describe::object(_world, o));
	message("\n");
	if (!usable(o.type)) {
		message("\nNot usable\n");
		return;
	}
	if (!carried) {
		// within one square of the acting member, else "Out of range!"
		int a = _world.actorObject(actingActor());
		if (a >= 0 && (std::abs((int)_world.objects[a].x - (int)o.x) > 1 || std::abs((int)_world.objects[a].y - (int)o.y) > 1) && o.type != 0x1AE && o.type != 0x1AF && o.type != 0x1AC) {
			message("\nOut of range!\n");
			return;
		}
	}
	int type = o.type;
	if (type == 0x3E) {
		if (!useVortexCube())
			message("\nNo effect\n");
	} else if (type == 0x3F || type == 0x40) {
		// use_key_or_lockpick: on what?
		message("On ");
		_useItem = idx;
		_pending = 'k';
		return;
	} else if (isFood(type)) {
		message("\nYou eat the food.\n");
		consumeOne(idx);
	} else if (isSearchable(type)) {
		if (!carried)
			searchHere(idx);
		else
			message("\nNot usable\n");
	} else if (type == 0x62) {
		if (carried)
			message("\nNot usable\n");
		else
			useChest(idx, false, false, false);
	} else if (type >= 0x129 && type <= 0x12C) {
		message("\n");
		useKeyOnDoor(idx, false, false, false);
	} else if (isLight(type)) {
		extinguishLight(idx);
	} else if (type == 0xD4) {
		message("\nYou feel a breeze.\n");
	} else if (type == 0xA9) {
		message("\nSqueak!\n");
	} else if (type == 0x61) {
		message("\nYou study the scroll!\n");
	} else if (type == 0x113) {
		message("On whom: ");
		_useItem = idx;
		_pending = 'p';
		return;
	} else if (type == 0x1AC || type == 0xE9) {
		fillBucket(type == 0x1AC);
	} else if (type == 0xB5) {
		// use_churn: a bucket of milk becomes butter and an empty bucket
		int milk = findInInventory(0xB4);
		if (milk < 0) {
			message("\nYou need some milk.\n");
		} else {
			_world.deleteObject(milk);
			_world.addToInventory(actingActor(), 0x60, 0, 1);
			_world.addToInventory(actingActor(), 0xB2, 0, 1);
			message("\nDone\n");
		}
	} else if (type == 0xB6) {
		// use_beehive: a honey jar is filled
		int jar = findInInventory(0xB7);
		if (jar < 0) {
			message("\nYou need a honey jar.\n");
		} else {
			_world.objects[jar].type = 0xB8;
			message("\nDone\n");
		}
	} else if (type == 0x108) {
		useFishingPole();
	} else if (type == 0xDF) {
		// light_powder_keg: frame 0 is lit and explodes three minutes later (APPROX(51): the blast is simplified)
		if (o.frame != 0) {
			message("\nNo effect\n");
		} else if (_kegTimer) {
			message("\nNot now\n");
		} else {
			o.frame = 1;
			_kegTimer = 3;
			_kegObject = idx;
			message("\nPowder lit!\n");
		}
	} else if (type == 0xDD) {
		message("\nFire!\n"); // APPROX(51): use_cannon (330C5) fires a ball along the cannon's direction; not done
	} else if (type == 0xEC || type == 0x1A3) {
		// ring_bell: an animation and a sound, no text
		} else if (type == 0x10C || type == 0xAE) {
		useLever(idx);
	} else if (type == 0x14E) {
		World::Obj &d = _world.objects[idx];
		d.frame = (uint8_t)((d.frame == 1 || d.frame == 3) ? d.frame - 1 : d.frame + 1); // APPROX(46)
	} else if (type == 0x131) {
		useLadder(idx);
	} else {
		// APPROX(47): the other handlers of the table are not written yet
		message("\nNot usable\n");
	}
	_world.spendMoves(actingActor(), 5); // the common tail of cmd_use: spend_moves(5)
}

// use_key_on_door (320D4): `lock` locks or unlocks (a key was used), `magic` locks by magic, `unlock` unlocks by magic. Door frames: 0-3 open, 4-7 closed,
// 8-11 locked, 12-15 magically locked.
void Game::useKeyOnDoor(int idx, bool lock, bool magic, bool unlock) {
	World::Obj &o = _world.objects[idx];
	int f = o.frame;
	if (f < 4) {
		if (!lock && !magic && !unlock) {
			message("closed!\n");
			o.frame = (uint8_t)(f ^ 4);
		} else {
			message("Can't (Un)lock an opened door\n");
		}
	} else if (f < 8) {
		if (lock) {
			message("locked!\n");
			o.frame = (uint8_t)(f ^ 0xC);
		} else if (magic) {
			message("magically locked!\n");
			o.frame = (uint8_t)(f ^ 8);
		} else {
			// APPROX(47): the trap test of an opening door (sub_19FA2 for type 0x151, trigger_trap) is not done
			message("opened!\n");
			o.frame = (uint8_t)(f ^ 4);
		}
	} else if (f < 12) {
		if (lock) {
			message("unlocked!\n");
			o.frame = (uint8_t)(f ^ 0xC);
		} else if (magic) {
			message("magically locked!\n");
			o.frame = (uint8_t)(f ^ 4);
		} else {
			message("locked\n");
		}
	} else if (unlock) {
		message("unlocked!\n");
		o.frame = (uint8_t)(f ^ 8);
	} else {
		message("magically locked!\n");
	}
}

// use_door (3224C), the chest: frame 0 open, 1 closed, 2 locked, 3 magically locked. Opening one lets its contents spill out (search_here).
void Game::useChest(int idx, bool lock, bool magic, bool unlock) {
	World::Obj &o = _world.objects[idx];
	int f = o.frame;
	message("\n");
	bool noEffect = false;
	if (f == 0) {
		if (lock || magic || unlock) {
			noEffect = true;
		} else {
			message("closed!\n");
			o.frame = 1;
		}
	} else if (f == 1) {
		if (!lock && !magic && !unlock) {
			message("opened!\n"); // APPROX(47): the trap test (sub_19FA2, trigger_trap) is not done
			o.frame = 0;
			searchHere(idx);
		} else if (lock) {
			message("locked!\n");
			o.frame = 2;
		} else {
			noEffect = true;
			if (magic) {
				message("magically locked!\n");
				o.frame = 3;
				noEffect = false;
			}
		}
	} else if (f == 2) {
		if (lock) {
			message("unlocked!\n");
			o.frame = 1;
		} else if (magic) {
			message("magically locked!\n");
			o.frame = 3;
		} else {
			noEffect = true;
		}
	} else if (unlock) {
		message("unlocked!\n");
		o.frame = 1;
	} else {
		noEffect = true;
	}
	if (noEffect)
		message("No effect\n");
}

// search_here (30031): the contents of a container come out onto the square it stands on, named one by one.
void Game::searchHere(int idx) {
	World::Obj &o = _world.objects[idx];
	int a = _world.actorObject(actingActor());
	if (a < 0 || o.actor >= 0 || std::abs((int)_world.objects[a].x - (int)o.x) > 1 || std::abs((int)_world.objects[a].y - (int)o.y) > 1)
		return;
	if (_world.flags.flags1[(_world.baseTile.firstTile[o.type] + o.frame) & (TileFlags::TILES - 1)] & 0x08 || o.type == 0x39)
		return;
	message("\nSearching here, you find ");
	std::vector<int> contents;
	for (size_t i = 0; i < _world.objects.size(); ++i) {
		const World::Obj &c = _world.objects[i];
		if (!c.deleted && c.location() == ObjRecord::IN_CONTAINER && c.parent == idx && c.type != 0x150 && c.type != 0x151)
			contents.push_back((int)i);
	}
	if (contents.empty()) {
		message("nothing.\n");
		return;
	}
	for (size_t n = 0; n < contents.size(); ++n) {
		World::Obj &c = _world.objects[contents[n]];
		message(Describe::object(_world, c));
		if (n + 2 < contents.size())
			message(", ");
		else if (n + 2 == contents.size())
			message(" and ");
		c.status = (uint8_t)(c.status & ~ObjRecord::LOCATION_MASK);
		c.parent = -1;
		c.x = o.x;
		c.y = o.y;
		c.z = o.z;
		_world.addToMap(contents[n]);
	}
	message(".\n");
}

// extinguish_light (32886): the name, then lit or doused (bit 0 of the frame).
void Game::extinguishLight(int idx) {
	World::Obj &o = _world.objects[idx];
	switch (o.type) {
	case 0xCE:
		message("brazier");
		break;
	case 0x91:
		message("candelabra");
		break;
	case 0xFD:
		message("campfire");
		break;
	case 0x7A:
		message("candle");
		break;
	default:
		message("fireplace");
		break;
	}
	if (o.frame & 1) {
		o.frame = (uint8_t)(o.frame & 2);
		message(" is doused.\n");
	} else {
		o.frame = (uint8_t)(o.frame | 1);
		message(" is lit.\n");
	}
}

// APPROX(46): levers (268) and switches (174) work as in Nuvie, with the original's texts; use_lever (33B09) and use_switch (33D02) are not traced
void Game::useLever(int idx) {
	World::Obj &o = _world.objects[idx];
	int type = o.type == 0x10C ? 268 : 174;
	// a lever or switch toggles every barrier at the doorways (type 301) that carry its quality: the portcullis (310) or electric field (175)
	int barrier = type == 268 ? 310 : 175;
	bool any = false;
	size_t count = _world.objects.size();
	for (size_t d = 0; d < count; ++d) {
		const World::Obj door = _world.objects[d];
		if (door.deleted || door.type != 301 || !door.onMap() || door.z != o.z || door.quality != o.quality)
			continue;
		any = true;
		std::vector<int> there;
		_world.objectsAt(door.x, door.y, door.z, there);
		int existing = -1;
		for (int t : there)
			if (_world.objects[t].type == barrier)
				existing = t;
		if (existing >= 0) {
			_world.deleteObject(existing);
		} else {
			World::Obj b = door;
			b.type = (uint16_t)barrier;
			b.quality = 0;
			b.frame = barrier == 310 && door.frame == 9 ? 1 : (barrier == 310 ? door.frame : 0);
			_world.objects.push_back(b);
			_world.addToMap((int)_world.objects.size() - 1);
		}
	}
	_world.objects[idx].frame = (uint8_t)(_world.objects[idx].frame ^ 1);
	message(std::string("\n") + (type == 268 ? "Switch the lever, " : "Operate the switch, ") + (any ? "you hear a noise.\n" : "strange, nothing happened.\n"));
}

// APPROX(30): ladders as in Nuvie (the original's climb_ladder, 186DE, is not traced)
void Game::useLadder(int idx) {
	const World::Obj o = _world.objects[idx];
	int nx = o.x, ny = o.y, nz = o.z;
	if (o.frame == 0) { // down
		if (o.z == 0) {
			nx = (o.x & 0x07) | ((o.x >> 2) & 0xF8);
			ny = (o.y & 0x07) | ((o.y >> 2) & 0xF8);
		}
		nz = o.z + 1;
	} else { // up; the quality says which surface chunk of the 4 x 4 around the ladder to come up in
		if (o.z == 1) {
			nx = o.x / 8 * 8 * 4 + ((o.quality & 0x03) * 8) + (o.x - o.x / 8 * 8);
			ny = o.y / 8 * 8 * 4 + (((o.quality >> 2) & 0x03) * 8) + (o.y - o.y / 8 * 8);
		}
		nz = o.z - 1;
	}
	message("\n");
	changeLevel(nx, ny, nz);
}

// The target of a key or lock pick (use_key_or_lockpick, 3241E): a door or a chest within one square.
void Game::useKeyOn(int tx, int ty) {
	int item = _useItem;
	_useItem = -1;
	struct Tail {
		Game &g;
		~Tail() {
			g._world.spendMoves(g.actingActor(), 5);
		}
	} tail{*this};
	World::Obj &key = _world.objects[item];
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	int target = -1;
	for (size_t i = here.size(); i-- > 0;) {
		if (_world.objects[here[i]].actor < 0 && !_world.objects[here[i]].invisible()) {
			target = here[i];
			break;
		}
	}
	if (target < 0) {
		message("nothing\n");
		return;
	}
	World::Obj &t = _world.objects[target];
	message(Describe::object(_world, t));
	message("\n");
	if (!((t.type >= 0x129 && t.type <= 0x12C) || t.type == 0x62)) {
		message("No effect\n");
		return;
	}
	int actor = actingActor();
	if (key.type == 0x3F) {
		// a lock pick may break: a random 1-30 below (45 - dexterity) / 2 (sub_323C4)
		int roll = 1 + _rng.next() % 30;
		if (roll < (45 - _combat.adjustedDexterity(actor)) / 2) {
			message("\nKey broke.\n");
			consumeOne(item);
			return;
		}
	}
	bool fits = (key.type == 0x40 && t.quality != 0 && key.quality == t.quality) || (key.type == 0x3F && t.quality == 0);
	if (!fits) {
		message("\nNo effect\n");
		return;
	}
	if (t.type == 0x62)
		useChest(target, true, false, false);
	else {
		message("\n");
		useKeyOnDoor(target, true, false, false);
	}
}

// use_potion (32EC2) on a party member: what the potion does depends on its frame (the colour); a potion is used up unless nobody could be chosen.
void Game::usePotionOn(int actor) {
	int item = _useItem;
	_useItem = -1;
	ObjList &a = _world.actors;
	int k = _world.actorObject(actor);
	if (actor < 0 || k < 0) {
		message("nothing\n");
		return;
	}
	message(_combat.nameOf(actor));
	message("\n");
	if (!(a.status[actor] & 0x80)) {
		message("No effect\n");
		return;
	}
	int frame = _world.objects[item].frame;
	World::Obj &body = _world.objects[k];
	const CreatureStats *cs = Combat::stats(body.type);
	bool noEffect = false;
	switch (frame) {
	case 0: // wake up
		if (a.status[actor] & 0x04) {
			a.status[actor] &= ~0x04;
			body.type = a.baseObjFrame[actor] & 0x3FF;
			body.frame = (uint8_t)(a.baseObjFrame[actor] >> 10);
		} else {
			noEffect = true;
		}
		break;
	case 1: // cure poison
		if (a.status[actor] & 0x08)
			a.status[actor] &= ~0x08;
		else
			noEffect = true;
		break;
	case 2: { // heal: sub_2220D(6), 1-30 points
		if (a.status[actor] & 0x10)
			break;
		int roll = 1 + _rng.next() % 30;
		int most = ActorStats::maxHealth(_world, actor);
		a.hp[actor] = (uint8_t)(most > a.hp[actor] + roll ? a.hp[actor] + roll : most);
		break;
	}
	case 3: // poison
		if (cs && (cs->flags & 4)) {
			noEffect = true;
		} else {
			a.status[actor] |= 0x08;
		}
		break;
	case 4: // sleep
		if (cs && (cs->flags & 0x80)) {
			noEffect = true;
		} else {
			a.status[actor] |= 0x04; // APPROX(47): sub_2B5A4's change of the sleeper's picture is not done
		}
		break;
	case 5: // protection
		if (a.status[actor] & 0x01)
			noEffect = true;
		else
			a.status[actor] |= 0x01;
		break;
	case 6: // invisibility (the object status bit 2)
		if (body.status & 0x02)
			noEffect = true;
		else
			body.status |= 0x02;
		break;
	default: // 7: sub_241CC(0x49), a light that shows the hidden; APPROX(47): not done
		break;
	}
	if (noEffect)
		message("\nNo effect\n");
	consumeOne(item);
	_world.spendMoves(actingActor(), 5);
}

// The first object of a type in the acting member's inventory (sub_19FA2), or -1.
int Game::findInInventory(int type) {
	std::vector<int> inv;
	_world.inventory(actingActor(), inv);
	for (int i : inv)
		if (_world.objects[i].type == type)
			return i;
	return -1;
}

// fill_bucket_milk (32D77) at a cow, fill_bucket_water (32CF1) at a well: an empty bucket (0xB2) becomes a bucket of milk (0xB4) or water (0xB3) if it can be carried.
void Game::fillBucket(bool milk) {
	int bucket = findInInventory(0xB2);
	if (bucket < 0) {
		message("\nYou need a bucket.\n");
		return;
	}
	int actor = actingActor();
	if (_world.actors.strength[actor] * 20 < _world.carriedWeight(actor) + 0x20) {
		message(milk ? "\nThe bucket of milk is too heavy for you.\n" : "\nThe bucket of water is too heavy for you.\n");
		return;
	}
	_world.deleteObject(bucket);
	_world.addToInventory(actor, milk ? 0xB4 : 0xB3, 0, 1);
	message("\nDone\n");
}

// use_fishing_pole (343D6): water (flag 1 of TILEFLAG) in the eight squares around the member; one fish a day, one try in five.
void Game::useFishingPole() {
	int a = _world.actorObject(actingActor());
	bool water = false;
	if (a >= 0) {
		int ax = _world.objects[a].x, ay = _world.objects[a].y;
		int mask = z == 0 ? MapData::SURFACE_SIZE - 1 : MapData::DUNGEON_SIZE - 1;
		for (int dy = -1; dy <= 1 && !water; ++dy)
			for (int dx = -1; dx <= 1 && !water; ++dx) {
				if (!dx && !dy)
					continue;
				int f = _world.flags.flags1[_world.map.tile(z, (ax + dx) & mask, (ay + dy) & mask) & (TileFlags::TILES - 1)];
				water = f & 1;
			}
	}
	if (!water) {
		message("\nYou need to stand next to water.\n");
		return;
	}
	if (_fishedToday || _rng.next() % 5 != 0) {
		message("\nDidn't get a fish.\n");
		return;
	}
	int actor = actingActor();
	if (_world.actors.strength[actor] * 20 < _world.carriedWeight(actor) + 5) {
		message("\nGot it, but can't carry it.\n");
		return;
	}
	message("\nGot it!\n");
	_world.addToInventory(actor, 0x109, 0, 1);
	_fishedToday = true;
}

// sub_333BB: the keg is gone and what stands around its square takes 1-60 points of damage. APPROX(51): the original asks sub_38962 for the objects in the blast
// (the explosion animation 0x189) and then calls blow_up_door; here the eight squares around and the square itself are hit and doors are not destroyed.
void Game::explodeKeg() {
	if (_kegObject < 0)
		return;
	World::Obj keg = _world.objects[_kegObject];
	int kx = keg.x, ky = keg.y, kz = keg.z;
	if (carriedObject(keg) && keg.parent >= 0) {
		int owner = _world.actorObject(keg.parent);
		if (owner >= 0) {
			kx = _world.objects[owner].x;
			ky = _world.objects[owner].y;
			kz = _world.objects[owner].z;
		}
	}
	_world.deleteObject(_kegObject);
	_kegObject = -1;
	message("\nBoom!\n");
	for (int actor = 0; actor < ObjList::ACTORS; ++actor) {
		int k = _world.actorObject(actor);
		if (k < 0 || _world.objects[k].deleted || _world.objects[k].z != kz)
			continue;
		if (std::abs((int)_world.objects[k].x - kx) <= 1 && std::abs((int)_world.objects[k].y - ky) <= 1)
			_combat.applyDamage(1, actor, 1 + _rng.next() % 60);
	}
}

// The Use command's target: the objects on the square the player points at (cmd_use selects the top one).
void Game::useAt(int tx, int ty) {
	if (useHorse(tx, ty)) {
		prompt();
		return;
	}
	if (boardOrLeave(tx, ty)) {
		prompt();
		return;
	}
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	int pick = -1;
	for (size_t i = here.size(); i-- > 0;) {
		const World::Obj &o = _world.objects[here[i]];
		if (o.actor >= 0 || o.invisible())
			continue;
		if (pick < 0)
			pick = here[i];
		if (usable(o.type)) {
			pick = here[i];
			break;
		}
	}
	if (pick < 0) {
		message("nothing\n");
	} else {
		useObject(pick);
	}
	if (!_pending)
		prompt();
}

} // namespace Ultima6
} // namespace Ultima6
