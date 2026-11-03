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

// APPROX(7): not the original's exact behaviour, see docs/approximations.md
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
	scroll.setFont(&world.font);
	if (world.game != GameId::Ultima6)
		scroll.setColours(0, world.paper.pixels[170 * world.paper.width + 200]); // black on the colour of the frame's message area
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
	updateMoons();
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

// APPROX(10): not the original's exact behaviour, see docs/approximations.md
// What one game minute does (sub_13305): the clock, the timers, the hourly schedules and magic points, the party's hazards, the eggs.
void Game::minutePassed() {
	clock.advance(1);
	if (clock.minute == 0) {
		_actors.hourChanged(clock);
		updateMoons();
	}
	windChanges();
	// the timers of light and the like count down a minute at a time
	for (int t = 0; t < 16; ++t)
		if (_world.actors.globals[0x12 + t])
			--_world.actors.globals[0x12 + t];
	if (clock.minute == 0) {
		// every hour the people with magic recover as many magic points as their level
		ObjList &al = _world.actors;
		for (int m = 0; m < al.partySize; ++m) {
			int actor = al.partyRoster[m];
			int most = ActorStats::maxMagic(_world, actor);
			if (most)
				al.unknown13f1[actor] = (uint8_t)std::min<int>(most, al.unknown13f1[actor] + al.level[actor]);
		}
	}
	if (clock.hour == 0 && clock.minute == 0)
		_fishedToday = false; // the fish bite again each day
	if (_kegTimer > 0 && --_kegTimer == 0)
		explodeKeg();
	actorStatusDecay();
	partyHazards();
	_creatures.partyFights();
	_creatures.despawn(x, y, z);
	_creatures.checkEggs(x, y, z, clock.hour);
	if (_world.actors.hp[1] == 0 && !_dead) {
		_dead = true;
		message("\nThou art dead!\n*");
	}
}

// Time that passes without anyone acting (resting): minutes at a time.
void Game::advanceTime(int minutes) {
	for (int i = 0; i < minutes && !_dead; ++i) {
		minutePassed();
		_actors.scheduleUpdate(x, y, z);
	}
}

// sub_2AB7A (after every command): the schedule-walkers first, then actors act in the order of the moves they have left until it is the player-controlled
// party member's turn. Every actor gets as many moves as its dexterity each minute (what is left over, or owed, carries on); when no actor has moves
// a minute passes and they are refilled. An actor 1.. that is on the party's level, near the party (within the 40 x 40 square), not paralysed, not
// asleep, not dead, not carried and not a follower (worktype 1) is a candidate; the first with all its moves, else the one with the most moves per
// dexterity, acts. Actors far away simply lose their moves.
void Game::runTurns() {
	ObjList &a = _world.actors;
	_actors.scheduleUpdate(x, y, z);
	int mask = z == 0 ? 0x3FF : 0xFF;
	int leader = a.partyRoster[0];
	for (int guard = 0; guard < 4000 && !_dead; ++guard) {
		int best = -1, bestMoves = 0, bestDex = 1;
		int left = (x - 16) & 0x3F8, top = (y - 16) & 0x3F8;
		for (int actor = 0; actor < ObjList::ACTORS; ++actor) {
			int k = _world.actorObject(actor);
			if (k < 0 || _world.objects[k].deleted)
				continue;
			int wt = a.worktype[actor], st = a.status[actor];
			int moves = (int8_t)a.moves[actor];
			if (!(st & 0x80) && a.globals[0x12 + 14]) { // Time Stop: only the party moves
				a.moves[actor] = 0;
				continue;
			}
			if (wt == 0 || (st & 0x02) || ((st & 0x04) && wt != 0x80) || (st & 0x10) || moves <= 0)
				continue;
			const World::Obj &o = _world.objects[k];
			if (!o.onMap() || o.z != z)
				continue;
			bool inside = (((int)o.x - left) & mask) <= 0x27 && (((int)o.y - top) & mask) <= 0x27;
			if (!inside) {
				a.moves[actor] = 0;
				continue;
			}
			if (wt == 1)
				continue; // followers move with the leader
			int dex = a.dexterity[actor];
			int cross = moves * bestDex - dex * bestMoves;
			if (moves >= dex || cross > 0 || (cross == 0 && dex > bestDex)) {
				best = actor;
				bestMoves = moves;
				bestDex = dex;
			}
			if (moves >= dex)
				break;
		}
		if (best < 0) {
			// nobody can act: everyone's moves are refilled and a minute passes
			for (int actor = 0; actor < ObjList::ACTORS; ++actor) {
				int moves = (int8_t)a.moves[actor];
				a.moves[actor] = (uint8_t)(moves < 0 ? moves + a.dexterity[actor] : a.dexterity[actor]);
			}
			minutePassed();
			_actors.scheduleUpdate(x, y, z);
			continue;
		}
		if (best == leader || a.worktype[best] == 2)
			return; // the player's turn
		if (best >= Creatures::FIRST_TEMP_ACTOR)
			_creatures.act(best, x, y, z);
		else
			_actors.act(best, x, y, z);
	}
}

