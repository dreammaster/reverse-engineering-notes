#include "ultima6/session.h"

#include "ultima6/describe.h"
#include "ultima6/stats_view.h"
#include "ultima6/ui.h"

#include <algorithm>
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

Game::Game(World &world) : scroll(22, 14, 17, 10), _world(world), _renderer(world), _movement(world), _actors(world, _movement), _combat(world, _rng, scroll), _creatures(world, _movement, _actors, _combat, _rng, scroll), _magic(world, _combat, _rng, scroll) {
	world.avatarPosition(x, y, z);
	_leaderObject = world.actorObject(1);
	_rules.questFlag = !world.actors.globals.empty() && world.actors.globals[0] != 0;
	prompt();
	const std::vector<uint8_t> &g = world.actors.globals;
	// objlist +0x1BF3: minute, hour, day, month, year (u16)
	clock.minute = g[2];
	clock.hour = g[3];
	clock.day = g[4];
	clock.month = g[5];
	clock.year = g[6] | (g[7] << 8);
	_actors.hourChanged(clock);
	_creatures.checkEggs(x, y, z, clock.hour);
}

bool Game::save() {
	std::vector<uint8_t> &g = _world.actors.globals;
	g[2] = (uint8_t)clock.minute;
	g[3] = (uint8_t)clock.hour;
	g[4] = (uint8_t)clock.day;
	g[5] = (uint8_t)clock.month;
	g[6] = (uint8_t)(clock.year & 0xFF);
	g[7] = (uint8_t)(clock.year >> 8);
	return !saveDir.empty() && _world.save(saveDir);
}

void Game::advanceTime(int minutes) {
	for (int i = 0; i < minutes; ++i) {
		clock.advance(1);
		if (clock.minute == 0)
			_actors.hourChanged(clock);
		// the timers of light and the like count down a minute at a time; magic points come back slowly
		for (int t = 0; t < 16; ++t)
			if (_world.actors.globals[0x12 + t])
				--_world.actors.globals[0x12 + t];
		if (clock.minute % 10 == 0) {
			ObjList &al = _world.actors;
			for (int m = 0; m < al.partySize; ++m) {
				int actor = al.partyRoster[m];
				if (al.unknown13f1[actor] < ActorStats::maxMagic(_world, actor))
					++al.unknown13f1[actor];
			}
		}
		_actors.tick(x, y, z);
		_creatures.tick(x, y, z);
		_creatures.checkEggs(x, y, z, clock.hour);
		if (_world.actors.hp[1] == 0 && !_dead) {
			_dead = true;
			message("\nThou art dead!\n*");
			break;
		}
	}
}

void Game::prompt() {
	// the name of the party member in control, a colon and '>' (command_loop prints it with %s:\n%>)
	message(std::string((const char *)_world.actors.partyNames[0]) + ":\n>");
}

bool Game::canWalk(int tx, int ty, int tz) {
	MoveResult r = _movement.check(tx, ty, tz, _leaderObject, _rules);
	if (r.denied) {
		message("\n\"Thou art not upon a Sacred Quest!\nPassage denied!\"\n");
		_rules.deniedPending = false;
	}
	return r.passable;
}

bool Game::step(int dir) {
	int mask = z == 0 ? MapData::SURFACE_SIZE - 1 : MapData::DUNGEON_SIZE - 1;
	int nx = (x + DIR_DX[dir]) & mask, ny = (y + DIR_DY[dir]) & mask;
	if (!canWalk(nx, ny, z))
		return false; // the original only plays a sound
	if (DIR_DX[dir] && DIR_DY[dir]) {
		// a diagonal step needs one of the two squares it cuts across to be open (sub_2787E)
		if (!canWalk((x + DIR_DX[dir]) & mask, y, z) && !canWalk(x, (y + DIR_DY[dir]) & mask, z))
			return false;
	}
	int oldX = x, oldY = y;
	x = nx;
	y = ny;
	_world.moveObject(_leaderObject, x, y, z);
	_actors.face(1, DIR_DY[dir] < 0 ? 0 : DIR_DY[dir] > 0 ? 2 : DIR_DX[dir] > 0 ? 1 : 3);
	followLeader(oldX, oldY);
	_rules.deniedPending = true;
	advanceTime(1);
	return true;
}

