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

	// a seed mound (0xFB, quality 0xF) over a hole (0x101) holding a seed (0xFC) stamped with a day and a month four days old grows a frame; a flame (0x9D) of quality 1 takes the next lower frame
	{
		auto put = [&](int type, int frame) {
			World::Obj o;
			o.status = 0;
			o.x = (uint16_t)(g.x + 1);
			o.y = (uint16_t)g.y;
			o.z = 0;
			o.type = (uint16_t)type;
			o.frame = (uint8_t)frame;
			o.quantity = 1;
			o.quality = 0;
			o.actor = -1;
			o.parent = -1;
			w.objects.push_back(o);
			w.addToMap((int)w.objects.size() - 1);
			return (int)w.objects.size() - 1;
		};
		int mound = put(0xFB, 0), hole = put(0x101, 0);
		w.objects[mound].quality = 0x0F;
		World::Obj seed = w.objects[hole];
		seed.status = ObjRecord::IN_CONTAINER;
		seed.type = 0xFC;
		seed.parent = hole;
		seed.quality = 1;  // the day
		seed.quantity = 5; // the month
		w.objects.push_back(seed);
		g.gameClock.month = 5;
		g.gameClock.day = 10;
		g.gameClock.hour = 12;
		g.gameClock.minute = 0;
		g.advanceTime(1);
		CHECK(w.objects[mound].frame == 1);
		int flame = put(0x9D, 2);
		w.objects[flame].quality = 1;
		g.advanceTime(1);
		CHECK(w.objects[flame].frame == 1 && w.objects[flame].quality >= 10 && w.objects[flame].quality <= 15);
	}

	// a steam jet (0x1D0: length 4, no period, direction east) with the power on burns whoever is in its path (10-50); with the power off nobody is hurt
	{
		World::Obj o;
		o.status = 0;
		o.x = (uint16_t)(g.x + 3);
		o.y = (uint16_t)g.y;
		o.z = 0;
		o.type = 0x1D0;
		o.frame = 0;
		o.quantity = 2 << 5;
		o.quality = 4 << 4;
		o.actor = -1;
		o.parent = -1;
		w.objects.push_back(o);
		w.addToMap((int)w.objects.size() - 1);
		int victim = al.partyRoster[0];
		int vk = w.actorObject(victim);
		w.moveObject(vk, g.x + 5, g.y, 0);
		al.hp[victim] = 200;
		g.gameClock.hour = 12;
		al.talkFlags[0x73] &= 0xFB;
		g.advanceTime(1);
		CHECK(al.hp[victim] == 200);
		al.talkFlags[0x73] |= 4;
		g.advanceTime(1);
		CHECK(al.hp[victim] < 200 && al.hp[victim] >= 150);
	}

	// a thing whose tile has the hazard flag (flag 8 of the first set) hurts the person who stands on it each minute (0xA8: 1-20): the types are found in the data
	{
		int type = -1;
		for (int t : {0xA8, 0xD1, 0x17D, 0xD7})
			if (w.flags.flags1[(w.baseTile.firstTile[t]) & (TileFlags::TILES - 1)] & 8)
				type = t;
		if (type >= 0 && !(w.flags.flags3[(w.baseTile.firstTile[type]) & (TileFlags::TILES - 1)] & 4)) {
			World::Obj o;
			o.status = 0;
			o.x = (uint16_t)(g.x + 7);
			o.y = (uint16_t)g.y;
			o.z = 0;
			o.type = (uint16_t)type;
			o.frame = 0;
			o.quantity = 1;
			o.quality = 0;
			o.actor = -1;
			o.parent = -1;
			w.objects.push_back(o);
			w.addToMap((int)w.objects.size() - 1);
			int victim = al.partyRoster[0];
			w.moveObject(w.actorObject(victim), g.x + 7, g.y, 0);
			al.hp[victim] = 250;
			g.gameClock.hour = 12;
			g.advanceTime(1);
			CHECK(al.hp[victim] < 250 || victim == 6);
		}
	}

	// the ray guns (sub_18A64): a shot along the line hurts the first person in it for the gun's damage (20 for the 0x2D) unless he dodges; the freeze ray (0xF1) freezes water (0xA0, frame 1)
	{
		int shooter = al.partyRoster[0], victim = al.partyRoster[al.partySize > 1 ? 1 : 0];
		int sk = w.actorObject(shooter);
		w.moveObject(sk, g.x, g.y, 0);
		if (victim != shooter) {
			int vk = w.actorObject(victim);
			w.moveObject(vk, g.x + 3, g.y, 0);
			int before = al.hp[victim] = 250;
			for (int shot = 0; shot < 40 && al.hp[victim] == before; ++shot)
				g.mdRayShot(shooter, 0x2D, g.x + 3, g.y);
			CHECK(al.hp[victim] == before - 20);
		}
		World::Obj o;
		o.status = 0;
		o.x = (uint16_t)(g.x + 2);
		o.y = (uint16_t)g.y;
		o.z = 0;
		o.type = 0xA0;
		o.frame = 1;
		o.quantity = 1;
		o.quality = 0;
		o.actor = -1;
		o.parent = -1;
		w.objects.push_back(o);
		w.addToMap((int)w.objects.size() - 1);
		int water = (int)w.objects.size() - 1;
		g.mdRayShot(shooter, 0xF1, g.x + 3, g.y);
		CHECK(w.objects[water].frame == 3);
	}

	std::printf(failures ? "FAILED\n" : "ok\n");
	return failures ? 1 : 0;
}