void Game::prompt() {
	// the name of the party member in control, a colon and '>' (command_loop prints it with %s:\n%>)
	// command_loop prints "%s:\n%>" the first time and "\n%s:\n%>" after each later command
	message(std::string(_firstPrompt ? "" : "\n") + (const char *)_world.actors.partyNames[0] + ":\n>");
	_firstPrompt = false;
}

bool Game::canWalk(int tx, int ty, int tz) {
	MoveResult r = _movement.check(tx, ty, tz, _leaderObject, _rules);
	if (r.denied) {
		message("\n\"Thou art not upon a Sacred Quest!\nPassage denied!\"\n");
		_rules.deniedPending = false;
	}
	return r.passable;
}

// APPROX(5): not the original's exact behaviour, see docs/approximations.md
// party_move (278A0..27988): a raft (0x19F) or balloon (0x1A7) goes where the wind and the player's wish together send it: the wish (or, on a raft over a current tile 8-15, the
// current) and the opposite of the wind's direction index the table at ds:181A; with no wind the wish is followed; without a wish and without wind nothing moves.
int Game::vehicleDirection(int requested) {
	static const int8_t SAIL[8][8] = {{0, 0, 1, 1, 0, 7, 7, 0}, {1, 1, 1, 2, 2, 1, 0, 0}, {1, 2, 2, 2, 3, 3, 2, 1}, {2, 2, 3, 3, 3, 4, 4, 3},
									  {4, 3, 3, 4, 4, 4, 5, 5}, {6, 5, 4, 4, 5, 5, 5, 6}, {7, 7, 6, 5, 5, 6, 6, 6}, {7, 0, 0, 7, 6, 6, 7, 7}};
	int type = _world.objects[_leaderObject].type;
	if (type != 0x19F && type != 0x1A7)
		return requested;
	int wind = _world.actors.globals.size() > 9 ? (int8_t)_world.actors.globals[9] : -1;
	int col = wind ^ 4, want = requested;
	if (type == 0x19F) {
		int tile = _world.map.tile(z, x, y) & 0xFF;
		if (tile >= 8 && tile < 16)
			want = tile - 8;
	}
	int d = col < 0 ? want : (want >= 0 && col < 8 ? SAIL[want][col] : -1);
	return d;
}

// sub_132A6: the wind changes now and then (one minute in 64, on the surface and on level 5): a random direction 0-6 or none (-1); under ground there is none.
void Game::windChanges() {
	std::vector<uint8_t> &g = _world.actors.globals;
	if (g.size() <= 9)
		return;
	if (z > 0 && z < 5) {
		g[9] = 0xFF;
	} else if (_rng.next() % 64 == 0) {
		g[9] = (uint8_t)((int)(_rng.next() % 8) - 1);
	}
}