void Game::message(const std::string &text) {
	scroll.print(text);
	log.push_back(text);
	if (log.size() > 64)
		log.erase(log.begin());
}

bool Game::selectTarget(int key, int &tx, int &ty) const {
	int dir;
	switch (key) {
	case KEY_UP:
		dir = 0;
		break;
	case KEY_RIGHT:
		dir = 2;
		break;
	case KEY_DOWN:
		dir = 4;
		break;
	case KEY_LEFT:
		dir = 6;
		break;
	default:
		return false;
	}
	int mask = z == 0 ? MapData::SURFACE_SIZE - 1 : MapData::DUNGEON_SIZE - 1;
	tx = (x + DIR_DX[dir]) & mask;
	ty = (y + DIR_DY[dir]) & mask;
	return true;
}

void Game::talkAt(int tx, int ty) {
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	int actor = -1;
	for (int i : here)
		if (_world.objects[i].actor >= 0 && _world.objects[i].actor != 1)
			actor = _world.objects[i].actor;
	if (actor < 0) {
		message("\nNo response\n");
		prompt();
		return;
	}
	std::unique_ptr<Talk> t(new Talk(_world, scroll, _rng, clock, actor));
	if (!Talk::canTalk(_world, actor)) {
		message("\nFunny, no response.\n");
		prompt();
		return;
	}
	_talk = std::move(t);
	_talk->start();
	if (_talk->actor() < 0)
		_talk.reset();
}

void Game::look(int tx, int ty) {
	message("Thou dost see ");
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	int top = -1;
	for (int i : here)
		if (!_world.objects[i].invisible())
			top = i;
	std::string text;
	if (top < 0) {
		int tile = _world.map.tile(z, tx, ty);
		text = Describe::article(_world, tile) + Describe::tileName(_world, tile, false) + ".\n";
	} else {
		const World::Obj &o = _world.objects[top];
		text = Describe::object(_world, o);
		if (o.actor < 0) {
			int w = _world.weightOf(top);
			if (w > 0) {
				bool one = _world.quantityOf(o) == 1;
				text += std::string(one ? ". It weighs" : ". They weigh") + " " + std::to_string(w / 10) + "." + std::to_string(w % 10) + " stones";
			}
		}
		text += ".\n";
	}
	message(text);
	prompt();
}

void Game::command(int key) {
	int c = key >= 'a' && key <= 'z' ? key - 32 : key;
	switch (c) {
	case 'T':
		message("Talk-");
		_pending = 'T';
		break;
	case 'C':
		message("Cast-");
		_pending = 'C';
		_invocation.clear();
		break;
	case 'A':
		message("Attack-");
		_pending = 'A';
		break;
	case 'G':
		message("Get-");
		_pending = 'G';
		break;
	case 'U':
		message("Use-");
		_pending = 'U';
		break;
	case 'R':
		message("Rest-");
		message("\nHow many hours? ");
		_pending = 'R';
		_number.clear();
		break;
	case 'L':
		message("Look-");
		_pending = 'L';
		_cursor = true;
		_cx = _cy = 5;
		break;
	case 'I':
		_inventory = true;
		_selected = 0;
		break;
	case 19: // ctrl-S
		message("Save game? ");
		_confirmSave = true;
		break;
	case ' ':
		message("Pass!\n");
		advanceTime(1);
		prompt();
		break;
	default:
		break;
	}
}

