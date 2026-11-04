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
	if (!carried && !_telekinesis) {
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
	} else if (type == 0x4E) {
		useStaff(idx);
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
		_scrollStudied = true;
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
		// use_cannon (330C5): the ball flies along the cannon's facing (frame 0 north, 1 east, 2 south, 3 west) up to five squares and hurts the first person it meets for 1-60.
		// APPROX(51): the flight test (`sub_2BB2F`), the objects it can hit and the blown-up doors (`blow_up_door`) are not done; a wall stops it
		message("\nFire!\n");
		static const int CX[4] = {0, 1, 0, -1}, CY[4] = {-1, 0, 1, 0};
		int dir = o.frame & 3, mask = z == 0 ? MapData::SURFACE_SIZE - 1 : MapData::DUNGEON_SIZE - 1;
		for (int d = 1; d <= 5; ++d) {
			int tx = (o.x + CX[dir] * d) & mask, ty = (o.y + CY[dir] * d) & mask;
			if (_world.flags.flags1[_world.map.tile(z, tx, ty) & (TileFlags::TILES - 1)] & 0x02)
				break;
			std::vector<int> there;
			_world.objectsAt(tx, ty, z, there);
			int victim = -1;
			for (int t : there)
				if (_world.objects[t].actor >= 0)
					victim = _world.objects[t].actor;
			if (victim >= 0) {
				_combat.applyDamage(actingActor(), victim, 1 + _rng.next() % 60);
				break;
			}
		}
	} else if (type == 0x99 || type == 0x9C || type == 0x9D || type == 0x9E || type == 0x128) {
		// play_instrument (329EA): a tune, no text (there is no sound yet)
	} else if (type == 0xEC || type == 0x1A3) {
		// ring_bell: an animation and a sound, no text
		} else if (type == 0x5D) {
		// use_sextant (2536C): latitude and longitude in degrees ({ is the degree sign of the font), only on the surface and on level 5
		if (z != 0 && z != 5) {
			message("\nNot usable\n");
		} else {
			int sx = z ? x * 4 : x, sy = z ? y * 4 : y;
			int lat = (sy - 360) >> 3, lon = (sx - 304) >> 3;
			message("\n" + std::to_string(std::abs(lat)) + "{" + (lat > 0 ? "S" : "N"));
			message(", " + std::to_string(std::abs(lon)) + "{" + (lon > 0 ? "E\n" : "W\n"));
		}
	} else if (type == 0xEA) {
		// use_fountain (3452B): a wish that is one of five foods gives some food one time in five
		message("Make a wish?");
		_useItem = idx;
		_pending = 'w';
		return;
	} else if (type >= 0xF2 && type <= 0xF9) {
		message("Mantra: ");
		_useItem = idx;
		_textActive = true;
		_textBuf.clear();
		_textMax = 4;
		_textPurpose = 1;
		return;
	} else if (type == 0x4D) {
		// use_gems (3282F): the overhead map (sub_39866) is shown and one gem is used up
		_gem = true;
		consumeOne(idx);
		_world.spendMoves(actingActor(), 5);
		return;
	} else if (type == 0x120) {
		operateDrawbridge(idx);
	} else if (type == 0x1A4) {
		// sub_34053 (the balloon, on the surface or on level 5 only): it is used up and a balloon (0x1A7, frame 3) is set up on the first free square next to the user
		if (z != 0 && z != 5) {
			message("\nNot usable\n");
		} else {
			int a = _world.actorObject(actingActor());
			int ax = _world.objects[a].x, ay = _world.objects[a].y;
			_world.deleteObject(idx);
			bool placed = false;
			MoveRules rules;
			rules.partyMayShuffle = false;
			for (int dx = -1; dx <= 1 && !placed; ++dx)
				for (int dy = -1; dy <= 1 && !placed; ++dy) {
					if (!dx && !dy)
						continue;
					if (!_movement.check(ax + dx, ay + dy, z, _leaderObject, rules).passable)
						continue;
					World::Obj b;
					b.status = 0;
					b.x = (uint16_t)(ax + dx);
					b.y = (uint16_t)(ay + dy);
					b.z = (uint8_t)z;
					b.type = 0x1A7;
					b.frame = 3;
					b.quantity = 1;
					b.quality = 0;
					b.actor = -1;
					b.parent = -1;
					_world.objects.push_back(b);
					_world.addToMap((int)_world.objects.size() - 1);
					message("\nDone!\n");
					placed = true;
				}
			if (!placed)
				message("\nNo room!\n");
		}
	} else if (type == 0x139) {
		useSilverHorn();
	} else if (type == 0x116 || type == 0x118) {
		// operate_passthrough (35482): frame 0 is shut; opening slides it one square north (0x116) or west (0x118) and sets bit 1 of the frame, shutting slides it back
		// unless somebody stands in that square
		int f = o.frame, ox = o.x, oy = o.y, oz = o.z;
		if (f == 0) {
			_world.moveObject(idx, type == 0x116 ? ox : ox - 1, type == 0x116 ? oy - 1 : oy, oz);
			_world.objects[idx].frame = (uint8_t)(f | 2);
			message("\nOpen the passthrough.\n");
		} else {
			int nx = type == 0x116 ? ox : ox + 1, ny = type == 0x116 ? oy + 1 : oy;
			std::vector<int> there;
			_world.objectsAt(nx, ny, oz, there);
			bool blocked = false;
			for (int t : there)
				blocked |= _world.objects[t].actor >= 0;
			if (blocked) {
				message("\nNot now!\n");
			} else {
				_world.moveObject(idx, nx, ny, oz);
				_world.objects[idx].frame = (uint8_t)(f & 1);
				message("\nClose the passthrough.\n");
			}
		}
	} else if (type == 0x9B) {
		// use_crystal_ball (34FC5): a place given by latitude and longitude is shown until a key is pressed. APPROX(61): the test at its start (`sub_323F1`, which can hurt the
		// user with `print_wound_text`) is left out
		message("\nEnter degrees followed by N, S, E or W.\n\nAt latitude=");
		_textActive = true;
		_textBuf.clear();
		_textMax = 4;
		_textPurpose = 3;
		return;
	} else if (type == 0x9A) {
		message("Direction-");
		_useItem = idx;
		_pending = 'e';
		return;
	} else if (type == 0x67 || type == 0x68) {
		message("Direction:");
		_useItem = idx;
		_pending = 'v';
		return;
	} else if (type == 0x49) {
		buryMoonstone(idx);
	} else if (type == 0x57) {
		useOrb(idx);
		if (_pending)
			return;
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

// use_lever (33B09) and use_switch (33D02): the lever or switch turns over (frame ^ 1), and with a quality that is not 0 every doorway (type 0x12D) of the level with that
// quality has its barrier toggled: a lever's portcullis (0x136: frame 3 when the doorway's frame has bit 1, else 1; "can't close the portcullis" if somebody stands there),
// a switch's electric field (0xAF). The texts after "Switch the lever, " / "Operate the switch, " are "strange, nothing happened.", "you hear a noise." or the refusal.
void Game::useLever(int idx) {
	bool lever = _world.objects[idx].type == 0x10C;
	_world.objects[idx].frame = (uint8_t)(_world.objects[idx].frame ^ 1);
	int quality = _world.objects[idx].quality;
	int result = 0;
	message(lever ? "\nSwitch the lever, " : "\nOperate the switch, ");
	if (quality != 0) {
		int barrier = lever ? 0x136 : 0xAF;
		size_t count = _world.objects.size();
		for (size_t d = 0; d < count && result != 2; ++d) {
			const World::Obj door = _world.objects[d];
			if (door.deleted || door.type != 0x12D || !door.onMap() || door.z != z || door.quality != quality)
				continue;
			std::vector<int> there;
			_world.objectsAt(door.x, door.y, door.z, there);
			int existing = -1;
			bool occupied = false;
			for (int t : there) {
				if (_world.objects[t].type == barrier)
					existing = t;
				if (_world.objects[t].actor >= 0)
					occupied = true;
			}
			if (existing >= 0) {
				_world.deleteObject(existing);
				result = 1;
			} else if (lever && occupied) {
				result = 2;
			} else {
				World::Obj b = door;
				b.type = (uint16_t)barrier;
				b.quality = 0;
				b.frame = lever ? ((door.frame & 2) ? 3 : 1) : 0;
				b.status = 0;
				_world.objects.push_back(b);
				_world.addToMap((int)_world.objects.size() - 1);
				result = 1;
			}
		}
	}
	message(result == 1 ? "you hear a noise.\n" : result == 2 ? "can't close the portcullis.\n" : "strange, nothing happened.\n");
}

// climb_ladder (186DE): the squares come out exactly as the original computes them (surface chunk of the 4 x 4 around a dungeon ladder named by the quality bits); the sound, time
// advance and the `sub_183B2` / `sub_185BE` bookkeeping are not done
void Game::useLadder(int idx) {
	const World::Obj o = _world.objects[idx];
	int nx = o.x, ny = o.y, nz = o.z;
	bool down = z == 0 ? true : (z == 5 || o.frame == 1 ? false : true); // climb_ladder 186F2: on the surface always down, on level 5 always up, else by the ladder's frame
	if (down) { // down
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

// sub_38962 (the explosion 0x189): the people on the squares sixteen scatter bolts (reach 3) from a square cross, and on the square itself
void Game::blastActors(int tx, int ty, std::vector<int> &out) {
	static const int8_t SX[16] = {2, 1, 0, 1, -1, -1, -2, -1, 0, -1, -2, -2, 2, 2, 1, 1}, SY[16] = {-1, 2, -2, -1, 2, -1, 0, 1, 2, -2, -1, 1, 1, 0, 1, -2};
	bool hit[ObjList::ACTORS] = {};
	auto mark = [&](int qx, int qy) {
		std::vector<int> there;
		_world.objectsAt(qx, qy, z, there);
		for (int i : there) {
			int t = _world.objects[i].actor;
			if (t >= 0 && t < ObjList::ACTORS && !(_world.actors.status[t] & 0x10))
				hit[t] = true;
		}
	};
	for (int b = 0; b < 16; ++b) {
		std::vector<std::pair<int, int>> squares;
		_world.boltPath(z, tx, ty, tx + SX[b], ty + SY[b], 3, squares);
		for (const std::pair<int, int> &q : squares)
			mark(q.first, q.second);
	}
	mark(tx, ty);
	for (int t = 0; t < ObjList::ACTORS; ++t)
		if (hit[t])
			out.push_back(t);
}

// blow_up_door (33260): in the 5 x 5 squares round (cx, cy) a powder keg is lit and goes off at once (type 0xDF, frame 1: sub_333BB), and of the doors the first kind found there
// (0x12C, else 0x129, 0x12A, 0x12B) that is not of the unbreakable frames (frame bits 2 and 3 both set) goes: "The door is blown up!"
void Game::blowUpDoors(int cx, int cy) {
	int mask = z == 0 ? 0x3FF : 0xFF;
	auto find = [&](int type, int qx, int qy) {
		std::vector<int> there;
		_world.objectsAt(qx, qy, z, there);
		for (int i : there)
			if (_world.objects[i].type == type && _world.objects[i].actor < 0)
				return i;
		return -1;
	};
	for (int dy = -2; dy <= 2; ++dy)
		for (int dx = -2; dx <= 2; ++dx) {
			int qx = (cx + dx) & mask, qy = (cy + dy) & mask;
			int keg = find(0xDF, qx, qy);
			if (keg >= 0) {
				_world.objects[keg].frame = 1;
				explodeKegObject(keg);
			}
			int door = -1;
			static const int KINDS[4] = {0x12C, 0x129, 0x12A, 0x12B};
			for (int k = 0; k < 4 && door < 0; ++k) {
				int d = find(KINDS[k], qx, qy);
				if (d >= 0 && (_world.objects[d].frame & 0x0C) != 0x0C)
					door = d;
			}
			if (door >= 0 && (_world.objects[door].frame & 0x0C) != 0x0C) {
				_world.deleteObject(door);
				message("\nThe door is blown up!\n");
			}
		}
}

// sub_333BB: the keg goes (from where it lies, or from the one who carries it), what is in the blast takes 1-60 points of damage (people only: the damage routine does nothing to
// the things that stack), and the doors round it are blown up
void Game::explodeKegObject(int idx) {
	World::Obj keg = _world.objects[idx];
	int kx = keg.x, ky = keg.y, kz = keg.z;
	if (carriedObject(keg) && keg.parent >= 0) {
		int owner = _world.actorObject(keg.parent);
		if (owner >= 0) {
			kx = _world.objects[owner].x;
			ky = _world.objects[owner].y;
			kz = _world.objects[owner].z;
		}
	}
	_world.deleteObject(idx);
	std::vector<int> hit;
	blastActors(kx, ky, hit);
	for (int actor : hit)
		_combat.applyDamage(1, actor, 1 + (int)(_rng.next() % 60));
	(void)kz;
	blowUpDoors(kx, ky);
}

void Game::explodeKeg() {
	if (_kegObject < 0)
		return;
	int keg = _kegObject;
	_kegObject = -1;
	explodeKegObject(keg);
}

// read_book (6B7A): "\n\n", then the text of BOOK.DAT for the object's quality. In the text `&...&` marks a translation (shown only once a scroll has been
// studied), `<...>` runic letters (the glyphs with the high bit), `*` a wait for a key.
// APPROX(52): the wait for a key at `*` (and the window's page breaks, flag 0x10) is a blank line; the other escape characters (%, ^, \, @) are dropped
void Game::readBook(int idx) {
	message("\n\n");
	std::string t = _world.bookText(_world.objects[idx].quality);
	bool translation = false, runic = false;
	std::string out;
	for (char c : t) {
		if (c == '&') {
			translation = !translation;
		} else if (translation && !_scrollStudied) {
			continue;
		} else if (c == '<') {
			runic = true;
		} else if (c == '>') {
			runic = false;
		} else if (c == '*') {
			out += "\n";
		} else if (c == '%' || c == '^' || c == '\\' || c == '@') {
			continue;
		} else {
			out += runic && (uint8_t)c > 0x20 ? (char)((uint8_t)c | 0x80) : c;
		}
	}
	message(out);
}

// cmd_drop (sub_30B6A), after the object has been chosen in the inventory: its name; an amulet that is worn cannot be taken off and a lit torch burns out (sub_1E8B6); a stack
// asks "How many? " (ask_quantity); then "Location: " takes a square from the cursor.
void Game::dropStart(int idx) {
	bool fromCommand = _dropPending;
	_dropPending = false;
	World::Obj &o = _world.objects[idx];
	message(Describe::object(_world, o) + "\n");
	if (fromCommand)
		_inventory = false;
	if (o.location() == ObjRecord::READIED && o.type == 0x4C) {
		message("\nMagical energy prevents you from removing the amulet.\n");
		prompt();
		return;
	}
	if (o.location() == ObjRecord::READIED && o.type == 0x5A && o.frame == 1) {
		_world.deleteObject(idx);
		message("\nA torch burned out.\n");
		prompt();
		return;
	}
	int have = _world.isStackable(o.type) ? _world.quantityOf(o) : 1;
	_dropItem = idx;
	if (have > 1 && o.location() != ObjRecord::READIED) { // ask_quantity
		message("How many? ");
		_inventory = false;
		_textActive = true;
		_textBuf.clear();
		_textMax = 3;
		_textPurpose = 5;
		return;
	}
	dropWhere(1);
}

// "Location: " and the cursor for the place the object goes to.
void Game::dropWhere(int count) {
	_dropCount = count;
	_inventory = false;
	message("Location: ");
	_pending = 'd';
	_cursor = true;
	_cx = _cy = 5;
}

// sub_309C0: the first thing on the square, from the top, that is a surface (flags3 bit 2) puts the dropped object on it; anything that blocks (flags1 bit 2) before one does not.
bool Game::dropSurface(int tx, int ty) const {
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	for (size_t i = here.size(); i-- > 0;) {
		int tile = _world.baseTile.firstTile[_world.objects[here[i]].type] + _world.objects[here[i]].frame;
		if (_world.flags.flags3[tile & (TileFlags::TILES - 1)] & 2)
			return true;
		if (_world.flags.flags1[tile & (TileFlags::TILES - 1)] & 2)
			return false;
	}
	return false;
}

// sub_2F739 / sub_2F712: a container (types 0x62, 0x63, 0x39, 0xBA, 0xBC, 0xBF, 0xC0, 0x153, 0x3E) takes things, except 0x39 and 0x153 and, shut (a frame but 0), 0x62, 0xBA and 0xC0;
// the object itself must be none of the three that hold things themselves.
bool Game::dropRecipient(int target, int item) const {
	static const int HOLDERS[9] = {0x62, 0x63, 0x39, 0xBA, 0xBC, 0xBF, 0xC0, 0x153, 0x3E};
	if (target < 0)
		return false;
	const World::Obj &t = _world.objects[target];
	bool holder = false;
	for (int h : HOLDERS)
		holder |= t.type == h;
	if (!holder || t.type == 0x39 || t.type == 0x153)
		return false;
	if ((t.type == 0x62 || t.type == 0xBA || t.type == 0xC0) && t.frame != 0)
		return false;
	int it = _world.objects[item].type;
	return it != 0x62 && it != 0xBA && it != 0xC0;
}

// The place has been chosen: it must be somewhere the object can lie (can_actor_enter), a surface, or a container that takes it, and be in the line of sight within 8 squares; the
// throw costs 3 moves; from further than one square a glass thing breaks (sub_2F7BC); otherwise it goes into the container, or onto the square, and is free to take (sub_2F82E).
// APPROX(32): the flight of the object is not drawn (sub_2BB2F), and neither is the sound of breaking
void Game::dropAt(int tx, int ty) {
	int idx = _dropItem, count = _dropCount;
	_dropItem = -1;
	int member = actingActor(), mk = _world.actorObject(member);
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	int target = here.empty() ? -1 : here.back(); // sub_19C1A: the top thing there
	bool ok = false;
	MoveRules rules;
	if (idx >= 0 && mk >= 0) {
		ok = _movement.check(tx, ty, z, idx, rules).passable || dropSurface(tx, ty) || (target >= 0 && dropRecipient(target, idx));
		std::vector<std::pair<int, int>> path;
		if (ok)
			ok = _world.boltPath(z, _world.objects[mk].x, _world.objects[mk].y, tx, ty, 8, path);
	}
	if (!ok) {
		message("\nNot possible\n");
		prompt();
		return;
	}
	World::Obj &o = _world.objects[idx];
	bool stack = _world.isStackable(o.type);
	int type = o.type;
	if (o.location() == ObjRecord::READIED)
		o.status = (uint8_t)((o.status & ~ObjRecord::LOCATION_MASK) | ObjRecord::IN_INVENTORY);
	_world.spendMoves(member, 3);
	int mx = _world.objects[mk].x, my = _world.objects[mk].y;
	bool near = std::abs(tx - mx) <= 1 && std::abs(ty - my) <= 1;
	static const int GLASS[15] = {0x53, 0x5C, 0x9B, 0x7B, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0xB7, 0xB8, 0x113, 0xA1, 0x1A1};
	bool glass = false;
	for (int g : GLASS)
		glass |= type == g;
	if (!near && glass) {
		if (stack) {
			int left = _world.quantityOf(o) - count;
			if (left > 0)
				_world.setQuantity(o, left);
			else
				_world.deleteObject(idx);
		} else if (type == 0x1A1 || type == 0x7B) { // the broken one lies there
			o.frame = type == 0x1A1 ? 1 : 2;
			dropPlace(idx, 1, tx, ty, -1);
		} else {
			_world.deleteObject(idx);
		}
		sfx(0xF);
		message("\nIt broke!\n");
		prompt();
		return;
	}
	dropPlace(idx, count, tx, ty, target >= 0 && dropRecipient(target, idx) && !dropSurface(tx, ty) ? target : -1);
	prompt();
}

// Puts `count` of an object into a container (a stack joins the one of its kind there) or onto a square, and marks it free to take.
void Game::dropPlace(int idx, int count, int tx, int ty, int recipient, bool free) {
	World::Obj &o = _world.objects[idx];
	int have = _world.quantityOf(o);
	int placed = idx;
	if (_world.isStackable(o.type) && count < have) {
		World::Obj part = o;
		_world.setQuantity(part, count);
		_world.setQuantity(o, have - count);
		_world.objects.push_back(part);
		placed = (int)_world.objects.size() - 1;
	}
	if (recipient >= 0) {
		if (_world.isStackable(_world.objects[placed].type)) {
			for (size_t i = 0; i < _world.objects.size(); ++i) {
				World::Obj &c = _world.objects[i];
				if ((int)i != placed && !c.deleted && c.location() == ObjRecord::IN_CONTAINER && c.parent == recipient && c.type == _world.objects[placed].type && c.frame == _world.objects[placed].frame) {
					_world.setQuantity(c, _world.quantityOf(c) + _world.quantityOf(_world.objects[placed]));
					if (free)
						c.status |= 0x01;
					_world.deleteObject(placed);
					return;
				}
			}
		}
		World::Obj &d = _world.objects[placed];
		d.status = (uint8_t)((d.status & ~ObjRecord::LOCATION_MASK) | ObjRecord::IN_CONTAINER | (free ? 0x01 : 0));
		d.parent = recipient;
		d.x = d.y = 0;
		d.z = 0;
		return;
	}
	World::Obj &d = _world.objects[placed];
	d.status = (uint8_t)((d.status & ~ObjRecord::LOCATION_MASK) | 0x01);
	d.parent = -1;
	d.x = (uint16_t)tx;
	d.y = (uint16_t)ty;
	d.z = (uint8_t)z;
	_world.addToMap(placed);
}

// A line of text typed after a prompt (sub_1737A): letters and digits are echoed, Backspace takes one back, Enter ends it, Escape gives it up.
void Game::textKey(int key) {
	if (key == KEY_ENTER || key == KEY_ESC) {
		bool escaped = key == KEY_ESC;
		if (escaped)
			_textBuf.clear();
		_textActive = false;
		message("\n");
		if (_textPurpose == 7) { // Enchant: the spell for the staff, as the words of casting (a circle number and the letters)
			int staff = _useItem;
			_useItem = -1;
			const Magic::SpellInfo *spell = _textBuf.size() > 1 ? Magic::find(_textBuf[0] - '0', _textBuf.substr(1)) : nullptr;
			int caster = actingActor();
			if (staff < 0 || !spell || !_magic.bookHas(caster, spell->index())) {
				message("none\n");
			} else {
				message(Magic::words(*spell) + "\n");
				World::Obj charge;
				charge.status = ObjRecord::IN_CONTAINER;
				charge.x = charge.y = 0;
				charge.z = 0;
				charge.type = 0x150;
				charge.frame = 0;
				charge.quantity = 1;
				charge.quality = (uint8_t)spell->index();
				charge.actor = -1;
				charge.parent = staff;
				_world.objects.push_back(charge);
			}
			prompt();
			return;
		}
		if (_textPurpose == 6) { // Gate Travel
			int n = _textBuf.empty() ? 0 : std::atoi(_textBuf.c_str()); // sub_2F895 reads the digits (up to four were typed)
			if (n >= 1 && n <= 8)
				gateTo(n - 1);
			prompt(); // a gate that is not there, or no number: the original only plays the failing sound (sub_36E85)
			return;
		}
		if (_textPurpose == 5 || _textPurpose == 8) { // ask_quantity: nothing typed is all of them (sub_2F895: -1); more than there are, or none, is "0   " and ends the dropping
			int idx = _dropItem;
			int have = idx >= 0 ? _world.quantityOf(_world.objects[idx]) : 0;
			int n = _textBuf.empty() ? (escaped ? -2 : have) : std::atoi(_textBuf.c_str());
			if (n < 1 || n > have) {
				_dropItem = -1;
				message("0   \n");
				prompt();
				return;
			}
			if (_textBuf.empty())
				message(std::to_string(n) + "\n");
			if (_textPurpose == 8)
				moveWhere(n);
			else
				dropWhere(n);
			return;
		}
		if (_textPurpose == 2) {
			wishDone(_textBuf);
		} else if (_textPurpose == 3) { // the crystal ball's latitude, then the longitude
			int lat;
			if (!degrees(_textBuf, 'S', 'N', lat) || lat > 0x50 || lat < -44) {
				message("0   \n");
			} else {
				_crystalLat = lat;
				message("\n  longitude=");
				_textActive = true;
				_textBuf.clear();
				_textMax = 4;
				_textPurpose = 4;
				return;
			}
		} else if (_textPurpose == 4) {
			int lon;
			if (!degrees(_textBuf, 'E', 'W', lon) || lon > 0x58 || lon < -37) {
				message("0   \n");
			} else {
				message("\n");
				int vx = lon * 8 + 0x130, vy = _crystalLat * 8 + 0x168;
				if (z != 0) {
					vx >>= 2;
					vy >>= 2;
				}
				_peekX = vx;
				_peekY = vy;
				_peek = true;
				_world.spendMoves(actingActor(), 5);
				return;
			}
		} else {
			shrineChant(_textBuf);
		}
		prompt();
	} else if (key == KEY_BACKSPACE) {
		if (!_textBuf.empty()) {
			_textBuf.erase(_textBuf.size() - 1);
			message("");
		}
	} else if (key >= 32 && key < 127 && (int)_textBuf.size() < _textMax) {
		_textBuf += (char)key;
		message(std::string(1, (char)key));
	}
}

// use_fountain: the wish (Food, Mutton, Wine, Fruit or Meat) is looked up case-blind; one try in five gives a food (0x81) if it can be carried.
void Game::wishDone(const std::string &wish) {
	static const char *const WORDS[6] = {"food", "mutton", "wine", "fruit", "meat", ""};
	std::string t = wish;
	for (char &c : t)
		c = (char)std::tolower((unsigned char)c);
	bool known = false;
	for (const char *w : WORDS)
		known |= t == w;
	if (!known) {
		message("\n\nNo effect\n");
	} else if (_rng.next() % 5 != 0) {
		message("\n\nFailed\n");
	} else {
		int actor = actingActor();
		message("\n\nYou got food");
		if (_world.actors.strength[actor] * 20 < _world.carriedWeight(actor) + 2) {
			message(", but you can't carry it.\n");
		} else {
			_world.addToInventory(actor, 0x81, 0, 1);
			message(".\n");
		}
	}
	_world.spendMoves(actingActor(), 5);
}

// use_shrine (34228): the mantra must be that of this rune's virtue (Ahm, Mu, Ra, Beh, Cah, Summ, Om, Lum); then a moonstone of the colour of the virtue within a square of the
// acting member is the key: the spawners around (sub_3419B) are cleared. "Done" when that happened, else "No effect".
// APPROX(58): sub_3419B's clearing of the creature spawners (eggs of type 0x14F holding 0x16A/0x16B) in the 40 x 40 around is done for those eggs; the object of type 0x33 at the stone is deleted as the original does
void Game::shrineChant(const std::string &mantra) {
	int item = _useItem;
	_useItem = -1;
	static const char *const MANTRAS[8] = {"Ahm", "Mu", "Ra", "Beh", "Cah", "Summ", "Om", "Lum"};
	int virtue = _world.objects[item].type - 0xF2, said = 8;
	for (int i = 0; i < 8; ++i) {
		std::string m = MANTRAS[i], t = mantra;
		for (char &c : m)
			c = (char)std::tolower((unsigned char)c);
		for (char &c : t)
			c = (char)std::tolower((unsigned char)c);
		if (m == t) {
			said = i;
			break;
		}
	}
	if (said == 8 || said != virtue) {
		message("\nWrong mantra!\n");
		_world.spendMoves(actingActor(), 5);
		return;
	}
	int a = _world.actorObject(actingActor());
	bool done = false;
	if (a >= 0) {
		int ax = _world.objects[a].x, ay = _world.objects[a].y;
		for (int dx = -1; dx <= 1 && !done; ++dx)
			for (int dy = -1; dy <= 1 && !done; ++dy) {
				if (dx == 1 && dy == 1)
					continue;
				std::vector<int> here;
				_world.objectsAt(ax + dx, ay + dy, z, here);
				int stone = -1;
				for (int i : here)
					if (_world.objects[i].type == 0x49 && _world.objects[i].frame == virtue)
						stone = i;
				if (stone < 0)
					continue;
				for (int i : here)
					if (_world.objects[i].type == 0x33)
						_world.deleteObject(i);
				int sx = _world.objects[stone].x, sy = _world.objects[stone].y;
				for (size_t e = 0; e < _world.objects.size(); ++e) {
					const World::Obj &egg = _world.objects[e];
					if (egg.deleted || egg.type != 0x14F || !egg.onMap() || egg.z != z || std::abs((int)egg.x - sx) > 20 || std::abs((int)egg.y - sy) > 20)
						continue;
					bool spawner = false;
					for (size_t c = 0; c < _world.objects.size(); ++c)
						if (!_world.objects[c].deleted && _world.objects[c].location() == ObjRecord::IN_CONTAINER && _world.objects[c].parent == (int)e &&
							(_world.objects[c].type == 0x16A || _world.objects[c].type == 0x16B))
							spawner = true;
					if (spawner)
						_world.deleteObject((int)e);
				}
				done = true;
			}
	}
	message(done ? "\nDone\n" : "\nNo effect\n");
	_world.spendMoves(actingActor(), 5);
}

// What a minute does to the conditions of every actor (advance_time 135C3-1373E): protection and invisibility wear off one minute in 64 and curses one in 16 when a roll of
// 1-30 reaches the actor's intelligence (less 3 when cursed, at least 1); paralysis one minute in 4 against its strength; sleep (except the sleep of a schedule, worktype 0x91)
// ends one minute in 16 and the actor takes its own picture again.
void Game::actorStatusDecay() {
	ObjList &a = _world.actors;
	auto roll = [&](int hi) { return 1 + (int)(_rng.next() % hi); };
	for (int actor = 0; actor < ObjList::ACTORS; ++actor) {
		int k = _world.actorObject(actor);
		if (k < 0 || _world.objects[k].deleted)
			continue;
		World::Obj &o = _world.objects[k];
		int intel = a.intelligence[actor];
		if (o.status & 0x40)
			intel = std::max(1, intel - 3);
		if ((o.status & 0x02) && _rng.next() % 64 == 0 && !(intel > roll(30)))
			o.status &= ~0x02;
		if ((a.status[actor] & 0x01) && _rng.next() % 64 == 0 && !(intel > roll(30)))
			a.status[actor] &= ~0x01;
		if ((o.status & 0x40) && _rng.next() % 16 == 0 && !(intel > roll(30)))
			o.status &= ~0x40;
		if ((o.status & 0x04) && _rng.next() % 8 == 0 && !(intel > roll(30)))
			endCharm(actor);
		if ((a.status[actor] & 0x02) && _rng.next() % 4 == 0 && !(_combat.adjustedStrength(actor) > roll(30)))
			a.status[actor] &= ~0x02;
		if ((a.status[actor] & 0x04) && a.worktype[actor] != 0x91 && _rng.next() % 16 == 0) {
			a.status[actor] &= ~0x04;
			o.type = a.baseObjFrame[actor] & 0x3FF;
			o.frame = (uint8_t)(a.baseObjFrame[actor] >> 10);
		}
	}
}

// operate_drawbridge (339CD) with sub_335D7: the bridge pieces (type 0x10D) that have the crank's quality are looked for; the first one's frame says what is done. Frame 3 (the
// flat bridge: rows of 3, 4 ... 4, 5 below the moat's edge) is raised into one row above of 6, 7 ... 7, 8 and the rest of its rows are taken away ("Close the drawbridge.";
// "Can't close" if somebody stands on it); frame 6 is lowered again over the squares whose terrain is not one of 16, 28, 32, 44, 214 ("Open the drawbridge."; "Can't open" if
// something stands there). APPROX(68): the displacement of whoever stands on the last square after lowering (`sub_1B26F`) and the animation/sound calls are not done
void Game::operateDrawbridge(int crank) {
	int quality = _world.objects[crank].quality;
	auto findAt = [&](int px, int py) {
		std::vector<int> there;
		_world.objectsAt(px, py, z, there);
		for (int t : there)
			if (_world.objects[t].type == 0x10D)
				return t;
		return -1;
	};
	auto blocked = [&](int px, int py) {
		std::vector<int> there;
		_world.objectsAt(px, py, z, there);
		for (int t : there)
			if (_world.objects[t].type != 0x10D)
				return true;
		return false;
	};
	auto make = [&](int px, int py, int frame) {
		World::Obj o;
		o.status = 0;
		o.x = (uint16_t)px;
		o.y = (uint16_t)py;
		o.z = (uint8_t)z;
		o.type = 0x10D;
		o.frame = (uint8_t)frame;
		o.quantity = 1;
		o.quality = 0;
		o.actor = -1;
		o.parent = -1;
		_world.objects.push_back(o);
		_world.addToMap((int)_world.objects.size() - 1);
		return (int)_world.objects.size() - 1;
	};
	int first = -1;
	for (size_t i = 0; i < _world.objects.size(); ++i)
		if (!_world.objects[i].deleted && _world.objects[i].onMap() && _world.objects[i].type == 0x10D && _world.objects[i].quality == quality && _world.objects[i].z == z) {
			first = (int)i;
			break;
		}
	if (first < 0) {
		message("\nNo correspondent drawbridge\n");
		return;
	}
	int frame = _world.objects[first].frame, x0 = _world.objects[first].x, y0 = _world.objects[first].y;
	static const int WATERISH[5] = {16, 28, 32, 44, 214};
	auto edge = [&](int px, int py) {
		int t = _world.map.tile(z, px, py) & 0xFF;
		for (int w : WATERISH)
			if (w == t)
				return true;
		return false;
	};
	int code = 0;
	if (frame == 3) {
		// can the bridge be raised? nobody may stand on any of its squares
		bool clear = true;
		for (int row = y0; findAt(x0, row) >= 0 && clear; ++row)
			for (int col = x0; findAt(col, row) >= 0; ++col)
				if (blocked(col, row)) {
					clear = false;
					break;
				}
		if (!clear) {
			code = 3;
		} else {
			int length = 0;
			while (findAt(x0 + length, y0) >= 0)
				++length;
			// the rows go; the raised bridge stands in the row above: 6 at the left end, 7 between and 8 at the right end
			std::vector<int> gone;
			for (int row = y0; findAt(x0, row) >= 0; ++row)
				for (int col = x0; findAt(col, row) >= 0; ++col)
					gone.push_back(findAt(col, row));
			for (int g : gone)
				_world.deleteObject(g);
			_world.objects[make(x0 - 1, y0 - 1, 6)].quality = (uint8_t)quality;
			for (int col = x0; col < x0 + length - 1; ++col)
				make(col, y0 - 1, 7);
			make(x0 + length - 1, y0 - 1, 8);
			code = 1;
		}
	} else if (frame == 6) {
		int count = 1;
		for (int col = x0; findAt(col, y0) >= 0; ++col)
			++count;
		bool clear = true;
		for (int row = y0 + 1; !edge(x0 + 1, row) && clear && row < y0 + 64; ++row)
			for (int e = 0; e < count - 4; ++e)
				if (blocked(x0 + 1 + e, row)) {
					clear = false;
					break;
				}
		if (!clear) {
			code = 4;
		} else {
			std::vector<int> gone;
			for (int col = x0; findAt(col, y0) >= 0; ++col)
				gone.push_back(findAt(col, y0));
			for (int g : gone)
				_world.deleteObject(g);
			for (int row = y0 + 1; !edge(x0 + 1, row) && row < y0 + 64; ++row) {
				_world.objects[make(x0 + 1, row, 3)].quality = (uint8_t)quality;
				for (int e = 0; e < count - 4; ++e)
					make(x0 + 2 + e, row, 4);
				make(x0 + 2 + (count - 4), row, 5);
			}
			code = 2;
		}
	}
	static const char *const TEXT[5] = {"\nStrange, nothing happened.\n", "\nClose the drawbridge.\n", "\nOpen the drawbridge.\n", "\nCan't close the drawbridge.\n",
										"\nCan't open the drawbridge.\n"};
	message(TEXT[code]);
}

// sub_39866 / sub_397A4: the 40 x 40 squares around the party as cells of 4 x 4 pixels at 8,8, drawn on every other pixel: dry walls 7, water 9, harmful ground 12, other ground 10, the
// party white on the other pixels; underground only what has been seen is shown (here: nothing is known, all of it is). APPROX(61): the window is the 40 x 40 around the party (the original
// uses the terrain cache), objects that are not harmful or blocking are left out as there; the colour numbers are those of the game's palette
void Game::drawGem(Framebuffer &screen) const {
	screen.fillRect(8, 8, 160, 160, 0);
	int mask = z == 0 ? MapData::SURFACE_SIZE - 1 : MapData::DUNGEON_SIZE - 1;
	auto classify = [&](int tile) {
		uint8_t f = _world.flags.flags1[tile & (TileFlags::TILES - 1)];
		if (f & 8)
			return 12;
		if (f & 2)
			return (f & 1) ? 9 : 7;
		return -1;
	};
	auto cell = [&](int cx, int cy, int colour, int phase) {
		for (int v = 0; v < 4; ++v)
			for (int u = 0; u < 4; ++u)
				if (((v + u) & 1) == phase)
					screen.pixels()[(8 + cy * 4 + v) * Framebuffer::WIDTH + 8 + cx * 4 + u] = (uint8_t)colour;
	};
	for (int cy = 0; cy < 40; ++cy)
		for (int cx = 0; cx < 40; ++cx) {
			int wx = (x - 20 + cx) & mask, wy = (y - 20 + cy) & mask;
			int c = classify(_world.map.tile(z, wx, wy));
			cell(cx, cy, c < 0 ? 10 : c, 1);
		}
	for (size_t i = 0; i < _world.objects.size(); ++i) {
		const World::Obj &o = _world.objects[i];
		if (o.deleted || !o.onMap() || o.z != z)
			continue;
		int cx = ((int)o.x - (x - 20)) & mask, cy = ((int)o.y - (y - 20)) & mask;
		if (cx >= 40 || cy >= 40)
			continue;
		if (o.actor >= 0 && (_world.actors.status[o.actor] & 0x80)) {
			cell(cx, cy, 15, 0);
			continue;
		}
		int c = classify(_world.baseTile.firstTile[o.type] + o.frame);
		if (c >= 0)
			cell(cx, cy, c, 1);
	}
}

// sub_2B0B8 / sub_2B1AC: "Begin combat!" gives every member (but the one the player controls) the combat worktype kept in the objlist (+0x12F1: 3 front, 4 rear, 5 flank ...);
// "Break off combat!" makes the living members followers (worktype 1) again and the dead ones motionless (0).
// Nothing else starts or ends combat: the flag (byte_40512) is written only here, so the companions never fight unless the player says so (the engine has no solo mode:
// the original answers "Not in solo mode!" to this key there).
// APPROX(65): the combat worktypes act as wt_combat_assault does (the originals, wt_combat_front 2E454, wt_combat_rear 2E4B9 and wt_combat_flank 2E7E2, are not traced)
void Game::toggleCombat() {
	ObjList &a = _world.actors;
	if (!_combatMode && _aboard) {
		message("Not while aboard ship!\n");
		return;
	}
	_combatMode = !_combatMode;
	message(_combatMode ? "Begin combat!\n" : "Break off combat!\n");
	chooseMusic(); // sub_2B0B8 plays ENGAGE, sub_2B1AC chooses again
	for (int m = 1; m < a.partySize; ++m) {
		int actor = a.partyRoster[m];
		if (a.status[actor] & 0x10)
			a.worktype[actor] = 0;
		else
			a.worktype[actor] = _combatMode ? (a.unknown12f1[actor] ? a.unknown12f1[actor] : 3) : 1;
	}
}

// sub_2B04E: a charm ends: the side the actor had before (kept in its movement flags' bits 0x60) comes back and the charm bit (object status 4) goes.
void Game::endCharm(int actor) {
	ObjList &a = _world.actors;
	int k = _world.actorObject(actor);
	if (k < 0)
		return;
	a.status[actor] = (uint8_t)((a.status[actor] & 0x9F) | (a.movementFlags[actor] & 0x60));
	_world.objects[k].status &= ~0x04;
}

// use_silver_horn (3525F): three times a spot is looked for at the edge of the view (a random side, a random place along it) where a silver serpent (0x19D) and the
// squares around it can be entered; an egg with a serpent there is hatched at once (status 0x60: the serpent fights both sides). Not while serpents are about.
// APPROX(62): the egg is skipped and the serpent made directly; its placement search and the egg's counts (4-8 inside, one hatched) follow the disassembly only roughly
void Game::useSilverHorn() {
	for (int actor = Creatures::FIRST_TEMP_ACTOR; actor < ObjList::ACTORS; ++actor) {
		int k = _world.actorObject(actor);
		if (k >= 0 && !_world.objects[k].deleted && _world.objects[k].type == 0x19D) {
			message("\nNot now!\n");
			return;
		}
	}
	int made = 0;
	MoveRules rules;
	rules.partyMayShuffle = false;
	for (int i = 0; i < 3; ++i) {
		for (int tries = 0; tries < 8; ++tries) {
			int side = _rng.next() % 4, r = _rng.next() % 11;
			int sx = side == 1 ? x + r - 6 : side == 2 ? x + 7 : side == 3 ? x + r - 6 : x - 7;
			int sy = side == 1 ? y - 7 : side == 2 ? y + r - 6 : side == 3 ? y + 6 : y + r - 6;
			static const int OFF[6][2] = {{0, 0}, {-1, 0}, {0, 1}, {-1, 1}, {1, 0}, {0, -1}};
			bool open = true;
			for (const auto &o : OFF)
				open &= _movement.check(sx + o[0], sy + o[1], z, _leaderObject, rules).passable;
			if (!open)
				continue;
			int actor = _creatures.spawn(0x19D, 0x08, sx, sy, z);
			if (actor >= 0) {
				_world.actors.status[actor] = (uint8_t)((_world.actors.status[actor] & 0x9F) | 0x60);
				++made;
			}
			break;
		}
	}
	message(made ? "\nSilver snakes are generated!\n" : "\nNone generated!\n");
}

// A number of degrees followed by a letter (sub_2F895): `plus` makes it positive, `minus` negative.
bool Game::degrees(const std::string &s, char plus, char minus, int &out) {
	size_t i = 0;
	int n = 0;
	while (i < s.size() && std::isdigit((unsigned char)s[i]))
		n = n * 10 + (s[i++] - '0');
	if (i == 0 || i >= s.size())
		return false;
	char c = (char)std::toupper((unsigned char)s[i]);
	if (c == plus)
		out = n;
	else if (c == minus)
		out = -n;
	else
		return false;
	return true;
}

// use_telescope (35127): the view goes out from the telescope in the chosen direction, up to 40 squares or until a terrain tile of 0xFC or more, and stays there until a key is pressed.
// APPROX(61): the original scrolls the view square by square; here the last view is shown at once
void Game::telescopeAt(int key) {
	int item = _useItem;
	_useItem = -1;
	static const char *const NAMES[8] = {"North", "Northeast", "East", "Southeast", "South", "Southwest", "West", "Northwest"};
	static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1}, DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
	int dir = key == KEY_UP ? 0 : key == KEY_RIGHT ? 2 : key == KEY_DOWN ? 4 : key == KEY_LEFT ? 6 : -1;
	if (dir < 0) {
		message("nowhere.\n");
		prompt();
		return;
	}
	message(std::string(NAMES[dir]) + ".\n");
	int mask = z == 0 ? MapData::SURFACE_SIZE - 1 : MapData::DUNGEON_SIZE - 1;
	int vx = _world.objects[item].x, vy = _world.objects[item].y;
	for (int steps = 0x28; steps >= 0; --steps) {
		vx = (vx + DX[dir]) & mask;
		vy = (vy + DY[dir]) & mask;
		if ((_world.map.tile(z, vx, vy) & 0xFF) >= 0xFC)
			break;
	}
	_peekX = vx;
	_peekY = vy;
	_peek = true;
	_world.spendMoves(actingActor(), 5);
}

// use_shovel (34656): "Direction:", then the square next to the user is dug at. On the surface at 707,835 a hole (type 0x134, quality 0x13) appears if there is none yet;
// in the caves (levels 1-4, terrain tiles 108-111) one try in 32 finds a water fountain (0xEA) and one in ten a gold nugget (0x59); anything else fails.
// APPROX(58): whether a hole already exists is looked for at that square only (`sub_334B1`'s arguments are not traced)
void Game::digAt(int key) {
	int item = _useItem;
	_useItem = -1;
	(void)item;
	static const char *const NAMES[8] = {"North", "Northeast", "East", "Southeast", "South", "Southwest", "West", "Northwest"};
	static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1}, DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
	int dir = key == KEY_UP ? 0 : key == KEY_RIGHT ? 2 : key == KEY_DOWN ? 4 : key == KEY_LEFT ? 6 : -1;
	if (dir < 0) {
		message("nowhere.\n");
		prompt();
		return;
	}
	message(std::string(NAMES[dir]) + ".\n");
	int mask = z == 0 ? MapData::SURFACE_SIZE - 1 : MapData::DUNGEON_SIZE - 1;
	int tx = (x + DX[dir]) & mask, ty = (y + DY[dir]) & mask;
	int tile = _world.map.tile(z, tx, ty) & 0xFF;
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	bool hole = false;
	for (int i : here)
		hole |= _world.objects[i].type == 0x134;
	auto place = [&](int type, int frame, int quality, int quantity) {
		World::Obj o;
		o.status = 0;
		o.x = (uint16_t)tx;
		o.y = (uint16_t)ty;
		o.z = (uint8_t)z;
		o.type = (uint16_t)type;
		o.frame = (uint8_t)frame;
		o.quantity = (uint8_t)quantity;
		o.quality = (uint8_t)quality;
		o.actor = -1;
		o.parent = -1;
		_world.objects.push_back(o);
		_world.addToMap((int)_world.objects.size() - 1);
	};
	if (z == 0 && tx == 707 && ty == 835 && !hole) {
		place(0x134, 0, 0x13, 1);
		message("\nYou dig a hole.\n");
	} else if (z >= 1 && z <= 4 && tile >= 108 && tile <= 111) {
		if (_rng.next() % 32 == 0) {
			place(0xEA, 0, 0, 1);
			message("\nYou find a water fountain.\n");
		} else if (_rng.next() % 10 == 0) {
			place(0x59, 0, 0, 1);
			message("\nYou find a gold nugget.\n");
		} else {
			message("\nFailed\n");
		}
	} else {
		message("\nNo effect\n");
	}
	_world.spendMoves(actingActor(), 5);
	prompt();
}

// The Move command (sub_3151B), first half: the top thing on the chosen square (a person counts); within one square of the party, else "Out of range!"; "To " asks where to.
// APPROX(54): the cursor feedback of the original (sub_146A9, sub_263D4) is left out, and the inventory half (sub_3151B from loc_318D9) hands things only to party members: putting
// things into and out of the containers of a pack (the pack view does not show them) is not done
void Game::moveSelect(int tx, int ty) {
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	int pick = -1;
	for (size_t i = here.size(); i-- > 0;) {
		if (!_world.objects[here[i]].invisible()) {
			pick = here[i];
			break;
		}
	}
	if (pick < 0) {
		message("nothing\n");
		prompt();
		return;
	}
	const World::Obj &o = _world.objects[pick];
	message(Describe::object(_world, o));
	message("\n");
	int a = _world.actorObject(actingActor());
	if (a >= 0 && (std::abs((int)_world.objects[a].x - (int)o.x) > 1 || std::abs((int)_world.objects[a].y - (int)o.y) > 1)) {
		message("\nOut of range!\n");
		prompt();
		return;
	}
	_useItem = pick;
	_pending = 'm';
	message("To ");
}

// sub_3143B: whether the square a step from (x, y) in direction dir is closed to the object: it must be somewhere the object can lie (can_actor_enter) or a surface (sub_309C0);
// for a diagonal step one of the two squares beside the way must be as well.
bool Game::moveBlocked(int idx, int x, int y, int dir) {
	static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1}, DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1}; // ds:01A2 / 01AA
	int mask = z == 0 ? MapData::SURFACE_SIZE - 1 : MapData::DUNGEON_SIZE - 1;
	MoveRules rules;
	auto open = [&](int sx, int sy) {
		sx &= mask;
		sy &= mask;
		return _movement.check(sx, sy, z, idx, rules).passable || dropSurface(sx, sy);
	};
	if (!open(x + DX[dir], y + DY[dir]))
		return true;
	if (!(dir & 1))
		return false;
	return !(open(x + DX[(dir + 7) & 7] , y + DY[(dir + 7) & 7]) || open(x + DX[(dir + 1) & 7], y + DY[(dir + 1) & 7]));
}

