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
