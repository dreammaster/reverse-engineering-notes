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
					_actors.face(a.partyRoster[i], ActorManager::directionOf(dx, dy));
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
	int attacker = _world.actors.partyRoster[_member];
	if (target < 0) {
		message("nothing!\n");
		prompt();
		return;
	}
	int weapon = _aboard && _world.objects[_leaderObject].type == 0x19C ? 0x19C : _combat.weaponType(attacker); // a ship fires its cannons (weapon_range of the vehicle: range 5, damage 30)
	int k = _world.actorObject(target);
	int dist = std::max(std::abs((int)_world.objects[k].x - x), std::abs((int)_world.objects[k].y - y));
	if (dist > Combat::rangeOf(weapon)) {
		message("Out of range!\n");
		prompt();
		return;
	}
	// ammunition: arrows (0x37) for the bows, bolts (0x38) for the crossbows
	int ammo = (weapon == 41 || weapon == 54) ? 0x37 : (weapon == 42 || weapon == 50) ? 0x38 : 0;
	if (ammo) {
		if (!_world.removeFromInventory(attacker, ammo, 0, 1)) {
			message(ammo == 0x37 ? "Out of arrows!\n" : "Out of bolts!\n");
			prompt();
			return;
		}
	}
	_combat.attack(attacker, target);
	_world.spendMoves(attacker, 10); // sub_2DA99: sub_26CC4(attacker, 0x0A)
	if (_world.actors.hp[1] == 0)
		message("\nThou art dead!\n");
	prompt();
}

// Keys while the inventory is on show: select, ready or unready, drop, leave.
// APPROX(28): not the original's exact behaviour, see docs/approximations.md
// use_vortex_cube: the Codex at the end of the game is complete when both lenses lie in the Codex chamber's pedestals; using the cube then ends the game.
// APPROX(49): the check for the three objects and the text come from END.EXE's documentation (docs/u6_ending.md); the handler itself is not traced
bool Game::useVortexCube() {
	struct Need {
		int type, x, y;
	};
	static const Need NEEDED[3] = {{0x18A, 921, 851}, {0x18C, 925, 851}, {0x3B, 923, 851}};
	for (const Need &n : NEEDED) {
		bool found = false;
		for (const World::Obj &o : _world.objects)
			found |= !o.deleted && o.onMap() && o.type == n.type && o.x == n.x && o.y == n.y;
		if (!found)
			return false;
	}
	message("\nThe Codex has vanished!\n");
	_ending = 1;
	return true;
}

void Game::timePlayed(int &years, int &months, int &days) const {
	days = clock.day - 4;
	months = clock.month - 7;
	years = clock.year - 161;
	if (days < 0) {
		days += 28;
		--months;
	}
	if (months < 0) {
		months += 13;
		--years;
	}
	if (years < 0)
		years = months = days = 0;
}

