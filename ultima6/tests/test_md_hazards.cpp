// Martian Dreams' hazards of time (md_hazards.cpp, sub_22B27): the cold of the nights, the thin air and the tanks of oxygen, the lamps that burn down, the radium that poisons, the daylight
// (skipped without data).

#include "ultima6/session.h"

#include <cstdio>
#include <string>
#include <vector>

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
	w.game = GameId::MartianDreams;
	if (!w.load("md/game/MARTIAN")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	w.objects.reserve(w.objects.size() + 32);
	Game g(w);
	ObjList &al = w.actors;
	int me = al.partyRoster[0];
	for (int i = 0; i < al.partySize; ++i)
		w.moveObject(w.actorObject(al.partyRoster[i]), g.x, g.y, 0);
	g.z = 0;

	// the daylight: 7 at noon, 0 at night (below ground too)
	g.gameClock.hour = 12;
	g.gameClock.minute = 0;
	CHECK(g.mdLight() == 7);
	g.gameClock.hour = 2;
	CHECK(g.mdLight() == 0);
	g.gameClock.hour = 7;
	g.gameClock.minute = 0;
	CHECK(g.mdLight() == 4); // (7:00 is 420 minutes: (420 - 400) / 10 + 2)

	// the cold: at night a member of the party freezes, by day not
	al.status[me] &= 0xFE;
	g.gameClock.hour = 12;
	g.gameClock.minute = 0;
	g.advanceTime(5);
	CHECK(!(al.status[me] & 1));
	g.gameClock.hour = 23;
	g.gameClock.minute = 0;
	g.log.clear();
	g.advanceTime(3);
	CHECK((al.status[me] & 1) || me == 0);
	if (me != 0)
		CHECK(logText(g).find("is freezing!") != std::string::npos);
	for (int i = 0; i < al.partySize; ++i) { // (hit points back for the rest)
		al.hp[al.partyRoster[i]] = 200;
		al.status[al.partyRoster[i]] &= 0xFE;
	}

	// the thin air: without a tank a member gasps, with one he breathes again; the hour takes a charge
	g.gameClock.hour = 12;
	g.gameClock.minute = 0;
	al.talkFlags[0x10] |= 0x20;
	int mate = -1;
	for (int i = 0; i < al.partySize; ++i)
		if (al.partyRoster[i] != 1 && al.partyRoster[i] != 6 && al.partyRoster[i] != 0)
			mate = al.partyRoster[i];
	if (mate >= 0) {
		int k = w.actorObject(mate);
		w.objects[k].status &= 0xBF;
		g.log.clear();
		g.advanceTime(1);
		CHECK((w.objects[k].status & 0x40) && logText(g).find("gasps for air!") != std::string::npos);
		int tank = w.addToInventory(mate, 0x83, 0, 3);
		CHECK(tank >= 0);
		g.log.clear();
		g.advanceTime(1);
		CHECK(!(w.objects[k].status & 0x40) && logText(g).find("breathes easier!") != std::string::npos);
		g.gameClock.minute = 59;
		g.advanceTime(1); // the hour changes: a charge is breathed
		CHECK(w.objects[tank].quantity == 2);
	}
	al.talkFlags[0x10] &= 0xDF;

	// a lit oil lamp (0x74) in hand burns down and goes out of fuel (0x73), a lit candle-kind (0x6E) goes out and is gone
	{
		int lamp = w.addToInventory(me, 0x74, 3, 1);
		CHECK(lamp >= 0);
		w.objects[lamp].status = (uint8_t)((w.objects[lamp].status & ~ObjRecord::LOCATION_MASK) | ObjRecord::READIED);
		g.log.clear();
		g.advanceTime(120);
		CHECK(w.objects[lamp].type == 0x73 && w.objects[lamp].quality == 0);
		CHECK(logText(g).find("is out of fuel.") != std::string::npos);
	}

	// radium (0x1C0) in the pack poisons, inside a lead container (0x8B) it does not
	{
		for (int i = 0; i < al.partySize; ++i)
			al.status[al.partyRoster[i]] &= 0xF7;
		int box = w.addToInventory(me, 0x8B, 0, 1);
		int radium = w.addToInventory(me, 0x1C0, 0, 1);
		CHECK(box >= 0 && radium >= 0);
		g.log.clear();
		g.advanceTime(1);
		CHECK(logText(g).find("is poisoned!") != std::string::npos && (al.status[me] & 8));
		al.status[me] &= 0xF7;
		w.deleteObject(radium);
	}

	std::printf(failures ? "FAILED\n" : "ok\n");
	return failures ? 1 : 0;
}