// The second half: a direction (the arrow keys and the number pad), then the thing moves one square: a person has to be overpowered ("Failed"), a thing goes into a
// container on the square that takes it, or onto the square if that is open ("Blocked" if not); a cannon turns to face the way it is pushed before it moves.
void Game::moveTo(int key) {
	int idx = _useItem;
	_useItem = -1;
	static const char *const NAMES[8] = {"North", "Northeast", "East", "Southeast", "South", "Southwest", "West", "Northwest"};
	int dir = key == KEY_UP || key == '8' ? 0 : key == '9' ? 1 : key == KEY_RIGHT || key == '6' ? 2 : key == '3' ? 3 : key == KEY_DOWN || key == '2' ? 4 : key == '1' ? 5 : key == KEY_LEFT || key == '4' ? 6 : key == '7' ? 7 : -1;
	if (dir < 0) {
		message("nowhere.\n");
		prompt();
		return;
	}
	message(std::string(NAMES[dir]) + ".\n");
	World::Obj &o = _world.objects[idx];
	int w = _world.weightOf(idx) + _world.contentsWeight(idx);
	bool reduced = o.type == 0x58 || (o.type >= 0x41 && o.type <= 0x48);
	if ((w == 0 && !reduced) || o.type == 0x19B) {
		message("\nNot possible\n");
		prompt();
		return;
	}
	int member = actingActor();
	if (o.actor >= 0) { // sub_212AC: the person's strength against the pusher's
		int chance = (_combat.adjustedStrength(o.actor) / 2 + 30 - _combat.adjustedStrength(member)) / 2;
		bool fails = chance > 1 + (int)(_rng.next() % 30) || o.actor == member || ((o.frame & 3) == 3 && o.type != 0x153);
		if (fails) {
			message("\nFailed\n");
			prompt();
			return;
		}
	}
	_world.spendMoves(member, 5);
	static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1}, DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
	int mask = z == 0 ? MapData::SURFACE_SIZE - 1 : MapData::DUNGEON_SIZE - 1;
	int ox = o.x, oy = o.y;
	int nx = (ox + DX[dir]) & mask, ny = (oy + DY[dir]) & mask;
	std::vector<int> there;
	_world.objectsAt(nx, ny, z, there);
	int occupant = there.empty() ? -1 : there.back();
	if (occupant >= 0 && dropRecipient(occupant, idx) && w < 0xFF) { // a container takes it
		dropPlace(idx, 1, nx, ny, occupant, false);
		prompt();
		return;
	}
	bool turns = o.type == 0xDD && o.frame != (dir >> 1); // a cannon
	if (moveBlocked(idx, ox, oy, dir)) {
		message("\nBlocked\n");
		if (turns)
			o.frame = (uint8_t)((dir >> 1) & 3);
	} else if (turns) {
		o.frame = (uint8_t)((dir >> 1) & 3);
	} else {
		_world.moveObject(idx, nx, ny, z);
	}
	prompt();
}

