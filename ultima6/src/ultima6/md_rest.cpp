// Martian Dreams' camp: the tent (recipe 0 of Use, sub_59480) and the Rest it starts (sub_553F7). The party sleeps in a tent of five pieces, which is the only way to rest in this game, and
// the sleep is how a member gains a level: when the experience has come to the number the level asks for (ds:2143) the member dreams of three obelisks and chooses what grows: strength (the
// sword), dexterity (the heart) or intelligence (the book) (sub_55191).

#include "ultima6/describe.h"
#include "ultima6/session.h"
#include "ultima6/stats_view.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace Ultima6 {
namespace Ultima6 {

namespace {

// ds:2143: the experience a level asks for before the member can have the next one (18 words, the ones past the eighth level are stray data)
const uint16_t LEVEL_EXPERIENCE[18] = {0, 100, 200, 400, 800, 1600, 3200, 6400, 32767, 0, 65281, 65281, 65281, 511, 0, 511, 65281, 4};

// sub_26469: the difference of two coordinates, wrapped round the map
int delta(int a, int b) {
	int d = std::abs(a - b);
	return d > 0x200 ? 0x400 - d : d;
}

// the name of a thing without its article (sub_2F7E9)
std::string bare(std::string s) {
	for (const char *a : {"a ", "an ", "the ", "A ", "An ", "The "})
		if (s.compare(0, strlen(a), a) == 0)
			return s.substr(strlen(a));
	return s;
}

} // namespace

// sub_29C84: whether a member is a man (by the race of the object: 0x156 + 0-15), a random answer for anything else
bool Game::mdMale(int actor) {
	static const uint8_t MALE[16] = {1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0};
	int k = _world.actorObject(actor);
	int t = k >= 0 ? _world.objects[k].type - 0x156 : -1;
	return t >= 0 && t < 16 ? MALE[t] != 0 : seRoll(0, 1) != 0;
}

// The name of a member of the party (sub_56762)
std::string Game::mdMemberName(int actor) const {
	const ObjList &a = _world.actors;
	for (int m = 0; m < a.partySize; ++m)
		if (a.partyRoster[m] == actor)
			return (const char *)a.partyNames[m];
	return "";
}

// sub_59480: the tent. Not in solo mode; the ground round it (the three squares north of the place and the one to the west) must be clear of things in the way and flat; then the camp.
void Game::mdTent(int idx) {
	if (_solo >= 0) {
		message("Not while in solo mode!\n");
		sfx(5);
		return;
	}
	const World::Obj &t = _world.objects[idx];
	int tx = x, ty = y;
	if (!carriedObject(t)) {
		tx = t.x;
		ty = t.y;
	}
	int mask = z == 0 ? 0x3FF : 0xFF;
	std::vector<int> here;
	for (int sx = tx - 1; sx <= tx + 1; ++sx)
		for (int sy = ty - 2; sy <= ty; ++sy) {
			here.clear();
			_world.objectsAt(sx & mask, sy & mask, z, here);
			for (int i : here) {
				const World::Obj &o = _world.objects[i];
				if (o.deleted || (o.actor >= 0 && (_world.actors.status[o.actor] & 0x80)) || o.type == 0x6A)
					continue;
				int tile = _world.baseTile.firstTile[o.type] + o.frame;
				if (_world.flags.flags3[tile & (TileFlags::TILES - 1)] & 0x10)
					continue;
				message("The " + bare(Describe::object(_world, o)) + " is in the way.\n");
				sfx(5);
				return;
			}
		}
	for (int sx = tx - 1; sx <= tx; ++sx)
		for (int sy = ty - 2; sy <= ty; ++sy)
			if (_world.flags.flags1[_world.map.tile(z, sx & mask, sy & mask) & (TileFlags::TILES - 1)] & 2) {
				message("The ground is not flat enough.\n");
				sfx(5);
				return;
			}
	mdRestStart(tx, ty);
}

// sub_553F7 up to the question: the things that stop the camp, the tent and "Rest until sunrise? "
void Game::mdRestStart(int tx, int ty) {
	ObjList &al = _world.actors;
	message("Rest");
	auto refuse = [&](const std::string &text) {
		message(text);
		sfx(5);
	};
	if (al.globals.size() > 0x34 && al.globals[0x34]) {
		message(" - ");
		refuse("Not while in combat mode!\n");
		return;
	}
	// sub_55060: somebody of the evil side (status 0x20) within 39 squares
	for (int a = 0x10; a < 0x100; ++a) {
		int k = _world.actorObject(a);
		if (k < 0 || _world.objects[k].deleted || !(al.status[a] & 0x20) || (al.status[a] & 0x10))
			continue;
		const World::Obj &o = _world.objects[k];
		if (delta(o.x, x) <= 0x27 && delta(o.y, y) <= 0x27 && o.z == z) {
			refuse(" - Not while foes are near!\n");
			return;
		}
	}
	// sub_550E7: somebody who is awake, not in the party, can be seen and is within 5 squares
	for (int a = 1; a < 0x100; ++a) {
		int k = _world.actorObject(a);
		if (k < 0 || _world.objects[k].deleted || (al.status[a] & 0x94))
			continue;
		const World::Obj &o = _world.objects[k];
		if (delta(o.x, x) > 5 || delta(o.y, y) > 5 || o.z != z || o.invisible())
			continue;
		int tile = _world.baseTile.firstTile[o.type] + o.frame;
		if (_world.flags.flags3[tile & (TileFlags::TILES - 1)] & 0x10)
			continue;
		message(" - " + Describe::object(_world, o) + " is too near to set up camp.\n");
		sfx(5);
		return;
	}
	if (_solo >= 0) {
		message(" - ");
		refuse("Not while in solo mode!\n");
		return;
	}
	bool poisoned = false;
	for (int m = 0; m < al.partySize; ++m)
		if (al.status[al.partyRoster[m]] & 8) {
			message("\n" + std::string((const char *)al.partyNames[m]) + " is poisoned!");
			poisoned = true;
		}
	_mdRestX = tx;
	_mdRestY = ty;
	if (poisoned) {
		message("\nDo you really want to sleep anyway? ");
		mdAskKey([this](int key) {
			if (key != 'y' && key != 'Y') { // (any other key is "no")
				message(std::string(1, (char)key) + "\n");
				return true;
			}
			message("Y\n");
			return mdRestCamp();
		});
		return;
	}
	message("\n");
	mdRestCamp(); // (it waits for a key unless it ended at once)
}

// sub_553F7 from the psychic complaints on: nobody who sees too much (the second and third powers) can sleep; the tent goes up and the question comes. Returns true when it ended at once.
bool Game::mdRestCamp() {
	ObjList &al = _world.actors;
	int mask = z == 0 ? 0x3FF : 0xFF;
	bool cannot = false;
	for (int m = 0; m < al.partySize; ++m) {
		int actor = al.partyRoster[m];
		bool light = mdPsychic(actor, 2) != 0, talking = mdPsychic(actor, 1) != 0;
		if (!light && !talking)
			continue;
		cannot = true;
		std::string name = (const char *)al.partyNames[m];
		if (light && talking)
			message("\n" + name + " complains of too much light and inanimate things keep talking.\n");
		else if (talking)
			message("\n" + name + " complains of inanimate things talking.\n");
		else
			message("\n" + name + " complains of too much light.\n");
	}
	if (cannot) {
		message(al.partySize > 1 ? "\nNobody can sleep.\n" : "\nYou can't sleep.\n");
		sfx(5);
		return true;
	}
	// the tent: five pieces (type 0x86, frames 3, 5, 6, 9 and 8), the last one with its flap open while the party sleeps
	static const int PIECE[5][3] = {{0, -1, 3}, {1, -1, 5}, {-1, 0, 6}, {1, 0, 9}, {0, 0, 8}};
	for (int p = 0; p < 5; ++p) {
		_mdTent[p] = seMake(0x86, (_mdRestX + PIECE[p][0]) & mask, (_mdRestY + PIECE[p][1]) & mask, z, 1, 0);
		_world.objects[_mdTent[p]].frame = (uint8_t)PIECE[p][2];
	}
	_world.objects[_mdTent[4]].frame = 7;
	// "Rest until sunrise? " (the hours 7-16 are day)
	int hour = gameClock.hour;
	message("\nRest until ");
	if (hour < 7 || hour > 16) {
		message("sunrise? ");
		_mdRestHours = hour < 7 ? 7 - hour : 24 - hour + 7;
	} else {
		message("sunset? ");
		_mdRestHours = 18 - hour;
	}
	mdAskKey([this](int key) {
		if (key == 'N' || key == 'n') {
			message("N\nHow many hours? ");
			mdAskKey([this](int digit) {
				if (digit >= '1' && digit <= '9') {
					message(std::string(1, (char)digit) + "\n");
					_mdRestHours = digit - '0';
				} else {
					message("0\n");
					_mdRestHours = 0;
				}
				return mdRestSleep();
			});
			return false;
		}
		message("Y\n");
		return mdRestSleep();
	});
	return false;
}

// The sleep (sub_553F7 55880-55A8F): more than four hours heal everybody (twice the hours' half of two rolls of 3-8) once every few hours; then three steps of twenty minutes an hour, in the
// first of which each member whose experience asks for it has a dream. True when it is over.
bool Game::mdRestSleep() {
	ObjList &al = _world.actors;
	if (_restCooldown == 0 && _mdRestHours > 4) {
		for (int m = 0; m < al.partySize; ++m) {
			int actor = al.partyRoster[m];
			int amount = (seRoll(3, 8) + seRoll(3, 8)) * (_mdRestHours >> 1);
			int most = ActorStats::maxHealth(_world, actor);
			al.hp[actor] = (uint8_t)(most >= al.hp[actor] + amount ? al.hp[actor] + amount : most);
		}
		_restCooldown = 6;
	}
	_mdRestStep = 0;
	return mdRestRun();
}

bool Game::mdRestRun() {
	ObjList &al = _world.actors;
	int steps = _mdRestHours * 3;
	bool dreams = steps > al.partySize;
	while (_mdRestStep < steps) {
		int s = _mdRestStep++;
		advanceTime(20);
		if (dreams && s < al.partySize && mdDream(al.partyRoster[s]))
			return false;
	}
	// the poisoned: they may get better, else they lose twice the hours (to 5 at the least)
	for (int m = 0; m < al.partySize; ++m) {
		int actor = al.partyRoster[m];
		if (!(al.status[actor] & 8))
			continue;
		if (seRoll(1, 8) + seRoll(1, 8) >= 15) {
			al.status[actor] &= 0xF7;
			message(std::string((const char *)al.partyNames[m]) + " feels better.\n");
		} else {
			int lose = _mdRestHours * 2;
			al.hp[actor] = (uint8_t)(al.hp[actor] < lose + 5 ? 5 : al.hp[actor] - lose);
		}
	}
	// the tent comes down
	_world.objects[_mdTent[4]].frame = 8;
	for (int p = 0; p < 5; ++p)
		_world.deleteObject(_mdTent[p]);
	return true;
}

// sub_55191: a member whose experience has come to what the level asks for goes up a level, is healed 30, and dreams; waits for B, H or S. True when the dream is waiting for its key.
bool Game::mdDream(int actor) {
	ObjList &al = _world.actors;
	if (actor < 1 || actor > 15 || al.level[actor] >= 18 || LEVEL_EXPERIENCE[al.level[actor]] > al.exp[actor])
		return false;
	++al.level[actor];
	std::string name = mdMemberName(actor), he = mdMale(actor) ? "He" : "She";
	int most = ActorStats::maxHealth(_world, actor);
	al.hp[actor] = (uint8_t)(most > al.hp[actor] + 30 ? al.hp[actor] + 30 : most);
	message(name + " has a dream...\n\n");
	message(he + " sees three stone obelisks, one inscribed with a book, one a heart, and one a sword.\n\n");
	message(he + " feels drawn to one of the obelisks.\n\n");
	message("Does " + name + " touch the obelisk with the book, the heart, or the sword?\n");
	message("Which (B H S):\n");
	mdAskKey([this, actor](int key) {
		ObjList &a = _world.actors;
		int c = key >= 'a' && key <= 'z' ? key - 32 : key;
		if (c == 'S') {
			if (a.strength[actor] < 0x1E)
				++a.strength[actor];
		} else if (c == 'H') {
			if (a.dexterity[actor] < 0x1E)
				++a.dexterity[actor];
		} else if (c == 'B') {
			if (a.intelligence[actor] < 0x1E)
				++a.intelligence[actor];
		} else {
			return false;
		}
		message("\n\n");
		return mdRestRun();
	});
	return true;
}

} // namespace Ultima6
} // namespace Ultima6
