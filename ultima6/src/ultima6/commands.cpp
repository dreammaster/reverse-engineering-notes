#include "ultima6/session.h"

#include "ultima6/describe.h"
#include "ultima6/stats_view.h"

#include <algorithm>
#include <cstdlib>

namespace Ultima6 {
namespace Ultima6 {

// The party follows its leader (sub_275BD): each member steps into the square the member ahead of it has just left.
// APPROX(4): not the original's exact behaviour, see docs/approximations.md
void Game::followLeader(int oldX, int oldY) {
	const ObjList &a = _world.actors;
	int prevX = oldX, prevY = oldY;
	MoveRules rules;
	rules.partyMayShuffle = true;
	for (int i = 1; i < a.partySize; ++i) {
		int k = _world.actorObject(a.partyRoster[i]);
		if (k < 0)
			continue;
		World::Obj &o = _world.objects[k];
		int ox = o.x, oy = o.y;
		int mask = z == 0 ? MapData::SURFACE_SIZE - 1 : MapData::DUNGEON_SIZE - 1;
		int dx = (prevX - ox) & mask, dy = (prevY - oy) & mask;
		if (dx > mask / 2)
			dx -= mask + 1;
		if (dy > mask / 2)
			dy -= mask + 1;
		if (o.z == z && (dx || dy)) {
			if (std::abs(dx) <= 1 && std::abs(dy) <= 1) {
				// the square the member ahead left is free to step into
				if (_movement.check(prevX, prevY, z, k, rules).passable) {
					_world.moveObject(k, prevX, prevY, z);
					_actors.face(a.partyRoster[i], dx > 0 ? 1 : dx < 0 ? 3 : dy > 0 ? 2 : 0);
				}
			} else if (std::abs(dx) <= 8 && std::abs(dy) <= 8) {
				_actors.walkToward(a.partyRoster[i], k, prevX, prevY, z);
			} else {
				_world.moveObject(k, prevX, prevY, z); // too far behind: caught up at once
			}
		}
		prevX = ox;
		prevY = oy;
	}
}

// Attack: hit the creature or person on the square.
// APPROX(18): not the original's exact behaviour, see docs/approximations.md
void Game::attackAt(int tx, int ty) {
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	int target = -1;
	for (int i : here)
		if (_world.objects[i].actor >= 0 && _world.objects[i].actor != 1)
			target = _world.objects[i].actor;
	if (target < 0) {
		message("\nnothing!\n");
		prompt();
		return;
	}
	int attacker = _world.actors.partyRoster[_member];
	bool died = _combat.attack(attacker, target);
	(void)died;
	_world.spendMoves(attacker, 10); // sub_2DA99: sub_26CC4(attacker, 0x0A)
	if (_world.actors.hp[1] == 0) {
		message("\nThou art dead!\n");
	}
	prompt();
}

// Keys while the inventory is on show: select, ready or unready, drop, leave.
// APPROX(28): not the original's exact behaviour, see docs/approximations.md
bool Game::inventoryKey(int key) {
	inventoryItems(_world, _member, _items);
	int count = (int)_items.size();
	switch (key) {
	case KEY_LEFT:
		_selected = std::max(0, _selected - 1);
		break;
	case KEY_RIGHT:
		_selected = std::min(std::max(0, count - 1), _selected + 1);
		break;
	case KEY_UP:
		_selected = std::max(0, _selected - 8);
		break;
	case KEY_DOWN:
		_selected = std::min(std::max(0, count - 1), _selected + 8);
		break;
	case KEY_ENTER:
		if (_selected < count) {
			World::Obj &o = _world.objects[_items[_selected]];
			int actor = _world.actors.partyRoster[_member];
			if (o.type == 90 && (o.frame & 1)) { // a lit torch: put it out
				int idx = _items[_selected];
				_world.deleteObject(idx);
				_world.addToInventory(actor, 90, 0, 1);
				message("\nA torch is doused.\n");
			} else if (o.type == 90) { // light one of the torches, which burns for a while in the hand
				World::Obj lit = o;
				_world.removeFromInventory(actor, 90, 0, 1);
				lit.frame = 1;
				lit.quantity = 100;
				lit.deleted = false;
				lit.status = (uint8_t)((lit.status & ~ObjRecord::LOCATION_MASK) | ObjRecord::READIED);
				lit.parent = actor;
				_world.objects.push_back(lit);
				message("\nA torch is lit.\n");
			} else if (o.location() == ObjRecord::READIED) {
				o.status = (uint8_t)((o.status & ~ObjRecord::LOCATION_MASK) | ObjRecord::IN_INVENTORY);
				message("\nUnreadied " + Describe::object(_world, o) + "\n");
			} else if (Combat::weaponDamageOf(o.type) >= 0 || Combat::armourOf(o.type) > 0) {
				o.status = (uint8_t)((o.status & ~ObjRecord::LOCATION_MASK) | ObjRecord::READIED);
				message("\nReadied " + Describe::object(_world, o) + "\n");
			} else {
				message("\nNot usable\n");
			}
		}
		break;
	case 'd':
	case 'D':
		if (_selected < count) {
			int idx = _items[_selected];
			World::Obj &o = _world.objects[idx];
			message("\nDrop-" + Describe::object(_world, o) + "\n");
			o.status = (uint8_t)(o.status & ~ObjRecord::LOCATION_MASK);
			o.parent = -1;
			o.x = (uint16_t)x;
			o.y = (uint16_t)y;
			o.z = (uint8_t)z;
			_world.addToMap(idx);
			_selected = std::max(0, _selected - 1);
		}
		break;
	case KEY_ESC:
	case 'i':
	case 'I':
	case KEY_SPACE:
		_inventory = false;
		prompt();
		break;
	default:
		break;
	}
	return true;
}

// What a minute does to the party (as the original's time advance does it): poison hurts now and then, swamps poison, fire, traps and
// force fields burn, and the lit torch that is carried burns down.
// APPROX(9): not the original's exact behaviour, see docs/approximations.md
void Game::partyHazards() {
	ObjList &a = _world.actors;
	for (int m = 0; m < a.partySize; ++m) {
		int actor = a.partyRoster[m];
		int k = _world.actorObject(actor);
		if (k < 0 || _world.objects[k].deleted || a.hp[actor] == 0)
			continue;
		const World::Obj &o = _world.objects[k];
		const CreatureStats *cs = Combat::stats(o.type);
		if ((a.status[actor] & 0x08) && !(a.status[actor] & 0x01) && _rng.next() % 8 == 0) // poisoned and not protected: 1 hp now and then (sub_13305 13716)
			_combat.applyDamage(actor, actor, 1);
		// the tiles on the square
		int mask = o.z == 0 ? 0x3FF : 0xFF;
		std::vector<int> tiles;
		tiles.push_back(_world.map.tile(o.z, o.x & mask, o.y & mask));
		std::vector<int> here;
		_world.objectsAt(o.x, o.y, o.z, here);
		for (int i : here)
			if (_world.objects[i].actor < 0)
				tiles.push_back(_world.baseTile.firstTile[_world.objects[i].type] + _world.objects[i].frame);
		for (int t : tiles) {
			if (((t >= 2 && t <= 5) || t == 1165) && !(cs && (cs->flags & (1 << 12))) && !(a.status[actor] & 0x08)) {
				a.status[actor] |= 0x08;
				message(_combat.nameOf(actor) + " poisoned!\n");
			} else if ((t >= 220 && t <= 223) || t == 890 || t == 1124 || t == 1125 || t == 1130 || t == 1131 || t == 1164 || t == 1193) {
				int dmg = t == 1164 ? 1 + _rng.next() % 30 : 1 + _rng.next() % 8;
				if (cs && (cs->flags & (1 << 10)))
					dmg *= 2;
				_combat.applyDamage(actor, actor, dmg);
			} else if (t == 732 || t == 1010) {
				_combat.applyDamage(actor, actor, 1 + _rng.next() % 30);
			} else if (t == 562 || t == 731 || (t >= 1458 && t <= 1463)) {
				_combat.applyDamage(actor, actor, 1 + _rng.next() % 8);
			}
		}
		// a lit torch in hand burns down
		std::vector<int> inv;
		_world.inventory(actor, inv, false);
		// sub_13305 (133E5-135AD), for the things held in the hands: a lit torch loses a minute of its burn time half of the time and is gone
		// when it has none ("A torch burned out."); a regeneration ring (0x101) gives a hit point a minute; the invisibility ring (0x102), the
		// regeneration ring and the storm cloak (0x51) vanish now and then ("A ring has vanished!")
		for (int i : inv) {
			World::Obj &t = _world.objects[i];
			if (t.location() != ObjRecord::READIED)
				continue;
			const char *vanished = nullptr;
			if (t.type == 0x5A && (t.frame & 1)) {
				if (_rng.next() % 2) {
					if (t.quantity > 1) {
						--t.quantity;
					} else {
						message("A torch burned out.\n");
						_world.deleteObject(i);
					}
				}
			} else if (t.type == 0x102) {
				if (_rng.next() % 1001 == 429)
					vanished = "ring";
			} else if (t.type == 0x101) {
				a.hp[actor] = (uint8_t)std::min<int>(ActorStats::maxHealth(_world, actor), a.hp[actor] + 1);
				if (_rng.next() % 1001 == 734)
					vanished = "ring";
			} else if (t.type == 0x51) {
				if (_rng.next() % 1001 == 588)
					vanished = "cloak";
			}
			if (vanished) {
				_world.deleteObject(i);
				message(std::string("A ") + vanished + " has vanished!\n");
			}
		}
	}
}

// Skiffs (414) and rafts (415): using one boards it, with the whole party aboard; using the shore from aboard puts the party ashore.
// APPROX(30): not the original's exact behaviour, see docs/approximations.md
bool Game::boardOrLeave(int tx, int ty) {
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	World::Obj &me = _world.objects[_leaderObject];
	if (_aboard) {
		int vehicleType = me.type, vehicleFrame = me.frame;
		me.type = (uint16_t)_footType;
		me.frame = (uint8_t)_footFrame;
		MoveRules rules;
		bool ok = _movement.check(tx, ty, z, _leaderObject, rules).passable;
		if (!ok) {
			me.type = (uint16_t)vehicleType;
			me.frame = (uint8_t)vehicleFrame;
			return false;
		}
		// the vehicle stays where it was; the party steps ashore
		World::Obj boat;
		boat.status = 0;
		boat.x = me.x;
		boat.y = me.y;
		boat.z = me.z;
		boat.type = (uint16_t)vehicleType;
		boat.frame = (uint8_t)vehicleFrame;
		boat.quantity = 1;
		boat.quality = 0;
		boat.actor = -1;
		boat.parent = -1;
		_world.objects.push_back(boat);
		_world.addToMap((int)_world.objects.size() - 1);
		_aboard = false;
		const ObjList &a = _world.actors;
		for (int i = 0; i < a.partySize; ++i) {
			int k = _world.actorObject(a.partyRoster[i]);
			if (k >= 0) {
				_world.objects[k].status &= ~0x02;
				_world.moveObject(k, tx, ty, z);
			}
		}
		x = tx;
		y = ty;
		message("\nDisembarked.\n");
		return true;
	}
	for (int i : here) {
		World::Obj &v = _world.objects[i];
		if (v.actor >= 0 || (v.type != 414 && v.type != 415))
			continue;
		_footType = me.type;
		_footFrame = me.frame;
		me.type = v.type;
		me.frame = v.frame;
		int vx = v.x, vy = v.y;
		_world.deleteObject(i);
		_aboard = true;
		const ObjList &a = _world.actors;
		for (int m = 1; m < a.partySize; ++m) {
			int k = _world.actorObject(a.partyRoster[m]);
			if (k >= 0)
				_world.objects[k].status |= 0x02; // aboard, out of sight
		}
		_world.moveObject(_leaderObject, vx, vy, z);
		x = vx;
		y = vy;
		message("\nBoarded.\n");
		return true;
	}
	return false;
}

// Cast: the circle (a digit) and the syllables' letters, then Enter; spells that need a target then ask for one with the cursor.
// APPROX(23): not the original's exact behaviour, see docs/approximations.md
void Game::castKey(int key) {
	if (key == KEY_ESC) {
		_pending = 0;
		message("\n");
		prompt();
		return;
	}
	if (key == KEY_BACKSPACE) {
		if (!_invocation.empty()) {
			_invocation.erase(_invocation.size() - 1);
			message("\b");
		}
		return;
	}
	if (key != KEY_ENTER) {
		if (key >= 'a' && key <= 'z' + 0 && !_invocation.empty() && _invocation.size() < 8) {
			_invocation += (char)key;
			message(std::string(1, (char)key));
		} else if (key >= '1' && key <= '8' && _invocation.empty()) {
			_invocation += (char)key;
			message(std::string(1, (char)key));
		}
		return;
	}
	_pending = 0;
	message("\n");
	int caster = _world.actors.partyRoster[_member];
	const Magic::SpellInfo *spell = _invocation.size() > 1 ? Magic::find(_invocation[0] - '0', _invocation.substr(1)) : nullptr;
	if (!spell) {
		message("That spell is not in thy spellbook!\n");
		prompt();
		return;
	}
	// cmd_cast: the spell and its words are printed first, then the level, magic points and reagents are checked, then they are paid
	// (before the target is chosen), then the effect asks for its target
	message(_magic.announce(*spell));
	if (!_magic.checkCost(caster, *spell)) {
		message("\n");
		prompt();
		return;
	}
	_magic.pay(caster, *spell);
	_spell = spell;
	if (spell->select == Magic::SEL_NONE) {
		_magic.cast(caster, *spell, caster);
		_world.spendMoves(caster, spell->circle * 3 + 10); // cmd_cast: sub_26CC4(caster, circle * 3 + 10)
		prompt();
		return;
	}
	bool onObject = spell->select == Magic::SEL_OBJ || spell->select == Magic::SEL_OBJ_PROJ;
	bool onPlace = spell->select == Magic::SEL_LOCATION || spell->select == Magic::SEL_LOCATION_PROJ;
	message(std::string("\n") + (onObject ? "On what: " : onPlace ? "Location: " : "On whom: "));
	_pending = 'c';
	_cursor = true;
	_cx = _cy = 5;
}

void Game::castAt(int tx, int ty) {
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	int target = -1;
	for (int i : here)
		if (_world.objects[i].actor >= 0)
			target = _world.objects[i].actor;
	int caster = _world.actors.partyRoster[_member];
	message("\n");
	_magic.cast(caster, *_spell, target);
	_world.spendMoves(caster, _spell->circle * 3 + 10);
	_spell = nullptr;
	prompt();
}

// Get: take the top object of a square that can be carried.
// APPROX(26): not the original's exact behaviour, see docs/approximations.md
void Game::getAt(int tx, int ty) {
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	// cmd_get: the top object of the square (anything but an actor); "nothing" if there is none
	int pick = -1;
	for (size_t i = here.size(); i-- > 0;) {
		const World::Obj &o = _world.objects[here[i]];
		if (o.actor >= 0 || o.invisible())
			continue;
		pick = here[i];
		break;
	}
	if (pick < 0) {
		message("nothing\n");
		prompt();
		return;
	}
	World::Obj &o = _world.objects[pick];
	int actor = _world.actors.partyRoster[_member];
	message(Describe::object(_world, o));
	int tile = _world.baseTile.firstTile[o.type] + o.frame;
	int w = _world.weightOf(pick) + _world.contentsWeight(pick);
	bool reduced = o.type == 0x58 || (o.type >= 0x41 && o.type <= 0x48);
	if (_world.flags.flags1[tile & (TileFlags::TILES - 1)] & 0x08) {
		message("\nNot possible");
	} else if ((w == 0 && !reduced) || w >= 10000 || o.type == 0x19B) {
		message("\nNot possible\n");
	} else if (_world.actors.strength[actor] * 20 < w + _world.carriedWeight(actor)) {
		message("\nThe total is too heavy.\n");
	} else {
		int qty = _world.quantityOf(o);
		int type = o.type, quality = o.quality, frame = o.frame;
		bool litTorch = type == 0x5A && frame == 1;
		World::Obj taken = o;
		_world.spendMoves(actor, 3); // cmd_get: sub_26CC4(actor, 3)
		_world.deleteObject(pick);
		if (litTorch) {
			// a lit torch goes straight into a hand
			taken.deleted = false;
			taken.status = (uint8_t)((taken.status & ~ObjRecord::LOCATION_MASK) | ObjRecord::READIED);
			taken.parent = actor;
			_world.objects.push_back(taken);
		} else {
			_world.addToInventory(actor, type, quality, qty);
			// what a thing turns into once it is picked up (cmd_get 3128E-3135C)
			if (type == 0x7A || type == 0x91 || type == 0x1A7) {
				std::vector<int> inv;
				_world.inventory(actor, inv, false);
				for (int i : inv)
					if (_world.objects[i].type == type) {
						_world.objects[i].frame = 0;
						if (type == 0x1A7)
							_world.objects[i].type = 0x1A4;
					}
			}
		}
	}
	prompt();
}

// Takes the whole party to a position on another level (ladders and stairs).
void Game::changeLevel(int nx, int ny, int nz) {
	const ObjList &a = _world.actors;
	for (int i = 0; i < a.partySize; ++i) {
		int k = _world.actorObject(a.partyRoster[i]);
		if (k >= 0)
			_world.moveObject(k, nx, ny, nz);
	}
	x = nx;
	y = ny;
	z = nz;
}

// Use: what the objects of a square do when used (the doors and ladders of the Use table; the rest answers "Not usable").
// APPROX(30): not the original's exact behaviour, see docs/approximations.md
void Game::useAt(int tx, int ty) {
	if (boardOrLeave(tx, ty)) {
		prompt();
		return;
	}
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	for (size_t n = here.size(); n-- > 0;) {
		int idx = here[n];
		World::Obj &o = _world.objects[idx];
		int type = o.type;
		if (type >= 297 && type <= 300) { // doors
			if (o.frame == 13 || o.frame == 15) {
				message("\nmagically locked\n");
			} else if (o.frame == 9 || o.frame == 11) {
				int actor = _world.actors.partyRoster[_member];
				std::vector<int> inv;
				_world.inventory(actor, inv);
				bool key = false;
				for (int i : inv)
					key |= _world.objects[i].type == 64 && o.quality != 0 && _world.objects[i].quality == o.quality;
				if (key) {
					o.frame -= 4;
					message("\nunlocked\n");
				} else {
					message("\nlocked\n");
				}
			} else if (o.frame <= 3) { // open: close it unless something stands in the way
				MoveRules rules;
				std::vector<int> there;
				_world.objectsAt(tx, ty, z, there);
				bool blocked = false;
				for (int i : there)
					blocked |= _world.objects[i].actor >= 0;
				if (blocked) {
					message("\nNot now!\n");
				} else {
					o.frame += 4;
					message("\nclosed!\n");
				}
			} else {
				o.frame -= 4;
				message("\nopened!\n");
			}
			prompt();
			return;
		}
		if (type == 98 || type == 99 || type == 176 || type == 186 || type == 188 || type == 191 || type == 192) { // containers
			if (type == 98 && o.frame >= 3) {
				message("\nlocked\n");
				prompt();
				return;
			}
			std::vector<int> contents;
			for (size_t i = 0; i < _world.objects.size(); ++i)
				if (!_world.objects[i].deleted && _world.objects[i].location() == ObjRecord::IN_CONTAINER && _world.objects[i].parent == idx)
					contents.push_back((int)i);
			if (contents.empty()) {
				message("\nSearching here, you find nothing.\n");
			} else {
				// opening it, the party takes what is inside
				int actor = _world.actors.partyRoster[_member];
				std::string list;
				for (size_t n = 0; n < contents.size(); ++n) {
					World::Obj &c = _world.objects[contents[n]];
					list += (n == 0 ? "" : n + 1 == contents.size() ? " and " : ", ") + Describe::object(_world, c);
					int qty = _world.quantityOf(c), t = c.type, q = c.quality;
					bool stack = _world.isStackable(t);
					_world.deleteObject(contents[n]);
					_world.addToInventory(actor, t, q, stack ? qty : 1);
				}
				message("\nSearching here, you find " + list + ".\n");
			}
			if (type == 98 && o.frame > 0)
				o.frame = 0; // the chest stands open
			prompt();
			return;
		}
		if (type == 305 || type == 106) { // ladders
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
			prompt();
			return;
		}
	}
	message("\nNot usable\n");
	prompt();
}

// Rest: ask for the hours, then let them pass; every hour heals the party a little.
// APPROX(31): not the original's exact behaviour, see docs/approximations.md
void Game::restKey(int key) {
	if (key == KEY_ESC) {
		_pending = 0;
		_number.clear();
		message("0\n");
		prompt();
		return;
	}
	if (key >= '0' && key <= '9' && _number.size() < 2) {
		_number += (char)key;
		message(std::string(1, (char)key));
		return;
	}
	if (key == KEY_BACKSPACE && !_number.empty()) {
		_number.erase(_number.size() - 1);
		message("\b");
		return;
	}
	if (key != KEY_ENTER)
		return;
	int hours = std::atoi(_number.c_str());
	_number.clear();
	_pending = 0;
	message("\n");
	for (int h = 0; h < hours; ++h) {
		advanceTime(60);
		ObjList &a = _world.actors;
		for (int i = 0; i < a.partySize; ++i) {
			int actor = a.partyRoster[i];
			int most = ActorStats::maxHealth(_world, actor);
			a.hp[actor] = (uint8_t)std::min<int>(most, a.hp[actor] + 1 + a.level[actor] / 2);
		}
	}
	prompt();
}

} // namespace Ultima6
} // namespace Ultima6