// The inventory half of Move: the thing chosen in the pack, then "To " and a party member (sub_3151B from loc_318D9). A worn amulet stays, a lit torch burns out (sub_1E8B6);
// the owner must be near (sub_1EE03: within four squares of the party); a stack asks "How many? "; the amulet cannot be given (the text comes after "To ").
void Game::moveItemStart(int idx) {
	World::Obj &o = _world.objects[idx];
	message("Move-" + Describe::object(_world, o) + "\n");
	if (o.location() == ObjRecord::READIED && o.type == 0x4C) {
		message("\nMagical energy prevents you from removing the amulet.\n");
		prompt();
		return;
	}
	if (o.location() == ObjRecord::READIED && o.type == 0x5A && o.frame == 1) {
		_world.deleteObject(idx);
		message("\nA torch burned out.\n");
		prompt();
		return;
	}
	if (!memberNear(actingActor())) {
		message("\nThis person is not around!\n");
		prompt();
		return;
	}
	_dropItem = idx;
	int have = _world.isStackable(o.type) ? _world.quantityOf(o) : 1;
	if (have > 1 && o.location() != ObjRecord::READIED) { // ask_quantity
		message("How many? ");
		_textActive = true;
		_textBuf.clear();
		_textMax = 3;
		_textPurpose = 8;
		return;
	}
	moveWhere(1);
}

