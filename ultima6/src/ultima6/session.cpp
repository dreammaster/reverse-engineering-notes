#include "ultima6/session.h"

#include "ultima6/describe.h"
#include "ultima6/stats_view.h"
#include "ultima6/ui.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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
	while (month > 12) { // advance_time 137A0: the original checks the month against 0Ch, not Nuvie's 13
		month -= 12;
		++year;
	}
}

Game::Game(World &world) : scroll(22, 14, 17, 10), _world(world), _renderer(world), _movement(world), _actors(world, _movement), _combat(world, _rng, scroll), _creatures(world, _movement, _actors, _combat, _rng, scroll), _magic(world, _combat, _rng, scroll) {
	scroll.setFont(&world.font);
	_combat.onHurt = [this](int actor) {
		_creatures.divide(actor);
	};
	_combat.onSfx = [this](int effect, int param) { sfx(effect, param); };
	_combat.onSwing = [this](int defender) {
		// perform_attack 2D63A: a blow that is not a missile makes its noise (effect 0x11) when it is struck within five squares of the party's place
		int k = _world.actorObject(defender);
		if (k < 0)
			return;
		int dx = std::abs((int)_world.objects[k].x - x), dy = std::abs((int)_world.objects[k].y - y);
		if (dx < 8 && dy < 8 && Combat::distance(dx, dy) < 6)
			sfx(0x11);
	};
	if (world.game != GameId::Ultima6)
		scroll.setColours(0, world.paper.pixels[170 * world.paper.width + 200]); // black on the colour of the frame's message area
	world.avatarPosition(x, y, z);
	_leaderObject = world.actorObject(1);
	chooseMusic();
	_rules.questFlag = !world.actors.globals.empty() && world.actors.globals[0] != 0;
	prompt();
	const std::vector<uint8_t> &g = world.actors.globals;
	// objlist +0x1BF3: minute, hour, day, month, year (u16)
	gameClock.minute = g[2];
	gameClock.hour = g[3];
	gameClock.day = g[4];
	gameClock.month = g[5];
	gameClock.year = g[6] | (g[7] << 8);
	_actors.hourChanged(gameClock);
	updateMoons();
	_creatures.checkEggs(x, y, z, gameClock.hour);
}

bool Game::save() {
	std::vector<uint8_t> &g = _world.actors.globals;
	g[2] = (uint8_t)gameClock.minute;
	g[3] = (uint8_t)gameClock.hour;
	g[4] = (uint8_t)gameClock.day;
	g[5] = (uint8_t)gameClock.month;
	g[6] = (uint8_t)(gameClock.year & 0xFF);
	g[7] = (uint8_t)(gameClock.year >> 8);
	return !saveDir.empty() && _world.save(saveDir);
}

// APPROX(10): not the original's exact behaviour, see docs/approximations.md
// What one game minute does (sub_13305): the clock, the timers, the hourly schedules and magic points, the party's hazards, the eggs.
void Game::minutePassed() {
	// Time Stop (byte_404BA, advance_time 1330F-1334C): while it lasts the whole minute is skipped, the clock included; the minute it runs out goes on as usual
	if (_timeStop > 1) {
		--_timeStop;
		return;
	}
	_timeStop = 0;
	gameClock.advance(1);
	if (gameClock.minute == 0) {
		_actors.hourChanged(gameClock);
		updateMoons();
		if (_restCooldown > 0)
			--_restCooldown;
	}
	windChanges();
	// the timers of light and the like count down a minute at a time
	for (int t = 0; t < 16; ++t)
		if (_world.actors.globals[0x12 + t])
			--_world.actors.globals[0x12 + t];
	if (gameClock.minute == 0) {
		// every hour the people with magic recover as many magic points as their level
		ObjList &al = _world.actors;
		for (int m = 0; m < al.partySize; ++m) {
			int actor = al.partyRoster[m];
			int ko = _world.actorObject(actor);
			int ot = ko >= 0 ? _world.objects[ko].type : 0;
			if (ot != 0x19A && ot != 0x17A && ot != 0x179 && ot != 0x182)
				continue; // advance_time 137D4-1384D: only the Avatar and the human kinds recover magic points
			int most = ActorStats::maxMagic(_world, actor);
			if (most)
				al.unknown13f1[actor] = (uint8_t)std::min<int>(most, al.unknown13f1[actor] + al.level[actor]);
		}
	}
	if (gameClock.hour == 0 && gameClock.minute == 0)
		_fishedToday = false; // the fish bite again each day
	if (_kegTimer > 0 && --_kegTimer == 0)
		explodeKeg();
	actorStatusDecay();
	partyHazards();
	_creatures.despawn(x, y, z);
	_creatures.checkEggs(x, y, z, gameClock.hour);
	if (_world.actors.hp[1] == 0 && (_world.actors.status[1] & 0x10))
		avatarDied();
}

