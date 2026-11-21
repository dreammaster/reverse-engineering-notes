// Martian Dreams' dream machines (md_dream.cpp): a chair, a pod, a control and the cabinet's panel start the dream of whoever sits in the pod; the Avatar's body walks level 2 alone, the squares
// of type 0x1CD lead on and wake the dreamer (skipped without data).

#include "ultima6/session.h"
#include "ultima6/stats_view.h"

#include <cstdio>
#include <cstring>
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

static int put(World &w, int type, int frame, int x, int y, int z) {
	World::Obj o;
	o.status = 0;
	o.x = (uint16_t)x;
	o.y = (uint16_t)y;
	o.z = (uint8_t)z;
	o.type = (uint16_t)type;
	o.frame = (uint8_t)frame;
	o.quantity = 1;
	o.quality = 0;
	o.actor = -1;
	o.parent = -1;
	w.objects.push_back(o);
	w.addToMap((int)w.objects.size() - 1);
	return (int)w.objects.size() - 1;
}

int main() {
	static World w;
	w.game = GameId::MartianDreams;
	if (!w.load("md/game/MARTIAN")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	w.objects.reserve(w.objects.size() + 64);
	Game g(w);
	int px = g.x, py = g.y;
	ObjList &al = w.actors;
	for (int i = 0; i < al.partySize; ++i)
		w.moveObject(w.actorObject(al.partyRoster[i]), px, py, 0);

	// the machine: the chair 0x122 (frame 1), the pod 0x121, the control 0x120, the thing 0x64 and the cabinet 0x1C9 (quality 4) within five squares
	int chair = put(w, 0x122, 1, px + 1, py + 4, 0);
	(void)chair;
	int podX = px + 1, podY = py + 6;
	put(w, 0x121, 0, podX, podY, 0);
	int control = put(w, 0x120, 0, px, py + 4, 0);
	put(w, 0x64, 0, px + 2, py + 4, 0);
	int cabinet = put(w, 0x1C9, 0, px + 3, py + 4, 0);
	w.objects[cabinet].quality = 4;

	// nobody sits in it
	g.log.clear();
	g.mdDreamMachine(cabinet);
	CHECK(logText(g).find("There is nobody sitting in the machine.") != std::string::npos);
	CHECK(!g.mdDreaming());

	// a machine that has ceased to work
	al.talkFlags[0x46] |= 8;
	g.log.clear();
	g.mdDreamMachine(cabinet);
	CHECK(logText(g).find("ceased functioning") != std::string::npos);
	al.talkFlags[0x46] &= 0xF7;

	// a companion sits there: it dreams until dawn (eight in the morning of the next day when it is past eight now)
	g.gameClock.hour = 10;
	g.gameClock.minute = 30;
	int day = g.gameClock.day;
	w.moveObject(w.actorObject(2), podX, podY, 0);
	g.log.clear();
	g.mdDreamMachine(cabinet);
	CHECK(logText(g).find("dreams until dawn while the party waits.") != std::string::npos);
	CHECK(!g.mdDreaming());
	CHECK(g.gameClock.hour >= 8 && g.gameClock.day == (day % 28) + 1);
	CHECK(!(al.status[2] & 4));
	CHECK(g.mdTakeMorning());
	CHECK(!g.mdTakeMorning());
	w.moveObject(w.actorObject(2), px, py, 0);

	// the Avatar sits there: the dream begins at 100, 59 of level 2, the body (actor 0) is the one that walks, alone
	g.gameClock.hour = 3;
	g.gameClock.minute = 0;
	int partySize = al.partySize;
	w.moveObject(w.actorObject(1), podX, podY, 0);
	g.x = podX;
	g.y = podY;
	g.log.clear();
	g.mdDreamMachine(cabinet);
	CHECK(g.mdDreaming());
	CHECK(g.x == 100 && g.y == 59 && g.z == 2);
	CHECK(al.partySize == partySize && al.partyRoster[partySize] == 0);
	int body = w.actorObject(0);
	CHECK(body >= 0 && w.objects[body].x == 100 && w.objects[body].y == 59 && w.objects[body].z == 2);
	CHECK(w.objects[body].type == 0x157 || w.objects[body].type == 0x158);
	CHECK(al.hp[0] == ::Ultima6::Ultima6::ActorStats::maxHealth(w, 1));
	CHECK(w.objects[w.actorObject(1)].x == podX && w.objects[w.actorObject(1)].z == 0); // the Avatar stays on the chair

	// a square of type 0x1CD without flags leads to y = quantity, x = quality
	int portal = put(w, 0x1CD, 0, 101, 59, 2);
	w.objects[portal].quantity = 20;
	w.objects[portal].quality = 30;
	w.moveObject(body, 101, 59, 2);
	g.x = 101;
	g.y = 59;
	g.mdStepOn();
	CHECK(g.x == 30 && g.y == 20 && g.z == 2);
	CHECK(w.objects[body].x == 30 && w.objects[body].y == 20);

	// the scene of actor 0x50 ends: its 0x1CD and the picture above go, the person stands where its schedule starts
	Schedule::Entry e = w.schedule.entry(w.schedule.index[0x50]);
	put(w, 0x1CD, 0, e.x, e.y, 2);
	put(w, 0x124, 3, e.x, e.y - 1, 2);
	g.mdDreamEnd(0x50);
	{ // (the game's own objects of the scene may have been there first: one of each is gone)
		std::vector<int> a, b;
		w.objectsAt(e.x, e.y, 2, a);
		w.objectsAt(e.x, e.y - 1, 2, b);
		int n1 = 0, n2 = 0;
		for (int i : a)
			n1 += w.objects[i].type == 0x1CD;
		for (int i : b)
			n2 += w.objects[i].type == 0x124;
		CHECK(n1 <= 1 && n2 <= 1);
	}
	int person = w.actorObject(0x50);
	CHECK(person >= 0 && w.objects[person].x == e.x && w.objects[person].y == e.y && w.objects[person].z == 2);

	// a square with flags 5 (sub_52159): the things of the scene are made anew (the 0x93 at 0xC1, 0x9C), actor 0x50 stands at 0xA5, 0xA4 with frame 9, the first psychic number is 2
	{
		int sq = put(w, 0x1CD, 0, 34, 20, 2);
		w.objects[sq].status = 5;
		w.objects[sq].quantity = 21;
		w.objects[sq].quality = 21;
		w.moveObject(body, 34, 20, 2);
		g.x = 34;
		g.y = 20;
		g.mdStepOn();
		CHECK(g.mdPsychic(0, 0) == 2);
		int p = w.actorObject(0x50);
		CHECK(w.objects[p].x == 0xA5 && w.objects[p].y == 0xA4 && w.objects[p].frame == 9);
		std::vector<int> here;
		w.objectsAt(0xC1, 0x9C, 2, here);
		bool made = false;
		for (int i : here)
			made |= w.objects[i].type == 0x93;
		CHECK(made);
	}

	// a square with flags 0x44 (sub_51944): the maze is made anew (a 0xA1 at 0x24, 0x36, hedges 0xD9), actor 0x54 stands at 0x37, 0x3D
	{
		int sq = put(w, 0x1CD, 0, 35, 20, 2);
		w.objects[sq].status = 0x44;
		w.objects[sq].quantity = 22;
		w.objects[sq].quality = 22;
		w.moveObject(body, 35, 20, 2);
		g.x = 35;
		g.y = 20;
		g.mdStepOn();
		std::vector<int> here;
		w.objectsAt(0x24, 0x36, 2, here);
		bool made = false;
		for (int i : here)
			made |= w.objects[i].type == 0xA1;
		CHECK(made);
		int hedges = 0;
		for (const World::Obj &o : w.objects)
			hedges += !o.deleted && o.type == 0xD9 && o.z == 2 && o.x >= 0x37 && o.x <= 0x3B && o.y >= 0x39 && o.y <= 0x41;
		CHECK(hedges >= 20);
		int p = w.actorObject(0x54);
		CHECK(w.objects[p].x == 0x37 && w.objects[p].y == 0x3D && w.objects[p].frame == 9);
	}

	// a square with flags 0x60 (sub_52843): the stones 0x84 and the four 0x70 corners are made anew, actor 0x55 stands at 0x4E, 0x4D
	{
		int sq = put(w, 0x1CD, 0, 36, 20, 2);
		w.objects[sq].status = 0x60;
		w.objects[sq].quantity = 23;
		w.objects[sq].quality = 23;
		w.moveObject(body, 36, 20, 2);
		g.x = 36;
		g.y = 20;
		g.mdStepOn();
		int stones = 0, corners = 0;
		for (const World::Obj &o : w.objects) {
			stones += !o.deleted && o.type == 0x84 && o.z == 2 && o.x >= 0x49 && o.x <= 0x57 && o.y >= 0x4A && o.y <= 0x51;
			corners += !o.deleted && o.type == 0x70 && o.z == 2 && o.quality == 0xFF;
		}
		CHECK(stones == 1 + 12 + 8 + 4 && corners >= 4);
		int p = w.actorObject(0x55);
		CHECK(w.objects[p].x == 0x4E && w.objects[p].y == 0x4D && w.objects[p].frame == 1);
	}

	// the flags 4 (sub_52491): the little room is made at x = quality, y = quantity (nine things 0x1A1 / 0x1A2), actor 0x57 stands at the square 2 north, a stored thing (0x78, quality 1) holds a 0x7A
	{
		int store = put(w, 0x78, 0, 100, 60, 2);
		w.objects[store].quality = 1;
		int sq = put(w, 0x1CD, 0, 37, 20, 2);
		w.objects[sq].status = 4;
		w.objects[sq].quantity = 60;
		w.objects[sq].quality = 100;
		w.moveObject(body, 37, 20, 2);
		g.x = 37;
		g.y = 20;
		g.mdStepOn();
		int room = 0, made7a = 0;
		for (const World::Obj &o : w.objects) {
			room += !o.deleted && (o.type == 0x1A1 || o.type == 0x1A2) && o.z == 2 && o.x >= 100 && o.x <= 102 && o.y >= 58 && o.y <= 60;
			made7a += !o.deleted && o.type == 0x7A && o.x == 100 && o.y == 60;
		}
		CHECK(room == 9 && made7a == 1);
		int p = w.actorObject(0x57);
		CHECK(w.objects[p].x == 100 && w.objects[p].y == 58 && w.objects[p].frame == 9);
		CHECK(w.objects[store].invisible());
	}

	// the flags 0x85 (sub_52B0C): the first time the people 0x51 and 0xA-0xF stand in the room (frame 9) and a 0x14B holding a 0xEB is made at 0x34, 0x50
	{
		int sq = put(w, 0x1CD, 0, 38, 20, 2);
		w.objects[sq].status = 0x85;
		w.objects[sq].quantity = 0x66;
		w.objects[sq].quality = 0x39;
		w.moveObject(body, 38, 20, 2);
		g.x = 38;
		g.y = 20;
		g.mdStepOn();
		int p = w.actorObject(0x0C);
		CHECK(w.objects[p].x == 0x3C && w.objects[p].y == 0x5E && w.objects[p].frame == 9);
		int box = -1;
		for (size_t i = 0; i < w.objects.size(); ++i)
			if (!w.objects[i].deleted && w.objects[i].type == 0x14B && w.objects[i].x == 0x34 && w.objects[i].y == 0x50 && w.objects[i].z == 2)
				box = (int)i;
		CHECK(box >= 0);
		bool oil = false;
		for (const World::Obj &o : w.objects)
			oil |= !o.deleted && o.type == 0xEB && o.parent == box && o.frame == 4;
		CHECK(oil);
	}

	// the flags 0xA1 (sub_51C96): a 0x77 at 0x7E, 0x21 and a 0x49 at 0x75, 0x21 are made, the box (0x14B) at 0x73, 0x21 holds a 0x81, actor 0x52 stands at 0x5F, 0x10 with frame 9
	{
		int sq = put(w, 0x1CD, 0, 39, 20, 2);
		w.objects[sq].status = 0xA1;
		w.objects[sq].quantity = 21;
		w.objects[sq].quality = 21;
		w.moveObject(body, 39, 20, 2);
		g.x = 39;
		g.y = 20;
		g.mdStepOn();
		int p = w.actorObject(0x52);
		CHECK(w.objects[p].x == 0x5F && w.objects[p].y == 0x10 && w.objects[p].frame == 9);
		int n77 = 0, n49 = 0;
		for (const World::Obj &o : w.objects) {
			n77 += !o.deleted && o.type == 0x77 && o.x == 0x7E && o.y == 0x21;
			n49 += !o.deleted && o.type == 0x49 && o.x == 0x75 && o.y == 0x21;
		}
		CHECK(n77 == 1 && n49 == 1);
		int box = -1;
		for (size_t i = 0; i < w.objects.size(); ++i)
			if (!w.objects[i].deleted && w.objects[i].type == 0x14B && w.objects[i].x == 0x73 && w.objects[i].y == 0x21 && w.objects[i].z == 2)
				box = (int)i;
		CHECK(box >= 0 && g.mdContents(box).size() == 1 && w.objects[g.mdContents(box)[0]].type == 0x81);
	}

	// a square with flags 0x20 (sub_514B0): the body is given a vial (0x14B) with a 0x38, the foes (0x16C) appear round it, a thing of the list in the region goes, and combat begins
	{
		int junk = put(w, 0x38, 0, 10, 20, 2);
		int sq = put(w, 0x1CD, 0, 33, 20, 2);
		w.objects[sq].status = 0x20;
		w.objects[sq].quantity = 40;
		w.objects[sq].quality = 20;
		w.moveObject(body, 33, 20, 2);
		g.x = 33;
		g.y = 20;
		auto foes = [&]() {
			int n = 0;
			for (const World::Obj &o : w.objects)
				n += !o.deleted && o.type == 0x16C;
			return n;
		};
		int before = foes();
		g.mdStepOn();
		CHECK(g.x == 20 && g.y == 40);
		CHECK(w.objects[junk].deleted);
		CHECK(foes() >= before + 1);
		std::vector<int> inv;
		w.inventory(0, inv, true);
		bool vial = false;
		for (int i : inv)
			vial |= w.objects[i].type == 0x14B;
		CHECK(vial);
	}

	// the battle's foes die (sub_19300 counts them, the last one is the faint, sub_50836): the things of the room x 0x21-0x27, y 0x19-0x1F go to the new room, the body wakes at 0x10, 0x33
	{
		int bed = put(w, 0x151, 0, 0x22, 0x1A, 2);
		int sword = put(w, 0x38, 0, 0x23, 0x1A, 2);
		int pile = put(w, 0x14E, 0, 0x24, 0x1B, 2);
		int n = al.partySize > 1 ? al.partySize - 1 : 4;
		g.log.clear();
		for (int i = 0; i < n; ++i)
			g.mdKilled(2);
		CHECK(logText(g).find("Overcome by fatigue, you pass out.") != std::string::npos);
		CHECK(logText(g).find("All your friends have been killed!") != std::string::npos);
		CHECK(w.objects[bed].deleted);
		bool bag = false;
		for (const World::Obj &o : w.objects)
			bag |= !o.deleted && o.type == 0x155 && o.x == 0x22 - 0x13 && o.y == 0x1A + 0x18 && o.z == 2;
		CHECK(bag);
		CHECK(w.objects[sword].type == 0x10 && w.objects[sword].x == 0x23 - 0x13 && w.objects[sword].y == 0x1A + 0x18);
		CHECK(w.objects[pile].x == 0x24 - 0x13 && w.objects[pile].y == 0x1B + 0x18);
		CHECK(w.objects[body].x == 0x10 && w.objects[body].y == 0x33 && w.objects[body].z == 2 && w.objects[body].frame == 0xB);
		CHECK(g.x == 0x10 && g.y == 0x33 && g.mdDreaming());
		CHECK(g.mdTakeMorning());
	}

	// a square with flags 0x40 (sub_5189D): the Avatar falls off the cliff, the body lies there; the dream number is 0x40
	int cliff = put(w, 0x1CD, 0, 32, 20, 2);
	w.objects[cliff].status = 0x40;
	w.objects[cliff].quantity = 22;
	w.objects[cliff].quality = 33;
	w.moveObject(body, 32, 20, 2);
	g.x = 32;
	g.y = 20;
	g.log.clear();
	g.mdStepOn();
	CHECK(logText(g).find("You fall off the cliff!") != std::string::npos);
	CHECK(w.objects[body].type == 0x126 || w.objects[body].type == 0x127);
	CHECK(g.x == 33 && g.y == 22);

	// a wake square (flags 0x24): the clock reads eight in the morning, the body goes back to level 6, the view to the Avatar on the chair
	int wake = put(w, 0x1CD, 0, 31, 20, 2);
	w.objects[wake].status = 0x24;
	w.moveObject(body, 31, 20, 2);
	g.x = 31;
	g.y = 20;
	g.log.clear();
	g.mdStepOn();
	CHECK(!g.mdDreaming());
	CHECK(g.gameClock.hour == 8 && g.gameClock.minute == 0);
	CHECK(g.x == podX && g.y == podY && g.z == 0);
	CHECK(w.objects[body].z == 6);
	CHECK(w.objects[body].type == w.objects[w.actorObject(1)].type && w.objects[body].frame == 9); // the number 0x40: the looks come back
	CHECK(logText(g).find("You wake up.") != std::string::npos);
	CHECK(g.mdTakeMorning());
	CHECK(al.worktype[0] == 0 && al.worktype[1] == 2);

	// the end of a scene and the waking in one (the mirror, the manuscript ...): only while dreaming
	g.mdDreamFinish(0x51);
	CHECK(!g.mdDreaming());
	(void)control;

	// Look at a picture of a dream (0x124, quality 3): MDREAM.LZC's picture fills the right panel until a key, then the prompt comes back
	{
		w.moveObject(w.actorObject(1), px, py, 0);
		g.x = px;
		g.y = py;
		g.z = 0;
		int pic = put(w, 0x124, 3, px + 1, py, 0);
		w.objects[pic].quality = 3;
		Framebuffer before, shown;
		g.render(before);
		g.handleKey('l');
		g.handleKey(KEY_RIGHT);
		g.render(shown);
		CHECK(std::memcmp(before.pixels(), shown.pixels(), Framebuffer::WIDTH * Framebuffer::HEIGHT) != 0);
		g.handleKey(' ');
		Framebuffer after;
		g.render(after);
		CHECK(std::memcmp(after.pixels(), shown.pixels(), Framebuffer::WIDTH * Framebuffer::HEIGHT) != 0);
	}

	std::printf(failures ? "FAILED\n" : "ok\n");
	return failures ? 1 : 0;
}
