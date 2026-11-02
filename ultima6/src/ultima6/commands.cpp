#include "ultima6/session.h"

#include "ultima6/describe.h"
#include "ultima6/stats_view.h"

#include <algorithm>
#include <cstdlib>

namespace Ultima6 {
namespace Ultima6 {

// The party follows its leader (sub_275BD): each member steps into the square the member ahead of it has just left.
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
	advanceTime(1);
	if (_world.actors.hp[1] == 0) {
		message("\nThou art dead!\n");
	}
	prompt();
}

// Keys while the inventory is on show: select, ready or unready, drop, leave.
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
			if (o.location() == ObjRecord::READIED) {
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

// Get: take the top object of a square that can be carried.
void Game::getAt(int tx, int ty) {
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	int pick = -1;
	for (size_t i = here.size(); i-- > 0;) {
		const World::Obj &o = _world.objects[here[i]];
		if (o.actor >= 0 || o.invisible())
			continue;
		if (_world.weightOf(here[i]) > 0) {
			pick = here[i];
			break;
		}
	}
	if (pick < 0) {
		message("\nNot possible\n");
		prompt();
		return;
	}
	World::Obj &o = _world.objects[pick];
	int actor = _world.actors.partyRoster[_member];
	message(Describe::object(_world, o) + "\n");
	int w = _world.weightOf(pick);
	int room = _world.actors.strength[actor] * 2 * 10 - _world.carriedWeight(actor);
	if (w > room) {
		message("\nThe weight is too great!\n");
		prompt();
		return;
	}
	int qty = _world.quantityOf(o);
	int type = o.type, quality = o.quality;
	_world.deleteObject(pick);
	_world.addToInventory(actor, type, quality, qty);
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
void Game::useAt(int tx, int ty) {
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
