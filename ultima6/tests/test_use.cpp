// The Use command's handlers: doors, chests, food, candles (skipped when the game data is absent).

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

// Uses what lies east of a spot in the open.
static void useEast(World &w, Game &g, int px, int py) {
	w.moveObject(w.actorObject(1), px, py, 0);
	g.x = px;
	g.y = py;
	g.z = 0;
	g.handleKey('u');
	g.handleKey(KEY_RIGHT);
}

int main() {
	World w;
	if (!w.load("u6/game")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	Game g(w);
	int px = g.x, py = g.y;

	// a chest: closed (frame 1) it opens and its contents lie on the ground; open (frame 0) it closes
	{
		int chest = put(w, 98, 1, px + 1, py, 0);
		int gold = put(w, 0x58, 0, 0, 0, 0);
		w.objects[gold].status = (uint8_t)ObjRecord::IN_CONTAINER;
		w.objects[gold].parent = chest;
		w.objects[gold].quantity = 5;
		w.addToMap(gold); // no effect for a contained object
		useEast(w, g, px, py);
		CHECK(w.objects[chest].frame == 0);
		CHECK(logText(g).find("opened!") != std::string::npos);
		CHECK(logText(g).find("Searching here, you find") != std::string::npos);
		CHECK(w.objects[gold].onMap() && w.objects[gold].x == px + 1 && w.objects[gold].y == py);
		useEast(w, g, px, py);
		CHECK(w.objects[chest].frame == 1);
		CHECK(logText(g).find("closed!") != std::string::npos);
	}

	// a locked door stays locked and says so; a magically locked one says so
	{
		w.objects.reserve(w.objects.size() + 16);
		std::vector<int> leftovers; // the chest of the test above: a door with something else on it is "Not now!"
		w.objectsAt(px + 1, py, 0, leftovers);
		for (int i : leftovers)
			w.deleteObject(i);
		int door = put(w, 297, 9, px + 1, py, 0);
		useEast(w, g, px, py);
		CHECK(w.objects[door].frame == 9);
		CHECK(logText(g).find("locked\n") != std::string::npos);
		w.objects[door].frame = 13;
		useEast(w, g, px, py);
		CHECK(w.objects[door].frame == 13);
		CHECK(logText(g).find("magically locked!") != std::string::npos);
	}

	// a door with a chest in front of it in the list ("first object of the square", sub_19C1A) is "Not now!"
	{
		std::vector<int> there;
		w.objectsAt(px + 1, py, 0, there);
		for (int i : there)
			w.deleteObject(i);
		put(w, 0x62, 1, px + 1, py, 0);
		int door = put(w, 297, 1, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("Not now!") != std::string::npos);
		CHECK(w.objects[door].frame == 1);
		w.deleteObject(door);
	}

	// food is eaten, a candle is lit and doused
	{
		int food = put(w, 0x5F, 0, px + 1, py, 0);
		useEast(w, g, px, py);
		CHECK(w.objects[food].deleted);
		CHECK(logText(g).find("You eat the food.") != std::string::npos);
		int candle = put(w, 0x7A, 0, px + 1, py, 0);
		useEast(w, g, px, py);
		CHECK(w.objects[candle].frame == 1);
		CHECK(logText(g).find("candle is lit.") != std::string::npos);
		useEast(w, g, px, py);
		CHECK(w.objects[candle].frame == 0);
		CHECK(logText(g).find("candle is doused.") != std::string::npos);
	}

	// a well fills an empty bucket
	{
		put(w, 0xE9, 0, px + 1, py, 0);
		w.addToInventory(1, 0xB2, 0, 1);
		useEast(w, g, px, py);
		CHECK(logText(g).find("Done") != std::string::npos);
		std::vector<int> inv;
		w.inventory(1, inv);
		bool water = false;
		for (int i : inv)
			water |= w.objects[i].type == 0xB3 && !w.objects[i].deleted;
		CHECK(water);
	}

	// looking at a sign reads BOOK.DAT: quality 4 is "The bones of Zog."
	{
		int sign = put(w, 0x98, 0, px + 1, py, 0);
		w.objects[sign].quality = 4;
		w.moveObject(w.actorObject(1), px, py, 0);
		g.x = px;
		g.y = py;
		g.handleKey('l');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find(":\n\nThe bones of Zog.") != std::string::npos);
	}

	// Move: a key to the east is pushed one square on (or is "Blocked")
	{
		int key = put(w, 0x40, 0, px + 1, py, 0);
		w.moveObject(w.actorObject(1), px, py, 0);
		g.x = px;
		g.y = py;
		g.handleKey('m');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_RIGHT);
		bool moved = w.objects[key].x == px + 2 && w.objects[key].y == py;
		bool stayed = w.objects[key].x == px + 1 && logText(g).find("Blocked") != std::string::npos;
		CHECK(moved || stayed);
		CHECK(logText(g).find("East.") != std::string::npos);
	}

	// a blue moon gate at the start (day 4, 8:00): the moons' phases are 4 and 6, so the party goes to the gate of the second moon, gate 6 at 23,22 (level 1)
	{
		World w2;
		w2.load("u6/game");
		Game g2(w2);
		int gx = g2.x, gy = g2.y;
		put(w2, 0x55, 0, gx, gy - 1, 0);
		CHECK(g2.step(0));
		CHECK(g2.x == 23 && g2.y == 22 && g2.z == 1);
	}

	// Create Food (circle 1, "imy"): 1-10 food for the caster
	{
		World w3;
		w3.load("u6/game");
		Game g3(w3);
		int book = w3.addToInventory(1, 57, 0, 1);
		w3.objects[book].status = (uint8_t)((w3.objects[book].status & ~0x18) | 0x18);
		World::Obj spell = w3.objects[book];
		spell.type = 58;
		spell.quality = 255;
		spell.status = 0x08;
		spell.parent = book;
		spell.actor = -1;
		w3.objects.push_back(spell);
		w3.addToInventory(1, 69, 0, 5);
		w3.addToInventory(1, 67, 0, 5);
		w3.addToInventory(1, 68, 0, 5);
		g3.handleKey('c');
		g3.handleKey('1');
		g3.handleKey('i');
		g3.handleKey('m');
		g3.handleKey('y');
		g3.handleKey(KEY_ENTER);
		std::string text;
		for (const std::string &l : g3.log)
			text += l;
		CHECK(text.find("Create ") != std::string::npos && text.find(" food.") != std::string::npos);
		std::vector<int> inv;
		w3.inventory(1, inv);
		bool food = false;
		for (int i : inv)
			food |= w3.objects[i].type == 0x81 && !w3.objects[i].deleted;
		CHECK(food);
	}

	// burying a moonstone on soft ground sets the place of its gate and, the moons being up, a blue moon gate appears there
	{
		World w4;
		w4.load("u6/game");
		Game g4(w4);
		int bx = -1, by = -1;
		for (int yy = g4.y - 20; yy < g4.y + 20 && bx < 0; ++yy)
			for (int xx = g4.x - 20; xx < g4.x + 20 && bx < 0; ++xx) {
				int t = w4.map.tile(0, xx, yy);
				if (t >= 0x10 && t <= 0x6F)
					bx = xx, by = yy;
			}
		CHECK(bx >= 0);
		if (bx >= 0) {
			w4.moveObject(w4.actorObject(1), bx, by, 0);
			g4.x = bx;
			g4.y = by;
			g4.z = 0;
			int stone = put(w4, 0x49, 3, bx, by, 0);
			(void)stone;
			g4.handleKey('u');
			g4.handleKey(KEY_ENTER); // the own square
			std::string text;
			for (const std::string &l : g4.log)
				text += l;
			CHECK(text.find("buried.") != std::string::npos);
			const std::vector<uint8_t> &gl = w4.actors.globals;
			CHECK((gl[0x2A + 18] | (gl[0x2A + 19] << 8)) == bx && (gl[0x2A + 20] | (gl[0x2A + 21] << 8)) == by);
			std::vector<int> here;
			w4.objectsAt(bx, by, 0, here);
			bool gate = false;
			for (int i : here)
				gate |= w4.objects[i].type == 0x55;
			CHECK(gate);
		}
	}

	// digging at 707,835 with a shovel makes a hole
	{
		World w5;
		w5.load("u6/game");
		Game g5(w5);
		w5.moveObject(w5.actorObject(1), 706, 835, 0);
		g5.x = 706;
		g5.y = 835;
		g5.z = 0;
		put(w5, 0x68, 0, 706, 835, 0);
		g5.handleKey('u');
		g5.handleKey(KEY_ENTER);
		g5.handleKey(KEY_RIGHT);
		std::string text;
		for (const std::string &l : g5.log)
			text += l;
		CHECK(text.find("You dig a hole.") != std::string::npos);
		std::vector<int> here;
		w5.objectsAt(707, 835, 0, here);
		bool hole = false;
		for (int i : here)
			hole |= w5.objects[i].type == 0x134;
		CHECK(hole);
	}

	// a shrine: the wrong mantra is refused, the right one with the moonstone of the virtue beside the party works
	{
		World w6;
		w6.load("u6/game");
		Game g6(w6);
		int sx = g6.x, sy = g6.y;
		put(w6, 0xF2, 0, sx + 1, sy, 0);
		g6.handleKey('u');
		g6.handleKey(KEY_RIGHT);
		g6.handleKey('x');
		g6.handleKey('x');
		g6.handleKey(KEY_ENTER);
		std::string text;
		for (const std::string &l : g6.log)
			text += l;
		CHECK(text.find("Wrong mantra!") != std::string::npos);
		put(w6, 0x49, 0, sx, sy + 1, 0);
		g6.handleKey('u');
		g6.handleKey(KEY_RIGHT);
		for (char c : std::string("ahm"))
			g6.handleKey(c);
		g6.handleKey(KEY_ENTER);
		text.clear();
		for (const std::string &l : g6.log)
			text += l;
		CHECK(text.find("Done") != std::string::npos);
	}

	// a fountain: a wish that is no food has no effect
	{
		World w7;
		w7.load("u6/game");
		Game g7(w7);
		put(w7, 0xEA, 0, g7.x + 1, g7.y, 0);
		g7.handleKey('u');
		g7.handleKey(KEY_RIGHT);
		g7.handleKey('y');
		for (char c : std::string("gold"))
			g7.handleKey(c);
		g7.handleKey(KEY_ENTER);
		std::string text;
		for (const std::string &l : g7.log)
			text += l;
		CHECK(text.find("Make a wish?") != std::string::npos && text.find("No effect") != std::string::npos);
	}

	// a telescope shows a far view until a key is pressed
	{
		World w8;
		w8.load("u6/game");
		Game g8(w8);
		put(w8, 0x9A, 0, g8.x + 1, g8.y, 0);
		g8.handleKey('u');
		g8.handleKey(KEY_RIGHT);
		g8.handleKey(KEY_RIGHT);
		Framebuffer fb;
		g8.render(fb);
		g8.handleKey(KEY_ENTER);
		std::string text;
		for (const std::string &l : g8.log)
			text += l;
		CHECK(text.find("East.") != std::string::npos && text.find("Done") != std::string::npos);
	}

	// a crystal ball shows the place at a latitude and longitude
	{
		World w9;
		w9.load("u6/game");
		Game g9(w9);
		put(w9, 0x9B, 0, g9.x + 1, g9.y, 0);
		g9.handleKey('u');
		g9.handleKey(KEY_RIGHT);
		for (char c : std::string("5s"))
			g9.handleKey(c);
		g9.handleKey(KEY_ENTER);
		for (char c : std::string("5e"))
			g9.handleKey(c);
		g9.handleKey(KEY_ENTER);
		Framebuffer fb;
		g9.render(fb);
		g9.handleKey(KEY_ENTER);
		std::string text;
		for (const std::string &l : g9.log)
			text += l;
		CHECK(text.find("At latitude=") != std::string::npos && text.find("longitude=") != std::string::npos && text.find("Done") != std::string::npos);
	}

	// a lever closes and opens the portcullis of the doorway with its quality
	{
		World wl;
		wl.load("u6/game");
		Game gl(wl);
		int door = put(wl, 0x12D, 1, gl.x + 5, gl.y + 5, 0);
		wl.objects[door].quality = 9;
		int lever = put(wl, 0x10C, 0, gl.x + 1, gl.y, 0);
		wl.objects[lever].quality = 9;
		gl.handleKey('u');
		gl.handleKey(KEY_RIGHT);
		int portcullis = -1;
		for (size_t i = 0; i < wl.objects.size(); ++i)
			if (!wl.objects[i].deleted && wl.objects[i].type == 0x136)
				portcullis = (int)i;
		CHECK(portcullis >= 0 && wl.objects[portcullis].frame == 1);
		CHECK(wl.objects[lever].frame == 1);
		gl.handleKey('u');
		gl.handleKey(KEY_RIGHT);
		CHECK(wl.objects[portcullis].deleted);
	}

	// a dead actor is resurrected from its body
	{
		World wa;
		wa.load("u6/game");
		Game ga(wa);
		wa.actors.hp[2] = 0;
		wa.actors.status[2] |= 0x10;
		int body = put(wa, 339, 0, ga.x + 3, ga.y, 0);
		wa.objects[body].quality = 2;
		CHECK(wa.resurrect(2));
		CHECK(!(wa.actors.status[2] & 0x10) && wa.actors.hp[2] > 0);
		CHECK(wa.objects[body].deleted);
		CHECK(!wa.resurrect(2));
	}

	// the silver horn makes serpents (0x19D) and then refuses
	{
		World wh;
		wh.load("u6/game");
		Game gh(wh);
		put(wh, 0x139, 0, gh.x + 1, gh.y, 0);
		gh.handleKey('u');
		gh.handleKey(KEY_RIGHT);
		int snakes = 0;
		for (int a = 0xE0; a < 256; ++a) {
			int k = wh.actorObject(a);
			snakes += k >= 0 && !wh.objects[k].deleted && wh.objects[k].type == 0x19D;
		}
		std::string text;
		for (const std::string &l : gh.log)
			text += l;
		CHECK(snakes > 0 ? text.find("generated") != std::string::npos : text.find("None generated") != std::string::npos);
		gh.handleKey('u');
		gh.handleKey(KEY_RIGHT);
		text.clear();
		for (const std::string &l : gh.log)
			text += l;
		if (snakes > 0)
			CHECK(text.find("Not now!") != std::string::npos);
	}

	// a gem shows the overhead map until a key is pressed and is used up
	{
		World wg;
		wg.load("u6/game");
		Game gg(wg);
		int gem = put(wg, 0x4D, 0, gg.x + 1, gg.y, 0);
		gg.handleKey('u');
		gg.handleKey(KEY_RIGHT);
		Framebuffer fb;
		gg.render(fb);
		bool drawn = false;
		for (int i = 0; i < 160; ++i)
			drawn |= fb.pixels()[(8 + 80) * Framebuffer::WIDTH + 8 + i] != 0;
		CHECK(drawn);
		CHECK(wg.objects[gem].deleted);
		gg.handleKey(KEY_ENTER);
		gg.render(fb);
	}

	// the Combat command: members take their combat worktypes and then follow again
	{
		World wc;
		wc.load("u6/game");
		Game gc(wc);
		int second = wc.actors.partyRoster[1];
		gc.handleKey('b');
		CHECK(wc.actors.worktype[second] == wc.actors.unknown12f1[second]);
		gc.handleKey('b');
		CHECK(wc.actors.worktype[second] == 1);
	}

	// a balloon is set up next to the user, and can be boarded
	{
		World wb;
		wb.load("u6/game");
		Game gb(wb);
		int item = put(wb, 0x1A4, 0, gb.x + 1, gb.y, 0);
		gb.handleKey('u');
		gb.handleKey(KEY_RIGHT);
		std::string text;
		for (const std::string &l : gb.log)
			text += l;
		CHECK(wb.objects[item].deleted);
		CHECK(text.find("Done!") != std::string::npos || text.find("No room!") != std::string::npos);
	}

	// a drawbridge crank raises a flat bridge of two rows into one row of 6, 7, 7, 8 and says so
	{
		World wd;
		wd.load("u6/game");
		Game gd(wd);
		int bx = gd.x + 3, by = gd.y + 3;
		for (int row = -1; row < 3; ++row)
			for (int col = -1; col < 5; ++col) {
				std::vector<int> there;
				wd.objectsAt(bx + col, by + row, 0, there);
				for (int t : there)
					if (wd.objects[t].actor < 0)
						wd.deleteObject(t);
			}
		for (int row = 0; row < 2; ++row) {
			int f[4] = {3, 4, 4, 5};
			for (int col = 0; col < 4; ++col) {
				int p = put(wd, 0x10D, f[col], bx + col, by + row, 0);
				if (col == 0)
					wd.objects[p].quality = 211;
			}
		}
		int crank = put(wd, 0x120, 0, gd.x + 1, gd.y, 0);
		wd.objects[crank].quality = 211;
		int before = 0;
		for (const World::Obj &o : wd.objects)
			before += !o.deleted && o.type == 0x10D;
		gd.handleKey('u');
		gd.handleKey(KEY_RIGHT);
		int pieces = 0, f6 = 0, f7 = 0, f8 = 0;
		for (const World::Obj &o : wd.objects)
			if (!o.deleted && o.type == 0x10D) {
				++pieces;
				f6 += o.frame == 6;
				f7 += o.frame == 7;
				f8 += o.frame == 8;
			}
		std::string text;
		for (const std::string &l : gd.log)
			text += l;
		CHECK(pieces == before - 8 + 5 && f6 >= 1 && f7 >= 3 && f8 >= 1);
		CHECK(text.find("Close the drawbridge.") != std::string::npos);
	}

	// the area spells of sub_23AD5 answer "Success" (guards the spell cases of spellSpecial, which once were lost in an edit)
	{
		static const char *const spells[] = {"asl", "vq", "qc", "axc"};
		static const char circle[] = {'5', '6', '7', '3'};
		for (int n = 0; n < 4; ++n) {
			World wc;
			wc.load("u6/game");
			Game gc(wc);
			int book = wc.addToInventory(1, 57, 0, 1);
			wc.objects[book].status = (uint8_t)((wc.objects[book].status & ~0x18) | 0x18);
			World::Obj spell = wc.objects[book];
			spell.type = 58;
			spell.quality = 255;
			spell.status = 0x08;
			spell.parent = book;
			spell.actor = -1;
			wc.objects.push_back(spell);
			for (int r = 65; r <= 72; ++r)
				wc.addToInventory(1, r, 0, 5);
			wc.actors.level[1] = 8;
			wc.actors.unknown13f1[1] = 99;
			int tag = put(wc, 0x15F, 0, gc.x + 1, gc.y, gc.z);
			wc.objects[tag].status |= 0x02;
			gc.handleKey('c');
			gc.handleKey(circle[n]);
			for (const char *k = spells[n]; *k; ++k)
				gc.handleKey(*k);
			gc.handleKey(KEY_ENTER);
			if (logText(gc).find("Success") == std::string::npos)
				std::printf("LOG[%s]\n", logText(gc).c_str());
			CHECK(logText(gc).find("Success") != std::string::npos);
			if (n == 0)
				CHECK(!(wc.objects[tag].status & 0x02));
		}
	}


	// Energy Wind (sub_24E78): the bolt along the line from the caster to the target square hurts whoever stands on it
	{
		World ww;
		ww.load("u6/game");
		Game gw(ww);
		int bx = -1, by = -1;
		for (int yy = gw.y - 20; yy < gw.y + 20 && bx < 0; ++yy)
			for (int xx = gw.x - 20; xx < gw.x + 20 && bx < 0; ++xx) {
				bool open = true;
				for (int d = 0; d < 5; ++d) {
					int t = ww.map.tile(0, xx + d, yy);
					open &= t >= 0x10 && t <= 0x6F;
					std::vector<int> there;
					ww.objectsAt(xx + d, yy, 0, there);
					open &= there.empty();
				}
				if (open)
					bx = xx, by = yy;
			}
		CHECK(bx >= 0);
		if (bx >= 0) {
			ww.moveObject(ww.actorObject(1), bx, by, 0);
			gw.x = bx;
			gw.y = by;
			gw.z = 0;
			int victim = ww.addActor(100, 427, bx + 3, by, 0);
			(void)victim;
			ww.actors.hp[100] = 200;
			int book = ww.addToInventory(1, 57, 0, 1);
			ww.objects[book].status = (uint8_t)((ww.objects[book].status & ~0x18) | 0x18);
			World::Obj spell = ww.objects[book];
			spell.type = 58;
			spell.quality = 255;
			spell.status = 0x08;
			spell.parent = book;
			spell.actor = -1;
			ww.objects.push_back(spell);
			for (int r = 65; r <= 72; ++r)
				ww.addToInventory(1, r, 0, 5);
			ww.actors.level[1] = 8;
			ww.actors.unknown13f1[1] = 99;
			gw.handleKey('c');
			gw.handleKey('7');
			gw.handleKey('g');
			gw.handleKey('h');
			gw.handleKey(KEY_ENTER);
			for (int i = 0; i < 3; ++i)
				gw.handleKey(KEY_RIGHT);
			gw.handleKey(KEY_ENTER);
			CHECK(ww.actors.hp[100] < 200);
		}
	}


	// Wizard Eye (sub_24A61): an eye is steered with the direction keys until Escape
	{
		World ww;
		ww.load("u6/game");
		Game gw(ww);
		int book = ww.addToInventory(1, 57, 0, 1);
		ww.objects[book].status = (uint8_t)((ww.objects[book].status & ~0x18) | 0x18);
		World::Obj spell = ww.objects[book];
		spell.type = 58;
		spell.quality = 255;
		spell.status = 0x08;
		spell.parent = book;
		spell.actor = -1;
		ww.objects.push_back(spell);
		for (int r = 65; r <= 72; ++r)
			ww.addToInventory(1, r, 0, 5);
		ww.actors.level[1] = 8;
		ww.actors.unknown13f1[1] = 99;
		auto eyes = [&]() {
			int n = 0, ex = -1;
			for (const World::Obj &o : ww.objects)
				if (!o.deleted && o.type == 0x34 && o.onMap()) {
					++n;
					ex = o.x;
				}
			return std::make_pair(n, ex);
		};
		gw.handleKey('c');
		gw.handleKey('7');
		gw.handleKey('p');
		gw.handleKey('o');
		gw.handleKey('w');
		gw.handleKey(KEY_ENTER);
		CHECK(eyes().first == 1 && eyes().second == gw.x);
		for (int i = 0; i < 3; ++i)
			gw.handleKey(KEY_RIGHT);
		CHECK(eyes().first == 1 && eyes().second == gw.x + 3);
		gw.handleKey(KEY_ESC);
		CHECK(eyes().first == 0);
		CHECK(logText(gw).find("Done") != std::string::npos);
	}

	// A lit powder keg goes off after three minutes and blows up the doors round it (sub_333BB, blow_up_door)
	{
		World ww;
		ww.load("u6/game");
		Game gw(ww);
		int keg = put(ww, 0xDF, 0, gw.x + 1, gw.y, 0);
		int door = put(ww, 0x12C, 0, gw.x + 3, gw.y, 0);
		int locked = put(ww, 0x129, 0x0C, gw.x + 3, gw.y + 1, 0); // frame bits 2 and 3 set: it stays
		gw.handleKey('u');
		gw.handleKey(KEY_RIGHT);
		CHECK(ww.objects[keg].frame == 1);
		gw.advanceTime(4);
		CHECK(ww.objects[keg].deleted);
		CHECK(ww.objects[door].deleted && !ww.objects[locked].deleted);
		CHECK(logText(gw).find("The door is blown up!") != std::string::npos);
	}

	// Pickpocket (sub_24235): the victim's things are shown and the one chosen goes to the Avatar
	{
		World ww;
		ww.load("u6/game");
		Game gw(ww);
		int book = ww.addToInventory(1, 57, 0, 1);
		ww.objects[book].status = (uint8_t)((ww.objects[book].status & ~0x18) | 0x18);
		World::Obj spell = ww.objects[book];
		spell.type = 58;
		spell.quality = 255;
		spell.status = 0x08;
		spell.parent = book;
		spell.actor = -1;
		ww.objects.push_back(spell);
		for (int r = 65; r <= 72; ++r)
			ww.addToInventory(1, r, 0, 5);
		ww.actors.level[1] = 8;
		ww.actors.unknown13f1[1] = 99;
		int victim = 5;
		ww.moveObject(ww.actorObject(victim), gw.x + 2, gw.y, 0);
		int gem = ww.addToInventory(victim, 0x4D, 0, 1);
		(void)gem;
		std::vector<int> mine, theirs;
		ww.inventory(1, mine, false);
		ww.inventory(victim, theirs, false);
		int karma = ww.actors.globals[8];
		gw.handleKey('c');
		gw.handleKey('5');
		gw.handleKey('p');
		gw.handleKey('y');
		gw.handleKey(KEY_ENTER);
		gw.handleKey(KEY_RIGHT);
		gw.handleKey(KEY_RIGHT);
		gw.handleKey(KEY_ENTER); // the cursor is on the victim
		CHECK(logText(gw).find("Which object: ") != std::string::npos);
		gw.handleKey(KEY_ENTER); // the first thing the victim carries
		std::vector<int> mine2, theirs2;
		ww.inventory(1, mine2, false);
		ww.inventory(victim, theirs2, false);
		CHECK(mine2.size() == mine.size() + 1 && theirs2.size() + 1 == theirs.size());
		CHECK(ww.actors.globals[8] == (karma > 5 ? karma - 5 : 0));
	}

	// Help (sub_1890C): the party is healed, raised and taken to Lord British's court while it is held
	{
		World ww;
		ww.load("u6/game");
		Game gw(ww);
		int book = ww.addToInventory(1, 57, 0, 1);
		ww.objects[book].status = (uint8_t)((ww.objects[book].status & ~0x18) | 0x18);
		World::Obj spell = ww.objects[book];
		spell.type = 58;
		spell.quality = 255;
		spell.status = 0x08;
		spell.parent = book;
		spell.actor = -1;
		ww.objects.push_back(spell);
		ww.actors.level[1] = 8;
		ww.actors.unknown13f1[1] = 99;
		int buddy = ww.actors.partyRoster[1];
		ww.actors.hp[buddy] = 3;
		ww.actors.status[buddy] |= 0x08;
		gw.advanceTime(60 * 12); // midday has passed
		gw.handleKey('c');
		gw.handleKey('1');
		gw.handleKey('k');
		gw.handleKey('l');
		gw.handleKey(KEY_ENTER);
		CHECK(ww.actors.hp[buddy] > 3 && !(ww.actors.status[buddy] & 0x08));
		CHECK(gw.x == 307 && gw.y == 352 && gw.z == 0);
		bool held = (gw.gameClock.hour >= 8 && gw.gameClock.hour <= 11) || (gw.gameClock.hour >= 14 && gw.gameClock.hour <= 17);
		CHECK(held);
	}

	// Enchant (sub_24810): a staff on the ground takes the spell typed at "With: " as a charge (0x150) whose quality is the spell
	{
		World ww;
		ww.load("u6/game");
		Game gw(ww);
		int book = ww.addToInventory(1, 57, 0, 1);
		ww.objects[book].status = (uint8_t)((ww.objects[book].status & ~0x18) | 0x18);
		World::Obj spell = ww.objects[book];
		spell.type = 58;
		spell.quality = 255;
		spell.status = 0x08;
		spell.parent = book;
		spell.actor = -1;
		ww.objects.push_back(spell);
		for (int r = 65; r <= 72; ++r)
			ww.addToInventory(1, r, 0, 5);
		ww.actors.level[1] = 8;
		ww.actors.unknown13f1[1] = 99;
		int staff = put(ww, 0x4E, 0, gw.x + 1, gw.y, 0);
		gw.handleKey('c');
		gw.handleKey('7');
		gw.handleKey('i');
		gw.handleKey('o');
		gw.handleKey('y');
		gw.handleKey(KEY_ENTER);
		gw.handleKey(KEY_RIGHT);
		gw.handleKey(KEY_ENTER);
		for (char c : std::string("1imy"))
			gw.handleKey(c);
		gw.handleKey(KEY_ENTER);
		int charges = 0, spellIndex = -1;
		for (const World::Obj &o : ww.objects)
			if (!o.deleted && o.type == 0x150 && o.parent == staff) {
				++charges;
				spellIndex = o.quality;
			}
		CHECK(charges == 1 && spellIndex == 0); // Create Food
		CHECK(logText(gw).find("With: ") != std::string::npos);
	}

	// Mass Awaken / Mass Protect (sub_2362A): the bolt flies to the target square and whoever stands there is affected; a wall in the way gives "Blocked"
	{
		World ww;
		ww.load("u6/game");
		Game gw(ww);
		int bx = -1, by = -1;
		for (int yy = gw.y - 20; yy < gw.y + 20 && bx < 0; ++yy)
			for (int xx = gw.x - 20; xx < gw.x + 20 && bx < 0; ++xx) {
				bool open = true;
				for (int d = 0; d < 5; ++d) {
					int t = ww.map.tile(0, xx + d, yy);
					open &= t >= 0x10 && t <= 0x6F;
					std::vector<int> there;
					ww.objectsAt(xx + d, yy, 0, there);
					open &= there.empty();
				}
				if (open)
					bx = xx, by = yy;
			}
		CHECK(bx >= 0);
		if (bx >= 0) {
			ww.moveObject(ww.actorObject(1), bx, by, 0);
			gw.x = bx;
			gw.y = by;
			gw.z = 0;
			ww.addActor(100, 427, bx + 3, by, 0);
			int book = ww.addToInventory(1, 57, 0, 1);
			ww.objects[book].status = (uint8_t)((ww.objects[book].status & ~0x18) | 0x18);
			World::Obj spell = ww.objects[book];
			spell.type = 58;
			spell.quality = 255;
			spell.status = 0x08;
			spell.parent = book;
			spell.actor = -1;
			ww.objects.push_back(spell);
			for (int r = 65; r <= 72; ++r)
				ww.addToInventory(1, r, 0, 5);
			ww.actors.level[1] = 8;
			ww.actors.unknown13f1[1] = 99;
			gw.handleKey('c');
			gw.handleKey('6');
			gw.handleKey('v');
			gw.handleKey('i');
			gw.handleKey('s');
			gw.handleKey(KEY_ENTER);
			for (int i = 0; i < 3; ++i)
				gw.handleKey(KEY_RIGHT);
			gw.handleKey(KEY_ENTER);
			CHECK(ww.actors.status[100] & 0x01);
			CHECK(logText(gw).find("Success") != std::string::npos);
		}
	}


	// Fire Field (sub_2386F): the field object stands on the target square
	{
		World ww;
		ww.load("u6/game");
		Game gw(ww);
		int bx = -1, by = -1;
		for (int yy = gw.y - 20; yy < gw.y + 20 && bx < 0; ++yy)
			for (int xx = gw.x - 20; xx < gw.x + 20 && bx < 0; ++xx) {
				bool open = true;
				for (int d = 0; d < 5; ++d) {
					int t = ww.map.tile(0, xx + d, yy);
					open &= t >= 0x10 && t <= 0x6F;
					std::vector<int> there;
					ww.objectsAt(xx + d, yy, 0, there);
					open &= there.empty();
				}
				if (open)
					bx = xx, by = yy;
			}
		CHECK(bx >= 0);
		if (bx >= 0) {
			ww.moveObject(ww.actorObject(1), bx, by, 0);
			gw.x = bx;
			gw.y = by;
			gw.z = 0;
			int book = ww.addToInventory(1, 57, 0, 1);
			ww.objects[book].status = (uint8_t)((ww.objects[book].status & ~0x18) | 0x18);
			World::Obj spell = ww.objects[book];
			spell.type = 58;
			spell.quality = 255;
			spell.status = 0x08;
			spell.parent = book;
			spell.actor = -1;
			ww.objects.push_back(spell);
			for (int r = 65; r <= 72; ++r)
				ww.addToInventory(1, r, 0, 5);
			ww.actors.level[1] = 8;
			ww.actors.unknown13f1[1] = 99;
			gw.handleKey('c');
			gw.handleKey('4');
			gw.handleKey('i');
			gw.handleKey('f');
			gw.handleKey('g');
			gw.handleKey(KEY_ENTER);
			for (int i = 0; i < 3; ++i)
				gw.handleKey(KEY_RIGHT);
			gw.handleKey(KEY_ENTER);
			bool field = false;
			std::vector<int> there;
			ww.objectsAt(bx + 3, by, 0, there);
			for (int i : there)
				field |= ww.objects[i].type == 0x13D;
			CHECK(field);
			CHECK(logText(gw).find("Success") != std::string::npos);
		}
	}


	// Web (sub_246AC): webs fill the nine squares around the target
	{
		World ww;
		ww.load("u6/game");
		Game gw(ww);
		int bx = -1, by = -1;
		for (int yy = gw.y - 20; yy < gw.y + 20 && bx < 0; ++yy)
			for (int xx = gw.x - 20; xx < gw.x + 20 && bx < 0; ++xx) {
				bool open = true;
				for (int d = 0; d < 5; ++d) {
					int t = ww.map.tile(0, xx + d, yy);
					open &= t >= 0x10 && t <= 0x6F;
					std::vector<int> there;
					ww.objectsAt(xx + d, yy, 0, there);
					open &= there.empty();
				}
				if (open)
					bx = xx, by = yy;
			}
		CHECK(bx >= 0);
		if (bx >= 0) {
			ww.moveObject(ww.actorObject(1), bx, by, 0);
			gw.x = bx;
			gw.y = by;
			gw.z = 0;
			int book = ww.addToInventory(1, 57, 0, 1);
			ww.objects[book].status = (uint8_t)((ww.objects[book].status & ~0x18) | 0x18);
			World::Obj spell = ww.objects[book];
			spell.type = 58;
			spell.quality = 255;
			spell.status = 0x08;
			spell.parent = book;
			spell.actor = -1;
			ww.objects.push_back(spell);
			for (int r = 65; r <= 72; ++r)
				ww.addToInventory(1, r, 0, 5);
			ww.actors.level[1] = 8;
			ww.actors.unknown13f1[1] = 99;
			gw.handleKey('c');
			gw.handleKey('6');
			gw.handleKey('i');
			gw.handleKey('d');
			gw.handleKey('p');
			gw.handleKey(KEY_ENTER);
			for (int i = 0; i < 3; ++i)
				gw.handleKey(KEY_RIGHT);
			gw.handleKey(KEY_ENTER);
			bool field = false;
			std::vector<int> there;
			ww.objectsAt(bx + 3, by, 0, there);
			for (int i : there)
				field |= ww.objects[i].type == 0x35;
			CHECK(field);
			CHECK(logText(gw).find("Success") != std::string::npos);
		}
	}


	// Trap and Untrap (sub_22EB4): a chest gets a trap object of quality 0x16 inside and loses it again
	{
		World ww;
		ww.load("u6/game");
		Game gw(ww);
		int bx = -1, by = -1;
		for (int yy = gw.y - 20; yy < gw.y + 20 && bx < 0; ++yy)
			for (int xx = gw.x - 20; xx < gw.x + 20 && bx < 0; ++xx) {
				bool open = true;
				for (int d = 0; d < 5; ++d) {
					int t = ww.map.tile(0, xx + d, yy);
					open &= t >= 0x10 && t <= 0x6F;
					std::vector<int> there;
					ww.objectsAt(xx + d, yy, 0, there);
					open &= there.empty();
				}
				if (open)
					bx = xx, by = yy;
			}
		CHECK(bx >= 0);
		if (bx >= 0) {
			ww.moveObject(ww.actorObject(1), bx, by, 0);
			gw.x = bx;
			gw.y = by;
			gw.z = 0;
			int book = ww.addToInventory(1, 57, 0, 1);
			ww.objects[book].status = (uint8_t)((ww.objects[book].status & ~0x18) | 0x18);
			World::Obj spell = ww.objects[book];
			spell.type = 58;
			spell.quality = 255;
			spell.status = 0x08;
			spell.parent = book;
			spell.actor = -1;
			ww.objects.push_back(spell);
			for (int r = 65; r <= 72; ++r)
				ww.addToInventory(1, r, 0, 5);
			ww.actors.level[1] = 8;
			ww.actors.unknown13f1[1] = 99;
			int chest = put(ww, 0x62, 0, bx + 2, by, 0);
			auto traps = [&]() {
				int n = 0;
				for (const World::Obj &c : ww.objects)
					n += !c.deleted && c.type == 0x151 && c.parent == chest && c.quality == 0x16;
				return n;
			};
			for (int round = 0; round < 2; ++round) {
				gw.handleKey('c');
				gw.handleKey('2');
				gw.handleKey(round ? 'a' : 'i');
				gw.handleKey('j');
				gw.handleKey(KEY_ENTER);
				for (int i = 0; i < 2; ++i)
					gw.handleKey(KEY_RIGHT);
				gw.handleKey(KEY_ENTER);
				CHECK(traps() == (round ? 0 : 1));
			}
		}
	}


	// walking east the Avatar takes the frames of facing east (4 to 7) in the cycle 1, 2, 1, 0 (sub_263D4)
	{
		World wf;
		wf.load("u6/game");
		Game gf(wf);
		int bx = -1, by = -1;
		for (int yy = gf.y - 20; yy < gf.y + 20 && bx < 0; ++yy)
			for (int xx = gf.x - 20; xx < gf.x + 20 && bx < 0; ++xx) {
				bool open = true;
				for (int d = 0; d < 8; ++d) {
					int t = wf.map.tile(0, xx + d, yy);
					open &= t >= 0x10 && t <= 0x6F;
					std::vector<int> there;
					wf.objectsAt(xx + d, yy, 0, there);
					open &= there.empty();
				}
				if (open)
					bx = xx, by = yy;
			}
		CHECK(bx >= 0);
		if (bx >= 0) {
			wf.moveObject(wf.actorObject(1), bx, by, 0);
			gf.x = bx;
			gf.y = by;
			gf.z = 0;
			int seen = 0;
			for (int i = 0; i < 6; ++i) {
				gf.handleKey(KEY_RIGHT);
				int frame = wf.objects[wf.actorObject(1)].frame;
				CHECK(frame / 4 == 1);
				seen |= 1 << (frame % 4);
			}
			CHECK((seen & 7) == 7);
		}
	}


	// an instrument: the digit keys play notes (effect 2 for a harp, 0x9D, with the note's number) until another key is pressed
	{
		World wi;
		wi.load("u6/game");
		Game gi(wi);
		std::vector<std::pair<int, int>> heard;
		gi.sound = [&](int e, int p) { heard.push_back({e, p}); };
		int px2 = gi.x, py2 = gi.y;
		put(wi, 0x9D, 0, px2 + 1, py2, 0);
		useEast(wi, gi, px2, py2);
		gi.handleKey('3');
		gi.handleKey('0');
		CHECK(heard.size() == 2 && heard[0].first == 2 && heard[0].second == 0xF56 && heard[1].second == 0x1EAB);
		gi.handleKey('x'); // ends it
		gi.handleKey('3');
		CHECK(heard.size() == 2);
	}

	std::printf("%s\n", failures ? "FAILED" : "ok");
	return failures ? 1 : 0;
}