// sub_377C8: the track for where the party is: the gargoyle world (level 5) GARGOYLE, the surface one of STONES, ULTIMA, FOREST or BRIT by chance, any other level DUNGEON;
// a ship or skiff under the leader HORNPIPE; combat ENGAGE. It is chosen again whenever the level, the vehicle or combat changes, and after resting.
void Game::chooseMusic() {
	if (_world.game != GameId::Ultima6) {
		_music = 0;
		return;
	}
	static const int SURFACE[4] = {8, 1, 5, 10};
	int track = z == 5 ? 11 : z == 0 ? SURFACE[_rng.next() % 4] : 9;
	int type = _leaderObject >= 0 ? _world.objects[_leaderObject].type : 0;
	if (type == 0x19C || type == 0x19E)
		track = 6;
	if (_combatMode)
		track = 7;
	_music = track;
}

// sub_2A86A: the Avatar has died: darkness, a voice, and the Help spell (sub_1890C) brings the party back to Lord British's court
void Game::avatarDied() {
	message("\nAn unending darkness engulfs thee...\n");
	message("\nA voice in the darkness intones, \"KAL LOR!\"\n");
	helpParty();
	_leaderObject = _world.actorObject(1);
	_world.avatarPosition(x, y, z);
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
	_actors.plans = 0;
	_actors.scheduleUpdate(x, y, z);
	_creatures.formation(x, y, z);
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
			if (!(st & 0x80) && _timeStop > 0) { // Time Stop: only the party moves
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
		if (_combat.dragStruggle(best)) { // run_actor_turns 2ADD2: the held lose their turn
			if (!_dead && a.hp[1] == 0 && (a.status[1] & 0x10))
				avatarDied();
			continue;
		}
		if (best == leader || a.worktype[best] == 2)
			return; // the player's turn
		int bw = a.worktype[best];
		bool fighting = (bw >= 3 && bw <= 0x10) || bw == 0x12 || bw == 0x13 || (bw >= 0x8B && bw <= 0x90) || bw == 0x94 || (bw >= 0x96 && bw <= 0x9B);
		if (best >= Creatures::FIRST_TEMP_ACTOR || (a.status[best] & 0x80) || fighting)
			_creatures.act(best, x, y, z);
		else
			_actors.act(best, x, y, z);
		int speaker = _creatures.takeTalkRequest();
		if (speaker >= 0 && !_talk) {
			message("\n");
			startTalk(speaker);
			return;
		}
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
	if (!canWalk(nx, ny, z)) {
		sfx(1, 0x137); // party_move 27AC9: a tone, the only answer
		return false;
	}
	if (DIR_DX[dir] && DIR_DY[dir]) {
		// a diagonal step needs one of the two squares it cuts across to be open (sub_2787E)
		if (!canWalk((x + DIR_DX[dir]) & mask, y, z) && !canWalk(x, (y + DIR_DY[dir]) & mask, z)) {
			sfx(1, 0x137);
			return false;
		}
	}
	int oldX = x, oldY = y;
	int cost = _world.moveCost(oldX, oldY, z);
	int vehicle = _aboard ? _world.objects[_leaderObject].type : 0, heading = 0;
	if (vehicle == 0x19C) {
		// the bow and stern of the ship's new heading need squares to sail in (can_actor_enter looks at the parts of a ship)
		heading = shipHeading(dir);
		int bx, by, sx, sy;
		shipEnds(nx, ny, heading, bx, by, sx, sy);
		if (!canWalk(bx, by, z) || !canWalk(sx, sy, z)) {
			sfx(1, 0x137);
			return false;
		}
	}
	// party_move 27B32-27C11: a member standing on the square the leader enters changes places with the leader (it goes to the square the leader leaves, facing the other way)
	for (int m = 0; m < _world.actors.partySize; ++m) {
		int member = _world.actors.partyRoster[m], mk = _world.actorObject(member);
		if (mk < 0 || mk == _leaderObject || (_world.actors.status[member] & 0x10) || _world.objects[mk].type == 0x162)
			continue;
		if (_world.objects[mk].x == nx && _world.objects[mk].y == ny && _world.objects[mk].z == z) {
			_world.moveObject(mk, oldX, oldY, z);
			_actors.face(member, (dir + 4) & 7);
		}
	}
	x = nx;
	y = ny;
	_world.moveObject(_leaderObject, x, y, z);
	_actors.face(_world.actors.partyRoster[_solo >= 0 ? _solo : 0], dir);
	// party_move 27C11: what a step costs a vessel. A ship goes by the wind (if there is any) through the table at ds:180A, a skiff by the current (tiles 8-15) through ds:1812
	static const uint8_t SHIP_COST[8] = {10, 5, 3, 4, 5, 4, 3, 5}, SKIFF_COST[8] = {3, 4, 5, 7, 10, 7, 5, 4};
	if (vehicle == 0x19C) {
		moveShipParts(heading);
		int wind = _world.actors.globals.size() > 9 ? (int8_t)_world.actors.globals[9] : -1;
		if (wind >= 0)
			cost = SHIP_COST[(dir - wind) & 7];
	} else if (vehicle == 0x19E) {
		int tile = _world.map.tile(z, x, y) & 0xFF, current = tile >= 8 && tile < 16 ? tile - 8 : dir;
		cost = SKIFF_COST[(dir - current) & 7];
	}
	followLeader();
	_rules.deniedPending = true;
	_world.spendMoves(1, cost); // sub_28F67, counted on the square that was left
	_creatures.checkEggs(x, y, z, gameClock.hour);
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
	// cmd_talk (35130): who cannot be talked to, in the original's order
	const ObjList &a = _world.actors;
	if ((a.status[actor] & 0x80) && !memberNear(actor)) {
		message("\nNot on screen.\n");
		prompt();
		return;
	}
	int wt = a.worktype[actor], side = a.status[actor] & 0x60;
	if ((a.status[actor] & 0x10) || (a.status[actor] & 0x04) || (a.status[actor] & 0x02) || wt == 0x9B || wt == 0x13 || wt == 7 || wt == 0x12 || side == 0x20 || side == 0x60) {
		message("\n\nNo response\n");
		prompt();
		return;
	}
	if (_solo > 0) { // cmd_talk 20B08: only the Avatar talks when walking alone
		message("\nNot in solo mode.\n");
		prompt();
		return;
	}
	if (actor == actingActor()) {
		message("\nTalking to yourself?\n");
		prompt();
		return;
	}
	startTalk(actor);
}

// sub_1EBEC: a conversation with a person begins (the player talked to them, or they walked up to the party)
void Game::startTalk(int actor) {
	std::unique_ptr<Talk> t(new Talk(_world, scroll, _rng, gameClock, actor));
	if (!Talk::canTalk(_world, actor)) {
		message("\nFunny, no response.\n");
		prompt();
		return;
	}
	t->onSleep = [this]() {
		// the party sleeps until six in the morning (at least an hour), healing as it rests. APPROX(31): the same healing as the Rest command
		int hours = (6 - gameClock.hour + 24) % 24;
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
		if (tile == 0xFF || tile == 0x1BC) { // cmd_look 30348: a blank cell (word_48F2F, set by sub_15747) is "darkness."
			text = "darkness.\n";
		} else {
			text = Describe::article(_world, tile) + Describe::tileName(_world, tile, false) + ".\n";
			if (std::abs(tx - x) <= 1 && std::abs(ty - y) <= 1) // looking at nothing on a square next to the party
				text += "\nSearching here, you find nothing.\n";
		}
	} else {
		const World::Obj &o = _world.objects[top];
		text = Describe::object(_world, o);
		if (o.actor >= 0) { // cmd_look: a person is named and shown (sub_2F969) unless the party is aboard a ship
			message(text + "\n");
			if (!_aboard) {
				_actorView = o.actor;
				_actorName = text;
				return;
			}
			prompt();
			return;
		}
		if (o.actor < 0 && _world.game == GameId::MartianDreams) { // MD's own look: the name and what the thing tells of its state
			message(text + ".\n" + mdLookExtra(o));
			prompt();
			return;
		}
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
		message("Attack");
		if (_aboard && _world.objects[_leaderObject].type != 0x19C) { // cmd_attack: only a ship can fight
			message("-Not while aboard ship!\n");
			prompt();
			break;
		}
		message(" ");
		// sub_2D7B5: the weapons the attacker holds in its hands (up to eight)
		_attackWeapons.clear();
		_attackIndex = 0;
		{
			std::vector<int> inv;
			_world.inventory(_world.actors.partyRoster[_member], inv, false);
			for (int i : inv)
				if (_world.objects[i].location() == ObjRecord::READIED && Combat::weaponDamageOf(_world.objects[i].type) >= 0 && _attackWeapons.size() < 8)
					_attackWeapons.push_back(i);
		}
		attackWeapon();
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
	case 'B': // the Combat command (key 0x42 is event 0x8A of command_loop, 14178): the party fights by its members' own combat worktypes, or follows again
		if (_solo >= 0) {
			message("Not in solo mode!\n");
			prompt();
			break;
		}
		toggleCombat();
		prompt();
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
		_inventory = true; // the object is chosen in the inventory view (Enter or D takes the selected one)
		_dropPending = true;
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

// sub_24A61, the Wizard Eye: the eye moves a square for each direction key (the eight directions), at most 0x28 times more; Escape, a tile of 0xFC or over under it or the last move
// ends it, the eye goes and the map is the party's again
void Game::eyeKey(int key) {
	int dir = key == KEY_UP || key == '8' ? 0 : key == '9' ? 1 : key == KEY_RIGHT || key == '6' ? 2 : key == '3' ? 3 : key == KEY_DOWN || key == '2' ? 4 : key == '1' ? 5 : key == KEY_LEFT || key == '4' ? 6 : key == '7' ? 7 : -1;
	bool done = key == KEY_ESC;
	if (dir >= 0) {
		int mask = z == 0 ? MapData::SURFACE_SIZE - 1 : MapData::DUNGEON_SIZE - 1;
		_eyeX = (_eyeX + DIR_DX[dir]) & mask;
		_eyeY = (_eyeY + DIR_DY[dir]) & mask;
		_world.moveObject(_eyeObject, _eyeX, _eyeY, z);
		--_eyeLeft;
		done = _eyeLeft < 0 || (_world.map.tile(z, _eyeX, _eyeY) & 0xFF) >= 0xFC;
	}
	if (!done)
		return;
	_world.deleteObject(_eyeObject);
	_eyeObject = -1;
	_eye = false;
	message("\nDone\n");
	prompt();
}

// The mouse (sub_16399 / sub_15D54): a click on the row of ten command icons (eight in the spin-offs) under the map (screen 8,176 to 167,191) is the command of that icon (Attack, Cast, Talk, Look, Get, Drop,
// Move, Use, Rest, Combat); on the map a click is a step towards the cell clicked (or, while a cursor asks for a square, the square: the cursor goes there and is confirmed) and
// a right click gives a question up. Returns the key it stands for, 0 for none. APPROX(43): the other regions of the original (the party panel and the inventory view) are not mapped,
// and the cursor of the mouse is not drawn.
int Game::mouseKey(int mx, int my, bool right) {
	if (mx >= 8 && mx <= 167 && my >= 176 && my <= 191 && !right && !_cursor && !_pending && !_talk && !_inventory && !_textActive) {
		static const char ICONS[10] = {'A', 'C', 'T', 'L', 'G', 'D', 'M', 'U', 'R', 'X'};
		static const char SPINOFF_ICONS[8] = {'A', 'T', 'L', 'G', 'D', 'M', 'U', 'X'}; // Martian Dreams and Savage Empire: eight icons, 20 pixels each, no Cast and no Rest
		if (_world.game != GameId::Ultima6)
			return SPINOFF_ICONS[(mx - 8) / 20];
		return ICONS[(mx - 8) >> 4];
	}
	if (mx >= 8 && mx <= 167 && my >= 8 && my <= 167) {
		int cx = mx / 16, cy = my / 16;
		if (_cursor) {
			if (right)
				return KEY_ESC;
			_cx = std::max(1, std::min(9, cx));
			_cy = std::max(1, std::min(9, cy));
			return KEY_ENTER;
		}
		if (right)
			return _pending || _talk ? KEY_ESC : 0;
		if (_pending || _talk || _inventory || _textActive)
			return 0;
		int dx = cx - 5, dy = cy - 5;
		if (!dx && !dy)
			return 0;
		int ax = std::abs(dx), ay = std::abs(dy);
		if (ax >= 2 * ay)
			return dx > 0 ? KEY_RIGHT : KEY_LEFT;
		if (ay >= 2 * ax)
			return dy > 0 ? KEY_DOWN : KEY_UP;
		return dx > 0 ? (dy > 0 ? KEY_PGDN : KEY_PGUP) : (dy > 0 ? KEY_END : KEY_HOME);
	}
	return 0;
}

bool Game::handleKey(int key) {
	if (_ending == 1 && key != KEY_NONE) {
		_ending = 2;
		return false;
	}
	bool more = handleKeyInner(key);
	// after every completed command the actors take their turns (command_loop 145FD)
	if (more && !_pending && !_instrument && !_talk && !_confirmSave && !_inventory && !_dead && !_eye)
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
		if (_gemSpell)
			message("\nDone\n");
		_gemSpell = false;
		prompt();
		return true;
	}
	if (_eye && key != KEY_NONE) {
		eyeKey(key);
		return true;
	}
	if (_instrument && key != KEY_NONE) {
		// play_instrument 329F5: a digit key plays the note of ds:1CEE[digit] in the instrument's effect; any other key ends it
		static const uint16_t NOTES[10] = {0x1EAB, 0x0C2C, 0x0DA9, 0x0F56, 0x103F, 0x123C, 0x1478, 0x16FA, 0x1857, 0x1B53};
		if (key >= '0' && key <= '9') {
			sfx(_instrument, NOTES[key - '0']);
		} else {
			_instrument = 0;
			prompt();
		}
		return true;
	}
	if (_actorView >= 0 && key != KEY_NONE) { // sub_2F969 waits for a key
		_actorView = -1;
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
	if (key >= '0' && key <= '8' && !_talk && !_pending && !_confirmSave && !_moveTarget && (key == '0' || key - '1' < _world.actors.partySize)) {
		if (_inventory) {
			if (key != '0') {
				_member = key - '1';
				_selected = 0;
			}
		} else {
			soloKey(key);
		}
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
	if (_pending == 'L' || _pending == 'Z' || _pending == 'c' || _pending == 'a' || _pending == 'o' || _pending == 'd') {
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
			bool mdPick = _pending == 'Z';
			bool casting = _pending == 'c', attacking = _pending == 'a', orb = _pending == 'o', dropping = _pending == 'd';
			_pending = 0;
			_cursor = false;
			if (mdPick) {
				mdChosen((x + _cx - 5) & mask, (y + _cy - 5) & mask);
			} else if (orb) {
				orbAt((x + _cx - 5) & mask, (y + _cy - 5) & mask);
				prompt();
			} else if (dropping)
				dropAt((x + _cx - 5) & mask, (y + _cy - 5) & mask);
			else if (attacking)
				attackAt((x + _cx - 5) & mask, (y + _cy - 5) & mask);
			else if (casting)
				castAt((x + _cx - 5) & mask, (y + _cy - 5) & mask);
			else
				look((x + _cx - 5) & mask, (y + _cy - 5) & mask);
			break;
		}
		case KEY_ESC:
			if (_pending == 'Z') { // Martian Dreams' Use: no partner chosen
				_pending = 0;
				_cursor = false;
				message("nothing!\n");
				mdContinue(_mdStage, -2);
				break;
			}
			if (_pending == 'd') { // cmd_drop: no place chosen is "Not possible"
				_dropItem = -1;
				_pending = 0;
				_cursor = false;
				message("\nNot possible\n");
				prompt();
				break;
			}
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
		} else if (cmd == 'W') {
			static const char *const NAMES[8] = {"North", "Northeast", "East", "Southeast", "South", "Southwest", "West", "Northwest"};
			int dir = key == KEY_UP || key == '8' ? 0 : key == '9' ? 1 : key == KEY_RIGHT || key == '6' ? 2 : key == '3' ? 3 : key == KEY_DOWN || key == '2' ? 4 : key == '1' ? 5 : key == KEY_LEFT || key == '4' ? 6 : key == '7' ? 7 : -1;
			if (dir >= 0)
				message(std::string(NAMES[dir]) + ".\n");
			else
				message("nowhere.\n");
			if (_world.actors.globals.size() > 9)
				_world.actors.globals[9] = (uint8_t)dir;
			prompt();
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
	case KEY_PGUP: // the diagonals of the number pad
		step(1);
		break;
	case KEY_PGDN:
		step(3);
		break;
	case KEY_END:
		step(5);
		break;
	case KEY_HOME:
		step(7);
		break;
	default:
		command(key);
		break;
	}
	return true;
}

void Game::render(Framebuffer &screen) {
	// sub_114D0: the palette turns round. The grey of colour 0xFF swells and fades (a triangle from 12 to 19 and back), 0xE0-0xE7 and 0xE8-0xEF each pass their eight colours on one place a
	// tick, 0xF0-0xF3, 0xF4-0xF7 and 0xF8-0xFB their four colours one place every second tick
	uint8_t cycled[256][3];
	std::memcpy(cycled, _world.palette.rgb, sizeof cycled);
	{
		int tri = _palTick & 0xF;
		if (tri > 7)
			tri = 15 - tri;
		cycled[0xFF][0] = cycled[0xFF][1] = cycled[0xFF][2] = (uint8_t)(tri + 0xC);
		for (int i = 0; i < 8; ++i) {
			std::memcpy(cycled[0xE0 + ((_palTick - i) & 7)], _world.palette.rgb[0xE7 - i], 3);
			std::memcpy(cycled[0xE8 + ((_palTick - i) & 7)], _world.palette.rgb[0xEF - i], 3);
		}
		for (int i = 0; i < 4; ++i) {
			int step = (_palTick >> 1) - i;
			std::memcpy(cycled[0xF0 + (step & 3)], _world.palette.rgb[0xF3 - i], 3);
			std::memcpy(cycled[0xF4 + (step & 3)], _world.palette.rgb[0xF7 - i], 3);
			std::memcpy(cycled[0xF8 + (step & 3)], _world.palette.rgb[0xFB - i], 3);
		}
	}
	_renderer.setFrame(_animTick);
	screen.setPalette(cycled);
	screen.clear(0);
	screen.blit(0, 0, &_world.paper.pixels[0], _world.paper.width, _world.paper.height, _world.paper.width);
	// the daylight (advance_time 13984-13A05): 0 in a dungeon, at night (hours 20-4) and during an eclipse (the first day of every third month, or the spell), 7 by day, climbing
	// 1-6 through the hour from five and falling 6-1 through the hour from seven in the evening; a torch or the Light spell make it 4 at least. It is the radius (in squares) in which
	// the party sees.
	const ObjList &al = _world.actors;
	bool eclipse = (gameClock.day == 1 && gameClock.month % 3 == 0) || _eclipse;
	int light;
	if (eclipse || gameClock.hour < 5 || gameClock.hour > 19 || (z > 0 && z < 5))
		light = 0;
	else if (gameClock.hour == 5)
		light = gameClock.minute / 10 + 1;
	else if (gameClock.hour == 19)
		light = (59 - gameClock.minute) / 10 + 1;
	else
		light = 7;
	bool lit = al.globals[0x12 + Magic::TIMER_LIGHT] != 0;
	for (int m = 0; m < al.partySize && !lit; ++m) {
		std::vector<int> inv;
		_world.inventory(al.partyRoster[m], inv, false);
		for (int i : inv)
			lit |= _world.objects[i].type == 90 && (_world.objects[i].frame & 1);
	}
	if (light < 4 && lit)
		light = 4;
	if (al.globals[0x12 + Magic::TIMER_LIGHT] > 100)
		light = 7; // Great Light (APPROX(15): the original's radius for it is not traced)
	if (_peek) {
		uint8_t dark[MapRenderer::CELLS * MapRenderer::CELLS];
		_renderer.computeDarkness(x, y, z, light >= 7 ? 12 : 0, light, dark);
		_renderer.draw(screen, _peekX, _peekY, z, dark);
	} else {
		// sub_18F86 and sub_1CB82: what the party sees from where it is (a Wizard Eye from where the eye is)
		ViewGrid view;
		ViewBuilder builder(_world);
		int vx = _eye ? _eyeX : x, vy = _eye ? _eyeY : y;
		builder.build(vx, vy, z, light, view);
		_renderer.draw(screen, vx, vy, z, view, al.globals[0x12 + Magic::TIMER_INFRAVISION] != 0);
	}
	if (_actorView >= 0)
		drawActorView(screen, _world, _actorView, _actorName);
	else if (_talk)
		drawTalkView(screen, _world, _talk->portraitActor());
	else if (_inventory)
		drawInventoryView(screen, _world, _stealing >= 0 ? 100 + _stealing : _member, _selected, _items, _packScroll);
	else
		drawStatsView(screen, _world, _member);
	if (_gem)
		drawGem(screen);
	if (_world.game == GameId::Ultima6) {
		// advance_time 13A13: month-day-year and the wind under the map (cell 2, row 21)
		static const char *const WIND[9] = {"C ", "N ", "NE", "E ", "SE", "S ", "SW", "W ", "NW"};
		int w = _world.actors.globals.size() > 9 ? (int8_t)_world.actors.globals[9] : -1;
		char line[40];
		std::snprintf(line, sizeof line, "%2d-%02d-%04d Wind:%2s", gameClock.month, gameClock.day, gameClock.year, WIND[w >= -1 && w < 8 ? w + 1 : 0]);
		Ui::drawText(screen, _world.font, 16, 168, line, 0x48);
	}
	scroll.draw(screen, _world.font);
	if (_cursor)
		Ui::drawTile(screen, _world, 0x16C, _cx * 16, _cy * 16);
}

} // namespace Ultima6
} // namespace Ultima6