bool Game::step(int dir) {
	dir = vehicleDirection(dir);
	if (dir < 0) {
		message("Thou canst not move without wind!\n");
		return false;
	}
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
	int cost = _world.moveCost(oldX, oldY, z);
	x = nx;
	y = ny;
	_world.moveObject(_leaderObject, x, y, z);
	_actors.face(1, DIR_DY[dir] < 0 ? 0 : DIR_DY[dir] > 0 ? 2 : DIR_DX[dir] > 0 ? 1 : 3);
	followLeader(oldX, oldY);
	_rules.deniedPending = true;
	_world.spendMoves(1, cost); // sub_28F67, counted on the square that was left
	_creatures.checkEggs(x, y, z, clock.hour);
	enterMoongate(x, y);
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

// APPROX(36): not the original's exact behaviour, see docs/approximations.md
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
	t->onSleep = [this]() {
		// the party sleeps until six in the morning (at least an hour), healing as it rests. APPROX(31): the same healing as the Rest command
		int hours = (6 - clock.hour + 24) % 24;
		if (hours == 0)
			hours = 24;
		for (int h = 0; h < hours; ++h) {
			advanceTime(60);
			ObjList &al = _world.actors;
			for (int i = 0; i < al.partySize; ++i) {
				int actor = al.partyRoster[i];
				int most = ActorStats::maxHealth(_world, actor);
				al.hp[actor] = (uint8_t)std::min<int>(most, al.hp[actor] + 1 + al.level[actor] / 2);
			}
		}
	};
	_talk = std::move(t);
	_talk->start();
	if (_talk->actor() < 0)
		_talk.reset();
}

// APPROX(29): not the original's exact behaviour, see docs/approximations.md
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
			// cmd_look (30567-30653): the weight when it is 1-254 (". It weighs" / ". They weigh" N.M "stones"), what the thing can do as a weapon and
			// absorb as armour
			int w = _world.weightOf(top) + _world.contentsWeight(top);
			bool one = _world.quantityOf(o) == 1;
			bool weighed = w > 0 && w < 0xFF;
			if (weighed)
				text += std::string(one ? ". It weighs" : ". They weigh") + " " + std::to_string(w / 10) + "." + std::to_string(w % 10) + " stones";
			int dmg = Combat::weaponDamageOf(o.type), arm = Combat::armourOf(o.type);
			if (dmg > 0)
				text += std::string(!weighed ? ". It" : arm == 0 ? " and" : ",") + " can do " + std::to_string(dmg) + " point" + (dmg == 1 ? "" : "s") + " of damage";
			if (arm > 0)
				text += " and can absorb " + std::to_string(arm) + " point" + (arm == 1 ? "" : "s") + " of damage";
		}
		// the readable things (the table at ds:1CDA: 0x97, 0x3D, 0x98, 0x10E, 0x3B) are followed by what they say instead of the full stop
		if (o.actor < 0 && (o.type == 0x97 || o.type == 0x3D || o.type == 0x98 || o.type == 0x10E || o.type == 0x3B)) {
			message(text);
			bool read = false;
			std::vector<int> stack;
			if (carriedObject(o))
				stack.push_back(top);
			else
				for (size_t i = here.size(); i-- > 0;)
					stack.push_back(here[i]);
			for (int i : stack)
				if (_world.objects[i].quality != 0) {
					if (!read)
						message(":");
					readBook(i);
					read = true;
				}
			message("\n");
			prompt();
			return;
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
		if (!_magic.checkBook(_world.actors.partyRoster[_member]) || !_magic.checkSpells(_world.actors.partyRoster[_member])) {
			prompt();
			break;
		}
		_pending = 'C';
		_invocation.clear();
		break;
	case 'A':
		message("Attack-");
		_pending = 'a';
		_cursor = true;
		_cx = _cy = 5;
		break;
	case 'G':
		message("Get-");
		_pending = 'G';
		break;
	case 'U':
		message("Use-");
		_pending = 'U';
		break;
	case 'M':
		message("Move-");
		_pending = 'M';
		break;
	case 'R':
		message("Rest");
		{
			std::string why = restCheck();
			if (!why.empty()) {
				message("-" + why + "\n");
				prompt();
				break;
			}
		}
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
	case 'D':
		message("Drop-");
		_inventory = true; // the item is chosen in the inventory view (D drops the selected one)
		_selected = 0;
		break;
	case 19: // ctrl-S
		message("Save game? ");
		_confirmSave = true;
		break;
	case ' ':
		message("Pass!\n");
		_world.actors.moves[_world.actors.partyRoster[_member]] = 0; // loc_14395: the leader's moves are set to 0
		prompt();
		break;
	default:
		break;
	}
}