bool Game::inventoryKey(int key) {
	inventoryItems(_world, _stealing >= 0 ? 100 + _stealing : _member, _items);
	int count = (int)_items.size();
	if (_stealing >= 0 && (key == KEY_ESC || key == KEY_SPACE)) { // "Which object: " and nothing is chosen
		_stealing = -1;
		_inventory = false;
		message("nothing\n");
		prompt();
		return true;
	}
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
		if (_stealing >= 0) {
			stealItem(_selected < count ? _items[_selected] : -1);
			break;
		}
		if (_selected < count) {
			World::Obj &o = _world.objects[_items[_selected]];
			int actor = _world.actors.partyRoster[_member];
			if (o.type == 90 && (o.frame & 1)) { // a lit torch: put it out
				int idx = _items[_selected];
				_world.deleteObject(idx);
				_world.addToInventory(actor, 90, 0, 1);
				message("\nA torch is doused.\n");
			} else if (o.type == 90) { // light one of the torches, which burns for a while in the hand
				// use_torch (32661): both hands full means no light; the torch burns for 200 minutes (0xC8)
				std::vector<int> held;
				_world.inventory(actor, held, false);
				int hands = 0;
				for (int h : held)
					hands += _world.objects[h].location() == ObjRecord::READIED;
				if (hands >= 2) {
					message("\nNo free hand to hold the torch.\n");
					break;
				}
				World::Obj lit = o;
				_world.removeFromInventory(actor, 90, 0, 1);
				lit.frame = 1;
				lit.quantity = 200;
				lit.deleted = false;
				lit.status = (uint8_t)((lit.status & ~ObjRecord::LOCATION_MASK) | ObjRecord::READIED);
				lit.parent = actor;
				_world.objects.push_back(lit);
				message("\nTorch is lit.\n");
			} else if (o.location() == ObjRecord::READIED) {
				o.status = (uint8_t)((o.status & ~ObjRecord::LOCATION_MASK) | ObjRecord::IN_INVENTORY);
				message("\nUnreadied " + Describe::object(_world, o) + "\n");
			} else if (Combat::weaponDamageOf(o.type) >= 0 || Combat::armourOf(o.type) > 0) {
				o.status = (uint8_t)((o.status & ~ObjRecord::LOCATION_MASK) | ObjRecord::READIED);
				message("\nReadied " + Describe::object(_world, o) + "\n");
			} else {
				int idx = _items[_selected];
				useObject(idx);
				if (_pending) // a key, lock pick or potion asks for its target on the map
					_inventory = false;
				inventoryItems(_world, _member, _items);
				_selected = std::max(0, std::min(_selected, (int)_items.size() - 1));
			}
		}
		break;
	case 'd':
	case 'D':
		if (_selected < count) {
			int idx = _items[_selected];
			World::Obj &o = _world.objects[idx];
			message("\nDrop-" + Describe::object(_world, o) + "\n");
			if (_world.isStackable(o.type) && _world.quantityOf(o) > 1) {
				// ask_quantity: how many of the stack
				message("How many? ");
				_useItem = idx;
				_textActive = true;
				_textBuf.clear();
				_textMax = 3;
				_textPurpose = 5;
				break;
			}
			dropObject(idx, 1);
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

// Pickpocket (sub_24235) after "Which object: ": the thing chosen must be one the victim carries; an amulet that is worn cannot be taken ("Magical energy prevents you from removing the
// amulet.") and a torch that is lit burns out in the taking; the thief must be able to carry it ("Can't carry!"); a ring or cloak of invisibility stops working for its wearer;
// the thing goes into the thief's pack, "Success", and 5 karma are lost
void Game::stealItem(int idx) {
	int victim = _stealing, thief = actingActor();
	_stealing = -1;
	_inventory = false;
	if (idx < 0) {
		message("nothing\n");
		prompt();
		return;
	}
	World::Obj &o = _world.objects[idx];
	message(Describe::object(_world, o) + "\n");
	if (!(o.status & 0x18)) {
		message("\nNo effect\n");
		prompt();
		return;
	}
	if ((o.status & 0x18) == 0x18 && o.type == 0x4C) {
		message("\nMagical energy prevents you from removing the amulet.\n");
		prompt();
		return;
	}
	if ((o.status & 0x18) == 0x18 && o.type == 0x5A && o.frame == 1) {
		_world.deleteObject(idx);
		message("\nA torch burned out.\n");
		prompt();
		return;
	}
	if (_world.carriedWeight(thief) + _world.weightOf(idx) > _world.actors.strength[thief] * 20) {
		message("\nCan't carry!\n");
		prompt();
		return;
	}
	if ((o.status & 0x18) == 0x18 && o.type == 0x102)
		_world.objects[_world.actorObject(victim)].status &= ~0x02;
	int type = o.type, quality = o.quality, quantity = _world.quantityOf(o);
	if (_world.isStackable(type)) {
		_world.deleteObject(idx);
		_world.addToInventory(thief, type, quality, quantity);
	} else {
		o.status = (uint8_t)((o.status & ~ObjRecord::LOCATION_MASK) | ObjRecord::IN_INVENTORY);
		o.parent = thief;
	}
	message("\nSuccess\n");
	if (_world.actors.globals.size() > 8)
		_world.actors.globals[8] = (uint8_t)(_world.actors.globals[8] > 5 ? _world.actors.globals[8] - 5 : 0);
	prompt();
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
			if (t == 1166 && !(a.status[actor] & 0x04)) { // a sleep field
				a.status[actor] |= 0x04;
				message(_combat.nameOf(actor) + " falls asleep!\n");
			}
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

// A ship (0x19C) is three objects in a line: bow, middle and stern. The high nibble of the tile flags1 of each part says which neighbours it is linked to (0x80 north, 0x40 east, 0x20
// south, 0x10 west; the middle has two, an end one); the frames are mid - 8 (bow), mid (9, 11, 13 or 15 for north, east, south and west) and mid + 8 (stern). Returns the middle part
// of the ship a part belongs to and the bow and stern (-1 when not found).
bool Game::shipParts(int part, int &mid, int &bow, int &stern) {
	static const int DX[4] = {0, 1, 0, -1}, DY[4] = {-1, 0, 1, 0};
	mid = part;
	bow = stern = -1;
	World::Obj p = _world.objects[part];
	if (p.type != 0x19C)
		return false;
	auto links = [&](const World::Obj &o) {
		return _world.flags.flags1[(_world.baseTile.firstTile[o.type] + o.frame) & (TileFlags::TILES - 1)] & 0xF0;
	};
	auto partAt = [&](int px, int py) {
		std::vector<int> here;
		_world.objectsAt(px, py, z, here);
		for (int i : here)
			if (_world.objects[i].type == 0x19C && _world.objects[i].actor < 0)
				return i;
		return -1;
	};
	int mask = z == 0 ? 0x3FF : 0xFF;
	int f = links(p);
	if (f == 0x80 || f == 0x40 || f == 0x20 || f == 0x10) { // an end: the middle lies the way it is linked
		int d = f == 0x80 ? 0 : f == 0x40 ? 1 : f == 0x20 ? 2 : 3;
		int m = partAt((p.x + DX[d]) & mask, (p.y + DY[d]) & mask);
		if (m < 0)
			return false;
		mid = m;
	}
	const World::Obj &m = _world.objects[mid];
	for (int d = 0; d < 4; ++d) {
		int i = partAt((m.x + DX[d]) & mask, (m.y + DY[d]) & mask);
		if (i < 0)
			continue;
		if (_world.objects[i].frame == m.frame - 8)
			bow = i;
		else if (_world.objects[i].frame == m.frame + 8)
			stern = i;
	}
	return bow >= 0 && stern >= 0;
}

// Skiffs (414), rafts (415), ships (412) and balloons (423): using one boards it, with the whole party aboard; using the shore from aboard puts the party ashore.
// A ship needs its deed (a deed, type 0x95, whose quality is the ship's) if it has a number, and is sailed as its middle part with the bow and stern following it
// (use_boat 34919, sub_2722E, sub_274B5; the stowing of a skiff in a ship and the launching of a skiff from one are not done).
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
		me.type = (uint16_t)vehicleType;
		me.frame = (uint8_t)vehicleFrame;
		if (!ok) {
			if (vehicleType == 0x19C) {
				message("\nOnly next to land.\n");
				return true;
			}
			return false;
		}
		me.type = (uint16_t)_footType;
		me.frame = (uint8_t)_footFrame;
		// the vehicle stays where it was; the party steps ashore
		World::Obj boat;
		boat.status = 0;
		boat.x = me.x;
		boat.y = me.y;
		boat.z = me.z;
		boat.type = (uint16_t)vehicleType;
		boat.frame = (uint8_t)vehicleFrame;
		boat.quantity = (uint8_t)(vehicleType == 0x19C ? _shipQuantity : 1);
		boat.quality = (uint8_t)(vehicleType == 0x19C ? _shipQuality : 0);
		boat.actor = -1;
		boat.parent = -1;
		_world.objects.push_back(boat);
		_world.addToMap((int)_world.objects.size() - 1);
		_aboard = false;
		_shipBow = _shipStern = -1;
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
		if (v.actor >= 0 || (v.type != 414 && v.type != 415 && v.type != 412 && v.type != 423))
			continue;
		int boardAt = i, bow = -1, stern = -1;
		if (v.type == 0x19C) {
			int mid;
			if (!shipParts(i, mid, bow, stern)) {
				message("\nNot usable\n");
				return true;
			}
			boardAt = mid;
		}
		const World::Obj &b = _world.objects[boardAt];
		if ((b.type == 0x19C || b.type == 0x19E) && b.quality != 0) {
			bool deed = false;
			for (int m = 0; m < _world.actors.partySize && !deed; ++m) {
				std::vector<int> inv;
				_world.inventory(_world.actors.partyRoster[m], inv, true);
				for (int k : inv)
					deed |= _world.objects[k].type == 0x95 && _world.objects[k].quality == b.quality;
			}
			if (!deed) {
				message("\nA deed is required.\n");
				return true;
			}
		}
		_footType = me.type;
		_footFrame = me.frame;
		me.type = b.type;
		me.frame = b.frame;
		_shipQuality = b.quality;
		_shipQuantity = b.quantity;
		_shipBow = bow;
		_shipStern = stern;
		int vx = b.x, vy = b.y;
		_world.deleteObject(boardAt);
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

// sub_263D4 for a ship under way: the heading (0 north, 1 east, 2 south, 3 west) after a step in a direction 0-7; a diagonal step keeps the heading if it lies on either side of
// the diagonal and turns the ship about otherwise
int Game::shipHeading(int dir) const {
	int heading = (_world.objects[_leaderObject].frame - 9) / 2 & 3;
	if (!(dir & 1))
		return dir >> 1;
	int q = dir >> 1;
	return ((q - heading + 1) & 3) <= 1 ? heading : (heading + 2) & 3;
}

// The bow and stern squares of a ship whose middle is at (mx, my) heading h
void Game::shipEnds(int mx, int my, int h, int &bx, int &by, int &sx, int &sy) const {
	static const int DX[4] = {0, 1, 0, -1}, DY[4] = {-1, 0, 1, 0};
	int mask = z == 0 ? 0x3FF : 0xFF;
	bx = (mx + DX[h]) & mask;
	by = (my + DY[h]) & mask;
	sx = (mx - DX[h]) & mask;
	sy = (my - DY[h]) & mask;
}

// the bow and stern follow the middle, and are turned with it
void Game::moveShipParts(int h) {
	int bx, by, sx, sy;
	shipEnds(x, y, h, bx, by, sx, sy);
	_world.objects[_leaderObject].frame = (uint8_t)(9 + 2 * h);
	if (_shipBow >= 0) {
		_world.moveObject(_shipBow, bx, by, z);
		_world.objects[_shipBow].frame = (uint8_t)(1 + 2 * h);
	}
	if (_shipStern >= 0) {
		_world.moveObject(_shipStern, sx, sy, z);
		_world.objects[_shipStern].frame = (uint8_t)(17 + 2 * h);
	}
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
	castSpell(spell, caster);
}

// cast_spell_effect: a spell that needs no target works at once, the others ask for one with the cursor
void Game::castSpell(const Magic::SpellInfo *spell, int caster, bool promptAfter) {
	_spell = spell;
	if (spell->select == Magic::SEL_NONE) {
		if (!spellSpecial(*spell, caster, x, y, caster))
			_magic.cast(caster, *spell, caster);
		_world.spendMoves(caster, spell->circle * 3 + 10); // cmd_cast: sub_26CC4(caster, circle * 3 + 10)
		if (promptAfter && _pending != 'W' && !_eye)
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
	// sub_223BC ends with the result of the effect: "Success" (0), "Failed" (1) or "No effect" (2); the spells that fly as a ball of fire or lightning (34, 68, 96) say nothing
	_spellQuiet = false;
	bool done = spellSpecial(*_spell, caster, tx, ty, target);
	if (!done)
		done = _magic.cast(caster, *_spell, target);
	int idx = _spell->index();
	if (done && !_spellQuiet && idx != 34 && idx != 68 && idx != 96 && idx != 18 && idx != 17 && idx != 20 && idx != 5 && idx != 50 && idx != 49 && idx != 119 && idx != 0 && idx != 53 && idx != 121 && idx != 100 && idx != 39 && idx != 105 && idx != 83 && idx != 87 && idx != 98 && idx != 113 && idx != 37 && idx != 38 && idx != 54 && idx != 65 && idx != 85 && idx != 102 && idx != 103 && idx != 115 && idx != 116 && idx != 51 && idx != 55 && idx != 56 && idx != 64 && idx != 84 && idx != 66 && idx != 89)
		message("\nSuccess\n");
	_world.spendMoves(caster, _spell->circle * 3 + 10);
	_spell = nullptr;
	if (!_pending && _stealing < 0)
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
	placeGates();
}

// Use: what the objects of a square do when used (the doors and ladders of the Use table; the rest answers "Not usable").
// APPROX(30): not the original's exact behaviour, see docs/approximations.md
// use_horse / rear_horse (GAME.EXE 34B..): using a horse mounts it (the leader becomes a horse with rider, type 0x1AF, and the horse is gone);
// using yourself while mounted dismounts onto a free square next to you (a new horse appears) or says "No room!".
bool Game::useHorse(int tx, int ty) {
	World::Obj &me = _world.objects[_leaderObject];
	if (me.type == 0x1AF && tx == x && ty == y) {
		static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1}, DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
		MoveRules rules;
		for (int d = 0; d < 8; ++d) {
			int nx = x + DX[d], ny = y + DY[d];
			if (!_movement.check(nx, ny, z, _leaderObject, rules).passable)
				continue;
			int horse = _creatures.spawn(0x1AE, 0x0C, nx, ny, z);
			if (horse < 0)
				break;
			me.type = (uint16_t)(_world.actors.baseObjFrame[1] & 0x3FF ? _world.actors.baseObjFrame[1] & 0x3FF : 0x19A);
			return true;
		}
		message("\nNo room!\n");
		return true;
	}
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	for (int i : here) {
		World::Obj &o = _world.objects[i];
		if (o.type != 0x1AE || o.actor < 0)
			continue;
		if (o.actor == 9) {
			message("\nSherry says: \"Eeek!!! I'm afraid of horses!\"\n");
			return true;
		}
		if (o.actor == 0xA4) {
			message("\nBehLem says: \"Horses are for food!\"\n");
			return true;
		}
		if (me.type == 0x1AF) {
			message("\nYou're already on a horse!\n");
			return true;
		}
		if (o.actor >= 0xE0 || true) {
			_world.actors.baseObjFrame[1] = (uint16_t)me.type;
			me.type = 0x1AF;
			_world.deleteObject(i);
			_world.actors.hp[o.actor] = 0;
			_world.moveObject(_leaderObject, tx, ty, z);
			x = tx;
			y = ty;
			return true;
		}
	}
	return false;
}

// cmd_rest's conditions (89459..): in the order the original tests them; each yields the text printed after "Rest-".
std::string Game::restCheck() {
	const ObjList &a = _world.actors;
	int mask = z == 0 ? 0x3FF : 0xFF;
	if (_leaderObject >= 0 && _world.objects[_leaderObject].type == 0x19C)
		return "Can not be repaired!";
	// sub_39D53: a building (a tile with flag 4 and a high nibble) within 5 squares, or standing on water on foot
	bool built = false;
	for (int dx = -5; dx <= 5 && !built; ++dx)
		for (int dy = -5; dy <= 5 && !built; ++dy) {
			int f = _world.flags.flags1[_world.map.tile(z, (x + dx) & mask, (y + dy) & mask) & (TileFlags::TILES - 1)];
			built = (f & 4) && (f & 0xF0);
		}
	int under = _world.flags.flags1[_world.map.tile(z, x, y) & (TileFlags::TILES - 1)];
	if (built || ((under & 1) && !_aboard))
		return "Only in the wilderness!";
	// sub_39DE0: a creature that attacks the party within the 40 x 40 square
	int left = (x - 16) & 0x3F8, top = (y - 16) & 0x3F8;
	for (int actor = 0; actor < ObjList::ACTORS; ++actor) {
		int k = _world.actorObject(actor);
		if (k < 0 || _world.objects[k].deleted)
			continue;
		int st = a.status[actor] & 0x60;
		const World::Obj &o = _world.objects[k];
		if ((st == 0x20 || st == 0x60) && (((int)o.x - left) & mask) < 0x27 && (((int)o.y - top) & mask) < 0x27 && o.z == z)
			return "Not while foes are near!";
	}
	// sub_39EA6: anyone not in the party within 5 squares (except a horse)
	for (int actor = 0; actor < ObjList::ACTORS; ++actor) {
		int k = _world.actorObject(actor);
		if (k < 0 || _world.objects[k].deleted || (a.status[actor] & 0x80))
			continue;
		const World::Obj &o = _world.objects[k];
		if (std::abs((int)o.x - x) <= 5 && std::abs((int)o.y - y) <= 5 && o.z == z && o.type != 0x1AE)
			return "Not while others are near!";
	}
	// sub_39F44: all the squares around must be enterable
	MoveRules rules;
	for (int dx = -1; dx <= 1; ++dx)
		for (int dy = -1; dy <= 1; ++dy)
			if (!_movement.check(x + dx, y + dy, z, _leaderObject, rules).passable)
				return "Not enough room!";
	if (_world.objects[_leaderObject].type == 0x1AF)
		return "Dismount first!";
	return "";
}

// Rest: "How many hours? " takes a single digit 1-9 (anything else prints "0" and gives up); with a party "Who will guard? " asks for a member
// (a digit) or none. APPROX(31): what the camp does afterwards is not traced (the campfire, "Mealtime!", food, sleeping, ambushes, healing).
void Game::restKey(int key) {
	if (_number == "g") {
		const ObjList &a = _world.actors;
		int who = key - '1';
		std::string guard = "none";
		if (who >= 0 && who < a.partySize)
			guard = (const char *)a.partyNames[who];
		message(guard + "\n");
		_number.clear();
		_pending = 0;
		message("\nMealtime!\n");
		for (int h = 0; h < _restHours; ++h) {
			advanceTime(60);
			ObjList &al = _world.actors;
			for (int i = 0; i < al.partySize; ++i) {
				int actor = al.partyRoster[i];
				int most = ActorStats::maxHealth(_world, actor);
				al.hp[actor] = (uint8_t)std::min<int>(most, al.hp[actor] + 1 + al.level[actor] / 2);
			}
		}
		prompt();
		return;
	}
	if (key < '1' || key > '9') {
		_pending = 0;
		message("0\n");
		prompt();
		return;
	}
	message(std::string(1, (char)key) + "\n");
	_restHours = key - '0';
	if (_world.actors.partySize > 1) {
		message("Who will guard? ");
		_number = "g";
		return;
	}
	_pending = 0;
	message("\nMealtime!\n");
	for (int h = 0; h < _restHours; ++h)
		advanceTime(60);
	prompt();
}

} // namespace Ultima6
} // namespace Ultima6
