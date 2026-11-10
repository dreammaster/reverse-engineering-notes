// Martian Dreams' Use (the recipes of seg059 / seg060): an object without a recipe "can't be used", a door of recipe 44 opens and shuts, a locked one says so (skipped without data).

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

static void useEast(World &w, Game &g, int px, int py) {
	w.moveObject(w.actorObject(1), px, py, 0);
	g.x = px;
	g.y = py;
	g.z = 0;
	g.handleKey('u');
	g.handleKey(KEY_RIGHT);
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
	int px = g.x, py = g.y;

	for (int i = 0; i < w.actors.partySize; ++i)
		w.moveObject(w.actorObject(w.actors.partyRoster[i]), px, py, 0);
	std::vector<int> there;
	w.objectsAt(px + 1, py, 0, there);
	for (int i : there)
		if (w.objects[i].actor < 0)
			w.deleteObject(i);
	// a door (type 0xDE takes recipe 44): frame bit 2 turns over
	int door = put(w, 0xDE, 0, px + 1, py, 0);
	useEast(w, g, px, py);
	CHECK(w.objects[door].frame == 2);
	useEast(w, g, px, py);
	CHECK(w.objects[door].frame == 0);
	// locked: quality bit 0x80
	w.objects[door].quality = 0x80;
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(w.objects[door].frame == 0);
	CHECK(logText(g).find("locked") != std::string::npos);
	w.deleteObject(door);

	// an unlit oil lamp (0x73) with a match: no fuel; with a can of lamp oil (0x7C) it takes fuel and lights (type 0x74), the match is used up
	{
		w.deleteObject(door);
		std::vector<int> here;
		w.objectsAt(px + 1, py, 0, here);
		for (int i : here)
			if (w.objects[i].actor < 0)
				w.deleteObject(i);
		int lamp = put(w, 0x73, 0, px + 1, py, 0);
		w.addToInventory(w.actors.partyRoster[0], 0x6B, 0, 1);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("There is no fuel") != std::string::npos);
		CHECK(w.objects[lamp].type == 0x73);
		w.addToInventory(w.actors.partyRoster[0], 0x7C, 0, 1);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(w.objects[lamp].type == 0x74 && w.objects[lamp].quality == 0xFF);
		w.deleteObject(lamp);
	}

	// a sliding gate (0xB5, recipe 70): frame 0 goes to 3; locked it will not
	{
		std::vector<int> here;
		w.objectsAt(px + 1, py, 0, here);
		for (int i : here)
			if (w.objects[i].actor < 0)
				w.deleteObject(i);
		int gate = put(w, 0xB5, 0, px + 1, py, 0);
		useEast(w, g, px, py);
		CHECK(w.objects[gate].frame == 3);
		w.objects[gate].frame = 0;
		w.objects[gate].quality = 0x80;
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(w.objects[gate].frame == 0 && logText(g).find("locked") != std::string::npos);
		w.deleteObject(gate);
	}

	// an instrument (0x11B, recipe 57) is played ("Wheeeeze"); a thing that has only a message (0x143, recipe 64) says it
	{
		for (int type : {0x11B, 0x143}) {
			std::vector<int> here;
			w.objectsAt(px + 1, py, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
			int thing = put(w, type, 0, px + 1, py, 0);
			g.log.clear();
			useEast(w, g, px, py);
			if (type == 0x11B)
				CHECK(logText(g).find("Wheeeeze") != std::string::npos);
			else
				CHECK(logText(g).find("You don't know how it works") != std::string::npos);
			w.deleteObject(thing);
		}
	}

	// digging (recipe 11): a shovel (0x42) used on a square of soil (the terrain classes of sub_3231F) makes a hole (0x101) there and a dirt pile (0x102) on a free square round the member; a square of the ground that
	// is not soil has no effect. Found by looking over the surface round the start for a place with soil to the south and room round it.
	{
		auto digClass = [&](int tile) { return ((tile >= 0x20 && tile <= 0x24) || tile == 0x0C || tile == 0x0D || (tile >= 0x28 && tile <= 0x3F)) ? 0x102 : (tile == 0x7C || tile == 0x7D || tile == 0x7F) ? 0x100 : 0; };
		auto clear = [&](int x, int y) {
			std::vector<int> here;
			w.objectsAt(x, y, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0 && !w.objects[i].deleted)
					return false;
			return true;
		};
		int fx = -1, fy = -1;
		for (int dy = -30; dy <= 30 && fx < 0; ++dy)
			for (int dx = -30; dx <= 30 && fx < 0; ++dx) {
				int cx = px + dx, cy = py + dy;
				int below = w.map.tile(0, cx, cy + 1);
				if (digClass(below) != 0x102 || !clear(cx, cy + 1) || !clear(cx + 1, cy) || !clear(cx, cy - 1) || !clear(cx - 1, cy))
					continue;
				if (w.flags.flags1[w.map.tile(0, cx, cy - 1)] & 2)
					continue;
				fx = cx;
				fy = cy;
			}
		if (fx < 0) {
			std::printf("no soil found to dig in (skipped)\n");
		} else {
			int shovel = put(w, 0x42, 0, fx + 1, fy, 0);
			w.moveObject(w.actorObject(1), fx, fy, 0);
			for (int i = 1; i < w.actors.partySize; ++i)
				w.moveObject(w.actorObject(w.actors.partyRoster[i]), fx, fy, 0);
			g.x = fx;
			g.y = fy;
			g.z = 0;
			size_t before = w.objects.size();
			g.log.clear();
			g.handleKey('u');
			g.handleKey(KEY_RIGHT);
			g.handleKey(KEY_DOWN);
			g.handleKey(KEY_ENTER);
			bool hole = false, pile = false;
			for (size_t i = before; i < w.objects.size(); ++i) {
				hole |= w.objects[i].type == 0x101 && w.objects[i].x == fx && w.objects[i].y == fy + 1;
				pile |= w.objects[i].type == 0x102;
			}
			CHECK(hole && pile);
			w.deleteObject(shovel);
			for (size_t i = before; i < w.objects.size(); ++i)
				if (w.objects[i].type == 0x101 || w.objects[i].type == 0x102)
					w.deleteObject((int)i);
			for (int i = 0; i < w.actors.partySize; ++i)
				w.moveObject(w.actorObject(w.actors.partyRoster[i]), px, py, 0);
			g.x = px;
			g.y = py;
		}
	}

	// a shovel (0x42): a pile of rocks (0x1BA) to the south goes into the wheelbarrow (0x10C) to the north (recipe 13), then a second one is refused: "There is no more room."
	{
		std::vector<int> here;
		for (auto sq : {std::make_pair(px + 1, py), std::make_pair(px, py + 1), std::make_pair(px, py - 1)}) {
			here.clear();
			w.objectsAt(sq.first, sq.second, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
		}
		int shovel = put(w, 0x42, 0, px + 1, py, 0);
		int rocks = put(w, 0x1BA, 0, px, py + 1, 0);
		int barrow = put(w, 0x10C, 0, px, py - 1, 0);
		w.objects[barrow].quantity = 0; // (empty)
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		g.handleKey(KEY_UP);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("You put the") != std::string::npos && logText(g).find("into the wheelbarrow") != std::string::npos);
		CHECK(w.objects[rocks].deleted && w.objects[barrow].quantity == 1 && w.objects[barrow].quality == 2);
		int rocks2 = put(w, 0x1BA, 0, px, py + 1, 0);
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		g.handleKey(KEY_UP);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("There is no more room") != std::string::npos && !w.objects[rocks2].deleted);
		w.deleteObject(rocks2);
		w.deleteObject(barrow);
		w.deleteObject(shovel);
	}

	// a book (0x94, recipe 1): its quality names the item of MDTEXT.LZC (quality 2: the power generation system); a quality that has no text can't be read
	{
		std::vector<int> here;
		w.objectsAt(px + 1, py, 0, here);
		for (int i : here)
			if (w.objects[i].actor < 0)
				w.deleteObject(i);
		int book = put(w, 0x94, 0, px + 1, py, 0);
		w.objects[book].quality = 2;
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("Operation of the Power Generation System") != std::string::npos);
		w.objects[book].quality = 120;
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("You can't read it") != std::string::npos);
		w.deleteObject(book);
	}

	// a shovel on a loaded wheelbarrow (rocks) to the north and then the ground to the south unloads it: a pile of rocks lands on the ground square and the barrow is empty
	{
		std::vector<int> here;
		for (auto sq : {std::make_pair(px + 1, py), std::make_pair(px, py + 1), std::make_pair(px, py - 1)}) {
			here.clear();
			w.objectsAt(sq.first, sq.second, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
		}
		int shovel = put(w, 0x42, 0, px + 1, py, 0);
		int barrow = put(w, 0x10C, 0, px, py - 1, 0);
		w.objects[barrow].quantity = 1;
		w.objects[barrow].quality = 2;
		size_t before = w.objects.size();
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_UP);
		g.handleKey(KEY_ENTER);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		bool pile = false;
		for (size_t i = before; i < w.objects.size(); ++i)
			pile |= w.objects[i].type == 0x1BA && !w.objects[i].deleted && w.objects[i].x == px && w.objects[i].y == py + 1;
		CHECK(pile && w.objects[barrow].quantity == 0);
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_UP);
		g.handleKey(KEY_ENTER);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("There is nothing to unload") != std::string::npos);
		for (size_t i = before; i < w.objects.size(); ++i)
			if (w.objects[i].type == 0x1BA)
				w.deleteObject((int)i);
		w.deleteObject(barrow);
		w.deleteObject(shovel);
	}

	// a switch bar (0x19B, recipe 31) with its base (0x19D) up and to the left and a rail car (0x19A) on the base's square turns over together; a loose one (quality 1) only turns loosely; without a base it has no effect
	{
		std::vector<int> here;
		for (auto sq : {std::make_pair(px + 1, py), std::make_pair(px, py - 1)}) {
			here.clear();
			w.objectsAt(sq.first, sq.second, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
		}
		int bar = put(w, 0x19B, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("It has no effect") != std::string::npos && w.objects[bar].frame == 0);
		int base = put(w, 0x19D, 0, px, py - 1, 0);
		int car = put(w, 0x19A, 0, px, py - 1, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(w.objects[bar].frame == 1 && w.objects[base].frame == 1 && w.objects[car].frame == 1);
		w.objects[bar].quality = 1;
		useEast(w, g, px, py);
		CHECK(w.objects[bar].frame == 0 && w.objects[base].frame == 1 && logText(g).find("It turns loosely") != std::string::npos);
		w.deleteObject(bar);
		w.deleteObject(base);
		w.deleteObject(car);
	}

	// a lens (0xCE, recipe 38) is put into its socket (0xCA, frame 0) to the south: it lands on the socket's square and the flag of that lens is set; a second one "has no effect"; a broken one says so
	{
		std::vector<int> here;
		for (auto sq : {std::make_pair(px + 1, py), std::make_pair(px, py + 1)}) {
			here.clear();
			w.objectsAt(sq.first, sq.second, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
		}
		w.actors.talkFlags[0x2E] &= 0x7F;
		int lens = put(w, 0xCE, 0, px + 1, py, 0);
		int socket = put(w, 0xCA, 0, px, py + 1, 0);
		w.objects[lens].quality = 2;
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("is broken") != std::string::npos);
		w.objects[lens].quality = 0;
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		CHECK(w.objects[lens].x == px && w.objects[lens].y == py + 1 && (w.actors.talkFlags[0x2E] & 0x80));
		w.moveObject(lens, px + 1, py, 0);
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("It has no effect") != std::string::npos);
		w.actors.talkFlags[0x2E] &= 0x7F;
		w.deleteObject(lens);
		w.deleteObject(socket);
	}

	// paint (0x77) into an empty spray gun (0x81) gives 20 charges of green paint; the oxium bin (0xD4, recipe 50) puts oxium in the pack
	{
		std::vector<int> here;
		for (auto sq : {std::make_pair(px + 1, py), std::make_pair(px, py + 1)}) {
			here.clear();
			w.objectsAt(sq.first, sq.second, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
		}
		int paint = put(w, 0x77, 0, px + 1, py, 0);
		int gun = put(w, 0x81, 0, px, py + 1, 0);
		w.objects[gun].quantity = 0;
		w.objects[gun].quality = 0;
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("20 charges of green paint") != std::string::npos && w.objects[gun].quantity == 20 && w.objects[gun].quality == 1 && w.objects[paint].deleted);
		w.deleteObject(gun);
		int bin = put(w, 0xD4, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("You get two handfuls of oxium") != std::string::npos || logText(g).find("carrying too much") != std::string::npos);
		w.deleteObject(bin);
	}

	// a seed plant (0x9D, recipe 58): "No seed to harvest yet." at frame 0, from frame 1 one seed (0x9E) goes into the pack and the plant starts over; a spray gun (0x105, recipe 49) with paint makes something invisible visible, with weed killer kills a plant (0xA8)
	{
		std::vector<int> here;
		for (auto sq : {std::make_pair(px + 1, py), std::make_pair(px, py + 1)}) {
			here.clear();
			w.objectsAt(sq.first, sq.second, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
		}
		int plant = put(w, 0x9D, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("No seed to harvest yet") != std::string::npos);
		w.objects[plant].frame = 1;
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(w.objects[plant].frame == 0 && w.objects[plant].quality >= 10 && w.objects[plant].quality <= 15);
		w.deleteObject(plant);
		int gun = put(w, 0x105, 0, px + 1, py, 0);
		w.objects[gun].quantity = 5;
		w.objects[gun].quality = 1;
		int ghost = put(w, 0x30, 0, px, py + 1, 0);
		w.objects[ghost].status |= 2;
		g.log.clear();
		g.handleKey('u');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		w.deleteObject(ghost);
		w.deleteObject(gun);
	}

	// the sextant (0x60, recipe 33) on the surface: (y - 512) * 240 / 1024 = 27 south, (x - 512) * 360 / 1024 gives east
	{
		std::vector<int> here;
		w.objectsAt(px + 1, py, 0, here);
		for (int i : here)
			if (w.objects[i].actor < 0)
				w.deleteObject(i);
		int sextant = put(w, 0x60, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("You are somewhere near") != std::string::npos);
		CHECK(logText(g).find("S") != std::string::npos);
		w.deleteObject(sextant);
	}

	// Look at a lit torch with 12 minutes left, and at a pocket watch
	{
		std::vector<int> here;
		w.objectsAt(px + 1, py, 0, here);
		for (int i : here)
			if (w.objects[i].actor < 0)
				w.deleteObject(i);
		int torch = put(w, 0x6E, 0, px + 1, py, 0);
		w.objects[torch].quality = 12;
		g.log.clear();
		g.handleKey('l');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("It has 12 more minutes of ") != std::string::npos);
		CHECK(logText(g).find("wick.") != std::string::npos);
		w.deleteObject(torch);
		int watch = put(w, 0x5B, 0, px + 1, py, 0);
		g.log.clear();
		g.handleKey('l');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("The time is ") != std::string::npos);
		w.deleteObject(watch);
	}

	// a thing with no recipe (type 0x1FE has none)
	int thing = put(w, 0x1FE, 0, px + 1, py, 0);
	(void)thing;
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(logText(g).find("can't be used") != std::string::npos || logText(g).find("nothing") != std::string::npos);

	std::printf(failures ? "FAILED\n" : "ok\n");
	return failures ? 1 : 0;
}