bool Game::handleKey(int key) {
	if (_ending == 1 && key != KEY_NONE) {
		_ending = 2;
		return false;
	}
	bool more = handleKeyInner(key);
	// after every completed command the actors take their turns (command_loop 145FD)
	if (more && !_pending && !_talk && !_confirmSave && !_inventory && !_dead)
		runTurns();
	return more;
}

bool Game::handleKeyInner(int key) {
	if (key == KEY_QUIT)
		return false;
	if (_dead)
		return false; // any key after the Avatar's death ends the game
	if (_gem && key != KEY_NONE) {
		_gem = false;
		prompt();
		return true;
	}
	if (_peek && key != KEY_NONE) {
		_peek = false;
		message("\nDone\n");
		prompt();
		return true;
	}
	if (_textActive) {
		textKey(key);
		return true;
	}
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
	if (_pending == 'L' || _pending == 'c' || _pending == 'a' || _pending == 'o') {
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
			bool casting = _pending == 'c', attacking = _pending == 'a', orb = _pending == 'o';
			_pending = 0;
			_cursor = false;
			if (orb) {
				orbAt((x + _cx - 5) & mask, (y + _cy - 5) & mask);
				prompt();
			} else if (attacking)
				attackAt((x + _cx - 5) & mask, (y + _cy - 5) & mask);
			else if (casting)
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
		if ((key == KEY_ENTER || key == KEY_SPACE) && cmd == 'U') {
			useAt(x, y); // the own square (rear horse)
		} else if (cmd == 'w') {
			if (key == 'y' || key == 'Y') {
				message("\nWish for: ");
				_textActive = true;
				_textBuf.clear();
				_textMax = 9;
				_textPurpose = 2;
			} else {
				message("\n");
				prompt();
			}
		} else if (cmd == 'm') {
			moveTo(key);
		} else if (cmd == 'v') {
			digAt(key);
		} else if (cmd == 'e') {
			telescopeAt(key);
		} else if (cmd == 'p' && key >= '1' && key <= '8') {
			// the potion's target: a party member by number
			int n = key - '1';
			usePotionOn(n < _world.actors.partySize ? _world.actors.partyRoster[n] : -1);
			prompt();
		} else if (selectTarget(key, tx, ty)) {
			if (cmd == 'k') {
				useKeyOn(tx, ty);
				prompt();
			} else if (cmd == 'p') {
				// an adjacent person
				std::vector<int> here;
				_world.objectsAt(tx, ty, z, here);
				int actor = -1;
				for (int i : here)
					if (_world.objects[i].actor >= 0)
						actor = _world.objects[i].actor;
				usePotionOn(actor);
				prompt();
			} else if (cmd == 'M') {
				moveSelect(tx, ty);
			} else if (cmd == 'T')
				talkAt(tx, ty);
			else if (cmd == 'G')
				getAt(tx, ty);
			else if (cmd == 'A')
				attackAt(tx, ty);
			else if (cmd == 'U')
				useAt(tx, ty);
		} else {
			if (cmd == 'k' || cmd == 'p')
				_useItem = -1;
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
	_renderer.computeDarkness(x, y, z, _world.actors.globals[0x12 + 15] ? 0 : clock.hour, radius, dark); // an eclipse makes it night
	_renderer.draw(screen, _peek ? _peekX : x, _peek ? _peekY : y, z, dark);
	if (_talk)
		drawTalkView(screen, _world, _talk->portraitActor());
	else if (_inventory)
		drawInventoryView(screen, _world, _member, _selected, _items);
	else
		drawStatsView(screen, _world, _member);
	if (_gem)
		drawGem(screen);
	scroll.draw(screen, _world.font);
	if (_cursor)
		Ui::drawTile(screen, _world, 0x16C, _cx * 16, _cy * 16);
}

} // namespace Ultima6
} // namespace Ultima6