bool Game::handleKey(int key) {
	if (key == KEY_QUIT)
		return false;
	if (_dead)
		return false; // any key after the Avatar's death ends the game
	if (key >= '1' && key <= '8' && !_talk && !_pending && !_confirmSave && key - '1' < _world.actors.partySize) {
		_member = key - '1';
		_selected = 0;
		return true;
	}
	if (_inventory && !_talk)
		return inventoryKey(key);
	if (_talk) {
		if (!_talk->key(key)) {
			_talk.reset();
			message("\n");
			prompt();
		}
		return true;
	}
	if (_confirmSave) {
		_confirmSave = false;
		if (key == 'y' || key == 'Y') {
			message("Yes\n");
			message(save() ? "Game saved.\n" : "Cannot save!\n");
		} else {
			message("No\n");
		}
		prompt();
		return true;
	}
	if (_pending == 'C') {
		castKey(key);
		return true;
	}
	if (_pending == 'L' || _pending == 'c') {
		// the cursor: arrows move it, Enter looks at the cell, Escape gives up
		switch (key) {
		case KEY_UP:
			_cy = std::max(1, _cy - 1);
			break;
		case KEY_DOWN:
			_cy = std::min(9, _cy + 1);
			break;
		case KEY_LEFT:
			_cx = std::max(1, _cx - 1);
			break;
		case KEY_RIGHT:
			_cx = std::min(9, _cx + 1);
			break;
		case KEY_ENTER:
		case KEY_SPACE: {
			int mask = z == 0 ? MapData::SURFACE_SIZE - 1 : MapData::DUNGEON_SIZE - 1;
			bool casting = _pending == 'c';
			_pending = 0;
			_cursor = false;
			if (casting)
				castAt((x + _cx - 5) & mask, (y + _cy - 5) & mask);
			else
				look((x + _cx - 5) & mask, (y + _cy - 5) & mask);
			break;
		}
		case KEY_ESC:
			_pending = 0;
			_cursor = false;
			message("what?\n");
			prompt();
			break;
		default:
			break;
		}
		return true;
	}
	if (_pending == 'R') {
		restKey(key);
		return true;
	}
	if (_pending) {
		int tx, ty;
		int cmd = _pending;
		_pending = 0;
		if (selectTarget(key, tx, ty)) {
			if (cmd == 'T')
				talkAt(tx, ty);
			else if (cmd == 'G')
				getAt(tx, ty);
			else if (cmd == 'A')
				attackAt(tx, ty);
			else if (cmd == 'U')
				useAt(tx, ty);
		} else {
			message("what?\n");
			prompt();
		}
		return true;
	}
	switch (key) {
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
		command(key);
		break;
	}
	return true;
}

void Game::render(Framebuffer &screen) {
	screen.setPalette(_world.palette.rgb);
	screen.clear(0);
	screen.blit(0, 0, &_world.paper.pixels[0], _world.paper.width, _world.paper.height, _world.paper.width);
	// the light the party makes: a lit torch carried by any member, or the Light spells
	int radius = 1;
	const ObjList &al = _world.actors;
	for (int m = 0; m < al.partySize && radius < 4; ++m) {
		std::vector<int> inv;
		_world.inventory(al.partyRoster[m], inv, false);
		for (int i : inv)
			if (_world.objects[i].type == 90 && (_world.objects[i].frame & 1))
				radius = 4;
	}
	if (al.globals[0x12 + Magic::TIMER_LIGHT])
		radius = al.globals[0x12 + Magic::TIMER_LIGHT] > 100 ? 7 : 5;
	uint8_t dark[MapRenderer::CELLS * MapRenderer::CELLS];
	_renderer.computeDarkness(x, y, z, clock.hour, radius, dark);
	_renderer.draw(screen, x, y, z, dark);
	if (_talk)
		drawTalkView(screen, _world, _talk->portraitActor());
	else if (_inventory)
		drawInventoryView(screen, _world, _member, _selected, _items);
	else
		drawStatsView(screen, _world, _member);
	scroll.draw(screen, _world.font);
	if (_cursor)
		Ui::drawTile(screen, _world, 0x16C, _cx * 16, _cy * 16);
}

} // namespace Ultima6
} // namespace Ultima6