void Game::moveWhere(int count) {
	_dropCount = count;
	message("To ");
	if (_world.objects[_dropItem].type == 0x4C) {
		_dropItem = -1;
		message("\n\nMagical energy prevents you from removing the amulet.\n");
		prompt();
		return;
	}
	_moveTarget = true;
}

// sub_1EE03: a person is around when within four squares of the party (and on its level)
bool Game::memberNear(int actor) const {
	int k = _world.actorObject(actor);
	if (k < 0 || _world.objects[k].deleted)
		return false;
	const World::Obj &o = _world.objects[k];
	return o.z == z && o.x > x - 5 && o.x < x + 5 && o.y > y - 5 && o.y < y + 5;
}

// The party member chosen (by number) to receive the thing: "Out of range!" if not near; the owner itself is "yourself."; otherwise the name and ".", the receiver must be in the party
// ("Only within the party!") and strong enough ("Can't carry!"); the thing leaves the owner (unreadied) and joins the receiver's pack; 5 moves.
void Game::moveGive(int target) {
	int idx = _dropItem, count = _dropCount;
	_dropItem = -1;
	_moveTarget = false;
	if (target < 0 || target >= _world.actors.partySize) {
		message("nobody.\n");
		prompt();
		return;
	}
	int receiver = _world.actors.partyRoster[target], member = actingActor();
	if (!memberNear(receiver)) {
		message("\n\nOut of range!\n");
		prompt();
		return;
	}
	if (receiver == member) {
		message("yourself.\n");
		_world.spendMoves(member, 5);
		prompt();
		return;
	}
	message(std::string((const char *)_world.actors.partyNames[target]) + ".\n");
	if (!(_world.actors.status[receiver] & 0x80)) {
		message("\nOnly within the party!\n");
		prompt();
		return;
	}
	World::Obj &o = _world.objects[idx];
	bool stack = _world.isStackable(o.type);
	int weight = stack ? _world.weightOfType(o.type, count) : _world.weightOf(idx) + _world.contentsWeight(idx);
	if (_world.actors.strength[receiver] * 20 < weight + _world.carriedWeight(receiver)) {
		message("\nCan't carry!\n");
		prompt();
		return;
	}
	if (o.location() == ObjRecord::READIED)
		o.status = (uint8_t)((o.status & ~ObjRecord::LOCATION_MASK) | ObjRecord::IN_INVENTORY);
	if (stack) {
		int have = _world.quantityOf(o), type = o.type, quality = o.quality;
		_world.addToInventory(receiver, type, quality, count);
		if (count >= have)
			_world.deleteObject(idx);
		else
			_world.setQuantity(_world.objects[idx], have - count);
	} else {
		o.parent = receiver;
	}
	_world.spendMoves(member, 5);
	prompt();
}

// sub_32BC7, using a staff: it must be readied; its first charge (0x150, the quality is the spell) names the spell, which is cast as if by the Avatar but at no cost
// ("With Kal Wis Ylem", then as cast_spell_effect), and the charge is used up
void Game::useStaff(int idx) {
	if ((_world.objects[idx].status & 0x18) != 0x18) {
		message("\nNot readied.\n");
		return;
	}
	int charge = -1;
	for (size_t i = 0; i < _world.objects.size() && charge < 0; ++i) {
		const World::Obj &c = _world.objects[i];
		if (!c.deleted && c.location() == ObjRecord::IN_CONTAINER && c.parent == idx && c.type == 0x150)
			charge = (int)i;
	}
	if (charge < 0) {
		message("\nNo effect\n");
		return;
	}
	const Magic::SpellInfo *spell = Magic::byIndex(_world.objects[charge].quality);
	_world.deleteObject(charge);
	if (!spell) {
		message("\nNo effect\n");
		return;
	}
	message("With " + Magic::words(*spell) + "\n");
	castSpell(spell, actingActor(), false);
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
