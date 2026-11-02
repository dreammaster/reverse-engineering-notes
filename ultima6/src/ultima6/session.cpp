#include "ultima6/session.h"

#include <cstdio>

namespace Ultima6 {
namespace Ultima6 {

// ds:01A2 / ds:01AA of GAME.EXE: north, north-east, east, ... north-west
static const int DIR_DX[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int DIR_DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

void GameClock::advance(int minutes) {
	minute += minutes;
	while (minute >= 60) {
		minute -= 60;
		++hour;
	}
	while (hour >= 24) {
		hour -= 24;
		++day;
	}
	while (day > 28) {
		day -= 28;
		++month;
	}
	while (month > 13) {
		month -= 13;
		++year;
	}
}

Game::Game(World &world) : _world(world), _renderer(world), _movement(world) {
	world.avatarPosition(x, y, z);
	_leaderObject = world.actorObject(1);
	_rules.questFlag = !world.actors.globals.empty() && world.actors.globals[0] != 0;
	const std::vector<uint8_t> &g = world.actors.globals;
	// objlist +0x1BF3: minute, hour, day, month, year (u16)
	clock.minute = g[2];
	clock.hour = g[3];
	clock.day = g[4];
	clock.month = g[5];
	clock.year = g[6] | (g[7] << 8);
}

bool Game::canWalk(int tx, int ty, int tz) {
	MoveResult r = _movement.check(tx, ty, tz, _leaderObject, _rules);
	if (r.denied) {
		message("\"Thou art not upon a Sacred Quest! Passage denied!\"");
		_rules.deniedPending = false;
	}
	return r.passable;
}

bool Game::step(int dir) {
	int mask = z == 0 ? MapData::SURFACE_SIZE - 1 : MapData::DUNGEON_SIZE - 1;
	int nx = (x + DIR_DX[dir]) & mask, ny = (y + DIR_DY[dir]) & mask;
	if (!canWalk(nx, ny, z)) {
		message("Blocked!");
		return false;
	}
	x = nx;
	y = ny;
	_world.moveObject(_leaderObject, x, y, z);
	_rules.deniedPending = true;
	clock.advance(1);
	return true;
}

void Game::message(const std::string &text) {
	log.push_back(text);
	if (log.size() > 64)
		log.erase(log.begin());
}

bool Game::handleKey(int key) {
	switch (key) {
	case KEY_QUIT:
		return false;
	case KEY_UP:
		step(0);
		break;
	case KEY_RIGHT:
		step(2);
		break;
	case KEY_DOWN:
		step(4);
		break;
	case KEY_LEFT:
		step(6);
		break;
	default:
		break;
	}
	return true;
}

void Game::render(Framebuffer &screen) {
	screen.setPalette(_world.palette.rgb);
	screen.clear(0);
	_renderer.draw(screen, x, y, z);
}

} // namespace Ultima6
} // namespace Ultima6
