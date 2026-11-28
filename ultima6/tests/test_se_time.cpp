// Savage Empire's advance of time (advance_time, sub_14746): the light by the hour and the place, the torches that burn down, the counters that fall by the hour (the liquor, the rest),
// poison for everyone, the end of a charm (skipped without the game data).

#include "ultima6/session.h"

#include <cstdio>
#include <string>

using namespace Ultima6;
using ::Ultima6::Ultima6::Game;
using ::Ultima6::Ultima6::World;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

static std::string logText(const Game &g) {
	std::string s;
	for (const std::string &l : g.log)
		s += l;
	return s;
}

int main() {
	static World w;
	w.game = GameId::SavageEmpire;
	if (!w.load("se/game/SAVAGE")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	w.objects.reserve(w.objects.size() + 64);
	Game g(w);
	ObjList &al = w.actors;
	std::vector<uint8_t> &gl = al.globals;
	const int who = al.partyRoster[0];

	// the light of the surface: night 1, the morning climbs from 2 (6:40) to 7 (7:20), noon 7, the evening falls from 7 (17:20) to 2 (18:00), then 1
	g.z = 0;
	auto lightAt = [&](int hour, int minute) {
		g.gameClock.hour = hour;
		g.gameClock.minute = minute;
		g.advanceTime(0);
		return g.seLight();
	};
	CHECK(lightAt(2, 0) == 1);
	CHECK(lightAt(6, 40) == 2);
	CHECK(lightAt(7, 0) == 4);
	CHECK(lightAt(12, 0) == 7);
	CHECK(lightAt(17, 20) == 7);
	CHECK(lightAt(17, 30) == 5);
	CHECK(lightAt(18, 0) == 2);
	CHECK(lightAt(18, 1) == 1);
	// below the surface it is dark; a lit torch held (or the totem's light) makes it 2
	g.z = 1;
	CHECK(lightAt(12, 0) == 0);
	gl[0x12] = 200;
	CHECK(lightAt(12, 0) == 2);
	gl[0x12] = 0;
	CHECK(lightAt(12, 0) == 0);
	g.z = 0;

	// a lit torch (0xD1) in the hand with two minutes in its quality: it burns half of the minutes, and goes out with the message when the minutes are used up
	int torch = w.addToInventory(who, 0xD1, 0, 1);
	w.objects[torch].status = (uint8_t)((w.objects[torch].status & ~ObjRecord::LOCATION_MASK) | ObjRecord::READIED);
	w.objects[torch].quality = 2;
	g.log.clear();
	for (int i = 0; i < 80 && !w.objects[torch].deleted; ++i)
		g.advanceTime(1);
	CHECK(w.objects[torch].deleted);
	CHECK(logText(g).find("A torch burned out.") != std::string::npos);
	// held below the surface it lights the way: 2
	torch = w.addToInventory(who, 0xD1, 0, 1);
	w.objects[torch].status = (uint8_t)((w.objects[torch].status & ~ObjRecord::LOCATION_MASK) | ObjRecord::READIED);
	w.objects[torch].quality = 100;
	g.z = 1;
	g.advanceTime(0);
	CHECK(g.seLight() == 2);
	g.z = 0;
	w.deleteObject(torch);

	// the hour: the liquor's counter (globals 0x26) and the rest cooldown fall by one at the turn of the hour, not by the minute
	g.gameClock.hour = 9;
	g.gameClock.minute = 30;
	gl[0x26] = 3;
	g.advanceTime(10);
	CHECK(gl[0x26] == 3);
	g.advanceTime(25);
	CHECK(gl[0x26] == 2 && g.gameClock.hour == 10 && g.gameClock.minute == 5);
	g.advanceTime(120);
	CHECK(gl[0x26] == 0);
	// a new hour clears the flags 0x24 and 0x25 and sets 0x27
	gl[0x24] = gl[0x25] = 5;
	gl[0x27] = 0;
	g.advanceTime(60);
	CHECK(gl[0x24] == 0 && gl[0x25] == 0 && gl[0x27] == 1);

	// poison hurts anybody, one minute in eight, unless protected (status 1)
	int victim = w.actorObject(al.partyRoster[1]) >= 0 ? al.partyRoster[1] : who;
	al.status[victim] |= 8;
	al.hp[victim] = 100;
	for (int i = 0; i < 200; ++i)
		g.advanceTime(1);
	CHECK(al.hp[victim] < 100 && al.hp[victim] > 50);
	al.status[victim] &= ~8;

	// a charm (object status 4) goes within a few dozen minutes; the member is of the good side again (status 0x40), and in party mode (no solo) follows (worktype 1)
	int k = w.actorObject(victim);
	w.objects[k].status |= 4;
	al.status[victim] = (uint8_t)((al.status[victim] & 0x9F) | 0x20);
	for (int i = 0; i < 400 && (w.objects[k].status & 4); ++i)
		g.advanceTime(1);
	CHECK(!(w.objects[k].status & 4));
	CHECK((al.status[victim] & 0x60) == 0x40);

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}
